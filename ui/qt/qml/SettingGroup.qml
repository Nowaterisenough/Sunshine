import QtQuick
import QtQuick.Layouts

Rectangle {
    default property alias rows: column.data
    implicitHeight: column.implicitHeight + 2
    Layout.fillWidth: true
    color: Theme.card
    radius: 12
    border.color: Theme.border
    ColumnLayout {
        id: column
        x: 1; y: 1; width: parent.width - 2
        spacing: 0
    }
}
