import QtQuick
import QtQuick.Controls
import TestApp

Item {
    width: 1280
    height: 720

    Button {
        text: "This is a Button"
    }

    // this is an Item, but no matter where it's placed it will render before the rest of the scene (hence "underlay")
    RhiUnderlay {
        NumberAnimation on triangleRotation { to: 360; duration: 1500; loops: -1 }
    }
}
