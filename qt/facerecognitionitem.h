#ifndef FACERECOGNITIONITEM_H
#define FACERECOGNITIONITEM_H

#include <QQuickItem>
#include <QSGNode>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QImage>
#include "facerecognitioncapturethread.h"
class FaceRecognitionCaptureThread;
//FaceRecognitionItem类声明了构造函数和析构函数
//                     声明了startCapture(bool start)、saveimage()、savefacefeature()、getname(QString)函数
//                     声明了槽函数updateImage(QImage)
//                     声明了信号captureStop()
//                     声明了重写函数updatePaintNode()
//                     声明了私有变量m_imageThumb、captureThread、picture_name、command
class FaceRecognitionItem : public QQuickItem
{
    Q_OBJECT
public:
    explicit FaceRecognitionItem(QQuickItem *parent = nullptr);
    ~FaceRecognitionItem();
    //使用'Q_INVOKABLE'标记的函数可以在QML中直接调用，这对于将c++后端逻辑暴露给qml前端非常有用
    Q_INVOKABLE void startCapture(bool start);
    Q_INVOKABLE void saveimage();
    Q_INVOKABLE void savefacefeature();
    Q_INVOKABLE int getname(QString);
    Q_INVOKABLE void rename(QString);
public slots:
    void updateImage(QImage);

protected:
    QSGNode * updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QImage m_imageThumb;
    FaceRecognitionCaptureThread *captureThread;
    QString picture_name;
    QString name;
    QString command;
signals:
    void captureStop();
    void imageSaved();
};

#endif // FACERECOGNITIONITEM_H
