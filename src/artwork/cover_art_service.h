#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class CoverArtService : public QObject
{
    Q_OBJECT

public:
    explicit CoverArtService(QObject *parent = nullptr);

    Q_INVOKABLE void requestCover(const QString &trackKey,
                                  const QString &picUrl,
                                  const QString &server = QString(),
                                  const QString &onlineId = QString());
    Q_INVOKABLE void cacheCoverFromUrl(const QString &trackKey, const QString &picUrl);
    Q_INVOKABLE QString cachedCoverPath(const QString &trackKey) const;
    // 将原始图片字节写入 cover_cache，成功返回 file:// 路径。
    Q_INVOKABLE QString saveCoverBytes(const QString &trackKey, const QByteArray &imageData);

signals:
    void coverReady(const QString &trackKey, const QString &coverPath);

private:
    QString normalizeTrackKey(const QString &trackKey) const;
    QString cacheFilePathForKey(const QString &trackKey) const;
    bool ensureCacheDir() const;
    void startDownload(const QString &trackKey, const QUrl &url);

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_pendingTrackKey;
};
