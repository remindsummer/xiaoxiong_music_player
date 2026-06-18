#include "lyric_parser.h"

#include <QRegularExpression>

#include <algorithm>

LyricParser::LyricParser(QObject *parent)
    : QObject(parent)
{
}

QVector<LyricLine> LyricParser::parseLrc(const QString &content) const
{
    static const QRegularExpression timeTagPattern(
        QStringLiteral("\\[(\\d{1,3}):(\\d{2})(?:[\\.:](\\d{1,3}))?\\]"));
    static const QRegularExpression fullTimeTagPattern(
        QStringLiteral("\\[(?:\\d{1,3}):(?:\\d{2})(?:[\\.:](?:\\d{1,3}))?\\]"));

    QVector<LyricLine> lines;
    const QStringList rawLines = content.split('\n');

    for (const QString &rawLine : rawLines) {
        const QString trimmedLine = rawLine.trimmed();
        if (trimmedLine.isEmpty()) {
            continue;
        }

        QRegularExpressionMatchIterator iterator = timeTagPattern.globalMatch(trimmedLine);
        if (!iterator.hasNext()) {
            continue;
        }

        const QString lyricText = QString(trimmedLine).remove(fullTimeTagPattern).trimmed();

        while (iterator.hasNext()) {
            const QRegularExpressionMatch match = iterator.next();
            const int minutes = match.capturedView(1).toInt();
            const int seconds = match.capturedView(2).toInt();
            if (seconds >= 60) {
                continue;
            }

            const qint64 baseMs = (static_cast<qint64>(minutes) * 60 + seconds) * 1000;
            const qint64 totalMs = baseMs + parseFractionToMs(match.captured(3));

            lines.append(LyricLine{totalMs, lyricText});
        }
    }

    std::stable_sort(lines.begin(), lines.end(), [](const LyricLine &left, const LyricLine &right) {
        return left.timestampMs < right.timestampMs;
    });

    return lines;
}

qint64 LyricParser::parseFractionToMs(const QString &fraction)
{
    if (fraction.isEmpty()) {
        return 0;
    }

    if (fraction.size() == 1) {
        return fraction.toInt() * 100;
    }

    if (fraction.size() == 2) {
        return fraction.toInt() * 10;
    }

    return fraction.left(3).toInt();
}
