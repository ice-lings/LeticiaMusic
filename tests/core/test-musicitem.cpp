#include <QtTest/QtTest>
#include <QGuiApplication>
#include <cstdio>
#include <music_entity_type.h>

static void testMsgHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(ctx)
    fprintf(stderr, "%s\n", qPrintable(msg));
    fflush(stderr);
}

class TestMusicItem: public QObject
{
    Q_OBJECT

private slots:
    void testMusicItemDefaultConstruction()
    {
        MusicItem item;
        QCOMPARE(item.title, QString(""));
        QCOMPARE(item.artist, QString("佚名"));
        QCOMPARE(item.album, QString(""));
        QCOMPARE(item.duration, QString("00:00"));
        QVERIFY(item.coverPath.isEmpty());
        QVERIFY(item.coverHash.isEmpty());
        QVERIFY(item.filePath.isEmpty());
    }

    void testMusicItemConstruction()
    {
        MusicItem item;
        item.title = "Test Song";
        item.artist = "Test Artist";
        item.album = "Test Album";
        item.duration = "03:45";
        item.filePath = "/path/to/song.mp3";

        QCOMPARE(item.title, QString("Test Song"));
        QCOMPARE(item.artist, QString("Test Artist"));
        QCOMPARE(item.album, QString("Test Album"));
        QCOMPARE(item.duration, QString("03:45"));
        QCOMPARE(item.filePath, QString("/path/to/song.mp3"));
    }

    void testFormatDuration()
    {
        QCOMPARE(formatDuration(0), QString("00:00"));
        QCOMPARE(formatDuration(30), QString("00:30"));
        QCOMPARE(formatDuration(60), QString("01:00"));
        QCOMPARE(formatDuration(90), QString("01:30"));
        QCOMPARE(formatDuration(225), QString("03:45"));
        QCOMPARE(formatDuration(3600), QString("60:00"));
    }

    void testMusicMetadataDefaultConstruction()
    {
        MusicMetadata meta;
        QVERIFY(meta.id == -1);
        QVERIFY(meta.title.isEmpty());
        QVERIFY(meta.artist.isEmpty());
        QVERIFY(meta.album.isEmpty());
        QVERIFY(meta.genre.isEmpty());
        QVERIFY(meta.composer.isEmpty());
    }
};

int main(int argc, char *argv[])
{
    qInstallMessageHandler(testMsgHandler);
    QGuiApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    TestMusicItem tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test-musicitem.moc"
