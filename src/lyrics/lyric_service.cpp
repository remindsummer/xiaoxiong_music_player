#include "lyric_service.h"

#include "lyric_parser.h"
#include "../network/meting_api.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>
#include <QUrl>

#include <algorithm>

LyricService::LyricService(QObject *parent)
    : QObject(parent)
    , m_parser(new LyricParser(this))
    , m_network(new QNetworkAccessManager(this))
{
}

QString LyricService::lyricFilePath() const
{
    return m_lyricFilePath;
}

bool LyricService::hasLyrics() const
{
    return !m_lines.isEmpty();
}

int LyricService::currentLineIndex() const
{
    return m_currentLineIndex;
}

QString LyricService::currentLineText() const
{
    return lineTextAt(m_currentLineIndex);
}

qint64 LyricService::playbackPositionMs() const
{
    return m_playbackPositionMs;
}

QString LyricService::onlineFetchState() const
{
    switch (m_onlineFetchState) {
    case OnlineFetchState::Idle:
        return QStringLiteral("idle");
    case OnlineFetchState::Searching:
        return QStringLiteral("searching");
    case OnlineFetchState::Ready:
        return QStringLiteral("ready");
    case OnlineFetchState::Failed:
        return QStringLiteral("failed");
    }

    return QStringLiteral("idle");
}

bool LyricService::onlineRetryRequired() const
{
    return m_onlineRetryRequired;
}

QString LyricService::onlineLastError() const
{
    return m_onlineLastError;
}

bool LyricService::loadLyricsForAudioFile(const QString &audioFilePath)
{
    return loadLyricsForTrack(audioFilePath, audioFilePath);
}

bool LyricService::loadLyricsForTrack(const QString &trackKey, const QString &audioFilePath)
{
    const QString normalizedKey = normalizeTrackKey(trackKey);
    if (normalizedKey.isEmpty()) {
        return false;
    }

    m_currentTrackKey = normalizedKey;
    setOnlineLastError(QString());

    const QString audio = audioFilePath.trimmed().isEmpty() ? trackKey : audioFilePath;
    if (tryLoadSidecarLyrics(audio)) {
        return true;
    }

    if (loadLyricsFromCache(normalizedKey)) {
        return true;
    }

    return false;
}

bool LyricService::tryLoadSidecarLyrics(const QString &audioFilePath)
{
    QFileInfo audioInfo(audioFilePath);
    if (!audioInfo.exists() || !audioInfo.isFile()) {
        return false;
    }

    const QString matchedLyricPath = audioInfo.absolutePath()
        + QLatin1Char('/')
        + audioInfo.completeBaseName()
        + QStringLiteral(".lrc");

    if (!QFileInfo::exists(matchedLyricPath)) {
        return false;
    }

    return loadLyricsFromFile(matchedLyricPath, m_currentTrackKey);
}

