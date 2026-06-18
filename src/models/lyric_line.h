#pragma once

#include <QtGlobal>
#include <QString>

struct LyricLine
{
    qint64 timestampMs = 0;
    QString text;
};
