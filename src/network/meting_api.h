#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

class QNetworkRequest;

namespace MetingApi {

// 内置默认 Meting API 镜像（每行一个，供设置页展示与回退）。
QStringList defaultApiBases();

QUrl buildUrl(const QString &server, const QString &type, const QString &id);

// 从 Meting 歌曲 JSON 提取平台歌曲 ID（优先 id 字段，否则从 url 参数解析）。
QString extractSongId(const QJsonObject &song);

// 将 Meting 搜索/歌曲 JSON 规范化为播放器 track map。
QVariantMap songToTrackMap(const QJsonObject &song, const QString &server);

// 从 Meting 歌曲 JSON 读取标题与歌手（兼容 title/name、author/artist）。
QString songTitle(const QJsonObject &song);
QString songArtist(const QJsonObject &song);

// 构造 search 关键词（歌名 + 歌手）。
QString buildSearchKeyword(const QString &title, const QString &artist);

// 从路径/标题/online:key 中提取嵌入的网易云歌曲 ID（如文件名末尾 - 26127770）。
QString extractEmbeddedNeteaseId(const QString &text);

struct MetingSearchParseResult {
    QJsonArray songs;
    QString errorMessage;
    bool shouldTryNextMirror = false;
};

// 解析 Meting search 响应；镜像不支持 search 时会标记 shouldTryNextMirror。
MetingSearchParseResult parseSearchResponse(const QByteArray &jsonData);

// 从 Meting search JSON 响应中提取首条结果的 pic URL，失败返回空。
QString picUrlFromSearchResponse(const QByteArray &jsonData);

// 确保镜像列表包含当前可用的 search 镜像（用于升级旧配置）。
QStringList ensureWorkingApiMirrors(const QStringList &bases);

void applyCommonRequestHeaders(QNetworkRequest &request);

// 当前 Meting API 根地址（含 /api 或 /meting 路径）。
QString apiBase();

// 当前生效的全部镜像（只读副本）。
QStringList apiBases();

// 从 settings.json 注入镜像列表；空或无效时回退到 defaultApiBases()。
void setApiBases(const QStringList &bases);

// 将旧镜像/失效域名上的 Meting 链接重写为当前 apiBase。
QUrl rewriteServiceUrl(const QUrl &url);

// 当前根地址请求失败时切换到下一个镜像，成功返回 true。
bool tryNextApiBase();

void resetApiBase();

} // namespace MetingApi
