import QtQuick
import QtQuick.Controls
import TestApp

Item {
    width: 1280
    height: 720

    RhiItem {
        anchors.fill: parent
        NumberAnimation on triangleRotation { to: 360; duration: 1500; loops: -1 }
    }

    Button {
        text: "This is a Button"
    }
}
