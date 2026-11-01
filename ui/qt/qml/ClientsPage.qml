import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    property string selectedUuid: ""
    property bool removing: false
    spacing: Theme.cardSpacing
    PanelText { text: qsTr("在 Moonlight 中添加这台主机，输入客户端显示的 PIN 完成配对。"); color: Theme.secondary; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.Wrap }
    SettingGroup {
        SettingCard {
            grouped: true; last: true; title: qsTr("配对新设备"); description: qsTr("输入设备名称与配对码")
            Field { id: deviceName; objectName: "pairName"; implicitWidth: 190; placeholderText: qsTr("设备名称"); enabled: !host.busy }
            Field { id: pin; objectName: "pairPin"; implicitWidth: 100; placeholderText: "PIN"; maximumLength: 4; validator: RegularExpressionValidator { regularExpression: /[0-9]{4}/ } enabled: !host.busy }
            PanelButton { text: qsTr("配对"); primary: true; enabled: host.connected && !host.busy && pin.acceptableInput && deviceName.text.trim() !== ""; onClicked: host.pair(pin.text, deviceName.text) }
        }
    }
    SectionTitle { first: true; title: qsTr("已配对设备"); description: qsTr("%1 台设备").arg(host.clients.length) }
    SettingGroup {
        SettingCard { grouped: true; last: true; visible: host.clients.length === 0; title: qsTr("暂无已配对设备"); description: qsTr("完成一次 PIN 配对后，设备会出现在这里。") }
        Repeater {
            model: host.clients
            delegate: SettingCard {
                id: row
                required property var modelData
                required property int index
                grouped: true; last: index === host.clients.length - 1; minimumHeight: Theme.cardHeight; prominent: true
                title: modelData.name || qsTr("未命名设备")
                description: modelData.streaming ? qsTr("● 正在串流") : modelData.enabled === false ? qsTr("● 已停用") : qsTr("● 已配对 · 允许连接")
                descriptionColor: modelData.streaming ? Theme.success : Theme.muted
                symbol: "clients"
                PanelButton { text: qsTr("重命名"); subtle: true; enabled: host.connected && !host.busy; onClicked: { page.selectedUuid = modelData.uuid; newName.text = modelData.name || ""; rename.open() } }
                PanelButton { text: qsTr("取消配对"); subtle: true; enabled: host.connected && !host.busy; onClicked: { page.selectedUuid = modelData.uuid; page.removing = true; confirm.open() } }
                Toggle {
                    checked: row.modelData.enabled !== false
                    enabled: host.connected && !host.busy
                    Accessible.name: qsTr("允许 %1 连接").arg(row.modelData.name)
                    onClicked: {
                        const requested = row.modelData.enabled === false
                        checked = Qt.binding(function() { return row.modelData.enabled !== false })
                        if (requested) host.setClientEnabled(row.modelData.uuid, true)
                        else { page.selectedUuid = row.modelData.uuid; page.removing = false; confirm.open() }
                    }
                }
            }
        }
    }
    PanelDialog {
        id: confirm
        width: 420
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true
        title: page.removing ? qsTr("取消此设备的配对？") : qsTr("停用此设备？")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label { text: page.removing ? qsTr("当前连接将中断，再次连接需要重新配对。") : qsTr("当前连接将中断。重新启用后可再次连接。"); wrapMode: Text.Wrap }
        onAccepted: { if (page.removing) host.unpair(page.selectedUuid); else host.setClientEnabled(page.selectedUuid, false) }
    }
    PanelDialog {
        id: rename
        width: 400
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true
        title: qsTr("设备名称")
        contentItem: Field { id: newName; implicitWidth: 300 }
        footer: DialogActions {
            PanelButton { text: qsTr("取消"); enabled: !host.busy; onClicked: rename.close() }
            PanelButton { text: qsTr("保存"); primary: true; enabled: !host.busy && newName.text.trim() !== ""; onClicked: host.renameClient(page.selectedUuid, newName.text) }
        }
    }
    Connections { target: host; function onOperationSucceeded(operation) { if (operation === "pair") { pin.text = ""; deviceName.text = "" } if (operation === "clients") rename.close() } }
}
