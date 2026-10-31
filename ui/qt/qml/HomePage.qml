import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    signal navigate(int page)
    spacing: 14
    Text { text: qsTr("通过 Moonlight 连接这台电脑，管理你的桌面、游戏和客户端。"); color: Theme.secondary; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.Wrap }
    SettingGroup {
        SettingCard {
            grouped: true; symbol: "clients"; minimumHeight: 62
            title: host.config.sunshine_name || "Sunshine"
            description: host.connected ? (host.sessions.length ? qsTr("● 已连接 · %1 个串流会话正在运行").arg(host.sessions.length) : qsTr("● 已连接 · 等待客户端串流")) : qsTr("● 未连接 · 启动主机后自动重连")
            descriptionColor: host.connected ? Theme.success : Theme.muted
            Text { text: host.config.version || ""; color: Theme.muted; font.pixelSize: 12 }
            PanelButton { text: qsTr("连接设置"); subtle: true; onClicked: page.navigate(6) }
        }
        SettingCard {
            grouped: true; symbol: "chip"; minimumHeight: 62
            title: qsTr("视频编码器")
            description: qsTr("采集与编码由 Sunshine 主机处理")
            PanelButton { text: host.config.active_encoder || qsTr("自动选择"); implicitWidth: 216; symbol: "chip"; onClicked: page.navigate(3) }
        }
        SettingCard {
            grouped: true; symbol: "gamepad"; minimumHeight: 62
            title: qsTr("Moonlight 客户端")
            description: qsTr("%1 台设备已配对 · 使用 PIN 连接新的设备").arg(host.clients.length)
            PanelButton { text: qsTr("管理"); subtle: true; onClicked: page.navigate(2) }
            PanelButton { text: qsTr("配对设备"); primary: true; onClicked: page.navigate(2) }
        }
        Item {
            Layout.fillWidth: true; implicitHeight: 40
            PanelButton { anchors.centerIn: parent; subtle: true; text: qsTr("查看 %1 个活动会话 ›").arg(host.sessions.length); onClicked: page.navigate(7) }
            PanelButton { anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; subtle: true; symbol: "settings"; text: qsTr("设置"); onClicked: page.navigate(3) }
        }
    }
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 4
        Text { text: qsTr("串流应用（%1）").arg(host.apps.length); color: Theme.muted; font.pixelSize: 13; Layout.fillWidth: true }
        PanelButton { text: qsTr("管理应用"); subtle: true; onClicked: page.navigate(1) }
    }
    SettingCard {
        minimumHeight: 42; title: qsTr("游戏与桌面"); symbol: "apps"
        Text { text: qsTr("从 Moonlight 中启动已配置的应用"); color: Theme.muted; font.pixelSize: 13 }
        PanelButton { text: "›"; subtle: true; onClicked: page.navigate(1) }
    }
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 6
        Text { text: host.connected ? qsTr("本地主机  ·  127.0.0.1:%1").arg(host.port) : qsTr("本地主机尚未连接"); color: Theme.muted; font.pixelSize: 12; Layout.fillWidth: true }
        PanelButton { text: qsTr("查看日志"); subtle: true; onClicked: page.navigate(4) }
    }
}
