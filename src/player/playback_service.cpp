#include "playback_service.h"

#include "../artwork/cover_art_service.h"
#include "../artwork/library_cover_prefetcher.h"
#include "../network/meting_api.h"
#include "audio_visualizer_service.h"

#include <QAudioBufferOutput>
#include <QAudioOutput>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaMetaData>
#include <QStandardPaths>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QUrl>
#include <QVariantMap>
#include <QtMath>

PlaybackService::PlaybackService(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_network(new QNetworkAccessManager(this))
{
    m_audioOutput->setVolume(0.8f);
    m_player->setAudioOutput(m_audioOutput);

    m_bufferOutput = new QAudioBufferOutput(this);
    m_player->setAudioBufferOutput(m_bufferOutput);

    connect(m_player, &QMediaPlayer::positionChanged, this, [this]() {
        emit positionChanged();
    });
    connect(m_player, &QMediaPlayer::durationChanged, this, [this]() {
        emit durationChanged();
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this]() {
        updateStatusText();
        emit playbackStateChanged();
        emit stateChanged();
    });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            handleEndOfMedia();
        } else {
            updateStatusText();
        }
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            updateTrackMetadata();
        }
        applyPendingRestorePosition(status);
    });
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this]() {
        updateTrackMetadata();
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &errorString) {
        setError(errorString);
        updateStatusText();
    });

    updateStatusText();
}

void PlaybackService::setCoverArtService(CoverArtService *coverArt)
{
    if (m_coverArt) {
        disconnect(m_coverArt, nullptr, this, nullptr);
    }
    m_coverArt = coverArt;
    if (m_coverArt) {
        connect(m_coverArt, &CoverArtService::coverReady, this, &PlaybackService::onCoverReady);
    }
}

void PlaybackService::setCoverPrefetcher(LibraryCoverPrefetcher *prefetcher)
{
    if (m_coverPrefetcher) {
        disconnect(m_coverPrefetcher, nullptr, this, nullptr);
    }
    m_coverPrefetcher = prefetcher;
    if (m_coverPrefetcher) {
        connect(m_coverPrefetcher,
                &LibraryCoverPrefetcher::fetchFinished,
                this,
                &PlaybackService::onCoverPrefetchFinished);
    }
}

void PlaybackService::setAudioVisualizer(AudioVisualizerService *visualizer)
{
    if (m_visualizer && m_bufferOutput) {
        disconnect(m_bufferOutput, nullptr, m_visualizer, nullptr);
    }
    m_visualizer = visualizer;
    if (m_visualizer && m_bufferOutput) {
        connect(m_bufferOutput, &QAudioBufferOutput::audioBufferReceived,
                m_visualizer, &AudioVisualizerService::feedBuffer);
    }
}

PlaybackService::QueueItem PlaybackService::makeItemFromPath(const QString &path)
{
    QueueItem item;
    const QFileInfo fileInfo(path);
    item.path = fileInfo.absoluteFilePath();
    item.url = QUrl::fromLocalFile(item.path);
    item.title = fileInfo.completeBaseName().isEmpty() ? fileInfo.fileName() : fileInfo.completeBaseName();
    return item;
}

