#ifndef FACEREGISTERITEM_H
#define FACEREGISTERITEM_H
#include <QQuickItem>
#include <QSGNode>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QImage>
#include "faceregistercapturethread.h"
class FaceRegisterCaptureThread;
class FaceRegisterItem : public QQuickItem
{
    Q_OBJECT
public:
    explicit FaceRegisterItem(QQuickItem *parent = nullptr);
    ~FaceRegisterItem();
    //使用'Q_INVOKABLE'标记的函数可以在QML中直接调用，这对于将c++后端逻辑暴露给qml前端非常有用
    Q_INVOKABLE void startCapture(bool start);
    Q_INVOKABLE void saveimage();
    Q_INVOKABLE void savefacefeature();
    Q_INVOKABLE void getname(QString);
public slots:
    void updateImage(QImage);

protected:
    QSGNode * updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QImage m_imageThumb;
    FaceRegisterCaptureThread *captureThread;
    QString name;
    QString picture_name;
    QString command;
signals:
    void captureStop();
    void imageSaved();
};
#endif // FACEREGISTERITEM_H
