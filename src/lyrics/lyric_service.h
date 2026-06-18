#pragma once

#include "../models/lyric_line.h"

#include <QObject>
#include <QVariantList>
#include <QVector>

class LyricParser;
class QNetworkAccessManager;
class QNetworkReply;

class LyricService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lyricFilePath READ lyricFilePath NOTIFY lyricFilePathChanged)
    Q_PROPERTY(bool hasLyrics READ hasLyrics NOTIFY lyricsChanged)
    Q_PROPERTY(int currentLineIndex READ currentLineIndex NOTIFY currentLineIndexChanged)
    Q_PROPERTY(QString currentLineText READ currentLineText NOTIFY currentLineTextChanged)
    Q_PROPERTY(qint64 playbackPositionMs READ playbackPositionMs WRITE setPlaybackPositionMs NOTIFY playbackPositionMsChanged)
    Q_PROPERTY(QString onlineFetchState READ onlineFetchState NOTIFY onlineFetchStateChanged)
    Q_PROPERTY(bool onlineRetryRequired READ onlineRetryRequired NOTIFY onlineFetchStateChanged)
    Q_PROPERTY(QString onlineLastError READ onlineLastError NOTIFY onlineLastErrorChanged)

public:
    enum class OnlineFetchState {
        Idle,
        Searching,
        Ready,
        Failed
    };
    Q_ENUM(OnlineFetchState)

    explicit LyricService(QObject *parent = nullptr);

    QString lyricFilePath() const;
    bool hasLyrics() const;
    int currentLineIndex() const;
    QString currentLineText() const;
    qint64 playbackPositionMs() const;
    QString onlineFetchState() const;
    bool onlineRetryRequired() const;
    QString onlineLastError() const;

    Q_INVOKABLE bool loadLyricsForAudioFile(const QString &audioFilePath);
    Q_INVOKABLE bool loadLyricsForTrack(const QString &trackKey, const QString &audioFilePath = QString());
    Q_INVOKABLE bool loadLyricsFromFile(const QString &lyricFilePath, const QString &trackKey = QString());
    Q_INVOKABLE int lineIndexForPosition(qint64 positionMs) const;
    Q_INVOKABLE QString lineTextAt(int index) const;
    Q_INVOKABLE QVariantList lyricLinesForQml() const;
    Q_INVOKABLE void setPlaybackPositionMs(qint64 positionMs);
    Q_INVOKABLE void clearLyrics();
    Q_INVOKABLE void requestOnlineLyrics(const QString &trackId,
                                         const QString &title,
                                         const QString &artist,
                                         const QString &server = QString(),
                                         const QString &onlineId = QString(),
                                         const QString &lrcUrl = QString());
    Q_INVOKABLE void retryOnlineLyrics();

signals:
    void lyricFilePathChanged();
    void lyricsChanged();
    void currentLineIndexChanged();
    void currentLineTextChanged();
    void playbackPositionMsChanged();
    void onlineFetchStateChanged();
    void onlineLastErrorChanged();

private:
    void setOnlineFetchState(OnlineFetchState state);
    void setOnlineLastError(const QString &message);
    void updateCurrentLineByPosition(qint64 positionMs);
    void startOnlineRequest();
    void startSearchRequest();
    void startLrcRequest(const QUrl &lrcApiUrl);
    void handleSearchReply(QNetworkReply *reply);
    void handleLrcReply(QNetworkReply *reply);
    bool applySyncedLyrics(const QString &syncedLyrics, const QString &sourceLabel);
    QString normalizeTrackKey(const QString &trackKey) const;
    QString lyricsCachePathForKey(const QString &trackKey) const;
    bool ensureLyricsCacheDir() const;
    bool tryLoadSidecarLyrics(const QString &audioFilePath);
    bool loadLyricsFromCache(const QString &trackKey);
    bool saveLyricsToCache(const QString &trackKey, const QString &lrcContent);
    bool applyLyricsContent(const QString &lrcContent, const QString &sourceLabel);

    LyricParser *m_parser = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_currentReply = nullptr;
    QVector<LyricLine> m_lines;
    QString m_lyricFilePath;
    qint64 m_playbackPositionMs = 0;
    int m_currentLineIndex = -1;
    OnlineFetchState m_onlineFetchState = OnlineFetchState::Idle;
    bool m_onlineRetryRequired = false;
    QString m_onlineLastError;
    QString m_pendingTrackId;
    QString m_pendingTitle;
    QString m_pendingArtist;
    QString m_pendingServer;
    QString m_pendingOnlineId;
    QString m_pendingLrcUrl;
    QString m_currentTrackKey;
    QString m_pendingTrackKey;
};
