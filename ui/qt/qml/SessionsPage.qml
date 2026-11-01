import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    spacing: 14
    RowLayout {
        Layout.fillWidth: true
        PanelText { text: qsTr("活动会话  ·  连接状态每 5 秒更新"); color: Theme.muted; font.pixelSize: 13; Layout.fillWidth: true }
        PanelButton { text: qsTr("断开全部"); destructive: true; enabled: host.connected && host.sessions.length > 0 && !host.busy; onClicked: stopDialog.open() }
    }
    SettingGroup {
        SettingCard { grouped: true; last: true; visible: host.sessions.length === 0; title: qsTr("暂无活动会话"); description: qsTr("从 Moonlight 连接后，分辨率、帧率与码率会显示在这里。"); symbol: "sessions" }
        Repeater {
            model: host.sessions
            delegate: SettingCard {
                required property var modelData
                required property int index
                grouped: true; last: index === host.sessions.length - 1; symbol: "clients"
                title: (modelData.client_name || qsTr("未命名客户端")) + " · " + (modelData.app_name || qsTr("桌面"))
                description: modelData.width + " × " + modelData.height + " · " + modelData.fps + " FPS · " + (Number(modelData.bitrate) / 1000).toFixed(1) + " Mbps" + (modelData.enable_hdr ? " · HDR" : "")
                PanelText { text: modelData.state === "RUNNING" ? qsTr("● 串流中") : modelData.state; color: Theme.success; font.pixelSize: 13 }
            }
        }
    }
    PanelDialog {
        id: stopDialog
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 420
        title: qsTr("断开当前全部串流？")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label { text: qsTr("所有 Moonlight 客户端的当前会话都会结束。"); wrapMode: Text.Wrap }
        onAccepted: host.stopSessions()
    }
}
