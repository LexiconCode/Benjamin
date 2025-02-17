#ifndef AB_WAV_WRITER_H
#define AB_WAV_WRITER_H

#include <QDir>

#include "ab_recorder.h"
#include "ab_mbr_base.h" //For BtWord

class AbWavWriter
{
public:
    AbWavWriter(int16_t *buffer, int sample_count);
    ~AbWavWriter();

    void write(QString filename);
    void setCategory(QString cat);

    QString category;

private:
    void writeWav();
    void writeWavHeader(int len);

    int16_t     *cy_buf;
    int          buf_size;
    QFile       *file;
};

#endif // AB_WAV_WRITER_H
