import QtQuick
import QtQuick.Controls

Rectangle {
    id: control
    property var model: []
    property int currentIndex: 0
    property bool flat: false
    signal activated(int index)
    implicitWidth: items.implicitWidth + (flat ? 0 : 4)
    implicitHeight: 30
    radius: 8
    color: flat ? "transparent" : Theme.control
    opacity: enabled ? 1 : 0.45
    Row {
        id: items
        anchors.centerIn: parent
        spacing: control.flat ? 2 : 0
        Repeater {
            model: control.model
            delegate: AbstractButton {
                id: segment
                required property string modelData
                required property int index
                width: Math.max(48, caption.implicitWidth + 24)
                height: 26
                hoverEnabled: true
                Accessible.name: modelData
                onClicked: control.activated(index)
                contentItem: Text { id: caption; text: segment.modelData; color: control.currentIndex === segment.index ? Theme.text : Theme.muted; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { radius: 6; color: control.currentIndex === segment.index ? (control.flat ? Theme.control : Theme.card) : segment.hovered ? Theme.hover : "transparent"; border.color: segment.activeFocus ? Theme.accent : "transparent" }
            }
        }
    }
}
