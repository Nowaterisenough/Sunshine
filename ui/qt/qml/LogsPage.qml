import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    spacing: 12
    Component.onCompleted: host.refreshLogs()
    Connections { target: host; function onStateChanged() { if (host.connected) host.refreshLogs() } }
    SectionTitle { first: true; title: qsTr("主机日志"); description: qsTr("查看最近的运行记录。日志只保留在本机，可手动导出排查问题。") }
    RowLayout {
        Layout.fillWidth: true
        Field { id: filter; placeholderText: qsTr("筛选日志内容…"); Layout.fillWidth: true }
        PanelButton { text: qsTr("重新读取"); enabled: host.connected; onClicked: host.refreshLogs() }
        PanelButton { text: qsTr("导出日志"); enabled: host.logs !== ""; onClicked: saveDialog.open() }
    }
    Rectangle {
        Layout.fillWidth: true; implicitHeight: 440
        color: Theme.card; radius: Theme.controlRadius; border.color: Theme.border
        ScrollView {
            anchors.fill: parent; anchors.margins: 12
            TextArea {
                readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText
                color: Theme.text; font.family: Qt.platform.os === "windows" ? "Cascadia Mono" : "Menlo"; font.pixelSize: 11
                wrapMode: TextEdit.WrapAnywhere
                placeholderText: host.connected ? qsTr("暂无日志。") : qsTr("连接主机后读取日志。")
                text: filter.text === "" ? host.logs : host.logs.split("\n").filter(function(line) { return line.toLowerCase().includes(filter.text.toLowerCase()) }).join("\n")
                background: null
            }
        }
    }
    PanelText { text: qsTr("显示最近最多 1 MiB 的日志；导出内容为当前读取的日志，不包含 AI 分析。"); color: Theme.muted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap }
    FileDialog { id: saveDialog; title: qsTr("导出日志"); fileMode: FileDialog.SaveFile; selectedFile: "file:sunshine.log"; nameFilters: [qsTr("日志文件 (*.log *.txt)")]; onAccepted: host.exportLogs(selectedFile) }
}
