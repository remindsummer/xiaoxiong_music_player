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
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

namespace {

int scoreLyricSearchMatch(const QJsonObject &song, const QString &title, const QString &artist)
{
    const QString resultTitle = MetingApi::songTitle(song);
    const QString resultArtist = MetingApi::songArtist(song);

    int score = 0;
    const QString normTitle = title.trimmed();
    const QString normArtist = artist.trimmed();

    if (!normTitle.isEmpty()) {
        if (resultTitle.compare(normTitle, Qt::CaseInsensitive) == 0) {
            score += 100;
        } else if (resultTitle.contains(normTitle, Qt::CaseInsensitive)
                   || normTitle.contains(resultTitle, Qt::CaseInsensitive)) {
            score += 50;
        }
    }

    if (!normArtist.isEmpty()
        && normArtist.compare(QStringLiteral("Unknown Artist"), Qt::CaseInsensitive) != 0) {
        if (resultArtist.compare(normArtist, Qt::CaseInsensitive) == 0) {
            score += 100;
        } else {
            static const QRegularExpression tokenSplit(
                QStringLiteral("[,&/\\\\|+·、]+"));
            const auto tokenize = [](const QString &value) {
                QStringList tokens;
                for (const QString &part : value.split(tokenSplit, Qt::SkipEmptyParts)) {
                    const QString trimmed = part.trimmed();
                    if (!trimmed.isEmpty()) {
                        tokens.append(trimmed);
                    }
                }
                return tokens;
            };

            const QStringList requestTokens = tokenize(normArtist);
            const QStringList resultTokens = tokenize(resultArtist);
            for (const QString &requestToken : requestTokens) {
                for (const QString &resultToken : resultTokens) {
                    if (requestToken.compare(resultToken, Qt::CaseInsensitive) == 0) {
                        score += 60;
                    } else if (requestToken.size() >= 3
                               && (resultToken.contains(requestToken, Qt::CaseInsensitive)
                                   || requestToken.contains(resultToken, Qt::CaseInsensitive))) {
                        score += 30;
                    }
                }
            }
        }
    }

    return score;
}

} // namespace

LyricService::LyricService(QObject *parent)
    : QObject(parent)
    , m_parser(new LyricParser(this))
    , m_network(new QNetworkAccessManager(this))
{
    QNetworkInformation::loadDefaultBackend();
    loadAliasesIndex();
    rebuildAliasesIndexFromCacheFiles();
}

QString LyricService::lyricFilePath() const
{
    return m_lyricFilePath;
}

bool LyricService::hasLyrics() const
{
    return !m_lines.isEmpty() || !m_plainLyricsContent.trimmed().isEmpty();
}

bool LyricService::lyricsSynced() const
{
    return m_lyricsSynced;
}

QString LyricService::plainLyricsText() const
{
    return m_plainLyricsContent;
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

bool LyricService::loadLyricsForTrack(const QString &trackKey,
                                      const QString &audioFilePath,
                                      const QString &onlineServer,
                                      const QString &onlineId,
                                      const QString &title,
                                      const QString &artist)
{
    const QString normalizedKey = normalizeTrackKey(trackKey);
    if (normalizedKey.isEmpty()) {
        return false;
    }

    const bool trackChanged = normalizedKey != m_currentTrackKey;
    cancelPendingOnlineFetch();

    if (!trackChanged && hasLyrics()) {
        setOnlineFetchState(OnlineFetchState::Idle);
        m_onlineRetryRequired = false;
        setOnlineLastError(QString());
        emit lyricsChanged();
        return true;
    }

    if (trackChanged) {
        clearLyricsWithoutCancel();
    }

    m_currentTrackKey = normalizedKey;
    m_pendingTrackKey = normalizedKey;
    setOnlineLastError(QString());

    const QString audio = audioFilePath.trimmed().isEmpty() ? trackKey : audioFilePath;
    if (!m_skipSidecarKeys.contains(normalizedKey) && tryLoadSidecarLyrics(audio)) {
        return true;
    }

    const QStringList cacheKeys = candidateCacheKeys(normalizedKey, onlineServer, onlineId, title, artist);
    if (tryLoadLyricsFromAnyCache(cacheKeys)) {
        return true;
    }

    return tryLoadLyricsFromCacheByMetadata(title, artist);
}

bool LyricService::tryLoadSidecarLyrics(const QString &audioFilePath)
{
    QString localPath = audioFilePath.trimmed();
    if (localPath.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        localPath = QUrl(localPath).toLocalFile();
    }

    QFileInfo audioInfo(localPath);
    if (!audioInfo.exists() || !audioInfo.isFile()) {
        return false;
    }

    const QString basePath = audioInfo.absolutePath() + QLatin1Char('/') + audioInfo.completeBaseName();
    const QString lrcPath = basePath + QStringLiteral(".lrc");
    if (QFileInfo::exists(lrcPath)) {
        return loadLyricsFromFile(lrcPath, m_currentTrackKey);
    }

    const QString txtPath = basePath + QStringLiteral(".txt");
    if (QFileInfo::exists(txtPath)) {
        return loadLyricsFromFile(txtPath, m_currentTrackKey);
    }

    return false;
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
        m_skipSidecarKeys.remove(associateKey);
    }

    const QString sourceLabel = QFileInfo(lyricFilePath).absoluteFilePath();
    if (!applyLyricsContent(content, sourceLabel)) {
        setOnlineLastError(QStringLiteral("歌词文件内容为空"));
        return false;
    }

    setOnlineFetchState(OnlineFetchState::Idle);
    m_onlineRetryRequired = false;
    setOnlineLastError(QString());

    if (!m_currentTrackKey.isEmpty()) {
        const QStringList cacheKeys = candidateCacheKeys(m_currentTrackKey,
                                                         QString(),
                                                         QString(),
                                                         QString(),
                                                         QString());
        saveLyricsToCacheAliases(cacheKeys, content);
    }

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
    if (!m_lyricsSynced) {
        return {};
    }

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
    if (m_lyricsSynced) {
        updateCurrentLineByPosition(positionMs);
    }
}

