#ifndef FACEREGISTERCAPTURETHREAD_H
#define FACEREGISTERCAPTURETHREAD_H
#include <QThread>
#include <QDebug>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QTime>
#include <QQuickItem>

#include "faceregistercamerathread.h"

class FaceRegisterCaptureThread : public QThread
{
    Q_OBJECT

signals:
    void resultReady(QImage);

private:

    bool startFlag = false;


public:
    FaceRegisterThread *m_RegisterThread;
    FaceRegisterCaptureThread(QObject *parent = nullptr) {
        Q_UNUSED(parent);
        m_RegisterThread = new FaceRegisterThread(this);//FaceRecognitionThread对象将以FaceRecognitionCaptureThread为父对象
        m_RegisterThread->setFlag(false);
        m_RegisterThread->start();
#ifdef __arm__
        msleep(0);
#endif
    }
    ~FaceRegisterCaptureThread() override{
        m_RegisterThread->setFlag(true);
        m_RegisterThread->quit();
    }

    void setThreadStart(bool start) {
        startFlag = start;
    }

    void run() override {
        msleep(800);
#ifdef __arm__
        while (startFlag && m_RegisterThread->camera_init_success) {
            msleep(33);
            FaceRegisterFrame *frame = GetFaceRegisterMediaBuffer();
            if (frame) {
                QImage qImage((unsigned char *)frame->file, 720, 1280, QImage::Format_RGB888);
                emit resultReady(qImage);
            }
            delete frame;
        }
#endif
    }
};

#endif // FACEREGISTERCAPTURETHREAD_H
