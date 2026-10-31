import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    signal themeSelected(bool dark)
    spacing: 14
    SettingGroup {
        SettingCard {
            grouped: true; title: qsTr("管理端口"); description: qsTr("默认 47990，自定义串流端口时为该端口 + 1")
            Field { id: portField; text: String(host.port); validator: IntValidator { bottom: 1; top: 65535 } implicitWidth: 100; enabled: !host.busy }
            PanelButton { text: qsTr("连接"); primary: true; enabled: !host.busy && portField.acceptableInput; onClicked: { host.port = Number(portField.text); host.refresh() } }
        }
        SettingCard {
            grouped: true; title: qsTr("连接状态"); description: "https://127.0.0.1:" + host.port
            Text { text: host.connected ? qsTr("● 已连接") : host.setupRequired ? qsTr("需要初始化") : qsTr("未连接"); color: host.connected ? Theme.success : Theme.muted; font.pixelSize: 13 }
        }
        SettingCard {
            grouped: true; title: qsTr("Sunshine 内核程序"); description: host.executable || qsTr("自动查找安装目录中的可执行文件")
            PanelButton { text: qsTr("选择文件"); subtle: true; onClicked: executableDialog.open() }
        }
        SettingCard {
            grouped: true; last: true; title: qsTr("启动主机"); description: qsTr("系统服务请通过服务管理器启动；便携版可从这里启动")
            PanelButton { text: qsTr("启动 Sunshine"); primary: true; enabled: !host.connected && !host.setupRequired && !host.busy; onClicked: host.startHost() }
        }
    }
    SectionTitle { visible: host.setupRequired; first: true; title: qsTr("首次使用"); description: qsTr("创建 Sunshine 管理账户") }
    Rectangle {
        visible: host.setupRequired
        Layout.fillWidth: true; implicitHeight: setupForm.implicitHeight + 32
        color: Theme.card; border.color: Theme.border; radius: 14
        ColumnLayout {
            id: setupForm
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16
            spacing: 10
            Field { id: username; Layout.fillWidth: true; placeholderText: qsTr("管理员用户名") }
            Field { id: password; Layout.fillWidth: true; placeholderText: qsTr("密码"); echoMode: TextInput.Password }
            Field { id: confirmation; Layout.fillWidth: true; placeholderText: qsTr("再次输入密码"); echoMode: TextInput.Password }
            PanelButton { text: qsTr("创建账户"); primary: true; enabled: !host.busy; onClicked: host.setup(username.text, password.text, confirmation.text) }
        }
    }
    FileDialog { id: executableDialog; title: qsTr("选择 Sunshine 内核程序"); fileMode: FileDialog.OpenFile; onAccepted: host.executable = selectedFile.toString() }
    Connections { target: host; function onOperationSucceeded(operation) { if (operation === "setup") { password.text = ""; confirmation.text = "" } } }
}