void LyricService::clearLyricsWithoutCancel()
{
    const bool hadLyrics = hasLyrics();
    const bool hadPath = !m_lyricFilePath.isEmpty();
    const bool hadSynced = m_lyricsSynced;

    m_lines.clear();
    m_plainLyricsContent.clear();
    m_lyricsSynced = true;
    m_lyricFilePath.clear();

    if (m_currentLineIndex != -1) {
        m_currentLineIndex = -1;
        emit currentLineIndexChanged();
        emit currentLineTextChanged();
    }

    if (hadLyrics) {
        emit lyricsChanged();
    }

    if (!hadSynced) {
        emit lyricsSyncedChanged();
    }

    if (hadPath) {
        emit lyricFilePathChanged();
    }
}

void LyricService::cancelPendingOnlineFetch()
{
    ++m_fetchGeneration;

    if (m_currentReply) {
        QNetworkReply *staleReply = m_currentReply;
        m_currentReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    if (m_onlineFetchState == OnlineFetchState::Searching) {
        setOnlineFetchState(OnlineFetchState::Idle);
    }
}

void LyricService::clearLyrics()
{
    cancelPendingOnlineFetch();
    clearLyricsWithoutCancel();
    m_onlineRetryRequired = false;
    setOnlineLastError(QString());
}

bool LyricService::removeCurrentLyricsBinding()
{
    if (m_currentTrackKey.isEmpty()) {
        return false;
    }

    const QStringList cacheKeys = candidateCacheKeys(m_currentTrackKey,
                                                     m_pendingServer,
                                                     m_pendingOnlineId,
                                                     m_pendingTitle,
                                                     m_pendingArtist);
    removeCacheForKeys(cacheKeys);

    m_skipSidecarKeys.insert(m_currentTrackKey);
    clearLyricsWithoutCancel();
    setOnlineFetchState(OnlineFetchState::Idle);
    m_onlineRetryRequired = false;
    setOnlineLastError(QString());
    return true;
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

    if (m_pendingOnlineId.isEmpty()) {
        m_pendingOnlineId = MetingApi::extractEmbeddedNeteaseId(trackId);
        if (m_pendingOnlineId.isEmpty() && !m_pendingTrackKey.isEmpty()) {
            m_pendingOnlineId = MetingApi::extractEmbeddedNeteaseId(m_pendingTrackKey);
        }
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

    if (tryLoadCachedLyricsForPendingKeys()) {
        return;
    }

    if (!isNetworkLikelyAvailable()) {
        if (tryLoadCachedLyricsForPendingKeys()) {
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("当前处于离线状态，且未找到已缓存的歌词。"));
        return;
    }

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
    cancelPendingOnlineFetch();
    m_searchCandidates.clear();
    m_searchCandidateIndex = -1;
    m_fetchedRawLrcByOriginalIndex.clear();
    m_plainFallbackFetchActive = false;
    const int generation = m_fetchGeneration;

    setOnlineFetchState(OnlineFetchState::Searching);
    setOnlineLastError(QString());

    if (!m_pendingLrcUrl.isEmpty()) {
        startLrcRequest(QUrl(m_pendingLrcUrl), generation);
        return;
    }

    if (!m_pendingOnlineId.isEmpty()) {
        const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
        startLrcRequest(MetingApi::buildUrl(safeServer, QStringLiteral("lrc"), m_pendingOnlineId), generation);
        return;
    }

    QString embeddedId = MetingApi::extractEmbeddedNeteaseId(m_pendingTrackId);
    if (embeddedId.isEmpty()) {
        embeddedId = MetingApi::extractEmbeddedNeteaseId(m_pendingTrackKey);
    }
    if (!embeddedId.isEmpty()) {
        m_pendingOnlineId = embeddedId;
        const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
        m_pendingTrackKey = QStringLiteral("online:%1:%2").arg(safeServer, embeddedId);
        m_currentTrackKey = m_pendingTrackKey;
        startLrcRequest(MetingApi::buildUrl(safeServer, QStringLiteral("lrc"), embeddedId), generation);
        return;
    }

    startSearchRequest(generation);
}

void LyricService::startSearchRequest(int fetchGeneration)
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

    m_searchRequestedTitle = m_pendingTitle;
    m_searchRequestedArtist = m_pendingArtist;
    m_searchCandidates.clear();
    m_searchCandidateIndex = -1;
    m_fetchedRawLrcByOriginalIndex.clear();
    m_plainFallbackFetchActive = false;

    QNetworkRequest request(MetingApi::buildUrl(server, QStringLiteral("search"), keyword));
    MetingApi::applyCommonRequestHeaders(request);

    QNetworkReply *reply = m_network->get(request);
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, fetchGeneration]() {
        if (fetchGeneration != m_fetchGeneration) {
            reply->deleteLater();
            return;
        }
        if (m_currentReply != reply) {
            reply->deleteLater();
            return;
        }
        m_currentReply = nullptr;
        handleSearchReply(reply);
        reply->deleteLater();
    });
}

