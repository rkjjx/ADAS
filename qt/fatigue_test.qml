/******************************************************************
Copyright 2022 Guangzhou ALIENTEK Electronincs Co.,Ltd. All rights reserved
Copyright © Deng Zhimao Co., Ltd. 1990-2030. All rights reserved.
* @projectName   facedetectionitem
* @brief         facedetectionitem_Window.qml
* @author        Deng Zhimao
* @email         dengzhimao@alientek.com
* @date          2022-10-29
* @link          http://www.openedv.com/forum.php
*******************************************************************/
import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5

import facerecognitionitem 1.0
import QtQuick.VirtualKeyboard 2.2
Rectangle{
    id:rect1
    width: 720
    height: 1280
    Rectangle{
        id:cameraBg
        width:720
        height:1280
        anchors.bottom: parent.bottom
        FaceRecognitionItem {
            id: orItem
            anchors.fill: cameraBg
            width:720
            height:800
            onCaptureStop: {
                console.log("facedetectionitem thread stop.")
            }
        }
    }
    Button{
        id: button_12
        width:140
        height:60
        anchors.right:parent.right
        anchors.rightMargin:0
        anchors.top:parent.top
        text:"back"
        checkable:true
        autoExclusive:true
        opacity: 0.5
        onClicked:{
            rect1.visible = false
            loader.source = "main.qml"
        }
    }
    Dialog {
        id: popup
        modal: true
        x:150
        y:350
        width: 500
        height: 800
        visible: false // 初始隐藏
        closePolicy: Popup.NoAutoClose
        standardButtons: Dialog.NoButton  // 去掉任何默认按钮，防止按键关闭对话框
        contentItem: Column {
            spacing: 10
            anchors.centerIn: parent
            Image {
                source: "file:///demo/bin/temp_face.jpg"// 假设你可以从 orItem 捕获截图
                width: 300
                height: 400
                fillMode: Image.PreserveAspectFit
                cache:false
            }
            InputPanel {
                id: vkb
                visible: false
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                onActiveChanged: {
                    if (!active) { visible = false; }
                }
            }

            TextField {
                id: inputname
                width: parent.width
                placeholderText: "Enter name"
                onPressed: {
                    vkb.visible = true;  // 显示虚拟键盘
                    vkb.active = true;   // 激活虚拟键盘
                }
            }
            Row {  // 用 Row 来将 Apply 和 Cancel 按钮放在同一行
                spacing: 10
                Button {
                    text: "Apply"
                    width: 200
                    opacity: 0.8
                    onClicked: {
                        console.log("Entered text: " + inputname.text)
                        var face_name = inputname.text
                        var ret = orItem.getname(face_name)
                        if(ret === -1)
                        {
                           inputname.text = ""
                           inputname.placeholderText = face_name + " already exists!"
                           return
                        }
                        orItem.rename(face_name)

                        orItem.savefacefeature()
                        popup.visible = false
                        button_13.visible = true
                        inputname.text = ""
                    }
                }
                Button {
                    text: "Cancel"
                    width: 200
                    opacity: 0.8
                    onClicked: {
                        console.log("Cancel clicked")
                        popup.visible = false
                        button_13.visible = true
                    }
                }
            }
        }
    }

    Button {
        id:button_13
        width:250
        height:80
        anchors.left:parent.left
        anchors.leftMargin:20
        anchors.top:parent.top
        anchors.topMargin:850
        opacity: 0.8
        text: "register"
        onClicked: {
            var usrname = "temp"
            orItem.getname(usrname)
            orItem.saveimage()
            popup.visible = true
            button_13.visible = false;
        }
    }


    Connections {
            target: orItem // 指定监听的 C++ 对象
            onImageSaved: {
                // 每次图片保存后，更新 source 强制刷新图片，防止缓存
                popup.contentItem.children[0].source = "file:///demo/bin/temp_face.jpg" + "?" + Date.now();
            }
        }
}