PlaybackService::QueueItem PlaybackService::makeItemFromVariant(const QVariantMap &map)
{
    const bool isOnline = map.value(QStringLiteral("isOnline")).toBool()
                          || map.value(QStringLiteral("path")).toString().startsWith(QStringLiteral("online:"));

    if (isOnline) {
        QueueItem item;
        item.isOnline = true;
        item.server = map.value(QStringLiteral("server")).toString().trimmed();
        if (item.server.isEmpty()) {
            item.server = QStringLiteral("netease");
        }
        item.onlineId = map.value(QStringLiteral("onlineId")).toString().trimmed();
        if (item.onlineId.isEmpty()) {
            item.onlineId = map.value(QStringLiteral("id")).toString().trimmed();
        }
        item.title = map.value(QStringLiteral("title")).toString().trimmed();
        if (item.title.isEmpty()) {
            item.title = map.value(QStringLiteral("name")).toString().trimmed();
        }
        item.artist = map.value(QStringLiteral("artist")).toString().trimmed();
        item.streamApiUrl = map.value(QStringLiteral("streamUrl")).toString().trimmed();
        if (item.streamApiUrl.isEmpty()) {
            item.streamApiUrl = map.value(QStringLiteral("url")).toString().trimmed();
        }
        item.lrcApiUrl = map.value(QStringLiteral("lrcUrl")).toString().trimmed();
        if (item.lrcApiUrl.isEmpty()) {
            item.lrcApiUrl = map.value(QStringLiteral("lrc")).toString().trimmed();
        }
        item.picUrl = map.value(QStringLiteral("picUrl")).toString().trimmed();
        if (item.streamApiUrl.isEmpty() && !item.onlineId.isEmpty()) {
            item.streamApiUrl = MetingApi::buildUrl(item.server, QStringLiteral("url"), item.onlineId).toString();
        }
        if (item.lrcApiUrl.isEmpty() && !item.onlineId.isEmpty()) {
            item.lrcApiUrl = MetingApi::buildUrl(item.server, QStringLiteral("lrc"), item.onlineId).toString();
        }
        item.streamApiUrl = MetingApi::rewriteServiceUrl(QUrl(item.streamApiUrl)).toString();
        item.lrcApiUrl = MetingApi::rewriteServiceUrl(QUrl(item.lrcApiUrl)).toString();
        item.picUrl = MetingApi::rewriteServiceUrl(QUrl(item.picUrl)).toString();
        item.path = QStringLiteral("online:%1:%2").arg(item.server, item.onlineId);
        item.url = QUrl(item.streamApiUrl);
        return item;
    }

    const QString path = map.value(QStringLiteral("path")).toString().trimmed();
    if (path.isEmpty()) {
        return QueueItem{};
    }

    QueueItem item = makeItemFromPath(path);

    const QString title = map.value(QStringLiteral("title")).toString().trimmed();
    if (!title.isEmpty()) {
        item.title = title;
    }
    item.artist = map.value(QStringLiteral("artist")).toString().trimmed();
    item.picUrl = map.value(QStringLiteral("picUrl")).toString().trimmed();
    return item;
}

bool PlaybackService::isValidQueueItem(const QueueItem &item)
{
    if (item.isOnline) {
        return !item.onlineId.isEmpty() && !item.streamApiUrl.isEmpty();
    }
    return !item.path.isEmpty();
}

QString PlaybackService::currentTrackName() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return tr("未选择歌曲");
    }
    const QueueItem &item = m_queue.at(m_currentIndex);
    if (!item.artist.isEmpty()) {
        return QStringLiteral("%1 - %2").arg(item.title, item.artist);
    }
    return item.title;
}

QString PlaybackService::currentTrackTitle() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).title;
}

QString PlaybackService::currentTrackArtist() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).artist;
}

QString PlaybackService::currentFileFormat() const
{
    return m_currentFileFormat;
}

int PlaybackService::currentAudioBitrate() const
{
    return m_currentAudioBitrate;
}

int PlaybackService::currentSampleRate() const
{
    return m_currentSampleRate;
}

int PlaybackService::currentBitDepth() const
{
    return m_currentBitDepth;
}

bool PlaybackService::currentIsHiRes() const
{
    return m_currentIsHiRes;
}

QString PlaybackService::currentPath() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).path;
}

bool PlaybackService::currentTrackIsOnline() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return false;
    }
    return m_queue.at(m_currentIndex).isOnline;
}

QString PlaybackService::currentOnlineId() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).onlineId;
}

QString PlaybackService::currentOnlineServer() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).server;
}

QString PlaybackService::currentLrcUrl() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).lrcApiUrl;
}

QString PlaybackService::currentPicUrl() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return QString();
    }
    return m_queue.at(m_currentIndex).picUrl;
}

QString PlaybackService::currentCoverPath() const
{
    return m_currentCoverPath;
}

QString PlaybackService::coverFetchState() const
{
    switch (m_coverFetchState) {
    case CoverFetchState::Fetching:
        return QStringLiteral("fetching");
    case CoverFetchState::Failed:
        return QStringLiteral("failed");
    case CoverFetchState::Idle:
    default:
        return QStringLiteral("idle");
    }
}

QString PlaybackService::coverFetchLastError() const
{
    return m_coverFetchLastError;
}

int PlaybackService::currentIndex() const
{
    return m_currentIndex;
}

int PlaybackService::queueCount() const
{
    return m_queue.size();
}

qint64 PlaybackService::position() const
{
    return m_player->position();
}

