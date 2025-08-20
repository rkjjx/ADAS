#ifndef FACEREGISTERCAMERATHREAD_H
#define FACEREGISTERCAMERATHREAD_H

#include <QThread>
#include <QObject>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>

#include <stddef.h>

#ifdef __arm__
#include "alientek/atk_register.h"
#endif

class FaceRegisterThread : public QThread
{
    //在使用信号与槽的类中，必须在类的定义中加Q_OBJECT宏。信号与槽的参数类型和个数必须一致
    Q_OBJECT
public:
    FaceRegisterThread(QObject *parent = nullptr);//QObject *parent参数用于指定该对象的父对象
    void setFlag(bool flag) {
        Q_UNUSED(flag)//Q_UNUSED()用于告诉编译器某个函数参数在函数内部并没有被使用，防止编译器生成警告或错误
#if __arm__
        atk_register_quit = flag;
#endif
    }
    void run() override {
#if __arm__
        //atk_face_recognition_quit = false;atk_recognition_init()函数在初始化VI、VO、RGA参数以及绑定VI和RGA后，创建子线程，子线程会先导入rknn模型。如果atk_face_recognition_quit为false，主线程0.5s循环检测该参数，子线程运行模型并进行后处理
        atk_register_init();           //如果atk_face_recognition_quit为true，父子线程都结束。
#endif
    }
    bool camera_init_success = true;
};

#endif // FACEREGISTERCAMERATHREAD_H
