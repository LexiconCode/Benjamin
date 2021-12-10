#include "kd_online2.h"

//#ifdef BT_ONLINE2
#include "feat/wave-reader.h"
#include "online2/online-feature-pipeline.h"
#include "online2/online-gmm-decoding.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"

using namespace kaldi;
using namespace fst;

KdOnline2::KdOnline2(QObject *parent): QObject(parent)
{
    cy_buf = new BtCyclic(BT_REC_RATE*BT_BUF_SIZE);
    parseWords(BT_WORDS_PATH);

    g_decoder  = new KdOnline2Gmm(parent);
    rec_thread = new QThread;
    rec_src = new BtRecorder(cy_buf);
    rec_src->moveToThread(rec_thread);
    rec_thread->start();

    connect(this,  SIGNAL(startRecord()), rec_src, SLOT(startStream()));
}

KdOnline2::~KdOnline2()
{

}

void KdOnline2::init()
{
    int sample_count = BT_REC_SIZE*BT_REC_RATE;
    int16_t raw[BT_REC_SIZE*BT_REC_RATE];

    emit startRecord();

    while( cy_buf->getDataSize()>BT_REC_SIZE*BT_REC_RATE )
    {
        QThread::msleep(2);
    }
    cy_buf->read(raw, (BT_REC_SIZE-BT_DEC_TIMEOUT)*BT_REC_RATE);

    while( true )
    {
        if( cy_buf->getDataSize()<BT_DEC_TIMEOUT*BT_REC_RATE )
        {
            QThread::msleep(2);
            continue;
        }

        cy_buf->rewind((BT_REC_SIZE-BT_DEC_TIMEOUT)*BT_REC_RATE);
        processData(sample_count);
    }
}

void KdOnline2::execute(std::vector<int32_t> word)
{
    QString cmd = KAL_SI_DIR"main.sh \"";
    for( int i=0 ; i<word.size() ; i++ )
    {
        QString word_str = lexicon[word[i]];
        cmd += word_str;
        cmd += " ";
        history.push_back(word_str);

        if( history.size()>10 )
        {
            history.pop_front();
        }
    }
    cmd += "\"";
    system(cmd.toStdString().c_str());
}

void KdOnline2::parseWords(QString filename)
{
    QFile words_file(filename);

    if (!words_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Error opening" << filename;
        return;
    }

    lexicon.clear();

    while (!words_file.atEnd())
    {
        QString line = QString(words_file.readLine());
        QStringList line_list = line.split(" ");
        lexicon.append(line_list[0]);
    }

    words_file.close();
}

void KdOnline2::print(CompactLattice *clat)
{
    KdMBR *mbr = NULL;

    mbr = new KdMBR(clat);
    QVector<BtWord> result = mbr->getResult(lexicon);
    history.clear();

    QString message;
    for( int i = 0; i<result.size() ; i++ )
    {
        message += result[i].word;
        message +=  "[";
        message += QString::number(result[i].conf);
        message +=  ", ";
        message += QString::number(result[i].start,'g', 3);
        message +=  "-";
        message += QString::number(result[i].end,'g', 3);
        message +=  " ]";
    }

    if( result.size()>10 )
    {
        CompactLatticeWriter clat_writer("ark:b.ark");
        clat_writer.Write("f", *clat);
        exit(0);
    }

    if( result.size() )
    {
//        execute(words);
//        printTime(start);
        qDebug() << "print" << message;
    }
    bt_writeBarResult(result);

//    emit resultReady(result);
}

void KdOnline2::processData(int len)
{
    g_decoder->init(NULL); ///FIXME: Replace with rec_src
    g_decoder->decodable->features->AcceptWaveform(cy_buf, len);
//    clock_t start = clock();

    g_decoder->AdvanceDecoding();
//    g_decoder->FinalizeDecoding();
//    printTime(start);
    CompactLattice clat;
    bool ret = g_decoder->GetLattice(true, &clat);

    if( ret )
    {
        print(&clat);
    }
}

//#endif
