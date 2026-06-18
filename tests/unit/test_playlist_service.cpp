#include <QtTest>

#include "../../src/playlist/playlist_service.h"

class TestPlaylistService : public QObject
{
    Q_OBJECT

private slots:
    void shouldCreatePlaylist();
    void shouldAddTrackToPlaylist();
};

void TestPlaylistService::shouldCreatePlaylist()
{
    PlaylistService service;

    QVERIFY(service.createPlaylist(QStringLiteral("我的收藏")));

    const QVariantList playlists = service.playlists();
    QCOMPARE(playlists.size(), 2);

    bool foundUserPlaylist = false;
    for (const QVariant &value : playlists) {
        const QVariantMap playlist = value.toMap();
        if (playlist.value(QStringLiteral("name")).toString() == QStringLiteral("我的收藏")) {
            foundUserPlaylist = true;
            QVERIFY(!playlist.value(QStringLiteral("id")).toString().isEmpty());
            QVERIFY(!playlist.value(QStringLiteral("isSystem")).toBool());
        }
    }
    QVERIFY(foundUserPlaylist);
}

void TestPlaylistService::shouldAddTrackToPlaylist()
{
    PlaylistService service;
    QVERIFY(service.createPlaylist(QStringLiteral("通勤")));

    QString playlistId;
    const QVariantList playlists = service.playlists();
    for (const QVariant &value : playlists) {
        const QVariantMap playlist = value.toMap();
        if (playlist.value(QStringLiteral("name")).toString() == QStringLiteral("通勤")) {
            playlistId = playlist.value(QStringLiteral("id")).toString();
            break;
        }
    }
    QVERIFY(!playlistId.isEmpty());

    QVERIFY(service.addTrack(playlistId, QStringLiteral("D:/Music/night.mp3"), QStringLiteral("Night Song")));

    const QVariantList tracks = service.tracks(playlistId);
    QCOMPARE(tracks.size(), 1);

    const QVariantMap track = tracks.first().toMap();
    QCOMPARE(track.value(QStringLiteral("path")).toString(), QStringLiteral("D:/Music/night.mp3"));
    QCOMPARE(track.value(QStringLiteral("title")).toString(), QStringLiteral("Night Song"));
}

QTEST_MAIN(TestPlaylistService)
#include "test_playlist_service.moc"

