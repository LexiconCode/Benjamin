#ifndef KD_ONLINE_H
#define KD_ONLINE_H

#include <QObject>
#include <thread>         // std::thread
#include <QTimer>
#include <QDebug>

#include "bt_state.h"
#include "bt_recorder.h"
#include "bt_captain.h" //For BtWord
#include "kd_online2_model.h"
#include "kd_online2_decodable.h"
#include "kd_online_ldecoder.h"

#ifdef BT_LAT_ONLINE
#define  BT_ONLINE_DECODER   KdOnlineLDecoder
#define  BT_ONLINE_DECODABLE KdOnline2Decodable
#define  BT_ONLINE_OPTS      KdOnlineLDecoderOpts
#define  BT_ONLINE_LAT       kaldi::CompactLattice
#else
#include "kd_online_decodable.h"
#include "kd_online_decoder.h"
#define  BT_ONLINE_DECODER   KdOnlineDecoder
#define  BT_ONLINE_DECODABLE KdOnlineDecodable
#define  BT_ONLINE_OPTS      KdOnlineDecoderOpts
//#define  BT_ONLINE_LAT       fst::VectorFst<kaldi::LatticeArc>
#define  BT_ONLINE_LAT       kaldi::CompactLattice
#endif

class KdOnline : public QObject
{
    Q_OBJECT
public:
    explicit KdOnline(QObject *parent = nullptr);
    ~KdOnline();

    void startDecode();

public slots:
    void init();

signals:
    void resultReady(QVector<BtWord> result);

private:
    void printTime(clock_t start);
    void parseWords(QString filename);
    void execute(std::vector<int32_t> word);

    void processLat(BT_ONLINE_LAT *clat, clock_t start);

    BtRecorder   *ab_src;
    QVector<QString>  history;
    QVector<QString>  lexicon;
    BtCyclic         *cy_buf;
};

#endif // KD_ONLINE_H