void LyricService::startLrcRequest(const QUrl &lrcApiUrl, int fetchGeneration)
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, fetchGeneration]() {
        if (fetchGeneration != m_fetchGeneration) {
            reply->deleteLater();
            return;
        }
        if (m_currentReply != reply) {
            reply->deleteLater();
            return;
        }
        m_currentReply = nullptr;
        handleLrcReply(reply);
        reply->deleteLater();
    });
}

void LyricService::fetchSearchCandidateAt(int index, int fetchGeneration)
{
    if (fetchGeneration != m_fetchGeneration) {
        return;
    }
    if (index < 0 || index >= m_searchCandidates.size()) {
        return;
    }

    m_searchCandidateIndex = index;
    const LyricSearchCandidate &candidate = m_searchCandidates.at(index);

    m_pendingOnlineId = candidate.songId;
    const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
    if (!candidate.songId.isEmpty()) {
        m_pendingTrackKey = QStringLiteral("online:%1:%2").arg(safeServer, candidate.songId);
        m_currentTrackKey = m_pendingTrackKey;
    }

    if (!candidate.lrcUrl.isEmpty()) {
        m_pendingLrcUrl = candidate.lrcUrl;
        startLrcRequest(QUrl(m_pendingLrcUrl), fetchGeneration);
        return;
    }

    if (!candidate.songId.isEmpty()) {
        m_pendingLrcUrl.clear();
        startLrcRequest(MetingApi::buildUrl(safeServer, QStringLiteral("lrc"), candidate.songId),
                         fetchGeneration);
    }
}

void LyricService::failOnlineLyricsFetch(const QString &message)
{
    if (tryLoadCachedLyricsForPendingKeys()) {
        return;
    }
    setOnlineFetchState(OnlineFetchState::Failed);
    m_onlineRetryRequired = true;
    emit onlineFetchStateChanged();
    setOnlineLastError(message);
}