qint64 PlaybackService::duration() const
{
    return m_player->duration();
}

int PlaybackService::volume() const
{
    return qRound(m_audioOutput->volume() * 100.0f);
}

QString PlaybackService::statusText() const
{
    return m_statusText;
}

QString PlaybackService::errorMessage() const
{
    return m_errorMessage;
}

int PlaybackService::playbackState() const
{
    switch (m_player->playbackState()) {
    case QMediaPlayer::PlayingState:
        return static_cast<int>(PlaybackState::Playing);
    case QMediaPlayer::PausedState:
        return static_cast<int>(PlaybackState::Paused);
    case QMediaPlayer::StoppedState:
    default:
        return static_cast<int>(PlaybackState::Stopped);
    }
}

bool PlaybackService::isPlaying() const
{
    return state() == PlaybackState::Playing;
}

QString PlaybackService::playbackStateText() const
{
    switch (state()) {
    case PlaybackState::Playing:
        return tr("播放中");
    case PlaybackState::Paused:
        return tr("已暂停");
    case PlaybackState::Stopped:
    default:
        return tr("已停止");
    }
}

int PlaybackService::playbackMode() const
{
    return static_cast<int>(m_playbackMode);
}

QString PlaybackService::playbackModeText() const
{
    switch (m_playbackMode) {
    case PlaybackMode::Sequential:
        return tr("顺序");
    case PlaybackMode::RepeatAll:
        return tr("列表循环");
    case PlaybackMode::RepeatOne:
        return tr("单曲循环");
    case PlaybackMode::Shuffle:
        return tr("随机");
    default:
        return tr("未知");
    }
}

bool PlaybackService::hasPrevious() const
{
    if (m_queue.size() <= 1) {
        return false;
    }
    // 顺序模式到达队首即禁用；列表循环/随机可环绕。
    if (m_playbackMode == PlaybackMode::Sequential) {
        return m_currentIndex > 0;
    }
    return true;
}

bool PlaybackService::hasNext() const
{
    if (m_queue.size() <= 1) {
        return false;
    }
    // 顺序模式到达队尾即禁用；列表循环/随机可环绕。
    if (m_playbackMode == PlaybackMode::Sequential) {
        return m_currentIndex >= 0 && m_currentIndex < m_queue.size() - 1;
    }
    return true;
}

PlaybackService::PlaybackState PlaybackService::state() const
{
    return static_cast<PlaybackState>(playbackState());
}

void PlaybackService::openFile(const QUrl &fileUrl)
{
    if (!fileUrl.isValid() || fileUrl.isEmpty()) {
        setError(tr("无效的文件地址"));
        return;
    }

    const QString localPath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    const QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        setError(tr("文件不存在或不可读取"));
        return;
    }

    clearError();

    const QString absolutePath = fileInfo.absoluteFilePath();
    int targetIndex = -1;
    for (int i = 0; i < m_queue.size(); ++i) {
        if (m_queue.at(i).path == absolutePath) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex < 0) {
        m_queue.append(makeItemFromPath(absolutePath));
        targetIndex = m_queue.size() - 1;
        emit queueChanged();
    }

    loadTrack(targetIndex, true);
}

void PlaybackService::openFilePath(const QString &filePath)
{
    const QString normalizedPath = filePath.trimmed();
    if (normalizedPath.isEmpty()) {
        setError(tr("无效的文件路径"));
        return;
    }

    openFile(QUrl::fromLocalFile(normalizedPath));
}

void PlaybackService::playQueue(const QVariantList &tracks, int startIndex)
{
    QList<QueueItem> nextQueue;
    nextQueue.reserve(tracks.size());
    for (const QVariant &value : tracks) {
        const QVariantMap map = value.toMap();
        QueueItem item = makeItemFromVariant(map);
        if (item.isOnline) {
            if (item.onlineId.isEmpty() || item.streamApiUrl.isEmpty()) {
                continue;
            }
            nextQueue.append(item);
            continue;
        }
        if (item.path.isEmpty()) {
            continue;
        }
        nextQueue.append(item);
    }

    if (nextQueue.isEmpty()) {
        setError(tr("没有可播放的曲目"));
        return;
    }

    m_queue = nextQueue;
    clearError();
    emit queueChanged();

    const int safeIndex = qBound(0, startIndex, m_queue.size() - 1);
    loadTrack(safeIndex, true);
}

