/******************************************************************
Copyright 2022 Guangzhou ALIENTEK Electronincs Co.,Ltd. All rights reserved
Copyright © Deng Zhimao Co., Ltd. 1990-2030. All rights reserved.
* @projectName   facedetection
* @brief         facedetectioncapturethread.h
* @author        Deng Zhimao
* @email         dengzhimao@alientek.com
* @date          2022-10-31
* @link          http://www.openedv.com/forum.php
*******************************************************************/
#ifndef FACERECOGNITIONCAPTURETHREAD_H
#define FACERECOGNITIONCAPTURETHREAD_H

#include <QThread>
#include <QDebug>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QTime>
#include <QQuickItem>

#include "facerecognitioncamerathread.h"
//FaceRecognitionCaptureThread定义了信号resultReady(QImage)
//                            定义了私有变量startFlag和m_RecognitionThreead
//                            定义了构造函数和析构函数
//                            定义了成员函数setThreadStart()
//                            重写了虚函数run()
class FaceRecognitionCaptureThread : public QThread
{
    Q_OBJECT

signals:
    void resultReady(QImage);

private:

    bool startFlag = false;


public:
    FaceRecognitionThread *m_RecognitionThread;
    FaceRecognitionCaptureThread(QObject *parent = nullptr) {
        Q_UNUSED(parent);
        m_RecognitionThread = new FaceRecognitionThread(this);//FaceRecognitionThread对象将以FaceRecognitionCaptureThread为父对象
        m_RecognitionThread->setFlag(false);
        m_RecognitionThread->start();
#ifdef __arm__
        msleep(0);
#endif
    }
    ~FaceRecognitionCaptureThread() override{
        m_RecognitionThread->setFlag(true);
        m_RecognitionThread->quit();
    }

    void setThreadStart(bool start) {
        startFlag = start;
    }

    void run() override {
        msleep(800);
#ifdef __arm__
        while (startFlag && m_RecognitionThread->camera_init_success) {
            msleep(33);
            FaceRecognitionFrame *frame = GetFaceRecognitionMediaBuffer();
            if (frame) {
                QImage qImage((unsigned char *)frame->file, 720, 1280, QImage::Format_RGB888);
                emit resultReady(qImage);
            }
            delete frame;
        }
#endif
    }
};

#endif // FACERECOGNITIONCAPTURETHREAD_H
