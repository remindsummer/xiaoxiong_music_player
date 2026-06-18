#include "meting_api.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace MetingApi {

namespace {

QStringList s_apiBases;
int s_apiBaseIndex = 0;

QString normalizeApiBase(const QString &base)
{
    QString normalized = base.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized;
}

void ensureApiBasesInitialized()
{
    if (s_apiBases.isEmpty()) {
        s_apiBases = defaultApiBases();
    }
}

bool isKnownDeadMetingHost(const QString &host)
{
    return host.contains(QStringLiteral("mikus.ink"), Qt::CaseInsensitive);
}

bool looksLikeMetingServiceUrl(const QUrl &url)
{
    if (!url.isValid()) {
        return false;
    }
    const QUrlQuery query(url);
    return query.hasQueryItem(QStringLiteral("server"))
           && query.hasQueryItem(QStringLiteral("type"))
           && query.hasQueryItem(QStringLiteral("id"));
}

} // namespace

QStringList defaultApiBases()
{
    return {
        QStringLiteral("https://meting.elysium-stack.cn/api"),
        QStringLiteral("https://api.injahow.cn/meting"),
    };
}

QString apiBase()
{
    ensureApiBasesInitialized();
    if (s_apiBaseIndex < 0 || s_apiBaseIndex >= s_apiBases.size()) {
        s_apiBaseIndex = 0;
    }
    return s_apiBases.at(s_apiBaseIndex);
}

QStringList apiBases()
{
    ensureApiBasesInitialized();
    return s_apiBases;
}

void setApiBases(const QStringList &bases)
{
    QStringList sanitized;
    sanitized.reserve(bases.size());
    for (const QString &base : bases) {
        const QString normalized = normalizeApiBase(base);
        if (!normalized.isEmpty()
            && normalized.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)) {
            sanitized.append(normalized);
        }
    }

    if (sanitized.isEmpty()) {
        sanitized = defaultApiBases();
    }

    s_apiBases = sanitized;
    s_apiBaseIndex = 0;
}

bool tryNextApiBase()
{
    ensureApiBasesInitialized();
    if (s_apiBaseIndex + 1 >= s_apiBases.size()) {
        return false;
    }
    ++s_apiBaseIndex;
    return true;
}

void resetApiBase()
{
    s_apiBaseIndex = 0;
}

QUrl rewriteServiceUrl(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty() || !looksLikeMetingServiceUrl(url)) {
        return url;
    }

    // 可用镜像返回的 url/lrc/pic 带 auth 参数，必须原样保留。
    if (!isKnownDeadMetingHost(url.host())) {
        return url;
    }

    const QUrlQuery query(url);
    return buildUrl(query.queryItemValue(QStringLiteral("server")),
                    query.queryItemValue(QStringLiteral("type")),
                    query.queryItemValue(QStringLiteral("id")));
}

QUrl buildUrl(const QString &server, const QString &type, const QString &id)
{
    QUrl url(apiBase());
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("server"), server);
    query.addQueryItem(QStringLiteral("type"), type);
    query.addQueryItem(QStringLiteral("id"), id);
    url.setQuery(query);
    return url;
}

QString extractSongId(const QJsonObject &song)
{
    const QString directId = song.value(QStringLiteral("id")).toString().trimmed();
    if (!directId.isEmpty()) {
        return directId;
    }

    const QString streamUrl = song.value(QStringLiteral("url")).toString().trimmed();
    if (streamUrl.isEmpty()) {
        return QString();
    }

    const QUrl url(streamUrl);
    if (!url.isValid()) {
        return QString();
    }
    return QUrlQuery(url.query()).queryItemValue(QStringLiteral("id")).trimmed();
}

QString songTitle(const QJsonObject &song)
{
    QString title = song.value(QStringLiteral("title")).toString().trimmed();
    if (title.isEmpty()) {
        title = song.value(QStringLiteral("name")).toString().trimmed();
    }
    return title;
}

QString songArtist(const QJsonObject &song)
{
    QString artist = song.value(QStringLiteral("author")).toString().trimmed();
    if (artist.isEmpty()) {
        artist = song.value(QStringLiteral("artist")).toString().trimmed();
    }
    return artist;
}

QVariantMap songToTrackMap(const QJsonObject &song, const QString &server)
{
    const QString safeServer = server.trimmed().isEmpty() ? QStringLiteral("netease") : server.trimmed();
    const QString onlineId = extractSongId(song);
    const QString title = songTitle(song);
    const QString artist = songArtist(song);
    const QString streamUrl = rewriteServiceUrl(QUrl(song.value(QStringLiteral("url")).toString().trimmed())).toString();
    const QString lrcUrl = rewriteServiceUrl(QUrl(song.value(QStringLiteral("lrc")).toString().trimmed())).toString();
    const QString picUrl = rewriteServiceUrl(QUrl(song.value(QStringLiteral("pic")).toString().trimmed())).toString();

    return QVariantMap{
        {QStringLiteral("isOnline"), true},
        {QStringLiteral("onlineId"), onlineId},
        {QStringLiteral("server"), safeServer},
        {QStringLiteral("title"), title},
        {QStringLiteral("artist"), artist},
        {QStringLiteral("path"), QStringLiteral("online:%1:%2").arg(safeServer, onlineId)},
        {QStringLiteral("streamUrl"), streamUrl},
        {QStringLiteral("lrcUrl"), lrcUrl},
        {QStringLiteral("picUrl"), picUrl},
        {QStringLiteral("durationMs"), 0},
    };
}

QString buildSearchKeyword(const QString &title, const QString &artist)
{
    const QString trimmedTitle = title.trimmed();
    const QString trimmedArtist = artist.trimmed();
    if (trimmedTitle.isEmpty()) {
        return trimmedArtist;
    }
    if (trimmedArtist.isEmpty()
        || trimmedArtist == QStringLiteral("Unknown Artist")) {
        return trimmedTitle;
    }
    return QStringLiteral("%1 %2").arg(trimmedTitle, trimmedArtist);
}

QString picUrlFromSearchResponse(const QByteArray &jsonData)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return QString();
    }

    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject songObj = value.toObject();
        if (MetingApi::extractSongId(songObj).isEmpty()) {
            continue;
        }
        const QString picUrl = songObj.value(QStringLiteral("pic")).toString().trimmed();
        if (!picUrl.isEmpty()) {
            return rewriteServiceUrl(QUrl(picUrl)).toString();
        }
    }
    return QString();
}

void applyCommonRequestHeaders(QNetworkRequest &request)
{
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("XiaoxiongMusicPlayer/1.0"));
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30000);
}

} // namespace MetingApi