void PlaybackService::enqueueAndPlay(const QVariantMap &track)
{
    const QueueItem item = makeItemFromVariant(track);
    if (!isValidQueueItem(item)) {
        setError(tr("没有可播放的曲目"));
        return;
    }

    const int insertIndex = m_currentIndex < 0 ? 0 : m_currentIndex + 1;
    m_queue.insert(insertIndex, item);
    clearError();
    emit queueChanged();
    loadTrack(insertIndex, true);
}

void PlaybackService::playTrackAt(int index)
{
    if (index < 0 || index >= m_queue.size()) {
        return;
    }
    loadTrack(index, true);
}

QVariantList PlaybackService::queue() const
{
    QVariantList list;
    list.reserve(m_queue.size());
    for (int i = 0; i < m_queue.size(); ++i) {
        const QueueItem &item = m_queue.at(i);
        list.append(QVariantMap{
            {QStringLiteral("index"), i},
            {QStringLiteral("path"), item.path},
            {QStringLiteral("title"), item.title},
            {QStringLiteral("artist"), item.artist},
            {QStringLiteral("isCurrent"), i == m_currentIndex},
            {QStringLiteral("isOnline"), item.isOnline},
            {QStringLiteral("onlineId"), item.onlineId},
            {QStringLiteral("server"), item.server},
        });
    }
    return list;
}

void PlaybackService::removeFromQueue(int index)
{
    if (index < 0 || index >= m_queue.size()) {
        return;
    }

    const bool removingCurrent = (index == m_currentIndex);
    m_queue.removeAt(index);

    if (m_queue.isEmpty()) {
        m_currentIndex = -1;
        m_player->stop();
        m_player->setSource(QUrl());
        m_currentCoverPath.clear();
        emit currentCoverPathChanged();
        resetTrackMetadata();
        if (m_visualizer) {
            m_visualizer->reset();
        }
        emit currentTrackChanged();
        emit queueChanged();
        emit positionChanged();
        emit durationChanged();
        updateStatusText();
        return;
    }

    if (removingCurrent) {
        // 当前曲被移除：播放原位置（即下一首），越界则回到末尾。
        const int nextIndex = qBound(0, index, m_queue.size() - 1);
        loadTrack(nextIndex, true);
        return;
    }

    if (index < m_currentIndex) {
        --m_currentIndex;
    }
    emit currentTrackChanged();
    emit queueChanged();
}

void PlaybackService::clearQueue()
{
    if (m_queue.isEmpty()) {
        return;
    }
    m_queue.clear();
    m_currentIndex = -1;
    m_player->stop();
    m_player->setSource(QUrl());
    emit currentTrackChanged();
    emit queueChanged();
    emit positionChanged();
    emit durationChanged();
    updateStatusText();
}

void PlaybackService::play()
{
    if (m_currentIndex < 0 && !m_queue.isEmpty()) {
        loadTrack(0, true);
        return;
    }
    if (m_currentIndex < 0) {
        updateStatusText();
        return;
    }

    clearError();
    m_player->play();
    updateStatusText();
}

void PlaybackService::pause()
{
    m_player->pause();
    updateStatusText();
}

void PlaybackService::stop()
{
    m_player->stop();
    updateStatusText();
}

void PlaybackService::togglePlayPause()
{
    if (state() == PlaybackState::Playing) {
        pause();
    } else {
        play();
    }
}

void PlaybackService::previous()
{
    if (m_queue.isEmpty()) {
        return;
    }

    int targetIndex = m_currentIndex;
    if (m_playbackMode == PlaybackMode::Shuffle) {
        targetIndex = randomNextIndex();
    } else if (m_currentIndex <= 0) {
        // 顺序模式停在队首；其余模式环绕到队尾。
        if (m_playbackMode == PlaybackMode::Sequential) {
            return;
        }
        targetIndex = m_queue.size() - 1;
    } else {
        targetIndex = m_currentIndex - 1;
    }

    loadTrack(targetIndex, true);
}

