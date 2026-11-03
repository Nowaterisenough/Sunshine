import QtQuick
import QtQuick.Controls

Rectangle {
    id: control
    property var model: []
    property int currentIndex: 0
    property bool flat: false
    signal activated(int index)
    implicitWidth: items.implicitWidth + (flat ? 0 : 4)
    implicitHeight: flat ? 24 : 28
    radius: 7
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
                width: Math.max(control.flat ? 0 : 48, caption.implicitWidth + (control.flat ? 14 : 20))
                height: 24
                hoverEnabled: true
                Accessible.name: modelData
                onClicked: control.activated(index)
                contentItem: PanelText { id: caption; text: segment.modelData; color: control.currentIndex === segment.index ? Theme.text : Theme.muted; font.pixelSize: control.flat ? 12 : 13; font.weight: control.flat ? Font.DemiBold : Font.Normal; font.letterSpacing: control.flat ? 0.55 : 0; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { radius: control.flat ? 6 : 5; color: control.currentIndex === segment.index ? (control.flat ? Theme.control : Theme.card) : segment.hovered ? Theme.hover : "transparent"; border.color: segment.visualFocus ? Theme.accent : "transparent" }
            }
        }
    }
}
