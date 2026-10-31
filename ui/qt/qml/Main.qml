import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

ApplicationWindow {
    id: root
    required property var host
    property bool trayAvailable: false
    property bool quitting: false
    property bool forceDark: false
    property bool forceLight: false
    property int initialPage: 0
    property int settingsCategory: 0
    property int pageIndex: initialPage
    readonly property bool settingsVisible: [3, 5, 6].includes(pageIndex)
    width: 1004; height: 631
    minimumWidth: 720; minimumHeight: 480
    visible: true
    title: "Sunshine"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    font.pixelSize: Theme.bodySize
    palette.window: Theme.background
    palette.base: Theme.popup
    palette.text: Theme.text
    palette.windowText: Theme.text
    palette.buttonText: Theme.text
    palette.button: Theme.control
    palette.highlight: Theme.accentFill
    palette.highlightedText: Theme.text
    Settings { id: preferences; property bool magpieDarkTheme: true }
    Component.onCompleted: Theme.dark = forceLight ? false : forceDark || preferences.magpieDarkTheme
    onClosing: function(close) { if (trayAvailable && !quitting) { close.accepted = false; root.hide() } }
    onPageIndexChanged: { if (scroll.contentItem) scroll.contentItem.contentY = 0 }
    Connections { target: host; function onStateChanged() { if (host.setupRequired) root.pageIndex = 6 } }

    Rectangle { anchors.fill: parent; radius: root.visibility === Window.Maximized ? 0 : 18; color: Theme.background }
    WindowChrome {
        id: chrome
        width: parent.width
        window: root
        currentPage: root.pageIndex
        onNavigate: function(page) { root.pageIndex = page }
        onRefresh: { host.refresh(); if (root.pageIndex === 4) host.refreshLogs() }
    }
    RowLayout {
        id: settingsBar
        visible: root.settingsVisible
        x: 18; y: 54; width: root.width - 36; height: 30
        spacing: 12
        SegmentedControl {
            flat: true
            model: [qsTr("常规"), qsTr("音视频"), qsTr("输入"), qsTr("网络"), qsTr("连接"), qsTr("关于")]
            currentIndex: root.pageIndex === 5 ? 5 : root.pageIndex === 6 ? 4 : root.settingsCategory
            onActivated: function(index) {
                if (index < 4) { root.settingsCategory = index; root.pageIndex = 3 }
                else root.pageIndex = index === 4 ? 6 : 5
            }
        }
        Item { Layout.fillWidth: true }
        Field {
            id: search
            implicitWidth: root.width < 800 ? 140 : 198
            placeholderText: qsTr("查找设置")
            onTextChanged: if (text !== "") root.pageIndex = 3
        }
    }
    ScrollView {
        id: scroll
        x: 0; y: root.settingsVisible ? 92 : 54
        width: root.width; height: root.height - y - 30
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: ScrollBar {
            id: bar
            width: 5; padding: 1; policy: ScrollBar.AsNeeded
            contentItem: Rectangle { implicitWidth: 3; radius: 2; color: Theme.muted; opacity: bar.active ? 0.65 : 0.4 }
            background: Item {}
        }
        Column {
            x: 18
            width: scroll.availableWidth - 36
            spacing: host.notice !== "" ? 12 : 0
            Rectangle {
                visible: host.notice !== ""
                width: parent.width
                implicitHeight: noticeLabel.implicitHeight + 24
                radius: 10; color: Theme.card; border.color: Theme.border
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 10
                    Rectangle { width: 6; height: 6; radius: 2; color: host.noticeIsError ? "#e7b554" : Theme.success }
                    Text { id: noticeLabel; text: host.notice; color: Theme.secondary; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    PanelButton { subtle: true; symbol: "close"; Accessible.name: qsTr("关闭提示"); onClicked: host.clearNotice() }
                }
            }
            Loader {
                id: pageLoader
                width: parent.width
                sourceComponent: [homePage, appsPage, clientsPage, settingsPage, logsPage, aboutPage, connectionPage, sessionsPage][root.pageIndex]
                onLoaded: reveal.restart()
                NumberAnimation { id: reveal; target: pageLoader; property: "opacity"; from: 0; to: 1; duration: 100 }
            }
            Item { width: 1; height: 14 }
        }
    }
    Rectangle {
        z: 9; anchors.fill: parent
        radius: root.visibility === Window.Maximized ? 0 : 18
        color: "transparent"; border.color: Theme.dark ? "#404047" : "#c7c7ce"
        visible: root.visibility !== Window.Maximized
    }
    Repeater {
        model: [
            {edge: Qt.LeftEdge, x: 0, y: 6, w: 5, h: root.height - 12, cursor: Qt.SizeHorCursor},
            {edge: Qt.RightEdge, x: root.width - 5, y: 6, w: 5, h: root.height - 12, cursor: Qt.SizeHorCursor},
            {edge: Qt.TopEdge, x: 6, y: 0, w: root.width - 12, h: 5, cursor: Qt.SizeVerCursor},
            {edge: Qt.BottomEdge, x: 6, y: root.height - 5, w: root.width - 12, h: 5, cursor: Qt.SizeVerCursor},
            {edge: Qt.LeftEdge | Qt.TopEdge, x: 0, y: 0, w: 6, h: 6, cursor: Qt.SizeFDiagCursor},
            {edge: Qt.RightEdge | Qt.TopEdge, x: root.width - 6, y: 0, w: 6, h: 6, cursor: Qt.SizeBDiagCursor},
            {edge: Qt.LeftEdge | Qt.BottomEdge, x: 0, y: root.height - 6, w: 6, h: 6, cursor: Qt.SizeBDiagCursor},
            {edge: Qt.RightEdge | Qt.BottomEdge, x: root.width - 6, y: root.height - 6, w: 6, h: 6, cursor: Qt.SizeFDiagCursor}
        ]
        delegate: MouseArea {
            required property var modelData
            property point dragOrigin
            property rect startBounds
            property bool systemResize: false
            z: 10; x: modelData.x; y: modelData.y; width: modelData.w; height: modelData.h
            enabled: root.visibility !== Window.Maximized
            cursorShape: modelData.cursor
            onPressed: function(mouse) {
                dragOrigin = mapToGlobal(mouse.x, mouse.y)
                startBounds = Qt.rect(root.x, root.y, root.width, root.height)
                systemResize = Qt.platform.os === "osx" ? false : root.startSystemResize(modelData.edge)
            }
            onPositionChanged: function(mouse) {
                if (!pressed || systemResize) return
                // Some Qt platform plugins do not implement startSystemResize.
                const point = mapToGlobal(mouse.x, mouse.y)
                const dx = point.x - dragOrigin.x
                const dy = point.y - dragOrigin.y
                if (modelData.edge & Qt.RightEdge) root.width = Math.max(root.minimumWidth, startBounds.width + dx)
                if (modelData.edge & Qt.BottomEdge) root.height = Math.max(root.minimumHeight, startBounds.height + dy)
                if (modelData.edge & Qt.LeftEdge) {
                    root.width = Math.max(root.minimumWidth, startBounds.width - dx)
                    root.x = startBounds.x + startBounds.width - root.width
                }
                if (modelData.edge & Qt.TopEdge) {
                    root.height = Math.max(root.minimumHeight, startBounds.height - dy)
                    root.y = startBounds.y + startBounds.height - root.height
                }
            }
        }
    }
    Component { id: homePage; HomePage { host: root.host; onNavigate: function(page) { root.pageIndex = page } } }
    Component { id: appsPage; AppsPage { host: root.host } }
    Component { id: clientsPage; ClientsPage { host: root.host } }
    Component { id: settingsPage; SettingsPage { host: root.host; category: root.settingsCategory; searchTerm: search.text } }
    Component { id: logsPage; LogsPage { host: root.host } }
    Component { id: sessionsPage; SessionsPage { host: root.host } }
    Component {
        id: connectionPage
        ConnectionPage {
            host: root.host
            onThemeSelected: function(dark) { preferences.magpieDarkTheme = dark; Theme.dark = dark }
        }
    }
    Component {
        id: aboutPage
        ColumnLayout {
            spacing: 12
            SettingGroup {
                SettingCard { grouped: true; title: qsTr("版本"); description: qsTr("Sunshine 控制面板"); Text { text: "1.0.0"; color: Theme.secondary; font.pixelSize: 13 } }
                SettingCard { grouped: true; title: qsTr("主机版本"); description: qsTr("当前连接的 Sunshine 主机"); Text { text: host.config.version || qsTr("未连接"); color: Theme.muted; font.pixelSize: 13 } }
                SettingCard { grouped: true; title: qsTr("界面框架"); description: qsTr("原生桌面界面"); Text { text: "Qt 6.12"; color: Theme.secondary; font.pixelSize: 13 } }
                SettingCard {
                    grouped: true; title: qsTr("应用主题"); description: qsTr("选择控制面板的外观")
                    SegmentedControl { model: [qsTr("浅色"), qsTr("深色")]; currentIndex: Theme.dark ? 1 : 0; onActivated: function(index) { preferences.magpieDarkTheme = index === 1; Theme.dark = preferences.magpieDarkTheme } }
                }
                SettingCard { grouped: true; last: true; title: qsTr("开源许可"); description: qsTr("Sunshine 的采集、编码和串流内核保持独立"); Text { text: "GPL-3.0"; color: Theme.muted; font.pixelSize: 13 } }
            }
        }
    }
}
