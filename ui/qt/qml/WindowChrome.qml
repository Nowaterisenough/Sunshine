import QtQuick
import QtQuick.Controls

Item {
    id: chrome
    required property var window
    property int currentPage: 0
    readonly property bool macControls: Qt.platform.os === "osx"
    readonly property int navigationY: macControls ? 10 : 40
    signal navigate(int page)
    signal refresh()
    height: macControls ? 48 : 78
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
                background: Rectangle { radius: 7; color: traffic.modelData === "close" ? "#ff6057" : traffic.modelData === "minimize" ? "#ffbd2e" : "#28c840" }
                contentItem: Item { LineIcon { visible: traffic.hovered; anchors.centerIn: parent; width: 8; height: 8; name: traffic.modelData; ink: "#a0000000" } }
                onClicked: chrome.captionAction(modelData)
            }
        }
    }
    Row {
        x: chrome.macControls ? 134 : 10
        y: chrome.macControls ? 16 : 8
        spacing: 6
        Image { source: "../assets/sunshine-mark.svg"; width: 16; height: 16; anchors.verticalCenter: parent.verticalCenter }
        PanelText { text: "sunshine"; color: Theme.text; font.pixelSize: chrome.macControls ? 14 : 12; font.weight: chrome.macControls ? Font.DemiBold : Font.Normal; height: 18; verticalAlignment: Text.AlignVCenter }
    }
    PanelButton {
        x: chrome.macControls ? 96 : 16; y: chrome.navigationY
        implicitWidth: 28; subtle: true; symbol: "panel"
        Accessible.name: qsTr("连接设置")
        ToolTip.visible: hovered; ToolTip.text: qsTr("连接设置")
        onClicked: chrome.navigate(6)
    }
    Flickable {
        readonly property real leftLimit: chrome.macControls ? 238 : 56
        readonly property real rightLimit: chrome.width - 84
        x: chrome.macControls ? leftLimit : Math.max(leftLimit, (chrome.width - contentWidth) / 2)
        y: chrome.navigationY
        width: Math.min(contentWidth, rightLimit - x); height: 28
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
                    width: caption.implicitWidth + 24; height: 26
                    hoverEnabled: true
                    Accessible.name: modelData.label
                    onClicked: chrome.navigate(modelData.page)
                    contentItem: PanelText { id: caption; text: nav.modelData.label; color: chrome.currentPage === nav.modelData.page ? Theme.text : Theme.muted; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: 6; color: chrome.currentPage === nav.modelData.page ? Theme.selected : nav.hovered ? Theme.hover : "transparent"; border.color: nav.activeFocus ? Theme.accent : "transparent" }
                }
            }
        }
    }
    Row {
        anchors.right: parent.right; anchors.rightMargin: 10
        y: chrome.navigationY; spacing: 4
        PanelButton { subtle: true; symbol: "refresh"; Accessible.name: qsTr("刷新"); ToolTip.visible: hovered; ToolTip.text: qsTr("刷新"); onClicked: chrome.refresh() }
        PanelButton { subtle: ![3,5,6].includes(chrome.currentPage); symbol: "connection"; implicitWidth: 28; Accessible.name: qsTr("设置"); ToolTip.visible: hovered; ToolTip.text: qsTr("设置"); onClicked: chrome.navigate(3) }
    }
    Row {
        visible: !chrome.macControls
        anchors.right: parent.right; anchors.rightMargin: 2; y: 2
        Repeater {
            model: ["minimize", "maximize", "close"]
            delegate: AbstractButton {
                id: captionButton
                required property string modelData
                width: 46; height: 28
                hoverEnabled: true
                Accessible.name: modelData === "close" ? qsTr("关闭") : modelData === "minimize" ? qsTr("最小化") : qsTr("最大化或还原")
                background: Rectangle { radius: 4; color: captionButton.hovered ? (captionButton.modelData === "close" ? "#c42b1c" : Theme.hover) : "transparent" }
                contentItem: Item { LineIcon { anchors.centerIn: parent; width: 13; height: 13; name: captionButton.modelData === "maximize" && chrome.window.visibility === Window.Maximized ? "restore" : captionButton.modelData; ink: Theme.secondary } }
                onClicked: chrome.captionAction(modelData)
            }
        }
    }
    function captionAction(action) {
        if (action === "close") window.close()
        else if (action === "minimize") window.showMinimized()
        else if (window.visibility === Window.Maximized) window.showNormal()
        else window.showMaximized()
    }
}
