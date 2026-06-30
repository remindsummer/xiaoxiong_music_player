#include "online_download_service.h"

#include "../artwork/cover_art_service.h"
#include "../library/library_repository.h"
#include "../network/meting_stream_resolver.h"
#include "../settings/settings_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

OnlineDownloadService::OnlineDownloadService(QObject *parent)
    : QObject(parent)
    , m_streamResolver(new MetingStreamResolver(this))
    , m_network(new QNetworkAccessManager(this))
{
    connect(m_streamResolver, &MetingStreamResolver::resolved, this, [this](const QUrl &streamUrl) {
        startFileDownload(streamUrl, m_pendingTargetPath, m_pendingTrack);
    });
    connect(m_streamResolver, &MetingStreamResolver::failed, this, [this](const QString &message) {
        setState(State::Failed);
        setLastError(message);
        emit downloadFailed(message);
    });
}

void OnlineDownloadService::setSettingsService(SettingsService *settings)
{
    m_settings = settings;
}

void OnlineDownloadService::setLibraryRepository(LibraryRepository *library)
{
    m_library = library;
}

void OnlineDownloadService::setCoverArtService(CoverArtService *coverArt)
{
    m_coverArt = coverArt;
}

QString OnlineDownloadService::downloadState() const
{
    switch (m_state) {
    case State::Idle:
        return QStringLiteral("idle");
    case State::Downloading:
        return QStringLiteral("downloading");
    case State::Success:
        return QStringLiteral("success");
    case State::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("idle");
}

QString OnlineDownloadService::lastError() const
{
    return m_lastError;
}

void OnlineDownloadService::downloadTrack(const QVariantMap &track)
{
    if (QNetworkReply *staleReply = m_downloadReply) {
        m_downloadReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    if (!m_settings) {
        setState(State::Failed);
        setLastError(QStringLiteral("设置服务未就绪"));
        emit downloadFailed(m_lastError);
        return;
    }

    const QString downloadDir = m_settings->defaultDownloadDirectory().trimmed();
    if (downloadDir.isEmpty()) {
        setState(State::Failed);
        setLastError(QStringLiteral("请先在设置中指定在线下载目录"));
        emit downloadFailed(m_lastError);
        return;
    }

    QDir dir(downloadDir);
    if (!dir.exists() && !QDir().mkpath(downloadDir)) {
        setState(State::Failed);
        setLastError(QStringLiteral("无法创建下载目录"));
        emit downloadFailed(m_lastError);
        return;
    }

    const QString streamUrl = track.value(QStringLiteral("streamUrl")).toString().trimmed();
    if (streamUrl.isEmpty()) {
        setState(State::Failed);
        setLastError(QStringLiteral("缺少在线播放地址"));
        emit downloadFailed(m_lastError);
        return;
    }

    m_pendingTrack = track;
    m_pendingTargetPath = buildTargetPath(track);
    setState(State::Downloading);
    setLastError(QString());
    m_streamResolver->resolve(streamUrl);
}

QString OnlineDownloadService::sanitizeFileNamePart(const QString &value)
{
    QString result = value.trimmed();
    result.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    if (result.isEmpty()) {
        result = QStringLiteral("unknown");
    }
    return result;
}

QString OnlineDownloadService::buildTargetPath(const QVariantMap &track) const
{
    const QString downloadDir = m_settings->defaultDownloadDirectory().trimmed();
    const QString artist = sanitizeFileNamePart(track.value(QStringLiteral("artist")).toString());
    const QString title = sanitizeFileNamePart(track.value(QStringLiteral("title")).toString());
    const QString onlineId = track.value(QStringLiteral("onlineId")).toString().trimmed();
    const QString baseName = onlineId.isEmpty()
        ? QStringLiteral("%1 - %2").arg(artist, title)
        : QStringLiteral("%1 - %2 - %3").arg(artist, title, onlineId);
    return QDir(downloadDir).filePath(baseName + QStringLiteral(".mp3"));
}

void OnlineDownloadService::startFileDownload(const QUrl &streamUrl,
                                              const QString &targetPath,
                                              const QVariantMap &track)
{
    if (!streamUrl.isValid() || streamUrl.isEmpty()) {
        setState(State::Failed);
        setLastError(QStringLiteral("无效的下载链接"));
        emit downloadFailed(m_lastError);
        return;
    }

    if (QNetworkReply *staleReply = m_downloadReply) {
        m_downloadReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    QNetworkRequest request(streamUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("XiaoxiongMusicPlayer/1.0"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(120000);

    m_downloadReply = m_network->get(request);
    connect(m_downloadReply, &QNetworkReply::finished, this, [this, targetPath, track]() {
        QNetworkReply *reply = m_downloadReply;
        if (!reply) {
            return;
        }
        m_downloadReply = nullptr;

        if (reply->error() != QNetworkReply::NoError) {
            setState(State::Failed);
            setLastError(QStringLiteral("下载失败：%1").arg(reply->errorString()));
            emit downloadFailed(m_lastError);
            reply->deleteLater();
            return;
        }

        const QByteArray data = reply->readAll();
        reply->deleteLater();

        if (data.isEmpty()) {
            setState(State::Failed);
            setLastError(QStringLiteral("下载内容为空"));
            emit downloadFailed(m_lastError);
            return;
        }

        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setState(State::Failed);
            setLastError(QStringLiteral("无法写入文件：%1").arg(targetPath));
            emit downloadFailed(m_lastError);
            return;
        }
        file.write(data);
        file.close();

        const QString absolutePath = QFileInfo(targetPath).absoluteFilePath();
        if (m_library) {
            m_library->addTrackFromPath(absolutePath);
        }

        const QString picUrl = track.value(QStringLiteral("picUrl")).toString().trimmed();
        if (m_coverArt && !picUrl.isEmpty()) {
            m_coverArt->cacheCoverFromUrl(absolutePath, picUrl);
        }

        setState(State::Success);
        setLastError(QString());
        emit downloadFinished(absolutePath);
    });
}

void OnlineDownloadService::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit downloadStateChanged();
}

void OnlineDownloadService::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}
