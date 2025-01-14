#ifndef MM_LABEL_H
#define MM_LABEL_H

#include <QObject>
#include <QQmlProperty>
#include <QVector>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include "mm_config.h"

typedef struct MmProperty
{
    QString bg = MM_DEFAULT_BG;
    QString fg = MM_DEFAULT_FG;
    QString ul = BPB_DEFAULT_UL;
    bool    ul_en = false;

    QString action_l; // Left click
    QString action_r; // Right click
    QString action_m; // Middle click
    QString action_u; // Scroll up
    QString action_d; // Scroll down
}MmProperty;

class MmLabel
{
public:
    MmLabel();

    void setVal(QString input);

    MmProperty prop;
    QString    val;
};

#endif // MM_LABEL_H
