#include "playback_service.h"

#include "../artwork/cover_art_service.h"
#include "../network/meting_api.h"

#include <QAudioOutput>
#include <QFileInfo>
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
        return QStringLiteral("未选择歌曲");
    }
    const QueueItem &item = m_queue.at(m_currentIndex);
    if (!item.artist.isEmpty()) {
        return QStringLiteral("%1 - %2").arg(item.title, item.artist);
    }
    return item.title;
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

QString PlaybackService::playbackStateText() const
{
    switch (state()) {
    case PlaybackState::Playing:
        return QStringLiteral("播放中");
    case PlaybackState::Paused:
        return QStringLiteral("已暂停");
    case PlaybackState::Stopped:
    default:
        return QStringLiteral("已停止");
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
        return QStringLiteral("顺序");
    case PlaybackMode::RepeatAll:
        return QStringLiteral("列表循环");
    case PlaybackMode::RepeatOne:
        return QStringLiteral("单曲循环");
    case PlaybackMode::Shuffle:
        return QStringLiteral("随机");
    default:
        return QStringLiteral("未知");
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
        setError(QStringLiteral("无效的文件地址"));
        return;
    }

    const QString localPath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    const QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        setError(QStringLiteral("文件不存在或不可读取"));
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
        setError(QStringLiteral("无效的文件路径"));
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
        setError(QStringLiteral("没有可播放的曲目"));
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
        setError(QStringLiteral("没有可播放的曲目"));
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

    const QueueItem &item = m_queue.at(index);
    if (item.isOnline) {
        resolveOnlineStreamAndLoad(index, autoplay);
        return;
    }

    m_currentIndex = index;
    clearError();
    m_player->setSource(m_queue.at(index).url);
    emit currentTrackChanged();
    updateCoverForCurrentTrack();
    emit queueChanged();
    emit positionChanged();
    emit durationChanged();

    if (autoplay) {
        m_player->play();
    } else {
        m_player->stop();
    }
    updateStatusText();
}

void PlaybackService::resolveOnlineStreamAndLoad(int index, bool autoplay)
{
    if (index < 0 || index >= m_queue.size()) {
        return;
    }

    if (m_resolveReply) {
        m_resolveReply->abort();
        m_resolveReply->deleteLater();
        m_resolveReply = nullptr;
    }

    const QueueItem &item = m_queue.at(index);
    if (item.streamApiUrl.isEmpty()) {
        setError(QStringLiteral("缺少在线播放地址"));
        return;
    }

    m_currentIndex = index;
    clearError();
    emit currentTrackChanged();
    updateCoverForCurrentTrack();
    emit queueChanged();

    QNetworkRequest request(QUrl(item.streamApiUrl));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("XiaoxiongMusicPlayer"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_resolveReply = m_network->get(request);
    connect(m_resolveReply, &QNetworkReply::finished, this, [this, index, autoplay]() {
        handleResolveReply(index, autoplay);
    });
}

void PlaybackService::handleResolveReply(int index, bool autoplay)
{
    QNetworkReply *reply = m_resolveReply;
    if (!reply) {
        return;
    }
    m_resolveReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        setError(QStringLiteral("获取在线播放链接失败：%1").arg(reply->errorString()));
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
        setError(QStringLiteral("无法解析在线播放链接"));
        updateStatusText();
        return;
    }

    if (index < 0 || index >= m_queue.size()) {
        return;
    }

    m_queue[index].url = streamUrl;
    m_player->setSource(streamUrl);
    updateCoverForCurrentTrack();
    emit positionChanged();
    emit durationChanged();

    if (autoplay) {
        m_player->play();
    } else {
        m_player->stop();
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
        nextStatus = QStringLiteral("错误: %1").arg(m_errorMessage);
    } else if (m_currentIndex < 0) {
        nextStatus = QStringLiteral("待机");
    } else {
        nextStatus = QStringLiteral("%1 | 模式: %2")
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
    if (m_currentCoverPath == coverPath) {
        return;
    }
    m_currentCoverPath = coverPath;
    emit currentCoverPathChanged();
}