bool LyricService::loadLyricsFromFile(const QString &lyricFilePath, const QString &trackKey)
{
    QFile file(lyricFilePath);
    if (!file.exists()) {
        setOnlineLastError(QStringLiteral("歌词文件不存在：%1").arg(lyricFilePath));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setOnlineLastError(QStringLiteral("歌词文件读取失败：%1").arg(lyricFilePath));
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#endif
    const QString content = stream.readAll();
    file.close();

    const QString associateKey = trackKey.trimmed().isEmpty() ? m_currentTrackKey : normalizeTrackKey(trackKey);
    if (!associateKey.isEmpty()) {
        m_currentTrackKey = associateKey;
    }

    const QString sourceLabel = QFileInfo(lyricFilePath).absoluteFilePath();
    if (!applyLyricsContent(content, sourceLabel)) {
        setOnlineLastError(QStringLiteral("歌词文件中未解析到有效时间行"));
        return false;
    }

    if (!m_currentTrackKey.isEmpty()) {
        saveLyricsToCache(m_currentTrackKey, content);
    }

    setOnlineLastError(QString());
    return true;
}

int LyricService::lineIndexForPosition(qint64 positionMs) const
{
    if (m_lines.isEmpty()) {
        return -1;
    }

    const auto it = std::upper_bound(
        m_lines.cbegin(),
        m_lines.cend(),
        positionMs,
        [](qint64 value, const LyricLine &line) {
            return value < line.timestampMs;
        });

    if (it == m_lines.cbegin()) {
        return -1;
    }

    return static_cast<int>(std::distance(m_lines.cbegin(), std::prev(it)));
}

QString LyricService::lineTextAt(int index) const
{
    if (index < 0 || index >= m_lines.size()) {
        return QString();
    }

    return m_lines.at(index).text;
}

QVariantList LyricService::lyricLinesForQml() const
{
    QVariantList result;
    result.reserve(m_lines.size());

    for (const LyricLine &line : m_lines) {
        QVariantMap map;
        map.insert(QStringLiteral("timeMs"), line.timestampMs);
        map.insert(QStringLiteral("text"), line.text);
        result.append(map);
    }

    return result;
}

void LyricService::setPlaybackPositionMs(qint64 positionMs)
{
    if (m_playbackPositionMs == positionMs) {
        return;
    }

    m_playbackPositionMs = positionMs;
    emit playbackPositionMsChanged();
    updateCurrentLineByPosition(positionMs);
}

void LyricService::clearLyrics()
{
    const bool hadLyrics = !m_lines.isEmpty();
    const bool hadPath = !m_lyricFilePath.isEmpty();

    m_lines.clear();
    m_lyricFilePath.clear();

    if (m_currentLineIndex != -1) {
        m_currentLineIndex = -1;
        emit currentLineIndexChanged();
        emit currentLineTextChanged();
    }

    if (hadLyrics) {
        emit lyricsChanged();
    }

    if (hadPath) {
        emit lyricFilePathChanged();
    }
}

void LyricService::requestOnlineLyrics(const QString &trackId,
                                       const QString &title,
                                       const QString &artist,
                                       const QString &server,
                                       const QString &onlineId,
                                       const QString &lrcUrl)
{
    m_pendingTrackId = trackId;
    m_pendingTitle = title;
    m_pendingArtist = artist;
    m_pendingServer = server.trimmed();
    m_pendingOnlineId = onlineId.trimmed();
    const QString trimmedLrc = lrcUrl.trimmed();
    if (trimmedLrc.contains(QStringLiteral("mikus.ink"), Qt::CaseInsensitive)) {
        m_pendingLrcUrl.clear();
    } else if (!trimmedLrc.isEmpty()) {
        m_pendingLrcUrl = MetingApi::rewriteServiceUrl(QUrl(trimmedLrc)).toString();
    } else {
        m_pendingLrcUrl.clear();
    }
    m_onlineRetryRequired = false;

    if (!trackId.trimmed().isEmpty()) {
        m_pendingTrackKey = normalizeTrackKey(trackId);
    } else if (!m_pendingOnlineId.isEmpty()) {
        const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
        m_pendingTrackKey = QStringLiteral("online:%1:%2").arg(safeServer, m_pendingOnlineId);
    } else {
        m_pendingTrackKey.clear();
    }

    if (m_pendingTrackKey.isEmpty()) {
        m_currentTrackKey.clear();
    } else {
        m_currentTrackKey = m_pendingTrackKey;
    }

    if (m_pendingLrcUrl.isEmpty() && m_pendingOnlineId.isEmpty() && m_pendingTitle.trimmed().isEmpty()) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("缺少歌曲信息，无法进行在线检索。"));
        return;
    }

    MetingApi::resetApiBase();
    startOnlineRequest();
}

void LyricService::retryOnlineLyrics()
{
    requestOnlineLyrics(m_pendingTrackId,
                        m_pendingTitle,
                        m_pendingArtist,
                        m_pendingServer,
                        m_pendingOnlineId,
                        m_pendingLrcUrl);
}

