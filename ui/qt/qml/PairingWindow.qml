import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: popup
    required property var pairing
    property string formRequest: ""
    width: 380; height: content.implicitHeight + 36
    minimumWidth: width; maximumWidth: width
    visible: false
    title: qsTr("Sunshine · Moonlight 配对")
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    onClosing: function(event) { event.accepted = false; popup.dismiss() }
    function present() {
        if (!pairing.pending) return
        if (formRequest !== pairing.requestId) {
            formRequest = pairing.requestId
            deviceName.text = pairing.clientName
            pin.text = ""
        }
        show(); raise(); requestActivate()
        pin.forceActiveFocus()
    }
    function dismiss() { pin.text = ""; hide() }
    function sendPin() {
        if (!pairing.pending || pairing.busy || !pin.acceptableInput || deviceName.text.trim() === "") return
        pairing.submit(pin.text, deviceName.text, formRequest)
        pin.text = ""
    }
    Shortcut { sequence: "Escape"; onActivated: popup.dismiss() }
    background: Rectangle { color: Theme.background; radius: 12; border.color: Theme.border }
    ColumnLayout {
        id: content
        x: 18; y: 18; width: parent.width - 36
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Image { source: Theme.dark ? "../assets/sunshine-mark.svg" : "../assets/sunshine-mark-light.svg"; sourceSize: Qt.size(48, 48); Layout.preferredWidth: 24; Layout.preferredHeight: 24 }
            PanelText { text: qsTr("Moonlight 配对"); color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold; Layout.fillWidth: true }
            PanelButton { subtle: true; symbol: "close"; Accessible.name: qsTr("稍后填写"); onClicked: popup.dismiss() }
        }
        PanelText {
            text: qsTr("%1 正在请求连接，请填写客户端显示的 4 位连接码。").arg(pairing.clientName)
            color: Theme.secondary; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            PanelText { text: qsTr("设备名称"); color: Theme.muted; font.pixelSize: 12 }
            Field { id: deviceName; objectName: "popupDeviceName"; Layout.fillWidth: true; maximumLength: 64; enabled: pairing.pending && !pairing.busy; Accessible.name: qsTr("设备名称") }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            PanelText { text: qsTr("连接码"); color: Theme.muted; font.pixelSize: 12 }
            Field {
                id: pin; objectName: "popupPin"
                Layout.fillWidth: true; implicitHeight: 44
                font.pixelSize: 23; font.letterSpacing: 8
                horizontalAlignment: Text.AlignHCenter
                placeholderText: "0000"; maximumLength: 4
                validator: RegularExpressionValidator { regularExpression: /[0-9]{4}/ }
                inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
                enabled: pairing.pending && !pairing.busy
                Accessible.name: qsTr("Moonlight 四位连接码")
                onAccepted: popup.sendPin()
            }
        }
        PanelText { visible: pairing.error !== ""; text: pairing.error; color: Theme.danger; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap }
        RowLayout {
            Layout.fillWidth: true
            PanelButton { text: qsTr("稍后"); subtle: true; onClicked: popup.dismiss() }
            Item { Layout.fillWidth: true }
            PanelButton { objectName: "submitPairing"; text: pairing.busy ? qsTr("提交中…") : qsTr("确认连接"); primary: true; enabled: pairing.pending && !pairing.busy && pin.acceptableInput && deviceName.text.trim() !== ""; onClicked: popup.sendPin() }
        }
    }
    Connections {
        target: pairing
        function onChanged() { if (!pairing.pending) popup.dismiss() }
    }
}
