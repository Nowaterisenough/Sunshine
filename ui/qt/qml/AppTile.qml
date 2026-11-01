import QtQuick
import QtQuick.Layouts

Rectangle {
    id: tile
    required property var application
    signal editRequested()
    signal deleteRequested()
    implicitHeight: 144
    Layout.fillWidth: true
    color: Theme.card; radius: 14; border.color: Theme.border
    Rectangle {
        x: 16; y: 14; width: 38; height: 38; radius: 10; color: Theme.control
        LineIcon { anchors.centerIn: parent; width: 23; height: 23; name: tile.application.cmd ? "gamepad" : "clients"; ink: Theme.secondary }
    }
    Column {
        x: 66; y: 16; width: tile.width - 158; spacing: 4
        PanelText { text: tile.application.name || qsTr("未命名应用"); color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold; width: parent.width; elide: Text.ElideRight }
        PanelText { text: tile.application.cmd ? qsTr("● 应用程序") : qsTr("● 桌面串流"); color: Theme.success; font.pixelSize: 12 }
    }
    PanelButton { anchors.right: parent.right; anchors.rightMargin: 14; y: 20; text: qsTr("编辑"); primary: true; onClicked: tile.editRequested() }
    PanelText {
        x: 16; y: 64; width: tile.width - 32
        text: tile.application.cmd ? String(tile.application.cmd) : qsTr("串流整个桌面，在 Moonlight 中远程使用这台电脑。")
        color: Theme.secondary; font.pixelSize: 13; wrapMode: Text.WrapAnywhere; maximumLineCount: 2; elide: Text.ElideRight; lineHeight: 1.35
    }
    PanelText { x: 16; anchors.bottom: parent.bottom; anchors.bottomMargin: 15; text: tile.application["working-dir"] ? qsTr("自定义工作目录") : qsTr("默认工作目录"); color: Theme.muted; font.pixelSize: 12 }
    PanelButton { anchors.right: parent.right; anchors.rightMargin: 10; anchors.bottom: parent.bottom; anchors.bottomMargin: 6; subtle: true; symbol: "trash"; Accessible.name: qsTr("删除应用"); onClicked: tile.deleteRequested() }
}
