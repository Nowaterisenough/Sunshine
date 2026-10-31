import QtQuick
import QtQuick.Controls

TextField {
    id: control
    implicitHeight: Theme.controlHeight
    implicitWidth: 200
    color: Theme.text
    placeholderTextColor: Theme.muted
    font.pixelSize: 13
    selectByMouse: true
    leftPadding: 10
    rightPadding: 10
    background: Rectangle {
        color: Theme.control
        radius: 7
        border.color: control.activeFocus ? Theme.accent : Theme.border
    }
}
