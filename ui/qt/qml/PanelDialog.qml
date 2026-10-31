import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog
    padding: 18
    topPadding: 16
    font.pixelSize: Theme.bodySize
    onOpened: {
        const confirm = standardButton(Dialog.Ok)
        const cancel = standardButton(Dialog.Cancel)
        if (confirm) confirm.text = qsTr("确认")
        if (cancel) cancel.text = qsTr("取消")
    }
    background: Rectangle { radius: 14; color: Theme.card; border.color: Theme.border }
    header: Label {
        visible: text !== ""; text: dialog.title
        leftPadding: 18; rightPadding: 18; topPadding: 18
        color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold
        wrapMode: Text.Wrap
    }
    footer: DialogActions {
        visible: dialog.standardButtons !== Dialog.NoButton
        standardButtons: dialog.standardButtons
        onAccepted: dialog.accept()
        onRejected: dialog.reject()
    }
    Overlay.modal: Rectangle { color: Theme.dark ? "#66000000" : "#33000000" }
}
