import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card
    property string title
    property string description
    property string symbol: ""
    property bool prominent: false
    property bool grouped: false
    property bool last: false
    property color descriptionColor: Theme.muted
    property int minimumHeight: Theme.cardHeight
    readonly property bool wrapped: width < 530 && actions.implicitWidth > 230
    default property alias controls: actions.data
    implicitHeight: wrapped ? words.implicitHeight + actions.implicitHeight + 34 : Math.max(minimumHeight, words.implicitHeight + 24, actions.implicitHeight + 24)
    Layout.fillWidth: true
    color: grouped ? "transparent" : Theme.card
    radius: grouped ? 0 : 12
    border.color: grouped ? "transparent" : Theme.border
    Rectangle { visible: card.grouped && !card.last; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.divider }
    Rectangle {
        id: iconBox
        visible: card.symbol !== ""
        x: 14; y: card.wrapped ? 15 : (card.height - height) / 2
        width: 28; height: 28
        color: "transparent"
        LineIcon { anchors.centerIn: parent; name: card.symbol; width: 23; height: 23; ink: Theme.secondary }
    }
    Column {
        id: words
        x: iconBox.visible ? 52 : 14
        y: card.wrapped ? 14 : (card.height - height) / 2
        width: Math.max(60, card.width - x - 14 - (card.wrapped || actions.implicitWidth === 0 ? 0 : actions.implicitWidth + 24))
        spacing: 3
        PanelText { text: card.title; color: Theme.text; font.pixelSize: Theme.bodySize; font.weight: card.prominent ? Font.DemiBold : Font.Normal; width: parent.width; wrapMode: Text.Wrap }
        PanelText { visible: text !== ""; text: card.description; color: card.descriptionColor; font.pixelSize: Theme.captionSize; width: parent.width; wrapMode: Text.Wrap }
    }
    RowLayout {
        id: actions
        x: card.width - width - 14
        y: card.wrapped ? words.y + words.height + 8 : (card.height - height) / 2
        spacing: 12
    }
}
