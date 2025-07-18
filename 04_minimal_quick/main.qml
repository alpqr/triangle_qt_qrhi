import QtQuick
import QtQuick.Controls
import TestApp

Item {
    width: 1280
    height: 720

    Button {
        text: "This is a Button"
    }

    // This is an Item, but without the ItemHasContents flag, and no matter
    // where it's placed it will render before the rest of the scene (hence
    // "underlay"). Size and positioning does not matter either.
    RhiUnderlay {
        NumberAnimation on triangleRotation { to: 360; duration: 1500; loops: -1 }
    }
}
