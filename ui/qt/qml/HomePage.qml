import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    signal navigate(int page)
    spacing: 12
    RowLayout {
        Layout.fillWidth: true
        Item { Layout.fillWidth: true }
        PanelButton { text: qsTr("连接设置"); subtle: true; symbol: "network"; onClicked: page.navigate(6) }
    }
    SettingGroup {
        SettingCard {
            grouped: true; symbol: "clients"; prominent: true
            title: host.config.sunshine_name || "Sunshine"
            description: host.connected ? qsTr("127.0.0.1:%1 · %2 个活动会话").arg(host.port).arg(host.sessions.length) : qsTr("本地主机 · 尚未连接")
            Rectangle { width: 6; height: 6; radius: 2; color: host.connected ? Theme.success : Theme.muted }
            PanelText { text: host.connected ? qsTr("已连接") : qsTr("未连接"); color: Theme.muted; font.pixelSize: 13 }
            PanelButton { text: "›"; subtle: true; Accessible.name: qsTr("主机连接设置"); onClicked: page.navigate(6) }
        }
        SettingCard {
            grouped: true; symbol: "chip"; prominent: true
            title: qsTr("视频编码器")
            description: qsTr("采集与编码 · 由主机自动选择")
            PanelButton { text: host.config.active_encoder || qsTr("自动选择"); implicitWidth: page.width < 800 ? 172 : 216; symbol: "chip"; onClicked: page.navigate(3) }
            PanelButton { text: "›"; subtle: true; Accessible.name: qsTr("编码设置"); onClicked: page.navigate(3) }
        }
        SettingCard {
            grouped: true; last: true; symbol: "gamepad"; prominent: true
            title: qsTr("Moonlight 客户端")
            description: qsTr("%1 台设备 · 使用 PIN 配对").arg(host.clients.length)
            PanelButton { text: qsTr("配对设备"); subtle: true; onClicked: page.navigate(2) }
            PanelButton { text: "›"; subtle: true; Accessible.name: qsTr("管理客户端"); onClicked: page.navigate(2) }
        }
    }
    PanelButton { text: qsTr("添加串流应用"); symbol: "add"; subtle: true; onClicked: page.navigate(1) }
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 10
        PanelText { text: qsTr("串流应用（%1）").arg(host.apps.length); color: Theme.muted; font.pixelSize: 13; Layout.fillWidth: true }
        PanelButton { text: qsTr("管理"); subtle: true; onClicked: page.navigate(1) }
    }
    SettingGroup {
        Repeater {
            model: host.apps.slice(0, 3)
            delegate: SettingCard {
                required property var modelData
                required property int index
                grouped: true; last: index === Math.min(host.apps.length, 3) - 1
                minimumHeight: 42; title: modelData.name || qsTr("未命名应用"); symbol: modelData.cmd ? "apps" : "clients"
                PanelText { text: modelData.cmd ? qsTr("应用程序") : qsTr("桌面串流"); color: Theme.muted; font.pixelSize: 13 }
                PanelButton { text: "›"; subtle: true; Accessible.name: qsTr("管理应用"); onClicked: page.navigate(1) }
            }
        }
        SettingCard { visible: host.apps.length === 0; grouped: true; last: true; title: qsTr("暂无串流应用"); description: qsTr("添加桌面或游戏后，可从 Moonlight 启动。") }
    }
}
