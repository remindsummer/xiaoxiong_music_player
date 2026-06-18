#include <QtTest>

#include "../../src/lyrics/lyric_parser.h"

class TestLyricParser : public QObject
{
    Q_OBJECT

private slots:
    void shouldParseAndSortByTimestamp();
    void shouldSupportMultiTimestampLine();
    void shouldIgnoreMetadataAndInvalidTags();
};

void TestLyricParser::shouldParseAndSortByTimestamp()
{
    LyricParser parser;
    const QString lrc = QStringLiteral(
        "[00:12.50]line c\n"
        "[00:03.00]line a\n"
        "[00:08.20]line b\n");

    const QVector<LyricLine> lines = parser.parseLrc(lrc);

    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines.at(0).timestampMs, 3000);
    QCOMPARE(lines.at(0).text, QStringLiteral("line a"));
    QCOMPARE(lines.at(1).timestampMs, 8200);
    QCOMPARE(lines.at(2).timestampMs, 12500);
}

void TestLyricParser::shouldSupportMultiTimestampLine()
{
    LyricParser parser;
    const QString lrc = QStringLiteral(
        "[00:01.00][00:02.00]echo\n");

    const QVector<LyricLine> lines = parser.parseLrc(lrc);

    QCOMPARE(lines.size(), 2);
    QCOMPARE(lines.at(0).timestampMs, 1000);
    QCOMPARE(lines.at(0).text, QStringLiteral("echo"));
    QCOMPARE(lines.at(1).timestampMs, 2000);
    QCOMPARE(lines.at(1).text, QStringLiteral("echo"));
}

void TestLyricParser::shouldIgnoreMetadataAndInvalidTags()
{
    LyricParser parser;
    const QString lrc = QStringLiteral(
        "[ar:someone]\n"
        "[ti:title]\n"
        "[00:61.20]bad second\n"
        "[00:15.20]valid line\n");

    const QVector<LyricLine> lines = parser.parseLrc(lrc);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(lines.at(0).timestampMs, 15200);
    QCOMPARE(lines.at(0).text, QStringLiteral("valid line"));
}

QTEST_MAIN(TestLyricParser)
#include "test_lyric_parser.moc"
