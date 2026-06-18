#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

class CoverArtService;

class PlaybackService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentTrackName READ currentTrackName NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentTrackChanged)
    Q_PROPERTY(bool currentTrackIsOnline READ currentTrackIsOnline NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentOnlineId READ currentOnlineId NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentOnlineServer READ currentOnlineServer NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentLrcUrl READ currentLrcUrl NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentPicUrl READ currentPicUrl NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentCoverPath READ currentCoverPath NOTIFY currentCoverPathChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY queueChanged)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY queueChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(QString playbackStateText READ playbackStateText NOTIFY playbackStateChanged)
    Q_PROPERTY(int playbackMode READ playbackMode WRITE setPlaybackMode NOTIFY playbackModeChanged)
    Q_PROPERTY(QString playbackModeText READ playbackModeText NOTIFY playbackModeChanged)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY queueChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY queueChanged)

public:
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    Q_ENUM(PlaybackState)

    // 顺序：放完即停；列表循环：到尾环绕；单曲循环：结尾重播；随机：随机下一首。
    enum class PlaybackMode {
        Sequential = 0,
        RepeatAll = 1,
        RepeatOne = 2,
        Shuffle = 3
    };
    Q_ENUM(PlaybackMode)

    explicit PlaybackService(QObject *parent = nullptr);

    void setCoverArtService(CoverArtService *coverArt);

    QString currentTrackName() const;
    QString currentPath() const;
    bool currentTrackIsOnline() const;
    QString currentOnlineId() const;
    QString currentOnlineServer() const;
    QString currentLrcUrl() const;
    QString currentPicUrl() const;
    QString currentCoverPath() const;
    int currentIndex() const;
    int queueCount() const;
    qint64 position() const;
    qint64 duration() const;
    int volume() const;
    QString statusText() const;
    QString errorMessage() const;

    int playbackState() const;
    QString playbackStateText() const;
    int playbackMode() const;
    QString playbackModeText() const;

    bool hasPrevious() const;
    bool hasNext() const;

    PlaybackState state() const;

    Q_INVOKABLE void openFile(const QUrl &fileUrl);
    Q_INVOKABLE void openFilePath(const QString &filePath);
    // 以整列为播放队列从 startIndex 播放（播放全部使用，替换队列）。
    Q_INVOKABLE void playQueue(const QVariantList &tracks, int startIndex);
    // 插入到当前曲之后并立即播放（单曲点播）。
    Q_INVOKABLE void enqueueAndPlay(const QVariantMap &track);
    // 按当前队列下标播放（播放队列面板使用）。
    Q_INVOKABLE void playTrackAt(int index);
    // 导出当前播放队列给 QML 渲染。
    Q_INVOKABLE QVariantList queue() const;
    Q_INVOKABLE void removeFromQueue(int index);
    Q_INVOKABLE void clearQueue();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void cyclePlaybackMode();
    Q_INVOKABLE void setPosition(qint64 position);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void setPlaybackMode(int mode);
    Q_INVOKABLE QString formatTime(qint64 ms) const;

signals:
    void currentTrackChanged();
    void currentCoverPathChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void statusTextChanged();
    void errorMessageChanged();
    void playbackStateChanged();
    void playbackModeChanged();
    void queueChanged();
    void stateChanged();

private:
    struct QueueItem {
        QUrl url;
        QString path;
        QString title;
        QString artist;
        bool isOnline = false;
        QString onlineId;
        QString server;
        QString streamApiUrl;
        QString lrcApiUrl;
        QString picUrl;
    };

    class QMediaPlayer *m_player = nullptr;
    class QAudioOutput *m_audioOutput = nullptr;
    class QNetworkAccessManager *m_network = nullptr;
    class QNetworkReply *m_resolveReply = nullptr;
    CoverArtService *m_coverArt = nullptr;

    QList<QueueItem> m_queue;
    int m_currentIndex = -1;
    PlaybackMode m_playbackMode = PlaybackMode::Sequential;
    QString m_errorMessage;

    void loadTrack(int index, bool autoplay);
    void resolveOnlineStreamAndLoad(int index, bool autoplay);
    void handleResolveReply(int index, bool autoplay);
    void clearError();
    void setError(const QString &message);
    void updateStatusText();
    void handleEndOfMedia();
    int randomNextIndex() const;
    void updateCoverForCurrentTrack();
    void onCoverReady(const QString &trackKey, const QString &coverPath);
    static QueueItem makeItemFromPath(const QString &path);
    static QueueItem makeItemFromVariant(const QVariantMap &map);
    static bool isValidQueueItem(const QueueItem &item);

    QString m_statusText = QStringLiteral("待机");
    QString m_currentCoverPath;
    QString m_coverRequestKey;
};
