SOURCES += \
    $$PWD/facedetectioncamerathread.cpp \
    $$PWD/facedetectionitem.cpp 

RESOURCES += \
    $$PWD/facedetection_icons.qrc \
    $$PWD/facedetection_qml.qrc


TARGET_ARCH = $${QT_ARCH}
contains(TARGET_ARCH, arm) {
    LIBS +=  -latk_face_detection  -lsample_common_isp -leasymedia
}

HEADERS += \
    $$PWD/facedetectioncamerathread.h \
    $$PWD/facedetectioncapturethread.h \
    $$PWD/facedetectionitem.h
