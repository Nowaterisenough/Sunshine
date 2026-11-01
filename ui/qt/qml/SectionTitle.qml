import QtQuick
import QtQuick.Layouts

RowLayout {
    id: section
    property string title
    property string description
    property bool first: false
    Layout.fillWidth: true
    Layout.topMargin: first ? 0 : 12
    Layout.bottomMargin: 0
    spacing: 8
    PanelText { visible: text !== ""; text: section.title; color: Theme.text; font.pixelSize: 14; font.weight: Font.Medium }
    PanelText { visible: text !== ""; text: section.description; color: Theme.muted; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.Wrap }
    Item { visible: section.description === ""; Layout.fillWidth: true }
}