void LyricService::handleSearchReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (MetingApi::tryNextApiBase()) {
            startSearchRequest(m_fetchGeneration);
            return;
        }
        if (tryLoadCachedLyricsForPendingKeys()) {
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("在线歌词检索失败：%1").arg(reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    const MetingApi::MetingSearchParseResult parsed = MetingApi::parseSearchResponse(data);
    if (parsed.shouldTryNextMirror) {
        if (MetingApi::tryNextApiBase()) {
            startSearchRequest(m_fetchGeneration);
            return;
        }
    }

    if (parsed.songs.isEmpty()) {
        if (tryLoadCachedLyricsForPendingKeys()) {
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(parsed.errorMessage.isEmpty()
                               ? QStringLiteral("在线歌词检索返回数据异常")
                               : parsed.errorMessage);
        return;
    }

    const QJsonArray results = parsed.songs;
    int originalIndex = 0;
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

        LyricSearchCandidate candidate;
        candidate.songId = songId;
        candidate.originalIndex = originalIndex++;
        candidate.score = scoreLyricSearchMatch(obj, m_searchRequestedTitle, m_searchRequestedArtist);
        if (!lrcUrl.isEmpty()) {
            candidate.lrcUrl = MetingApi::rewriteServiceUrl(QUrl(lrcUrl)).toString();
        }
        m_searchCandidates.append(candidate);
    }

    if (m_searchCandidates.isEmpty()) {
        if (tryLoadCachedLyricsForPendingKeys()) {
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("未找到匹配的在线歌词"));
        return;
    }

    std::stable_sort(m_searchCandidates.begin(),
                     m_searchCandidates.end(),
                     [](const LyricSearchCandidate &left, const LyricSearchCandidate &right) {
                         return left.score > right.score;
                     });

    fetchSearchCandidateAt(0, m_fetchGeneration);
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
        if (tryLoadCachedLyricsForPendingKeys()) {
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("获取歌词失败：%1").arg(reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    const QString lrcText = QString::fromUtf8(data);

    // Meting lrc 接口可能返回 302 重定向后的纯文本，或 JSON 错误。
    const QString trimmedLrc = lrcText.trimmed();
    if (trimmedLrc.startsWith(QLatin1Char('{')) || trimmedLrc.startsWith(QLatin1Char('['))) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (doc.isObject()) {
            const QString message = doc.object().value(QStringLiteral("message")).toString();
            if (MetingApi::tryNextApiBase()) {
                m_pendingLrcUrl.clear();
                startOnlineRequest();
                return;
            }
            if (tryLoadCachedLyricsForPendingKeys()) {
                return;
            }
            setOnlineFetchState(OnlineFetchState::Failed);
            m_onlineRetryRequired = true;
            emit onlineFetchStateChanged();
            setOnlineLastError(message.isEmpty() ? QStringLiteral("歌词接口返回错误") : message);
            return;
        }
    }

    if (trimmedLrc.isEmpty()) {
        if (m_searchCandidateIndex >= 0 && m_searchCandidateIndex < m_searchCandidates.size()) {
            const int originalIndex = m_searchCandidates.at(m_searchCandidateIndex).originalIndex;
            m_fetchedRawLrcByOriginalIndex.insert(originalIndex, QString());
        }
        if (m_plainFallbackFetchActive) {
            m_plainFallbackFetchActive = false;
            applyPlainFallbackFromSearch();
            return;
        }
        if (!m_searchCandidates.isEmpty()
            && m_searchCandidateIndex + 1 < m_searchCandidates.size()) {
            fetchSearchCandidateAt(m_searchCandidateIndex + 1, m_fetchGeneration);
            return;
        }
        if (!m_searchCandidates.isEmpty()) {
            applyPlainFallbackFromSearch();
            return;
        }
        if (tryLoadCachedLyricsForPendingKeys()) {
            return;
        }
        setOnlineFetchState(OnlineFetchState::Failed);
        m_onlineRetryRequired = true;
        emit onlineFetchStateChanged();
        setOnlineLastError(QStringLiteral("歌词内容为空"));
        return;
    }

    if (!m_pendingTrackKey.isEmpty() && m_pendingTrackKey != m_currentTrackKey) {
        return;
    }

    if (m_searchCandidateIndex >= 0 && m_searchCandidateIndex < m_searchCandidates.size()) {
        const int originalIndex = m_searchCandidates.at(m_searchCandidateIndex).originalIndex;
        m_fetchedRawLrcByOriginalIndex.insert(originalIndex, lrcText);
    }

    QString label = m_pendingTitle;
    if (!m_pendingArtist.isEmpty()) {
        label = QStringLiteral("%1 - %2").arg(m_pendingArtist, m_pendingTitle);
    }

    if (m_plainFallbackFetchActive) {
        m_plainFallbackFetchActive = false;
        if (applyPlainLyricsOnline(lrcText, label)) {
            return;
        }
        failOnlineLyricsFetch(QStringLiteral("未找到匹配的在线歌词"));
        return;
    }

    if (!applySyncedLyrics(lrcText, label)) {
        if (!m_searchCandidates.isEmpty()
            && m_searchCandidateIndex + 1 < m_searchCandidates.size()) {
            fetchSearchCandidateAt(m_searchCandidateIndex + 1, m_fetchGeneration);
            return;
        }

        if (!m_searchCandidates.isEmpty()) {
            applyPlainFallbackFromSearch();
            return;
        }

        if (applyPlainLyricsOnline(lrcText, label)) {
            return;
        }

        failOnlineLyricsFetch(QStringLiteral("未找到匹配的在线歌词"));
    }
}

bool LyricService::applySyncedLyrics(const QString &syncedLyrics, const QString &sourceLabel)
{
    const QVector<LyricLine> parsed = m_parser->parseLrc(stripCacheHeader(syncedLyrics));
    if (parsed.isEmpty()) {
        return false;
    }

    if (!applyLyricsContent(syncedLyrics, sourceLabel)) {
        return false;
    }

    if (!m_lyricsSynced) {
        return false;
    }

    m_onlineRetryRequired = false;
    setOnlineLastError(QString());
    setOnlineFetchState(OnlineFetchState::Ready);

    QStringList cacheKeys;
    const auto appendKey = [&cacheKeys, this](const QString &key) {
        const QString normalized = normalizeTrackKey(key);
        if (!normalized.isEmpty() && !cacheKeys.contains(normalized)) {
            cacheKeys.append(normalized);
        }
    };
    appendKey(m_currentTrackKey);
    appendKey(m_pendingTrackKey);
    appendKey(m_pendingTrackId);
    if (!m_pendingOnlineId.isEmpty()) {
        const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
        appendKey(QStringLiteral("online:%1:%2").arg(safeServer, m_pendingOnlineId));
    }
    appendKey(titleArtistCacheKey(m_pendingTitle, m_pendingArtist));

    saveLyricsToCacheAliases(cacheKeys, syncedLyrics);
    return true;
}

bool LyricService::applyPlainLyricsOnline(const QString &plainLyrics, const QString &sourceLabel)
{
    if (!applyLyricsContent(plainLyrics, sourceLabel)) {
        return false;
    }

    if (m_lyricsSynced) {
        return false;
    }

    m_onlineRetryRequired = false;
    setOnlineLastError(QString());
    setOnlineFetchState(OnlineFetchState::Ready);

    QStringList cacheKeys;
    const auto appendKey = [&cacheKeys, this](const QString &key) {
        const QString normalized = normalizeTrackKey(key);
        if (!normalized.isEmpty() && !cacheKeys.contains(normalized)) {
            cacheKeys.append(normalized);
        }
    };
    appendKey(m_currentTrackKey);
    appendKey(m_pendingTrackKey);
    appendKey(m_pendingTrackId);
    if (!m_pendingOnlineId.isEmpty()) {
        const QString safeServer = m_pendingServer.isEmpty() ? QStringLiteral("netease") : m_pendingServer;
        appendKey(QStringLiteral("online:%1:%2").arg(safeServer, m_pendingOnlineId));
    }
    appendKey(titleArtistCacheKey(m_pendingTitle, m_pendingArtist));

    saveLyricsToCacheAliases(cacheKeys, plainLyrics);
    return true;
}

int LyricService::searchCandidateVectorIndexForOriginal(int originalIndex) const
{
    for (int i = 0; i < m_searchCandidates.size(); ++i) {
        if (m_searchCandidates.at(i).originalIndex == originalIndex) {
            return i;
        }
    }
    return -1;
}

void LyricService::applyPlainFallbackFromSearch()
{
    QString label = m_pendingTitle;
    if (!m_pendingArtist.isEmpty()) {
        label = QStringLiteral("%1 - %2").arg(m_pendingArtist, m_pendingTitle);
    }

    int maxOriginalIndex = -1;
    for (const LyricSearchCandidate &candidate : m_searchCandidates) {
        maxOriginalIndex = qMax(maxOriginalIndex, candidate.originalIndex);
    }

    for (int originalIndex = 0; originalIndex <= maxOriginalIndex; ++originalIndex) {
        const auto fetched = m_fetchedRawLrcByOriginalIndex.constFind(originalIndex);
        if (fetched != m_fetchedRawLrcByOriginalIndex.constEnd()
            && !fetched.value().trimmed().isEmpty()) {
            if (applyPlainLyricsOnline(fetched.value(), label)) {
                return;
            }
        }

        if (fetched == m_fetchedRawLrcByOriginalIndex.constEnd()) {
            const int vectorIndex = searchCandidateVectorIndexForOriginal(originalIndex);
            if (vectorIndex >= 0) {
                m_plainFallbackFetchActive = true;
                fetchSearchCandidateAt(vectorIndex, m_fetchGeneration);
                return;
            }
        }
    }

    failOnlineLyricsFetch(QStringLiteral("未找到匹配的在线歌词"));
}

bool LyricService::applyLyricsContent(const QString &lrcContent, const QString &sourceLabel)
{
    const QString body = stripCacheHeader(lrcContent);
    const QVector<LyricLine> parsed = m_parser->parseLrc(body);
    const bool wasSynced = m_lyricsSynced;

    if (!parsed.isEmpty()) {
        m_lyricsSynced = true;
        m_plainLyricsContent.clear();
        m_lines = parsed;
    } else if (!body.trimmed().isEmpty()) {
        m_lyricsSynced = false;
        m_plainLyricsContent = body;
        m_lines.clear();
    } else {
        return false;
    }

    m_lyricFilePath = sourceLabel.isEmpty()
        ? QStringLiteral("在线歌词 (Meting)")
        : (m_lyricsSynced ? sourceLabel : QStringLiteral("%1 (静态)").arg(sourceLabel));
    emit lyricFilePathChanged();
    emit lyricsChanged();

    if (wasSynced != m_lyricsSynced) {
        emit lyricsSyncedChanged();
    }

    m_currentLineIndex = -1;
    if (m_lyricsSynced) {
        updateCurrentLineByPosition(m_playbackPositionMs);
    } else {
        emit currentLineIndexChanged();
        emit currentLineTextChanged();
    }
    return true;
}

QString LyricService::stripCacheHeader(const QString &lrcContent) const
{
    if (!lrcContent.startsWith(QStringLiteral("[xiaoxiong-keys]"))) {
        return lrcContent;
    }

    const int newlineIndex = lrcContent.indexOf(QLatin1Char('\n'));
    if (newlineIndex < 0) {
        return QString();
    }
    return lrcContent.mid(newlineIndex + 1);
}

QString LyricService::normalizeTrackKey(const QString &trackKey) const
{
    const QString trimmed = trackKey.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (trimmed.startsWith(QStringLiteral("online:")) || trimmed.startsWith(QStringLiteral("meta:"))) {
        return trimmed;
    }
    return QFileInfo(trimmed).absoluteFilePath();
}

QStringList LyricService::candidateCacheKeys(const QString &trackKey,
                                             const QString &onlineServer,
                                             const QString &onlineId,
                                             const QString &title,
                                             const QString &artist) const
{
    QStringList keys;
    const auto appendKey = [&keys](const QString &key) {
        const QString normalized = key.trimmed();
        if (!normalized.isEmpty() && !keys.contains(normalized)) {
            keys.append(normalized);
        }
    };

    appendKey(normalizeTrackKey(trackKey));
    if (!onlineId.trimmed().isEmpty()) {
        const QString safeServer = onlineServer.trimmed().isEmpty() ? QStringLiteral("netease") : onlineServer.trimmed();
        appendKey(QStringLiteral("online:%1:%2").arg(safeServer, onlineId.trimmed()));
    }
    appendKey(titleArtistCacheKey(title, artist));

    return keys;
}

QString LyricService::titleArtistCacheKey(const QString &title, const QString &artist) const
{
    const QString trimmedTitle = title.trimmed();
    if (trimmedTitle.isEmpty()) {
        return QString();
    }

    const QString trimmedArtist = artist.trimmed();
    const QString composite = trimmedArtist.isEmpty()
        ? trimmedTitle
        : QStringLiteral("%1 - %2").arg(trimmedTitle, trimmedArtist);
    const QByteArray hash = QCryptographicHash::hash(composite.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("meta:%1").arg(QString::fromLatin1(hash));
}

bool LyricService::tryLoadLyricsFromAnyCache(const QStringList &cacheKeys)
{
    for (const QString &key : cacheKeys) {
        if (loadLyricsFromCacheByAlias(key)) {
            m_currentTrackKey = normalizeTrackKey(key);
            m_pendingTrackKey = m_currentTrackKey;
            return true;
        }
    }
    return false;
}

bool LyricService::tryLoadCachedLyricsForPendingKeys()
{
    const QString trackSeed = !m_pendingTrackId.trimmed().isEmpty() ? m_pendingTrackId : m_currentTrackKey;
    QStringList cacheKeys = candidateCacheKeys(trackSeed,
                                              m_pendingServer,
                                              m_pendingOnlineId,
                                              m_pendingTitle,
                                              m_pendingArtist);

    const auto appendKey = [&cacheKeys, this](const QString &key) {
        const QString normalized = normalizeTrackKey(key);
        if (!normalized.isEmpty() && !cacheKeys.contains(normalized)) {
            cacheKeys.append(normalized);
        }
    };

    appendKey(m_currentTrackKey);
    appendKey(m_pendingTrackKey);

    if (tryLoadLyricsFromAnyCache(cacheKeys)) {
        return true;
    }

    return tryLoadLyricsFromCacheByMetadata(m_pendingTitle, m_pendingArtist);
}

void LyricService::removeCacheForKeys(const QStringList &cacheKeys)
{
    QSet<QString> candidateFiles;
    for (const QString &key : cacheKeys) {
        const QString normalized = normalizeTrackKey(key);
        if (normalized.isEmpty()) {
            continue;
        }

        const QString indexedFileName = m_cacheAliasIndex.value(normalized);
        if (!indexedFileName.isEmpty()) {
            m_cacheAliasIndex.remove(normalized);
            candidateFiles.insert(indexedFileName);
            continue;
        }

        const QString cachePath = lyricsCachePathForKey(normalized);
        if (!cachePath.isEmpty()) {
            QFile::remove(cachePath);
        }
    }

    for (const QString &fileName : candidateFiles) {
        bool stillReferenced = false;
        for (auto it = m_cacheAliasIndex.constBegin(); it != m_cacheAliasIndex.constEnd(); ++it) {
            if (it.value() == fileName) {
                stillReferenced = true;
                break;
            }
        }
        if (!stillReferenced) {
            const QString cachePath = lyricsCacheDir() + QLatin1Char('/') + fileName + QStringLiteral(".lrc");
            QFile::remove(cachePath);
        }
    }

    saveAliasesIndex();
}

void LyricService::saveLyricsToCacheAliases(const QStringList &cacheKeys, const QString &lrcContent)
{
    QStringList normalizedKeys;
    normalizedKeys.reserve(cacheKeys.size());
    for (const QString &key : cacheKeys) {
        const QString normalized = normalizeTrackKey(key);
        if (!normalized.isEmpty() && !normalizedKeys.contains(normalized)) {
            normalizedKeys.append(normalized);
        }
    }
    if (normalizedKeys.isEmpty()) {
        return;
    }

    const QString enriched = embedCacheKeysHeader(normalizedKeys, lrcContent);
    const QString primaryKey = normalizedKeys.first();
    const QString cacheFileName = cacheFileNameForKey(primaryKey);
    if (!saveLyricsToCache(primaryKey, enriched)) {
        return;
    }

    registerCacheAliases(cacheFileName, normalizedKeys);
    saveAliasesIndex();
}

QString LyricService::lyricsCachePathForKey(const QString &trackKey) const
{
    const QString cacheFileName = cacheFileNameForKey(trackKey);
    if (cacheFileName.isEmpty()) {
        return QString();
    }
    return lyricsCacheDir() + QLatin1Char('/') + cacheFileName + QStringLiteral(".lrc");
}

QString LyricService::lyricsCacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/lyrics_cache");
}

QString LyricService::aliasesIndexPath() const
{
    return lyricsCacheDir() + QStringLiteral("/aliases.json");
}

QString LyricService::cacheFileNameForKey(const QString &trackKey) const
{
    const QString normalized = normalizeTrackKey(trackKey);
    if (normalized.isEmpty()) {
        return QString();
    }
    return QString::fromLatin1(
        QCryptographicHash::hash(normalized.toUtf8(), QCryptographicHash::Sha1).toHex());
}

void LyricService::loadAliasesIndex()
{
    m_cacheAliasIndex.clear();

    QFile file(aliasesIndexPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject object = doc.object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        const QString alias = normalizeTrackKey(it.key());
        const QString cacheFileName = it.value().toString().trimmed();
        if (!alias.isEmpty() && !cacheFileName.isEmpty()) {
            m_cacheAliasIndex.insert(alias, cacheFileName);
        }
    }
}

void LyricService::saveAliasesIndex()
{
    if (!ensureLyricsCacheDir()) {
        return;
    }

    QJsonObject object;
    for (auto it = m_cacheAliasIndex.constBegin(); it != m_cacheAliasIndex.constEnd(); ++it) {
        object.insert(it.key(), it.value());
    }

    QFile file(aliasesIndexPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    file.close();
}

void LyricService::registerCacheAliases(const QString &cacheFileName, const QStringList &aliasKeys)
{
    for (const QString &alias : aliasKeys) {
        const QString normalized = normalizeTrackKey(alias);
        if (!normalized.isEmpty()) {
            m_cacheAliasIndex.insert(normalized, cacheFileName);
        }
    }
}

QString LyricService::embedCacheKeysHeader(const QStringList &aliasKeys, const QString &lrcContent) const
{
    if (aliasKeys.isEmpty()) {
        return lrcContent;
    }

    QString body = lrcContent;
    if (body.startsWith(QStringLiteral("[xiaoxiong-keys]"))) {
        const int newlineIndex = body.indexOf(QLatin1Char('\n'));
        if (newlineIndex >= 0) {
            body = body.mid(newlineIndex + 1);
        } else {
            body.clear();
        }
    }

    QStringList normalizedKeys;
    for (const QString &key : aliasKeys) {
        const QString normalized = normalizeTrackKey(key);
        if (!normalized.isEmpty() && !normalizedKeys.contains(normalized)) {
            normalizedKeys.append(normalized);
        }
    }

    return QStringLiteral("[xiaoxiong-keys]%1\n%2").arg(normalizedKeys.join(QLatin1Char('|')), body);
}

void LyricService::rebuildAliasesIndexFromCacheFiles()
{
    QDir dir(lyricsCacheDir());
    if (!dir.exists()) {
        return;
    }

    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.lrc")}, QDir::Files);
    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        const QString firstLine = QString::fromUtf8(file.readLine()).trimmed();
        if (firstLine.startsWith(QStringLiteral("[xiaoxiong-keys]"))) {
            const QStringList aliasKeys = firstLine.mid(QStringLiteral("[xiaoxiong-keys]").size()).split(
                QLatin1Char('|'),
                Qt::SkipEmptyParts);
            registerCacheAliases(info.completeBaseName(), aliasKeys);
            file.close();
            continue;
        }

        QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
#endif
        const QString content = stream.readAll();
        file.close();

        const QString metaKey = metaCacheKeyFromLrcTags(content);
        if (!metaKey.isEmpty()) {
            registerCacheAliases(info.completeBaseName(), {metaKey});
        }
    }

    saveAliasesIndex();
}

bool LyricService::loadLyricsFromCacheFile(const QString &cacheFilePath, const QString &labelKey)
{
    QFile file(cacheFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#endif
    const QString content = stream.readAll();
    file.close();

    const QString label = QStringLiteral("缓存歌词: %1").arg(normalizeTrackKey(labelKey));
    if (!applyLyricsContent(content, label)) {
        return false;
    }

    setOnlineFetchState(OnlineFetchState::Idle);
    m_onlineRetryRequired = false;
    setOnlineLastError(QString());
    return true;
}

bool LyricService::loadLyricsFromCacheByAlias(const QString &aliasKey)
{
    const QString normalized = normalizeTrackKey(aliasKey);
    if (normalized.isEmpty()) {
        return false;
    }

    if (loadLyricsFromCache(normalized)) {
        return true;
    }

    const QString indexedFileName = m_cacheAliasIndex.value(normalized);
    if (!indexedFileName.isEmpty()) {
        const QString cachePath = lyricsCacheDir() + QLatin1Char('/') + indexedFileName + QStringLiteral(".lrc");
        if (loadLyricsFromCacheFile(cachePath, normalized)) {
            return true;
        }
    }

    return scanCacheForAliasKey(normalized);
}

bool LyricService::scanCacheForAliasKey(const QString &aliasKey)
{
    QDir dir(lyricsCacheDir());
    if (!dir.exists()) {
        return false;
    }

    const QByteArray needle = aliasKey.toUtf8();
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.lrc")}, QDir::Files);
    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const QByteArray header = file.read(512);
        file.close();
        if (!header.contains(needle)) {
            continue;
        }

        if (loadLyricsFromCacheFile(info.absoluteFilePath(), aliasKey)) {
            registerCacheAliases(info.completeBaseName(), {aliasKey});
            saveAliasesIndex();
            return true;
        }
    }

    return false;
}

QString LyricService::metaCacheKeyFromLrcTags(const QString &lrcContent) const
{
    QString title;
    QString artist;

    for (const QString &rawLine : lrcContent.split(QLatin1Char('\n'))) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(QStringLiteral("[ti:"), Qt::CaseInsensitive)) {
            title = line.mid(4);
            if (title.endsWith(QLatin1Char(']'))) {
                title.chop(1);
            }
            title = title.trimmed();
        } else if (line.startsWith(QStringLiteral("[ar:"), Qt::CaseInsensitive)) {
            artist = line.mid(4);
            if (artist.endsWith(QLatin1Char(']'))) {
                artist.chop(1);
            }
            artist = artist.trimmed();
        }
    }

    return titleArtistCacheKey(title, artist);
}