void PlaybackService::next()
{
    if (m_queue.isEmpty()) {
        return;
    }

    int targetIndex = m_currentIndex;
    if (m_playbackMode == PlaybackMode::Shuffle) {
        targetIndex = randomNextIndex();
    } else if (m_currentIndex < 0 || m_currentIndex >= m_queue.size() - 1) {
        // 顺序模式停在队尾；其余模式环绕到队首。
        if (m_playbackMode == PlaybackMode::Sequential && m_currentIndex >= 0) {
            return;
        }
        targetIndex = 0;
    } else {
        targetIndex = m_currentIndex + 1;
    }

    loadTrack(targetIndex, true);
}

void PlaybackService::cyclePlaybackMode()
{
    const int mode = (playbackMode() + 1) % 4;
    setPlaybackMode(mode);
}

void PlaybackService::setPosition(qint64 position)
{
    if (position < 0) {
        position = 0;
    }
    if (duration() > 0 && position > duration()) {
        position = duration();
    }
    m_player->setPosition(position);
}

void PlaybackService::setVolume(int volume)
{
    const int clamped = qBound(0, volume, 100);
    const float normalized = static_cast<float>(clamped) / 100.0f;
    if (qFuzzyCompare(m_audioOutput->volume(), normalized)) {
        return;
    }

    m_audioOutput->setVolume(normalized);
    emit volumeChanged();
}

void PlaybackService::setPlaybackMode(int mode)
{
    const int safeMode = qBound(0, mode, 3);
    const PlaybackMode targetMode = static_cast<PlaybackMode>(safeMode);
    if (m_playbackMode == targetMode) {
        return;
    }

    m_playbackMode = targetMode;
    updateStatusText();
    emit playbackModeChanged();
    // 模式变化会影响上一首/下一首的可用性，通知 QML 重新求值。
    emit queueChanged();
}

QString PlaybackService::formatTime(qint64 ms) const
{
    if (ms < 0) {
        ms = 0;
    }
    const qint64 totalSeconds = ms / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void PlaybackService::loadTrack(int index, bool autoplay)
{
    if (index < 0 || index >= m_queue.size()) {
        return;
    }

    m_pendingRestorePositionMs = -1;

    const QueueItem &item = m_queue.at(index);
    if (item.isOnline) {
        resolveOnlineStreamAndLoad(index, autoplay);
        return;
    }

    m_currentIndex = index;
    clearError();
    resetTrackMetadata();
    resetCoverFetchState();
    m_player->setSource(m_queue.at(index).url);
    emit currentTrackChanged();
    updateCoverForCurrentTrack();
    emit queueChanged();
    emit positionChanged();
    emit durationChanged();

    if (autoplay) {
        m_player->play();
    }
    updateStatusText();
}

void PlaybackService::resolveOnlineStreamAndLoad(int index, bool autoplay)
{
    if (index < 0 || index >= m_queue.size()) {
        return;
    }

    if (m_resolveReply) {
        QNetworkReply *staleReply = m_resolveReply;
        m_resolveReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    const QueueItem &item = m_queue.at(index);
    if (item.streamApiUrl.isEmpty()) {
        setError(tr("缺少在线播放地址"));
        return;
    }

    m_currentIndex = index;
    clearError();
    resetCoverFetchState();
    emit currentTrackChanged();
    updateCoverForCurrentTrack();
    emit queueChanged();

    QNetworkRequest request(QUrl(item.streamApiUrl));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("XiaoxiongMusicPlayer"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    m_resolveReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, index, autoplay]() {
        handleResolveReply(reply, index, autoplay);
    });
}

void PlaybackService::handleResolveReply(QNetworkReply *reply, int index, bool autoplay)
{
    if (!reply) {
        return;
    }

    if (reply != m_resolveReply) {
        reply->deleteLater();
        return;
    }

    m_resolveReply = nullptr;

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        setError(tr("获取在线播放链接失败：%1").arg(reply->errorString()));
        updateStatusText();
        reply->deleteLater();
        return;
    }

    QUrl streamUrl = reply->url();
    const QVariant redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectTarget.isValid()) {
        streamUrl = redirectTarget.toUrl();
        if (streamUrl.isRelative()) {
            streamUrl = reply->url().resolved(streamUrl);
        }
    }

    const QByteArray body = reply->readAll().trimmed();
    reply->deleteLater();

    // Meting 有时直接返回文本 URL（以 @ 开头表示纯文本）。
    if (!body.isEmpty() && (body.startsWith("http://") || body.startsWith("https://"))) {
        streamUrl = QUrl(QString::fromUtf8(body));
    }

    if (!streamUrl.isValid() || streamUrl.isEmpty()) {
        setError(tr("无法解析在线播放链接"));
        updateStatusText();
        return;
    }

    if (index < 0 || index >= m_queue.size() || index != m_currentIndex) {
        return;
    }

    m_queue[index].url = streamUrl;
    m_player->setSource(streamUrl);
    updateCoverForCurrentTrack();
    emit positionChanged();
    emit durationChanged();

    if (autoplay) {
        m_player->play();
    }
    updateStatusText();
}

