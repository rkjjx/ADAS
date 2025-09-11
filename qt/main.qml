import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5

Window {
    id: app_facedetectionitem
    width: 720
    height: 1280
    visible: true
    title: qsTr("app_facedetectionitem")
    color: "white"  // 设置背景色

    Loader {
        id: loader
    }

    Button {
        id: button_01
        width: 300
        height: 100
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 100
        text: "ADAS"
        onClicked: {
            loader.source = "fatigue_test.qml"
            button_01.visible = false

        }
    }

}

