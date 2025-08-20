/******************************************************************
Copyright 2022 Guangzhou ALIENTEK Electronincs Co.,Ltd. All rights reserved
Copyright © Deng Zhimao Co., Ltd. 1990-2030. All rights reserved.
* @projectName   facedetection
* @brief         facedetectionitem.cpp
* @author        Deng Zhimao
* @email         dengzhimao@alientek.com
* @date          2022-10-31
* @link          http://www.openedv.com/forum.php
*******************************************************************/
#include "facerecognitionitem.h"
#include <QDebug>
#include <QCoreApplication>
FaceRecognitionItem::FaceRecognitionItem(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    captureThread = new FaceRecognitionCaptureThread(this);
    //m_imageThumb = QImage(640, 480, QImage::Format_RGB888);

    connect(captureThread, SIGNAL(resultReady(QImage)),
            this, SLOT(updateImage(QImage)));

    connect(captureThread, SIGNAL(finished()),
            this, SIGNAL(captureStop()));
    captureThread->start();
    captureThread->setThreadStart(true);
}

void FaceRecognitionItem::updateImage(QImage image)
{
    QMatrix leftmatrix;
    //    if (camerId == 2)
    //        leftmatrix.rotate(90);
    //    else
    //        leftmatrix.rotate(0);

    m_imageThumb = image.transformed(leftmatrix, Qt::SmoothTransformation);
    update();
}

FaceRecognitionItem::~FaceRecognitionItem()
{

    captureThread->setThreadStart(false);
    captureThread->quit();
    captureThread->wait();

    delete captureThread;

    captureThread = nullptr;
}
void FaceRecognitionItem::startCapture(bool start)
{
    if (!captureThread->isRunning())
        emit captureStop();
    captureThread->setThreadStart(start);
    if (start) {
        if (!captureThread->isRunning())
            captureThread->start();
    } else {
        captureThread->quit();
    }
}

QSGNode * FaceRecognitionItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    auto node = dynamic_cast<QSGSimpleTextureNode *>(oldNode);

    if(!node){
        node = new QSGSimpleTextureNode();
    }

    QSGTexture *m_texture = window()->createTextureFromImage(m_imageThumb, QQuickWindow::TextureIsOpaque);
    node->setOwnsTexture(true);
    node->setRect(boundingRect());
    node->markDirty(QSGNode::DirtyForceUpdate);
    node->setTexture(m_texture);

    return node;
}
void FaceRecognitionItem::saveimage()
{
    FaceRecognitionFrame *frame = GetFace();
    QImage image((unsigned char *)frame->file, 112, 112, QImage::Format_RGB888);
    QMatrix leftmatrix;
    m_imageThumb = image.transformed(leftmatrix, Qt::SmoothTransformation);
    QFile file(picture_name);
    if (file.exists()) {
        qDebug() << "File already exists: " << picture_name << ", overwriting...";// 若文件存在，会直接覆盖
    }
    m_imageThumb.save(picture_name,"JPEG",-1);
    emit imageSaved();
}
void FaceRecognitionItem::savefacefeature()
{
    command = "/demo/bin/import_face_library " + name + " " + picture_name;
    system(command.toLatin1());
    printf("import_face_library done.\n");
}
int FaceRecognitionItem::getname(QString name_end)
{
    name = name_end;
    picture_name = "/demo/bin/";
    picture_name.append(name_end);
    picture_name.append("_face.jpg");
    if (QFile::exists(picture_name)) {
            return -1;  // 如果文件存在，返回 -1
    }
    return 0;
}
void FaceRecognitionItem::rename(QString name)
{
    command = "mv /demo/bin/temp_face.jpg /demo/bin/" + name + "_face.jpg";
    system(command.toLatin1());
    printf("rename done.\n");
}


