import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    implicitHeight: Theme.controlHeight
    implicitWidth: 216
    leftPadding: 12
    rightPadding: 32
    font.pixelSize: 13
    hoverEnabled: true
    palette.text: Theme.text
    palette.buttonText: Theme.text
    palette.base: Theme.card
    palette.window: Theme.card
    palette.highlight: Theme.accent
    contentItem: Text {
        text: control.displayText; font: control.font; color: Theme.text
        verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
    }
    indicator: LineIcon { name: "chevron"; width: 12; height: 12; x: control.width - 25; anchors.verticalCenter: parent.verticalCenter }
    background: Rectangle {
        radius: 7; color: Theme.control; border.color: control.activeFocus ? Theme.accent : "transparent"
        Rectangle { anchors.fill: parent; radius: parent.radius; color: control.hovered ? Theme.hover : "transparent" }
    }
    delegate: ItemDelegate {
        id: option
        required property int index
        width: control.width - 8
        height: 36
        highlighted: control.highlightedIndex === index
        contentItem: Text {
            text: control.textRole ? control.model[option.index][control.textRole] : control.model[option.index]
            color: Theme.text; font: control.font; leftPadding: 12; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 4; color: option.highlighted ? Theme.selected : "transparent"
        }
    }
    popup: Popup {
        y: control.height + 4; width: control.width
        padding: 4
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 280)
        contentItem: ListView {
            clip: true; implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle { radius: 8; color: Theme.popup; border.color: Theme.controlBorder }
    }
}
