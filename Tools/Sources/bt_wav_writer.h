#ifndef BT_WAV_WRITER_H
#define BT_WAV_WRITER_H

#include <QObject>
#include <thread>         // std::thread
#include <QTimer>
#include <QDebug>

#include "bt_recorder.h"
#include "bt_captain.h" //For BtWord

class BtWavWriter : public QObject
{
    Q_OBJECT
public:
    explicit BtWavWriter(BtCyclic *buffer, QObject *parent = nullptr);
    ~BtWavWriter();

    void write(QVector<BtWord> result, int len, int dbg_id);

private:
    void writeWav(int len);
    void writeWavHeader(int len);
    void copyToTrain(QVector<BtWord> result, QString filename);
    bool isSleep();
    void readWordList();
    QString wordToId(QVector<BtWord> result);

    BtCyclic         *cy_buf;
    QFile            *file;
    QStringList       word_list;
};

#endif // BT_WAV_WRITER_H