## 1.开发平台与环境

1.rv1126开发板，内置2Tops算力的NPU，MIPI摄像头模块atk-mcimx415 800w像素

2.NPU推理库librknn_runtime version 1.7.5，python库rknn -toolkit  1.7.3

## 2.三个功能

驾驶员人脸注册识别、人脸+异常举动检测（打电话、吸烟）、瞌睡检测（眨眼、打哈切、低头）

![QQ_1759202348277](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/多进程.jpg)

1.主线程初始化rkmedia系统，创建VI通道采集数据，RGA通道硬件加速图像处理，绑定VI->RGA传输图像。启动人脸识别子线程，主线程while(end_flag)循环等待，直到end_flag==1，解绑VI->RGA，关闭RGA、VI通道。

2.子线程中通过API获得图像，resize至(1,3,640,640)送入yolov5s三分类模型，后处理后得到检测结果。如果存在人脸，imcrop()和imresize()函数裁剪人脸区域并resize至(1,3,112,112)送入pfpld 98点人脸关键点检测模型和mobilefacenet 人脸识别模型实现驾驶员是否疲劳与驾驶员身份核验功能。

3.QT中通过GetFaceRecognitionMediaBuffer()函数从RGA[0:0]通道得到图像后，发出ready信号，触发槽更新图像。

### 2.1人脸+异常举动检测（打电话、吸烟）

三分类：phone、smoking、face

![QQ_1759202289398](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/QQ_1759202289398.png)

![QQ_1759202348277](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/QQ_1759202348277.png)

### 2.2瞌睡检测（眨眼、打哈切、低头）

![QQ_1759202382031](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/QQ_1759202382031.png)

### 2.3驾驶员人脸注册识别

![QQ_1759202415101](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/QQ_1759202415101.png)



## 3.sqlite3

![sqlite](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/sqlite.jpg)

**/demo/bin目录下import_face_library可执行文件**

1.open_db()打开数据库，创建face_table;

2.使用开源图像库std操作图像，包括load、resize

3.送入人脸识别模型，生成特征向量

4.保存驾驶员信息到数据库

疲劳检测子线程中读取数据库信息到map<string, rockx_face_feature_t>。

## 4.QT

![qt](https://cdn.jsdelivr.net/gh/rkjjx/Pictures@main/qt.jpg)

**class FaceRecognitionThread : public Qthread**

1.控制退出标志end_flag

2.重写Qthread的run()方法，调用人脸识别主线程



**class FaceRecognitionCaptureThread : public Qthread**

1.通过end_flag管理FaceRecognitionThread的运行

2.重写run()函数，从RGA[0:0]获得疲劳检测后的图像，并emit resultReady(qImage);



**class FaceRecognitionItem : public QQuickItem**

1.通过信号槽机制刷新图像

2.使用Q_INVOKABLE提供 QML 可调用的函数

## 5.总结

基于瑞芯微提供的 SDK，实现了一个视觉疲劳驾驶检测系统。开发过程中无需操作底层驱动，通过 RKmedia 封装的 API 即可直接控制硬件，实现图像采集与处理。
