#ifndef BT_TEST_H
#define BT_TEST_H

// BT Test Mode
// Use wave file instead of online
// recorder

#include <QObject>
#include <QtEndian>
#include <QDebug>
#include <QFile>

#include "bt_captain.h" //For BtWord
#include "kd_online2_model.h"
#include "kd_online2_decodable.h"
#include "kd_online_ldecoder.h"

class BtTest: public QObject
{
    Q_OBJECT
public:
    explicit BtTest(QString filename, QObject *parent = nullptr);
    ~BtTest();

private:
    void readWav(QString filename, BtCyclic *out);

    BtCyclic         *cy_buf;
};

#endif // BT_TEST_H
