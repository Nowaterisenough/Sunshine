import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool primary: false
    property bool destructive: false
    property bool subtle: false
    property string symbol: ""
    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(subtle ? 28 : 64, contentItem.implicitWidth + (subtle ? 12 : 24))
    leftPadding: subtle ? 6 : 12
    rightPadding: subtle ? 6 : 12
    hoverEnabled: true
    opacity: enabled ? 1 : 0.4
    font.pixelSize: 13
    contentItem: Item {
        implicitWidth: contents.implicitWidth
        implicitHeight: contents.implicitHeight
        Row {
            id: contents
            anchors.centerIn: parent
            spacing: control.text !== "" && control.symbol !== "" ? 6 : 0
            LineIcon { visible: control.symbol !== ""; name: control.symbol; width: 15; height: 15; anchors.verticalCenter: parent.verticalCenter; ink: control.primary ? Theme.accent : Theme.muted }
            Text { text: control.text; font: control.font; color: control.primary ? Theme.accent : control.destructive ? Theme.danger : control.subtle ? Theme.muted : Theme.text }
        }
    }
    background: Rectangle {
        radius: control.primary ? height / 2 : 7
        color: control.primary ? Theme.accentFill : control.subtle ? "transparent" : Theme.control
        border.color: control.activeFocus ? Theme.accent : "transparent"
        Rectangle { anchors.fill: parent; radius: parent.radius; color: control.down ? "#12000000" : control.hovered ? "#0cffffff" : "transparent" }
    }
}
