import QtQuick
import QtQuick.Controls

DialogButtonBox {
    id: buttons
    padding: 18
    spacing: 8
    background: Rectangle { color: Theme.card; radius: 14 }
    delegate: PanelButton { primary: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole }
}