bool LyricService::tryLoadLyricsFromCacheByMetadata(const QString &title, const QString &artist)
{
    const QString targetKey = titleArtistCacheKey(title, artist);
    if (targetKey.isEmpty()) {
        return false;
    }

    if (loadLyricsFromCacheByAlias(targetKey)) {
        return true;
    }

    QDir dir(lyricsCacheDir());
    if (!dir.exists()) {
        return false;
    }

    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.lrc")}, QDir::Files);
    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
#endif
        const QString content = stream.readAll();
        file.close();

        if (metaCacheKeyFromLrcTags(content) != targetKey) {
            continue;
        }

        if (loadLyricsFromCacheFile(info.absoluteFilePath(), targetKey)) {
            registerCacheAliases(info.completeBaseName(), {targetKey});
            saveAliasesIndex();
            return true;
        }
    }

    return false;
}

bool LyricService::isNetworkLikelyAvailable() const
{
    const QNetworkInformation *info = QNetworkInformation::instance();
    if (!info) {
        return true;
    }

    switch (info->reachability()) {
    case QNetworkInformation::Reachability::Online:
    case QNetworkInformation::Reachability::Unknown:
        return true;
    case QNetworkInformation::Reachability::Disconnected:
    case QNetworkInformation::Reachability::Local:
        return false;
    default:
        return false;
    }
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

    return loadLyricsFromCacheFile(cachePath, trackKey);
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
    if (!m_lyricsSynced || m_lines.isEmpty()) {
        return;
    }

    const int nextIndex = lineIndexForPosition(positionMs);
    if (m_currentLineIndex == nextIndex) {
        return;
    }

    m_currentLineIndex = nextIndex;
    emit currentLineIndexChanged();
    emit currentLineTextChanged();
}
