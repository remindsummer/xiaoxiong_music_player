#pragma once

#include <QObject>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;

class SearchService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString onlineSearchState READ onlineSearchState NOTIFY onlineSearchStateChanged)
    Q_PROPERTY(QVariantList onlineResults READ onlineResults NOTIFY onlineResultsChanged)
    Q_PROPERTY(QString onlineLastError READ onlineLastError NOTIFY onlineLastErrorChanged)

public:
    explicit SearchService(QObject *parent = nullptr);

    QString onlineSearchState() const;
    QVariantList onlineResults() const;
    QString onlineLastError() const;

    Q_INVOKABLE QVariantList searchTracks(const QVariantList &tracks, const QString &keyword) const;
    // 通过 Meting API 在线搜歌（目前 search 类型仅网易云支持）。
    Q_INVOKABLE void searchOnline(const QString &keyword, const QString &server = QStringLiteral("netease"));
    Q_INVOKABLE void cancelOnlineSearch();
    Q_INVOKABLE void clearOnlineResults();

signals:
    void onlineSearchStateChanged();
    void onlineResultsChanged();
    void onlineLastErrorChanged();

private:
    enum class OnlineSearchState {
        Idle,
        Searching,
        Ready,
        Failed
    };

    void setOnlineSearchState(OnlineSearchState state);
    void setOnlineLastError(const QString &message);
    void handleOnlineSearchReply(QNetworkReply *reply);
    static QVariantMap metingSongToTrackMap(const QVariantMap &song, const QString &server);

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_currentReply = nullptr;
    OnlineSearchState m_onlineSearchState = OnlineSearchState::Idle;
    QVariantList m_onlineResults;
    QString m_onlineLastError;
    QString m_pendingSearchKeyword;
    QString m_pendingSearchServer;
};
