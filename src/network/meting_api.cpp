#include "meting_api.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QRegularExpression>
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
        QStringLiteral("https://music-api.shinegh.cn/api"),
        QStringLiteral("https://meting-api-omega.vercel.app/api"),
        QStringLiteral("https://api.injahow.cn/meting"),
        QStringLiteral("https://meting.elysium-stack.cn/api"),
    };
}

QStringList ensureWorkingApiMirrors(const QStringList &bases)
{
    static const QString kPrimaryMirror = QStringLiteral("https://music-api.shinegh.cn/api");
    for (const QString &base : bases) {
        if (normalizeApiBase(base) == kPrimaryMirror) {
            return bases;
        }
    }

    QStringList upgraded = bases;
    upgraded.prepend(kPrimaryMirror);
    return upgraded;
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

    s_apiBases = ensureWorkingApiMirrors(sanitized);
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

QString extractEmbeddedNeteaseId(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    if (trimmed.startsWith(QStringLiteral("online:"), Qt::CaseInsensitive)) {
        const QStringList parts = trimmed.split(QLatin1Char(':'));
        if (parts.size() >= 3) {
            const QString id = parts.at(2).trimmed();
            if (!id.isEmpty() && id.toLongLong() > 0) {
                return id;
            }
        }
    }

    static const QRegularExpression trailingIdInName(
        QStringLiteral(" - (\\d{5,})$"));
    static const QRegularExpression trailingIdBeforeExt(
        QStringLiteral("-(\\d{5,})\\.(?:mp3|flac|ogg|wav|m4a|aac|wma|opus)$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch nameMatch = trailingIdInName.match(trimmed);
    if (nameMatch.hasMatch()) {
        return nameMatch.captured(1);
    }

    const QRegularExpressionMatch pathMatch = trailingIdBeforeExt.match(trimmed);
    if (pathMatch.hasMatch()) {
        return pathMatch.captured(1);
    }

    const QString fileName = QFileInfo(trimmed).completeBaseName();
    if (!fileName.isEmpty() && fileName != trimmed) {
        return extractEmbeddedNeteaseId(fileName);
    }

    return QString();
}

QString buildSearchKeyword(const QString &title, const QString &artist)
{
    QString trimmedTitle = title.trimmed();
    const QString trimmedArtist = artist.trimmed();

    const QString embeddedId = extractEmbeddedNeteaseId(trimmedTitle);
    if (!embeddedId.isEmpty()) {
        static const QRegularExpression trailingIdInName(QStringLiteral(" - \\d{5,}$"));
        trimmedTitle.remove(trailingIdInName);
        trimmedTitle = trimmedTitle.trimmed();
    }

    if (trimmedTitle.isEmpty()) {
        return trimmedArtist;
    }
    if (trimmedArtist.isEmpty()
        || trimmedArtist == QStringLiteral("Unknown Artist")) {
        return trimmedTitle;
    }
    return QStringLiteral("%1 %2").arg(trimmedTitle, trimmedArtist);
}

MetingSearchParseResult parseSearchResponse(const QByteArray &jsonData)
{
    MetingSearchParseResult result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.errorMessage = QStringLiteral("JSON 解析失败");
        result.shouldTryNextMirror = true;
        return result;
    }

    if (doc.isArray()) {
        result.songs = doc.array();
        return result;
    }

    if (!doc.isObject()) {
        result.errorMessage = QStringLiteral("返回格式异常");
        result.shouldTryNextMirror = true;
        return result;
    }

    const QJsonObject object = doc.object();
    if (object.contains(QStringLiteral("error"))) {
        result.errorMessage = object.value(QStringLiteral("error")).toString();
        if (result.errorMessage.isEmpty()) {
            result.errorMessage = QStringLiteral("镜像不支持该请求");
        }
        result.shouldTryNextMirror = true;
        return result;
    }

    const QJsonValue dataValue = object.value(QStringLiteral("data"));
    if (dataValue.isArray()) {
        result.songs = dataValue.toArray();
        return result;
    }

    const QString message = object.value(QStringLiteral("message")).toString();
    result.errorMessage = message.isEmpty() ? QStringLiteral("在线请求被拒绝") : message;
    result.shouldTryNextMirror = true;
    return result;
}

QString picUrlFromSearchResponse(const QByteArray &jsonData)
{
    const MetingSearchParseResult parsed = parseSearchResponse(jsonData);
    const QJsonArray array = parsed.songs;
    if (array.isEmpty()) {
        return QString();
    }
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
