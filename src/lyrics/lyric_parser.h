#pragma once

#include <QObject>
#include <QVector>

#include "../models/lyric_line.h"

class LyricParser : public QObject
{
    Q_OBJECT

public:
    explicit LyricParser(QObject *parent = nullptr);

    QVector<LyricLine> parseLrc(const QString &content) const;

private:
    static qint64 parseFractionToMs(const QString &fraction);
};