void PlaybackService::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorMessageChanged();
}

void PlaybackService::setError(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorMessageChanged();
}

void PlaybackService::updateStatusText()
{
    QString nextStatus;
    if (!m_errorMessage.isEmpty()) {
        nextStatus = tr("错误: %1").arg(m_errorMessage);
    } else if (m_currentIndex < 0) {
        nextStatus = tr("待机");
    } else {
        nextStatus = tr("%1 | 模式: %2")
                         .arg(playbackStateText(), playbackModeText());
    }

    if (m_statusText == nextStatus) {
        return;
    }
    m_statusText = nextStatus;
    emit statusTextChanged();
}

void PlaybackService::handleEndOfMedia()
{
    if (m_queue.isEmpty() || m_currentIndex < 0) {
        return;
    }

    if (m_playbackMode == PlaybackMode::RepeatOne) {
        loadTrack(m_currentIndex, true);
        return;
    }

    if (m_playbackMode == PlaybackMode::Shuffle) {
        loadTrack(randomNextIndex(), true);
        return;
    }

    if (m_currentIndex < m_queue.size() - 1) {
        loadTrack(m_currentIndex + 1, true);
        return;
    }

    // 已到队尾：列表循环回到队首，顺序模式停止。
    if (m_playbackMode == PlaybackMode::RepeatAll) {
        loadTrack(0, true);
        return;
    }

    m_player->stop();
    m_player->setPosition(0);
    emit positionChanged();
    updateStatusText();
}

int PlaybackService::randomNextIndex() const
{
    if (m_queue.size() <= 1) {
        return qMax(0, m_currentIndex);
    }

    int nextIndex = m_currentIndex;
    while (nextIndex == m_currentIndex) {
        nextIndex = QRandomGenerator::global()->bounded(m_queue.size());
    }
    return nextIndex;
}

void PlaybackService::updateCoverForCurrentTrack()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        if (!m_currentCoverPath.isEmpty()) {
            m_currentCoverPath.clear();
            emit currentCoverPathChanged();
        }
        return;
    }

    const QueueItem &item = m_queue.at(m_currentIndex);
    m_coverRequestKey = item.path;

    if (!m_coverArt) {
        if (!m_currentCoverPath.isEmpty()) {
            m_currentCoverPath.clear();
            emit currentCoverPathChanged();
        }
        return;
    }

    m_coverArt->requestCover(item.path, item.picUrl, item.server, item.onlineId);
}

void PlaybackService::onCoverReady(const QString &trackKey, const QString &coverPath)
{
    if (trackKey != m_coverRequestKey) {
        return;
    }
    if (m_currentCoverPath != coverPath) {
        m_currentCoverPath = coverPath;
        emit currentCoverPathChanged();
    }

    if (m_coverFetchState != CoverFetchState::Fetching) {
        return;
    }

    if (coverPath.isEmpty()) {
        setCoverFetchState(CoverFetchState::Failed);
        m_coverFetchLastError = tr("未能获取封面");
    } else {
        resetCoverFetchState();
    }
    emit coverFetchStateChanged();
}

void PlaybackService::retryFetchCurrentCover()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return;
    }
    if (!m_currentCoverPath.isEmpty()) {
        return;
    }
    if (m_coverFetchState == CoverFetchState::Fetching) {
        return;
    }

    const QueueItem &item = m_queue.at(m_currentIndex);
    m_coverRequestKey = item.path;
    setCoverFetchState(CoverFetchState::Fetching);
    m_coverFetchLastError.clear();
    emit coverFetchStateChanged();

    if (m_coverArt && (!item.picUrl.trimmed().isEmpty() || !item.onlineId.trimmed().isEmpty())) {
        m_coverArt->requestCover(item.path, item.picUrl, item.server, item.onlineId);
        return;
    }

    if (m_coverPrefetcher) {
        m_coverPrefetcher->fetchNow(item.path, item.title, item.artist);
        return;
    }

    setCoverFetchState(CoverFetchState::Failed);
    m_coverFetchLastError = tr("封面服务不可用");
    emit coverFetchStateChanged();
}

