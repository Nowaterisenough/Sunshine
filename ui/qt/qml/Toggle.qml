import QtQuick
import QtQuick.Controls

Switch {
    id: control
    implicitWidth: 36
    implicitHeight: 24
    indicator: Rectangle {
        x: 0; y: 2; width: 36; height: 20; radius: 10
        color: control.checked ? "#ededf1" : Theme.selected
        border.color: control.activeFocus ? Theme.accent : "transparent"
        Rectangle {
            x: control.checked ? 19 : 3; y: 3; width: 14; height: 14; radius: 7
            color: control.checked ? "#232327" : "#b7b7c0"
            Behavior on x { NumberAnimation { duration: 100 } }
        }
    }
}
