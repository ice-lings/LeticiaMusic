#include <QtTest/QtTest>
#include <QGuiApplication>
#include <cstdio>
#include "sync/sync_worker.h"
#include "sync/sync_manifest.h"

static void testMsgHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(ctx)
    fprintf(stderr, "%s\n", qPrintable(msg));
    fflush(stderr);
}

static ManifestFileEntry makeEntry(const QString& path, qint64 size, qint64 modified)
{
    ManifestFileEntry e;
    e.relativePath = path;
    e.size = size;
    e.modified = modified;
    return e;
}

class TestSyncCompare : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyBoth()
    {
        SyncWorker w;
        w.setLocalManifest(SyncManifest());
        SyncPlan p = w.compareManifests(SyncManifest(), {});
        QCOMPARE(p.filesToUpload.size(), 0);
        QCOMPARE(p.filesToDownload.size(), 0);
        QCOMPARE(p.filesToDelete.size(), 0);
        QCOMPARE(p.totalOps(), 0);
    }

    void testLocalOnly()
    {
        SyncManifest local;
        local.files["hash_A"] = makeEntry("music/song.mp3", 5000, 100);

        SyncWorker w;
        w.setLocalManifest(local);
        SyncPlan p = w.compareManifests(SyncManifest(), {});

        QCOMPARE(p.filesToUpload.size(), 1);
        QCOMPARE(p.filesToUpload[0].hash, QString("hash_A"));
        QCOMPARE(p.filesToUpload[0].canonicalPath, QString("music/song.mp3"));
        QCOMPARE(p.filesToUpload[0].size, (qint64)5000);
        QCOMPARE(p.filesToUpload[0].modified, (qint64)100);
        QCOMPARE(p.filesToDownload.size(), 0);
        QCOMPARE(p.filesToDelete.size(), 0);
    }

    void testRemoteOnly()
    {
        SyncManifest remote;
        remote.files["hash_B"] = makeEntry("music/remote.mp3", 3000, 200);

        SyncWorker w;
        w.setLocalManifest(SyncManifest());
        SyncPlan p = w.compareManifests(remote, {});

        QCOMPARE(p.filesToUpload.size(), 0);
        QCOMPARE(p.filesToDownload.size(), 1);
        QCOMPARE(p.filesToDownload[0].hash, QString("hash_B"));
        QCOMPARE(p.filesToDownload[0].canonicalPath, QString("music/remote.mp3"));
        QCOMPARE(p.filesToDownload[0].size, (qint64)3000);
        QCOMPARE(p.filesToDownload[0].modified, (qint64)200);
        QCOMPARE(p.filesToDelete.size(), 0);
    }

    void testBothSameModified()
    {
        SyncManifest local, remote;
        local.files["hash_C"] = makeEntry("music/same.mp3", 4000, 100);
        remote.files["hash_C"] = makeEntry("music/same.mp3", 4000, 100);

        SyncWorker w;
        w.setLocalManifest(local);
        SyncPlan p = w.compareManifests(remote, {});

        QCOMPARE(p.filesToUpload.size(), 0);
        QCOMPARE(p.filesToDownload.size(), 0);
        QCOMPARE(p.filesToDelete.size(), 0);
        QCOMPARE(p.totalOps(), 0);
    }

    void testRemoteNewer()
    {
        SyncManifest local, remote;
        local.files["hash_D"] = makeEntry("music/newer.mp3", 4000, 100);
        remote.files["hash_D"] = makeEntry("music/newer.mp3", 5000, 200);

        SyncWorker w;
        w.setLocalManifest(local);
        SyncPlan p = w.compareManifests(remote, {});

        QCOMPARE(p.filesToUpload.size(), 0);
        QCOMPARE(p.filesToDownload.size(), 1);
        QCOMPARE(p.filesToDownload[0].hash, QString("hash_D"));
        QCOMPARE(p.filesToDownload[0].modified, (qint64)200);
    }

    void testLocalNewer()
    {
        SyncManifest local, remote;
        local.files["hash_E"] = makeEntry("music/local_newer.mp3", 4000, 200);
        remote.files["hash_E"] = makeEntry("music/local_newer.mp3", 3000, 100);

        SyncWorker w;
        w.setLocalManifest(local);
        SyncPlan p = w.compareManifests(remote, {});

        QCOMPARE(p.filesToUpload.size(), 1);
        QCOMPARE(p.filesToUpload[0].hash, QString("hash_E"));
        QCOMPARE(p.filesToUpload[0].modified, (qint64)200);
        QCOMPARE(p.filesToDownload.size(), 0);
    }

    void testDeletedHash()
    {
        SyncManifest local, remote;
        local.files["hash_F"] = makeEntry("music/to_delete.mp3", 1000, 50);
        remote.files["hash_F"] = makeEntry("music/to_delete.mp3", 1000, 50);

        SyncWorker w;
        w.setLocalManifest(local);
        SyncPlan p = w.compareManifests(remote, {"hash_F"});

        QCOMPARE(p.filesToUpload.size(), 0);
        QCOMPARE(p.filesToDownload.size(), 0);
        QCOMPARE(p.filesToDelete.size(), 1);
        QCOMPARE(p.filesToDelete[0].hash, QString("hash_F"));
    }

    void testMultipleHashesMixed()
    {
        SyncManifest local, remote;
        // A: 仅本地 → upload
        local.files["hash_A"] = makeEntry("music/a.mp3", 1000, 100);
        // B: 仅远程 → download
        remote.files["hash_B"] = makeEntry("music/b.mp3", 2000, 200);
        // C: 两端都有，modified 相同 → skip
        local.files["hash_C"] = makeEntry("music/c.mp3", 3000, 300);
        remote.files["hash_C"] = makeEntry("music/c.mp3", 3000, 300);
        // D: 远程较新 → download
        local.files["hash_D"] = makeEntry("music/d.mp3", 4000, 100);
        remote.files["hash_D"] = makeEntry("music/d.mp3", 4000, 500);
        // E: 已删除
        local.files["hash_E"] = makeEntry("music/e.mp3", 5000, 50);
        remote.files["hash_E"] = makeEntry("music/e.mp3", 5000, 50);

        SyncWorker w;
        w.setLocalManifest(local);
        SyncPlan p = w.compareManifests(remote, {"hash_E"});

        QCOMPARE(p.filesToUpload.size(), 1);
        QCOMPARE(p.filesToUpload[0].hash, QString("hash_A"));
        QCOMPARE(p.filesToDownload.size(), 2);
        QCOMPARE(p.filesToDelete.size(), 1);
        QCOMPARE(p.totalOps(), 4);
    }
};

int main(int argc, char *argv[])
{
    qInstallMessageHandler(testMsgHandler);
    QGuiApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    TestSyncCompare tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test-sync-compare.moc"
