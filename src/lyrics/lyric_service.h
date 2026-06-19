#pragma once

#include "../models/lyric_line.h"

#include <QHash>
#include <QObject>
#include <QSet>
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
    Q_PROPERTY(bool lyricsSynced READ lyricsSynced NOTIFY lyricsSyncedChanged)
    Q_PROPERTY(QString plainLyricsText READ plainLyricsText NOTIFY lyricsChanged)
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
    bool lyricsSynced() const;
    QString plainLyricsText() const;
    int currentLineIndex() const;
    QString currentLineText() const;
    qint64 playbackPositionMs() const;
    QString onlineFetchState() const;
    bool onlineRetryRequired() const;
    QString onlineLastError() const;

    Q_INVOKABLE bool loadLyricsForAudioFile(const QString &audioFilePath);
    Q_INVOKABLE bool loadLyricsForTrack(const QString &trackKey,
                                        const QString &audioFilePath = QString(),
                                        const QString &onlineServer = QString(),
                                        const QString &onlineId = QString(),
                                        const QString &title = QString(),
                                        const QString &artist = QString());
    Q_INVOKABLE bool loadLyricsFromFile(const QString &lyricFilePath, const QString &trackKey = QString());
    Q_INVOKABLE int lineIndexForPosition(qint64 positionMs) const;
    Q_INVOKABLE QString lineTextAt(int index) const;
    Q_INVOKABLE QVariantList lyricLinesForQml() const;
    Q_INVOKABLE void setPlaybackPositionMs(qint64 positionMs);
    Q_INVOKABLE void clearLyrics();
    Q_INVOKABLE bool removeCurrentLyricsBinding();
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
    void lyricsSyncedChanged();
    void currentLineIndexChanged();
    void currentLineTextChanged();
    void playbackPositionMsChanged();
    void onlineFetchStateChanged();
    void onlineLastErrorChanged();

private:
    void setOnlineFetchState(OnlineFetchState state);
    void setOnlineLastError(const QString &message);
    void cancelPendingOnlineFetch();
    void updateCurrentLineByPosition(qint64 positionMs);
    void startOnlineRequest();
    void startSearchRequest(int fetchGeneration);
    void startLrcRequest(const QUrl &lrcApiUrl, int fetchGeneration);
    void handleSearchReply(QNetworkReply *reply);
    void handleLrcReply(QNetworkReply *reply);
    void fetchSearchCandidateAt(int index, int fetchGeneration);
    void failOnlineLyricsFetch(const QString &message);
    void applyPlainFallbackFromSearch();
    int searchCandidateVectorIndexForOriginal(int originalIndex) const;
    bool applySyncedLyrics(const QString &syncedLyrics, const QString &sourceLabel);
    bool applyPlainLyricsOnline(const QString &plainLyrics, const QString &sourceLabel);
    QString normalizeTrackKey(const QString &trackKey) const;
    QString lyricsCachePathForKey(const QString &trackKey) const;
    bool ensureLyricsCacheDir() const;
    bool tryLoadSidecarLyrics(const QString &audioFilePath);
    bool loadLyricsFromCache(const QString &trackKey);
    bool tryLoadLyricsFromAnyCache(const QStringList &cacheKeys);
    bool tryLoadCachedLyricsForPendingKeys();
    void saveLyricsToCacheAliases(const QStringList &cacheKeys, const QString &lrcContent);
    void removeCacheForKeys(const QStringList &cacheKeys);
    QStringList candidateCacheKeys(const QString &trackKey,
                                   const QString &onlineServer,
                                   const QString &onlineId,
                                   const QString &title,
                                   const QString &artist) const;
    QString titleArtistCacheKey(const QString &title, const QString &artist) const;
    QString lyricsCacheDir() const;
    QString aliasesIndexPath() const;
    QString cacheFileNameForKey(const QString &trackKey) const;
    void loadAliasesIndex();
    void saveAliasesIndex();
    void registerCacheAliases(const QString &cacheFileName, const QStringList &aliasKeys);
    void rebuildAliasesIndexFromCacheFiles();
    QString embedCacheKeysHeader(const QStringList &aliasKeys, const QString &lrcContent) const;
    bool loadLyricsFromCacheFile(const QString &cacheFilePath, const QString &labelKey);
    bool loadLyricsFromCacheByAlias(const QString &aliasKey);
    bool scanCacheForAliasKey(const QString &aliasKey);
    bool isNetworkLikelyAvailable() const;
    QString metaCacheKeyFromLrcTags(const QString &lrcContent) const;
    bool tryLoadLyricsFromCacheByMetadata(const QString &title, const QString &artist);
    bool saveLyricsToCache(const QString &trackKey, const QString &lrcContent);
    bool applyLyricsContent(const QString &lrcContent, const QString &sourceLabel);
    void clearLyricsWithoutCancel();
    QString stripCacheHeader(const QString &lrcContent) const;

    LyricParser *m_parser = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_currentReply = nullptr;
    QVector<LyricLine> m_lines;
    QString m_plainLyricsContent;
    bool m_lyricsSynced = true;
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
    int m_fetchGeneration = 0;
    QHash<QString, QString> m_cacheAliasIndex;
    QSet<QString> m_skipSidecarKeys;

    struct LyricSearchCandidate {
        QString songId;
        QString lrcUrl;
        int score = 0;
        int originalIndex = -1;
    };
    QVector<LyricSearchCandidate> m_searchCandidates;
    int m_searchCandidateIndex = -1;
    QString m_searchRequestedTitle;
    QString m_searchRequestedArtist;
    QHash<int, QString> m_fetchedRawLrcByOriginalIndex;
    bool m_plainFallbackFetchActive = false;
};
