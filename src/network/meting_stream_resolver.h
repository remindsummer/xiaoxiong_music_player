#pragma once

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

// 解析 Meting stream API URL 为真实可播放/可下载地址。
class MetingStreamResolver : public QObject
{
    Q_OBJECT

public:
    explicit MetingStreamResolver(QObject *parent = nullptr);

    void resolve(const QString &streamApiUrl);

signals:
    void resolved(const QUrl &streamUrl);
    void failed(const QString &message);

private:
    void handleReply(QNetworkReply *reply);

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
};