void PlaybackService::onCoverPrefetchFinished(const QString &path, bool success, const QString &message)
{
    if (path != m_coverRequestKey) {
        return;
    }

    if (success) {
        updateCoverForCurrentTrack();
        if (m_coverFetchState == CoverFetchState::Fetching) {
            resetCoverFetchState();
            emit coverFetchStateChanged();
        }
        return;
    }

    if (m_coverFetchState != CoverFetchState::Fetching) {
        return;
    }

    setCoverFetchState(CoverFetchState::Failed);
    m_coverFetchLastError = message.isEmpty() ? tr("未找到匹配的封面") : message;
    emit coverFetchStateChanged();
}

void PlaybackService::setCoverFetchState(CoverFetchState state)
{
    m_coverFetchState = state;
}

void PlaybackService::resetCoverFetchState()
{
    const bool hadVisibleState = m_coverFetchState != CoverFetchState::Idle || !m_coverFetchLastError.isEmpty();
    m_coverFetchState = CoverFetchState::Idle;
    m_coverFetchLastError.clear();
    if (hadVisibleState) {
        emit coverFetchStateChanged();
    }
}

QString PlaybackService::defaultSessionPath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/playback_session.json");
}

QVariantList PlaybackService::queueTracksForStorage() const
{
    QVariantList list;
    list.reserve(m_queue.size());
    for (const QueueItem &item : m_queue) {
        QVariantMap map{
            {QStringLiteral("path"), item.path},
            {QStringLiteral("title"), item.title},
            {QStringLiteral("artist"), item.artist},
            {QStringLiteral("isOnline"), item.isOnline},
        };
        if (item.isOnline) {
            map.insert(QStringLiteral("onlineId"), item.onlineId);
            map.insert(QStringLiteral("server"), item.server);
            map.insert(QStringLiteral("streamUrl"), item.streamApiUrl);
            map.insert(QStringLiteral("lrcUrl"), item.lrcApiUrl);
            map.insert(QStringLiteral("picUrl"), item.picUrl);
        }
        list.append(map);
    }
    return list;
}

