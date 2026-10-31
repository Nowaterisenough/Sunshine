import QtQuick
import QtQuick.Controls

Item {
    id: chrome
    required property var window
    property int currentPage: 0
    readonly property bool macControls: Qt.platform.os === "osx"
    readonly property bool compact: width < 900
    signal navigate(int page)
    signal refresh()
    height: 50
    MouseArea {
        property point dragOrigin
        property point startPosition
        property bool systemMove: false
        anchors.fill: parent
        onPressed: function(mouse) {
            dragOrigin = mapToGlobal(mouse.x, mouse.y)
            startPosition = Qt.point(chrome.window.x, chrome.window.y)
            systemMove = chrome.window.startSystemMove()
        }
        onPositionChanged: function(mouse) {
            if (!pressed || systemMove || chrome.window.visibility === Window.Maximized) return
            const point = mapToGlobal(mouse.x, mouse.y)
            chrome.window.x = startPosition.x + point.x - dragOrigin.x
            chrome.window.y = startPosition.y + point.y - dragOrigin.y
        }
        onDoubleClicked: chrome.window.visibility === Window.Maximized ? chrome.window.showNormal() : chrome.window.showMaximized()
    }
    Row {
        visible: chrome.macControls
        x: 19; y: 19; spacing: 9
        Repeater {
            model: ["close", "minimize", "maximize"]
            delegate: AbstractButton {
                id: traffic
                required property string modelData
                width: 14; height: 14
                hoverEnabled: true
                Accessible.name: modelData === "close" ? qsTr("关闭") : modelData === "minimize" ? qsTr("最小化") : qsTr("最大化或还原")
                background: Rectangle { radius: 7; color: traffic.modelData === "close" ? "#ff6057" : traffic.modelData === "minimize" ? "#ffbd2e" : "#28c840"; border.color: "#30ffffff" }
                contentItem: Item { LineIcon { visible: traffic.hovered; anchors.centerIn: parent; width: 8; height: 8; name: traffic.modelData; ink: "#a0000000" } }
                onClicked: chrome.captionAction(modelData)
            }
        }
    }
    PanelButton {
        visible: chrome.macControls && !chrome.compact
        x: 100; y: 12; implicitWidth: 22; subtle: true; symbol: "panel"
        Accessible.name: qsTr("连接设置")
        ToolTip.visible: hovered; ToolTip.text: qsTr("连接设置")
        onClicked: chrome.navigate(6)
    }
    Row {
        x: chrome.macControls ? (chrome.compact ? 100 : 134) : 20
        y: 16
        spacing: 9
        LineIcon { name: "sun"; width: 18; height: 18; ink: Theme.text }
        Text { text: "sunshine"; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold; height: 18; verticalAlignment: Text.AlignVCenter }
    }
    Flickable {
        x: chrome.macControls ? (chrome.compact ? 212 : 242) : 156
        y: 11; width: chrome.width - x - (chrome.macControls ? 78 : 176); height: 30
        contentWidth: navigation.width; clip: true
        flickableDirection: Flickable.HorizontalFlick
        Row {
            id: navigation
            spacing: 2
            Repeater {
                model: [{label: qsTr("主机"), page: 0}, {label: qsTr("会话"), page: 7}, {label: qsTr("应用"), page: 1}, {label: qsTr("客户端"), page: 2}, {label: qsTr("日志"), page: 4}]
                delegate: AbstractButton {
                    id: nav
                    required property var modelData
                    width: caption.implicitWidth + 26; height: 28
                    hoverEnabled: true
                    Accessible.name: modelData.label
                    onClicked: chrome.navigate(modelData.page)
                    contentItem: Text { id: caption; text: nav.modelData.label; color: chrome.currentPage === nav.modelData.page ? Theme.text : Theme.muted; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: 6; color: chrome.currentPage === nav.modelData.page ? Theme.selected : nav.hovered ? Theme.hover : "transparent"; border.color: nav.activeFocus ? Theme.accent : "transparent" }
                }
            }
        }
    }
    Row {
        anchors.right: parent.right; anchors.rightMargin: chrome.macControls ? 10 : 108
        y: 11; spacing: 4
        PanelButton { subtle: true; symbol: "refresh"; Accessible.name: qsTr("刷新"); ToolTip.visible: hovered; ToolTip.text: qsTr("刷新"); onClicked: chrome.refresh() }
        PanelButton { subtle: ![3,5,6].includes(chrome.currentPage); symbol: "connection"; implicitWidth: 28; Accessible.name: qsTr("设置"); ToolTip.visible: hovered; ToolTip.text: qsTr("设置"); onClicked: chrome.navigate(3) }
    }
    Row {
        visible: !chrome.macControls
        anchors.right: parent.right; anchors.rightMargin: 8; y: 11
        Repeater {
            model: ["minimize", "maximize", "close"]
            delegate: PanelButton { required property string modelData; subtle: true; symbol: modelData; implicitWidth: 30; onClicked: chrome.captionAction(modelData) }
        }
    }
    function captionAction(action) {
        if (action === "close") window.close()
        else if (action === "minimize") window.showMinimized()
        else if (window.visibility === Window.Maximized) window.showNormal()
        else window.showMaximized()
    }
}
