#include "search_service.h"

#include "../network/meting_api.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QVariantMap>

#include <algorithm>

SearchService::SearchService(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

QString SearchService::onlineSearchState() const
{
    switch (m_onlineSearchState) {
    case OnlineSearchState::Idle:
        return QStringLiteral("idle");
    case OnlineSearchState::Searching:
        return QStringLiteral("searching");
    case OnlineSearchState::Ready:
        return QStringLiteral("ready");
    case OnlineSearchState::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("idle");
}

QVariantList SearchService::onlineResults() const
{
    return m_onlineResults;
}

QString SearchService::onlineLastError() const
{
    return m_onlineLastError;
}

QVariantList SearchService::searchTracks(const QVariantList &tracks, const QString &keyword) const
{
    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        return tracks;
    }

    struct SearchResult {
        QVariantMap trackMap;
        int matchPriority = 3;
        int sourceIndex = -1;
    };

    QList<SearchResult> matches;
    matches.reserve(tracks.size());

    for (qsizetype i = 0; i < tracks.size(); ++i) {
        const QVariantMap trackMap = tracks.at(i).toMap();
        if (trackMap.isEmpty()) {
            continue;
        }

        const QString title = trackMap.value(QStringLiteral("title")).toString();
        const QString artist = trackMap.value(QStringLiteral("artist")).toString();
        const QString album = trackMap.value(QStringLiteral("album")).toString();

        int matchPriority = 3;
        if (title.contains(trimmedKeyword, Qt::CaseInsensitive)) {
            matchPriority = 0;
        } else if (artist.contains(trimmedKeyword, Qt::CaseInsensitive)) {
            matchPriority = 1;
        } else if (album.contains(trimmedKeyword, Qt::CaseInsensitive)) {
            matchPriority = 2;
        } else {
            continue;
        }

        SearchResult result;
        result.trackMap = trackMap;
        result.matchPriority = matchPriority;
        result.sourceIndex = static_cast<int>(i);
        matches.append(result);
    }

    std::stable_sort(matches.begin(), matches.end(), [](const SearchResult &lhs, const SearchResult &rhs) {
        if (lhs.matchPriority != rhs.matchPriority) {
            return lhs.matchPriority < rhs.matchPriority;
        }

        const QString lhsTitle = lhs.trackMap.value(QStringLiteral("title")).toString();
        const QString rhsTitle = rhs.trackMap.value(QStringLiteral("title")).toString();
        const int titleOrder = QString::compare(lhsTitle, rhsTitle, Qt::CaseInsensitive);
        if (titleOrder != 0) {
            return titleOrder < 0;
        }

        const QString lhsArtist = lhs.trackMap.value(QStringLiteral("artist")).toString();
        const QString rhsArtist = rhs.trackMap.value(QStringLiteral("artist")).toString();
        const int artistOrder = QString::compare(lhsArtist, rhsArtist, Qt::CaseInsensitive);
        if (artistOrder != 0) {
            return artistOrder < 0;
        }

        const QString lhsAlbum = lhs.trackMap.value(QStringLiteral("album")).toString();
        const QString rhsAlbum = rhs.trackMap.value(QStringLiteral("album")).toString();
        const int albumOrder = QString::compare(lhsAlbum, rhsAlbum, Qt::CaseInsensitive);
        if (albumOrder != 0) {
            return albumOrder < 0;
        }

        return lhs.sourceIndex < rhs.sourceIndex;
    });

    QVariantList result;
    result.reserve(matches.size());
    for (const SearchResult &match : matches) {
        result.append(match.trackMap);
    }
    return result;
}

void SearchService::searchOnline(const QString &keyword, const QString &server)
{
    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        setOnlineSearchState(OnlineSearchState::Failed);
        setOnlineLastError(QStringLiteral("请输入搜索关键词"));
        return;
    }

    cancelOnlineSearch();
    setOnlineSearchState(OnlineSearchState::Searching);
    setOnlineLastError(QString());
    MetingApi::resetApiBase();

    const QString safeServer = server.trimmed().isEmpty() ? QStringLiteral("netease") : server.trimmed();
    m_pendingSearchKeyword = trimmedKeyword;
    m_pendingSearchServer = safeServer;
    if (safeServer != QStringLiteral("netease")) {
        setOnlineSearchState(OnlineSearchState::Failed);
        setOnlineLastError(QStringLiteral("在线搜歌目前仅支持网易云音乐（netease）"));
        return;
    }

    QNetworkRequest request(MetingApi::buildUrl(safeServer, QStringLiteral("search"), trimmedKeyword));
    MetingApi::applyCommonRequestHeaders(request);

    m_currentReply = m_network->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_currentReply;
        if (!reply) {
            return;
        }
        m_currentReply = nullptr;
        handleOnlineSearchReply(reply);
        reply->deleteLater();
    });
}

void SearchService::cancelOnlineSearch()
{
    if (QNetworkReply *staleReply = m_currentReply) {
        m_currentReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }
}

void SearchService::clearOnlineResults()
{
    if (m_onlineResults.isEmpty()) {
        return;
    }
    m_onlineResults.clear();
    emit onlineResultsChanged();
    setOnlineSearchState(OnlineSearchState::Idle);
}

void SearchService::handleOnlineSearchReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        if (MetingApi::tryNextApiBase()) {
            searchOnline(m_pendingSearchKeyword, m_pendingSearchServer);
            return;
        }
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString detail = reply->errorString();
        if (httpCode > 0) {
            detail = QStringLiteral("HTTP %1 - %2").arg(httpCode).arg(detail);
        }
        setOnlineSearchState(OnlineSearchState::Failed);
        setOnlineLastError(QStringLiteral("在线搜歌失败：%1").arg(detail));
        return;
    }

    const QByteArray data = reply->readAll();
    const MetingApi::MetingSearchParseResult parsed = MetingApi::parseSearchResponse(data);
    if (parsed.shouldTryNextMirror) {
        if (MetingApi::tryNextApiBase()) {
            searchOnline(m_pendingSearchKeyword, m_pendingSearchServer);
            return;
        }
    }

    if (parsed.songs.isEmpty()) {
        setOnlineSearchState(OnlineSearchState::Failed);
        setOnlineLastError(parsed.errorMessage.isEmpty()
                               ? QStringLiteral("在线搜歌返回格式异常")
                               : parsed.errorMessage);
        return;
    }

    const QUrl requestUrl = reply->request().url();
    const QString server = QUrlQuery(requestUrl.query()).queryItemValue(QStringLiteral("server"));
    const QString safeServer = server.isEmpty() ? QStringLiteral("netease") : server;

    QVariantList results;
    const QJsonArray array = parsed.songs;
    results.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject songObj = value.toObject();
        if (MetingApi::extractSongId(songObj).isEmpty()) {
            continue;
        }
        results.append(MetingApi::songToTrackMap(songObj, safeServer));
    }

    m_onlineResults = results;
    emit onlineResultsChanged();

    if (m_onlineResults.isEmpty()) {
        setOnlineSearchState(OnlineSearchState::Failed);
        setOnlineLastError(QStringLiteral("未找到匹配的在线歌曲"));
        return;
    }

    setOnlineSearchState(OnlineSearchState::Ready);
}

void SearchService::setOnlineSearchState(OnlineSearchState state)
{
    if (m_onlineSearchState == state) {
        return;
    }
    m_onlineSearchState = state;
    emit onlineSearchStateChanged();
}

void SearchService::setOnlineLastError(const QString &message)
{
    if (m_onlineLastError == message) {
        return;
    }
    m_onlineLastError = message;
    emit onlineLastErrorChanged();
}