bool PlaybackService::saveSession(const QString &storagePath)
{
    const QString targetPath = storagePath.trimmed().isEmpty() ? defaultSessionPath() : storagePath.trimmed();
    const QFileInfo info(targetPath);
    QDir().mkpath(info.absolutePath());

    QJsonObject object;
    object.insert(QStringLiteral("playbackMode"), playbackMode());
    object.insert(QStringLiteral("currentPath"), currentPath());
    object.insert(QStringLiteral("positionMs"), position());

    QJsonArray queueArray;
    const QVariantList tracks = queueTracksForStorage();
    for (const QVariant &value : tracks) {
        queueArray.append(QJsonObject::fromVariantMap(value.toMap()));
    }
    object.insert(QStringLiteral("queue"), queueArray);

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool PlaybackService::restoreQueueFromTracks(const QVariantList &tracks,
                                             const QString &currentPath,
                                             qint64 positionMs)
{
    QList<QueueItem> nextQueue;
    nextQueue.reserve(tracks.size());
    for (const QVariant &value : tracks) {
        const QVariantMap map = value.toMap();
        QueueItem item = makeItemFromVariant(map);
        if (!isValidQueueItem(item)) {
            continue;
        }
        if (!item.isOnline && !QFileInfo::exists(item.path)) {
            continue;
        }
        nextQueue.append(item);
    }

    if (nextQueue.isEmpty()) {
        return false;
    }

    int targetIndex = 0;
    if (!currentPath.trimmed().isEmpty()) {
        for (int i = 0; i < nextQueue.size(); ++i) {
            if (nextQueue.at(i).path == currentPath) {
                targetIndex = i;
                break;
            }
        }
    }

    m_queue = nextQueue;
    clearError();
    emit queueChanged();

    m_pendingRestorePositionMs = qMax<qint64>(0, positionMs);
    loadTrack(targetIndex, false);
    return true;
}

bool PlaybackService::restoreSession(const QString &storagePath)
{
    const QString targetPath = storagePath.trimmed().isEmpty() ? defaultSessionPath() : storagePath.trimmed();
    QFile file(targetPath);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QJsonObject object = doc.object();
    setPlaybackMode(object.value(QStringLiteral("playbackMode")).toInt(static_cast<int>(PlaybackMode::Sequential)));

    const QJsonArray queueArray = object.value(QStringLiteral("queue")).toArray();
    QVariantList tracks;
    tracks.reserve(queueArray.size());
    for (const QJsonValue &value : queueArray) {
        if (value.isObject()) {
            tracks.append(value.toObject().toVariantMap());
        }
    }

    if (tracks.isEmpty()) {
        return true;
    }

    const QString savedPath = object.value(QStringLiteral("currentPath")).toString();
    const qint64 positionMs = object.value(QStringLiteral("positionMs")).toInteger(0);
    return restoreQueueFromTracks(tracks, savedPath, positionMs);
}

void PlaybackService::applyPendingRestorePosition(QMediaPlayer::MediaStatus status)
{
    if (m_pendingRestorePositionMs < 0) {
        return;
    }
    if (status != QMediaPlayer::LoadedMedia && status != QMediaPlayer::BufferedMedia) {
        return;
    }

    const qint64 durationMs = m_player->duration();
    qint64 targetPosition = m_pendingRestorePositionMs;
    m_pendingRestorePositionMs = -1;

    if (durationMs > 0 && targetPosition > durationMs) {
        targetPosition = durationMs;
    }

    m_player->setPosition(targetPosition);
    emit positionChanged();
}

void PlaybackService::resetTrackMetadata()
{
    m_currentFileFormat.clear();
    m_currentAudioBitrate = 0;
    m_currentSampleRate = 0;
    m_currentBitDepth = 0;
    m_currentIsHiRes = false;
    emit trackMetadataChanged();
}

void PlaybackService::updateTrackMetadata()
{
    const QMediaMetaData metaData = m_player->metaData();
    int bitrate = metaData.value(QMediaMetaData::AudioBitRate).toInt();
    if (bitrate >= 10000) {
        bitrate /= 1000;
    }
    int sampleRate = 0;

    QString format;
    const QString fileFormat = metaData.stringValue(QMediaMetaData::FileFormat).trimmed();
    const QString audioCodec = metaData.stringValue(QMediaMetaData::AudioCodec).trimmed();
    if (!fileFormat.isEmpty()) {
        format = fileFormat.toUpper();
    } else if (!audioCodec.isEmpty()) {
        format = audioCodec.toUpper();
    }

    if (format.isEmpty() && m_currentIndex >= 0 && m_currentIndex < m_queue.size()) {
        const QueueItem &item = m_queue.at(m_currentIndex);
        if (item.isOnline) {
            format = QStringLiteral("STREAM");
        } else {
            format = QFileInfo(item.path).suffix().toUpper();
        }
    }

    int bitDepth = 0;
    if (format == QStringLiteral("FLAC")) {
        bitDepth = 24;
        if (sampleRate == 0) {
            sampleRate = 96000;
        }
    } else if (format == QStringLiteral("WAV")) {
        bitDepth = 16;
        if (sampleRate == 0) {
            sampleRate = 44100;
        }
    } else if (format == QStringLiteral("MP3") && sampleRate == 0) {
        sampleRate = 44100;
    }

    const bool hiRes = sampleRate >= 48000
                       || ((format == QStringLiteral("FLAC") || format == QStringLiteral("WAV"))
                           && bitrate >= 900);

    const bool changed = format != m_currentFileFormat
                         || bitrate != m_currentAudioBitrate
                         || sampleRate != m_currentSampleRate
                         || bitDepth != m_currentBitDepth
                         || hiRes != m_currentIsHiRes;

    m_currentFileFormat = format;
    m_currentAudioBitrate = bitrate;
    m_currentSampleRate = sampleRate;
    m_currentBitDepth = bitDepth;
    m_currentIsHiRes = hiRes;

    if (changed) {
        emit trackMetadataChanged();
    }
}

void PlaybackService::retranslateUi()
{
    updateStatusText();
    emit playbackStateChanged();
    emit playbackModeChanged();
    emit currentTrackChanged();
    emit errorMessageChanged();
    emit statusTextChanged();
}
