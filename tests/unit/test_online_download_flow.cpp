#include "../../src/artwork/cover_art_service.h"
#include "../../src/artwork/library_cover_prefetcher.h"
#include "../../src/download/online_download_service.h"
#include "../../src/library/library_repository.h"
#include "../../src/settings/settings_service.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class OnlineDownloadFlowTest : public QObject
{
    Q_OBJECT

private slots:
    void addTrackFromPath_withUnicodePath_doesNotCrash();
    void downloadTrack_withDirectHttpUrl_completesWithoutCrash();
    void downloadTrack_withMetingStreamUrl_completesWithoutCrash();
    void concurrentCoverRequests_doNotCrash();
};

void OnlineDownloadFlowTest::addTrackFromPath_withUnicodePath_doesNotCrash()
{
    const QString sourcePath =
        QStringLiteral("/home/xgh/Music/Downloads/竹内まりや - Plastic Love (New-Remix) - 26127770.mp3");
    if (!QFileInfo::exists(sourcePath)) {
        QSKIP("Sample downloaded mp3 not found on this machine");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString targetPath = tempDir.path() + QStringLiteral("/copy.mp3");
    QVERIFY(QFile::copy(sourcePath, targetPath));

    LibraryCoverPrefetcher prefetcher;
    CoverArtService coverArt;
    prefetcher.setCoverArtService(&coverArt);

    LibraryRepository repository;
    repository.setCoverPrefetcher(&prefetcher);

    QSignalSpy updatedSpy(&repository, &LibraryRepository::tracksUpdated);
    const int beforeCount = repository.trackCount();
    QVERIFY(repository.addTrackFromPath(targetPath));
    QVERIFY(updatedSpy.wait(5000));
    QCOMPARE(repository.trackCount(), beforeCount + 1);

    coverArt.cacheCoverFromUrl(QFileInfo(targetPath).absoluteFilePath(),
                               QStringLiteral("https://example.com/cover.jpg"));
    QTest::qWait(500);
}

void OnlineDownloadFlowTest::downloadTrack_withDirectHttpUrl_completesWithoutCrash()
{
    const QString sourcePath =
        QStringLiteral("/home/xgh/Music/Downloads/竹内まりや - Plastic Love (New-Remix) - 26127770.mp3");
    if (!QFileInfo::exists(sourcePath)) {
        QSKIP("Sample downloaded mp3 not found on this machine");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    SettingsService settings;
    settings.setDefaultDownloadDirectory(tempDir.path());
    settings.save();

    LibraryCoverPrefetcher prefetcher;
    CoverArtService coverArt;
    prefetcher.setCoverArtService(&coverArt);

    LibraryRepository repository;
    repository.setCoverPrefetcher(&prefetcher);

    OnlineDownloadService downloadService;
    downloadService.setSettingsService(&settings);
    downloadService.setLibraryRepository(&repository);
    downloadService.setCoverArtService(&coverArt);

    QSignalSpy finishedSpy(&downloadService, &OnlineDownloadService::downloadFinished);
    QSignalSpy failedSpy(&downloadService, &OnlineDownloadService::downloadFailed);

    const int beforeCount = repository.trackCount();
    const QUrl fileUrl = QUrl::fromLocalFile(sourcePath);
    QVariantMap track{
        {QStringLiteral("title"), QStringLiteral("Plastic Love Test")},
        {QStringLiteral("artist"), QStringLiteral("竹内まりや")},
        {QStringLiteral("onlineId"), QStringLiteral("99999999")},
        {QStringLiteral("streamUrl"), fileUrl.toString()},
        {QStringLiteral("picUrl"), QString()},
    };

    downloadService.downloadTrack(track);

    QVERIFY(finishedSpy.wait(15000) || failedSpy.wait(1));
    if (!finishedSpy.isEmpty()) {
        QCOMPARE(repository.trackCount(), beforeCount + 1);
    } else {
        qWarning() << "download failed:" << failedSpy.at(0).at(0).toString();
    }
    QTest::qWait(1000);
}

void OnlineDownloadFlowTest::downloadTrack_withMetingStreamUrl_completesWithoutCrash()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    SettingsService settings;
    settings.setDefaultDownloadDirectory(tempDir.path());
    settings.save();

    LibraryCoverPrefetcher prefetcher;
    CoverArtService coverArt;
    prefetcher.setCoverArtService(&coverArt);

    LibraryRepository repository;
    repository.setCoverPrefetcher(&prefetcher);

    OnlineDownloadService downloadService;
    downloadService.setSettingsService(&settings);
    downloadService.setLibraryRepository(&repository);
    downloadService.setCoverArtService(&coverArt);

    QSignalSpy finishedSpy(&downloadService, &OnlineDownloadService::downloadFinished);
    QSignalSpy failedSpy(&downloadService, &OnlineDownloadService::downloadFailed);

    const int beforeCount = repository.trackCount();
    const QUrl streamApiUrl =
        QUrl(QStringLiteral("https://meting-api-omega.vercel.app/api?server=netease&type=url&id=26127770"));
    if (!streamApiUrl.isValid()) {
        QSKIP("Invalid meting stream API URL");
    }

    QVariantMap track{
        {QStringLiteral("title"), QStringLiteral("Plastic Love")},
        {QStringLiteral("artist"), QStringLiteral("竹内まりや")},
        {QStringLiteral("onlineId"), QStringLiteral("26127770")},
        {QStringLiteral("streamUrl"), streamApiUrl.toString()},
        {QStringLiteral("picUrl"), QString()},
    };

    downloadService.downloadTrack(track);

    const bool finished = finishedSpy.wait(60000);
    const bool failed = !failedSpy.isEmpty();
    if (!finished && !failed) {
        QSKIP("Meting API unavailable in this environment");
    }
    if (finished) {
        QCOMPARE(repository.trackCount(), beforeCount + 1);
    } else {
        qWarning() << "meting download failed:" << failedSpy.at(0).at(0).toString();
    }
    QTest::qWait(1000);
}

void OnlineDownloadFlowTest::concurrentCoverRequests_doNotCrash()
{
    CoverArtService coverArt;
    coverArt.cacheCoverFromUrl(QStringLiteral("/tmp/track-a.mp3"),
                               QStringLiteral("https://example.com/a.jpg"));
    coverArt.cacheCoverFromUrl(QStringLiteral("/tmp/track-b.mp3"),
                               QStringLiteral("https://example.com/b.jpg"));
    QTest::qWait(500);

    const QString sourcePath =
        QStringLiteral("/home/xgh/Music/Downloads/竹内まりや - Plastic Love (New-Remix) - 26127770.mp3");
    if (!QFileInfo::exists(sourcePath)) {
        return;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return;
    }

    LibraryCoverPrefetcher prefetcher;
    CoverArtService sharedCoverArt;
    prefetcher.setCoverArtService(&sharedCoverArt);

    LibraryRepository repository;
    repository.setCoverPrefetcher(&prefetcher);

    const QString targetPath = tempDir.path() + QStringLiteral("/copy.mp3");
    QVERIFY(QFile::copy(sourcePath, targetPath));

    sharedCoverArt.cacheCoverFromUrl(QFileInfo(targetPath).absoluteFilePath(),
                                     QStringLiteral("https://example.com/cover.jpg"));
    QVERIFY(repository.addTrackFromPath(targetPath));
    QTest::qWait(500);
}

QTEST_MAIN(OnlineDownloadFlowTest)
#include "test_online_download_flow.moc"
