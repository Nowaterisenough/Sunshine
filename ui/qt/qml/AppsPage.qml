import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    property int editingIndex: -1
    property int deletingIndex: -1
    property int filterMode: 0
    readonly property var filteredApps: host.apps.map(function(app, index) { return {application: app, originalIndex: index} }).filter(function(entry) {
        const matchesType = page.filterMode === 0 || (page.filterMode === 1 ? !entry.application.cmd : !!entry.application.cmd)
        return matchesType && String(entry.application.name || "").toLowerCase().includes(search.text.toLowerCase())
    })
    spacing: 14
    function openEditor(index) {
        editingIndex = index
        const app = index >= 0 ? host.apps[index] : {}
        appName.text = app.name || ""
        command.text = app.cmd || ""
        directory.text = app["working-dir"] || ""
        imagePath.text = app["image-path"] || ""
        editor.open()
    }
    RowLayout {
        Layout.fillWidth: true
        SegmentedControl { model: [qsTr("全部"), qsTr("桌面"), qsTr("应用")]; currentIndex: page.filterMode; onActivated: function(index) { page.filterMode = index } }
        Item { Layout.fillWidth: true }
        Field { id: search; Layout.preferredWidth: Math.min(280, page.width * 0.3); placeholderText: qsTr("搜索应用…") }
        PanelButton { text: qsTr("添加应用"); symbol: "add"; primary: true; enabled: host.connected && !host.busy; onClicked: page.openEditor(-1) }
    }
    Rectangle {
        Layout.fillWidth: true; implicitHeight: 174
        radius: 14; border.color: Theme.border
        gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: Theme.card } GradientStop { position: 1; color: Theme.dark ? "#30313c" : "#e9eaf3" } }
        Column {
            x: 20; y: 20; width: parent.width - 210; spacing: 8
            PanelText { text: qsTr("把游戏与桌面带到每一块屏幕"); color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold; width: parent.width; wrapMode: Text.Wrap }
            PanelText { text: qsTr("应用会显示在已配对的 Moonlight 客户端中。配置启动命令、工作目录，或添加一个桌面入口，即可从其他设备开始串流。"); color: Theme.secondary; font.pixelSize: 13; width: parent.width; wrapMode: Text.Wrap; lineHeight: 1.5 }
            PanelText { text: qsTr("编辑应用时，现有准备命令与高级参数会保留。"); color: Theme.muted; font.pixelSize: 12; width: parent.width; wrapMode: Text.Wrap }
        }
        Repeater {
            model: ["clients", "gamepad", "apps"]
            delegate: Rectangle {
                required property string modelData
                required property int index
                x: parent.width - 152 + (index % 2) * 62; y: 38 + (index === 2 ? 56 : 0)
                width: 44; height: 44; radius: 12; rotation: index === 1 ? 9 : -9
                color: Theme.control
                LineIcon { anchors.centerIn: parent; name: modelData; width: 25; height: 25; ink: index === 1 ? Theme.accent : Theme.secondary }
            }
        }
    }
    SectionTitle { first: true; title: qsTr("应用程序"); description: qsTr("%1 个应用 · 从 Moonlight 启动").arg(host.apps.length) }
    SettingCard { visible: page.filteredApps.length === 0; title: host.connected ? qsTr("没有匹配的应用") : qsTr("等待主机连接"); description: qsTr("添加一个应用，或更改搜索条件。"); symbol: "apps" }
    Grid {
        id: applicationGrid
        Layout.fillWidth: true
        columns: width >= 900 ? 3 : width >= 620 ? 2 : 1
        spacing: 12
        Repeater {
            model: page.filteredApps
            delegate: AppTile {
                required property var modelData
                width: (applicationGrid.width - applicationGrid.spacing * (applicationGrid.columns - 1)) / applicationGrid.columns
                height: implicitHeight
                application: modelData.application
                enabled: host.connected && !host.busy
                onEditRequested: page.openEditor(modelData.originalIndex)
                onDeleteRequested: { page.deletingIndex = modelData.originalIndex; deletion.open() }
            }
        }
    }
    PanelDialog {
        id: editor
        parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(540, parent.width - 60)
        modal: true; closePolicy: Popup.NoAutoClose
        title: page.editingIndex < 0 ? qsTr("添加应用") : qsTr("编辑应用")
        contentItem: ColumnLayout {
            spacing: 10
            Label { text: qsTr("应用名称") }
            Field { id: appName; objectName: "appName"; Layout.fillWidth: true; placeholderText: qsTr("例如：桌面 / Steam Big Picture") }
            Label { text: qsTr("启动命令") }
            Field { id: command; Layout.fillWidth: true; placeholderText: qsTr("留空以串流桌面") }
            Label { text: qsTr("工作目录") }
            Field { id: directory; Layout.fillWidth: true; placeholderText: qsTr("可选") }
            Label { text: qsTr("封面文件路径") }
            Field { id: imagePath; Layout.fillWidth: true; placeholderText: qsTr("可选，本机图片路径") }
            Label { text: qsTr("命令仅在 Moonlight 启动该应用时执行。"); font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { visible: host.noticeIsError; text: host.notice; color: Theme.danger; wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
        footer: DialogActions {
            PanelButton { text: qsTr("取消"); enabled: !host.busy; onClicked: editor.close() }
            PanelButton { text: host.busy ? qsTr("保存中…") : qsTr("保存"); primary: true; enabled: host.connected && !host.busy && appName.text.trim() !== ""; onClicked: host.saveApp(page.editingIndex, {"name": appName.text, "cmd": command.text, "working-dir": directory.text, "image-path": imagePath.text}) }
        }
    }
    PanelDialog {
        id: deletion
        width: 420
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true
        title: qsTr("从串流列表移除应用？")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label { text: qsTr("游戏文件不会被删除。"); wrapMode: Text.Wrap }
        onAccepted: host.deleteApp(page.deletingIndex)
    }
    Connections {
        target: host
        function onOperationSucceeded(operation) { if (operation === "apps") editor.close() }
        function onAppListInvalidated() { editor.close() }
    }
}