void LyricService::startOnlineRequest()
{
    if (m_currentReply) {
        QNetworkReply *staleReply = m_currentReply;
        m_currentReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    setOnlineFetchState(OnlineFetchState::Searching);
    setOnlineLastError(QString());

    if (!m_pendingLrcUrl.isEmpty()) {
        startLrcRequest(QUrl(m_pendingLrcUrl));
        return;
    }

    if (!m_pendingOnlineId.isEmpty()) {
        const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
        startLrcRequest(MetingApi::buildUrl(safeServer, QStringLiteral("lrc"), m_pendingOnlineId));
        return;
    }

    startSearchRequest();
}

void LyricService::startSearchRequest()
{
    const QString server = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
    if (server != QStringLiteral("netease")) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("歌词检索目前仅支持网易云音乐（netease）"));
        return;
    }

    const QString keyword = MetingApi::buildSearchKeyword(m_pendingTitle, m_pendingArtist);
    if (keyword.trimmed().isEmpty()) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("缺少歌曲信息，无法进行在线检索。"));
        return;
    }

    QNetworkRequest request(MetingApi::buildUrl(server, QStringLiteral("search"), keyword));
    MetingApi::applyCommonRequestHeaders(request);

    QNetworkReply *reply = m_network->get(request);
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (m_currentReply != reply) {
            reply->deleteLater();
            return;
        }
        m_currentReply = nullptr;
        handleSearchReply(reply);
        reply->deleteLater();
    });
}

void LyricService::startLrcRequest(const QUrl &lrcApiUrl)
{
    const QUrl resolvedUrl = MetingApi::rewriteServiceUrl(lrcApiUrl);
    if (!resolvedUrl.isValid() || resolvedUrl.isEmpty()) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("歌词链接无效"));
        return;
    }

    QNetworkRequest request(resolvedUrl);
    MetingApi::applyCommonRequestHeaders(request);

    QNetworkReply *reply = m_network->get(request);
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (m_currentReply != reply) {
            reply->deleteLater();
            return;
        }
        m_currentReply = nullptr;
        handleLrcReply(reply);
        reply->deleteLater();
    });
}

void LyricService::handleSearchReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (MetingApi::tryNextApiBase()) {
            startSearchRequest();
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("在线歌词检索失败：%1").arg(reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("在线歌词检索返回数据异常"));
        return;
    }

    const QJsonArray results = doc.array();
    for (const QJsonValue &value : results) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        const QString lrcUrl = obj.value(QStringLiteral("lrc")).toString().trimmed();
        const QString songId = MetingApi::extractSongId(obj);
        if (lrcUrl.isEmpty() && songId.isEmpty()) {
            continue;
        }

        m_pendingOnlineId = songId;
        const QString trackName = MetingApi::songTitle(obj);
        const QString artistName = MetingApi::songArtist(obj);
        if (!trackName.isEmpty()) {
            m_pendingTitle = trackName;
        }
        if (!artistName.isEmpty()) {
            m_pendingArtist = artistName;
        }
        if (!songId.isEmpty()) {
            const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
            m_pendingTrackKey = QStringLiteral("online:%1:%2").arg(safeServer, songId);
            m_currentTrackKey = m_pendingTrackKey;
        }

        if (!lrcUrl.isEmpty()) {
            m_pendingLrcUrl = MetingApi::rewriteServiceUrl(QUrl(lrcUrl)).toString();
            startLrcRequest(QUrl(m_pendingLrcUrl));
        } else {
            const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
            startLrcRequest(MetingApi::buildUrl(safeServer, QStringLiteral("lrc"), songId));
        }
        return;
    }

    setOnlineFetchState(OnlineFetchState::Failed);
    m_onlineRetryRequired = true;
    emit onlineFetchStateChanged();
    setOnlineLastError(QStringLiteral("未找到匹配的在线歌词"));
}

void LyricService::handleLrcReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (MetingApi::tryNextApiBase()) {
            m_pendingLrcUrl.clear();
            startOnlineRequest();
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("获取歌词失败：%1").arg(reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    QString lrcText = QString::fromUtf8(data).trimmed();

    // Meting lrc 接口可能返回 302 重定向后的纯文本，或 JSON 错误。
    if (lrcText.startsWith(QLatin1Char('{')) || lrcText.startsWith(QLatin1Char('['))) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (doc.isObject()) {
            const QString message = doc.object().value(QStringLiteral("message")).toString();
            if (MetingApi::tryNextApiBase()) {
                m_pendingLrcUrl.clear();
                startOnlineRequest();
                return;
            }
            setOnlineFetchState(OnlineFetchState::Failed);
            m_onlineRetryRequired = true;
            emit onlineFetchStateChanged();
            setOnlineLastError(message.isEmpty() ? QStringLiteral("歌词接口返回错误") : message);
            return;
        }
    }

    if (lrcText.isEmpty()) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("歌词内容为空"));
        return;
    }

    QString label = m_pendingTitle;
    if (!m_pendingArtist.isEmpty()) {
        label = QStringLiteral("%1 - %2").arg(m_pendingArtist, m_pendingTitle);
    }
    if (!applySyncedLyrics(lrcText, label)) {
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("歌词解析失败"));
    }
}

