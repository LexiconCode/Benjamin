#include "bt_recorder.h"
#include <fcntl.h>

BtRecorder::BtRecorder(BtCyclic *buffer, QObject *parent): QObject(parent)
{
    cy_buf  = buffer;
    read_pf = 0;

    // Done to skip shitty alsa messages
    int saved_stdout = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_RDWR);
    dup2(devnull, STDERR_FILENO);  // Replace standard out
    PaError pa_err = Pa_Initialize();
    dup2(saved_stdout, STDERR_FILENO);
    if( pa_err )
    {
        qDebug() << "PortAudio initialization error";
    }

    pa_err = Pa_OpenDefaultStream(&pa_stream, 1, 0, paInt16
                         , BT_REC_RATE, 0, PaCallback, this);
    if( pa_err )
    {
        qDebug() << "PortAudio failed to open the default stream";
        pa_stream = NULL;
    }
}

BtRecorder::~BtRecorder()
{
    if( pa_stream )
    {
        Pa_StopStream(pa_stream);
    }
    if( pa_stream )
    {
        Pa_CloseStream(pa_stream);
        Pa_Terminate();
    }
}

// This function deosn't need to be called
// Only use if you don't plan on using read function
void BtRecorder::startStream()
{
    if( Pa_IsStreamStopped(pa_stream) )
    {
        PaError paerr = Pa_StartStream(pa_stream);
        if( paerr )
        {
            qDebug() << "Error while trying to open PortAudio stream";
            exit(0);
        }
    }
}

// cy_buf will be filled from the other thread using GStreamer
bool BtRecorder::Read(kaldi::Vector<float> *data)
{
    if( Pa_IsStreamStopped(pa_stream) )
    {
        PaError paerr = Pa_StartStream(pa_stream);
        if( paerr )
        {
            qDebug() << "Error while trying to open PortAudio stream";
            exit(0);
        }
    }
    int req = data->Dim();  // number of samples to request
    int16_t *raw = (int16_t *)malloc(sizeof(int16_t)*req);

    while( true )
    {
        QThread::msleep(2);
        if( req<cy_buf->getDataSize(&read_pf) )
        {
            break;
        }
    }

    int nsamples = cy_buf->read(raw, req, &read_pf);
    data->Resize(nsamples);
//    qDebug() << req << nsamples;

    for( int i = 0 ; i<nsamples ; i++ )
    {
        (*data)(i) = raw[i];
    }
    free(raw);

    if( nsamples==0 )
    {
        return false;
    }

    return true;
}

int BtRecorder::Callback(int16_t *data, int size)
{
    if( cy_buf->getFreeSize()<size )
    {
        qDebug() << "BtRecorder Overflow" << size << cy_buf->getFreeSize();
        return paContinue;
    }
    cy_buf->write(data, size);
    return paContinue;
}

int PaCallback(const void *input, void *output,
               long unsigned frame_count,
               const PaStreamCallbackTimeInfo *time_info,
               PaStreamCallbackFlags status_flags, void *user_data)
{
    BtRecorder *pa_src = reinterpret_cast<BtRecorder*>(user_data);

    int size = frame_count;
    void *ptr = const_cast<void *>(input);

    return pa_src->Callback((int16_t *)ptr, size);
}