bool LyricService::applySyncedLyrics(const QString &syncedLyrics, const QString &sourceLabel)
{
    if (!applyLyricsContent(syncedLyrics, sourceLabel)) {
        return false;
    }

    m_onlineRetryRequired = false;
    setOnlineLastError(QString());
    setOnlineFetchState(OnlineFetchState::Ready);

    const QString cacheKey = !m_currentTrackKey.isEmpty() ? m_currentTrackKey : m_pendingTrackKey;
    if (!cacheKey.isEmpty()) {
        saveLyricsToCache(cacheKey, syncedLyrics);
    }
    return true;
}

bool LyricService::applyLyricsContent(const QString &lrcContent, const QString &sourceLabel)
{
    const QVector<LyricLine> parsed = m_parser->parseLrc(lrcContent);
    if (parsed.isEmpty()) {
        return false;
    }

    m_lines = parsed;
    m_lyricFilePath = sourceLabel.isEmpty()
        ? QStringLiteral("在线歌词 (Meting)")
        : sourceLabel;
    emit lyricFilePathChanged();
    emit lyricsChanged();

    m_currentLineIndex = -1;
    updateCurrentLineByPosition(m_playbackPositionMs);
    return true;
}

QString LyricService::normalizeTrackKey(const QString &trackKey) const
{
    const QString trimmed = trackKey.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (trimmed.startsWith(QStringLiteral("online:"))) {
        return trimmed;
    }
    return QFileInfo(trimmed).absoluteFilePath();
}

QString LyricService::lyricsCachePathForKey(const QString &trackKey) const
{
    const QString normalized = normalizeTrackKey(trackKey);
    if (normalized.isEmpty()) {
        return QString();
    }
    const QByteArray hash = QCryptographicHash::hash(normalized.toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/lyrics_cache");
    return dir + QLatin1Char('/') + QString::fromLatin1(hash) + QStringLiteral(".lrc");
}

bool LyricService::ensureLyricsCacheDir() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/lyrics_cache");
    return QDir().mkpath(dir);
}

bool LyricService::loadLyricsFromCache(const QString &trackKey)
{
    const QString cachePath = lyricsCachePathForKey(trackKey);
    if (cachePath.isEmpty() || !QFileInfo::exists(cachePath)) {
        return false;
    }

    QFile file(cachePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#endif
    const QString content = stream.readAll();
    file.close();

    const QString label = QStringLiteral("缓存歌词: %1").arg(normalizeTrackKey(trackKey));
    return applyLyricsContent(content, label);
}

bool LyricService::saveLyricsToCache(const QString &trackKey, const QString &lrcContent)
{
    const QString cachePath = lyricsCachePathForKey(trackKey);
    if (cachePath.isEmpty() || lrcContent.trimmed().isEmpty()) {
        return false;
    }
    if (!ensureLyricsCacheDir()) {
        return false;
    }

    QFile file(cachePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#endif
    stream << lrcContent;
    file.close();
    return true;
}

void LyricService::setOnlineFetchState(OnlineFetchState state)
{
    if (m_onlineFetchState == state) {
        return;
    }

    m_onlineFetchState = state;
    emit onlineFetchStateChanged();
}

void LyricService::setOnlineLastError(const QString &message)
{
    if (m_onlineLastError == message) {
        return;
    }

    m_onlineLastError = message;
    emit onlineLastErrorChanged();
}

void LyricService::updateCurrentLineByPosition(qint64 positionMs)
{
    const int nextIndex = lineIndexForPosition(positionMs);
    if (m_currentLineIndex == nextIndex) {
        return;
    }

    m_currentLineIndex = nextIndex;
    emit currentLineIndexChanged();
    emit currentLineTextChanged();
}
