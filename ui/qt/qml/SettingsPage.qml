import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    property int category: 0
    property string searchTerm: ""
    readonly property var displayedOptions: {
        const term = searchTerm.trim().toLowerCase()
        const candidates = term ? groups.reduce(function(all, group) { return all.concat(group) }, []) : groups[category]
        return candidates.filter(function(option) { return !term || (option.title + " " + option.detail + " " + option.key).toLowerCase().includes(term) })
    }
    readonly property bool inputsValid: {
        for (let i = 0; i < settingRows.count; ++i) {
            const row = settingRows.itemAt(i)
            if (row && !row.valid) return false
        }
        return true
    }
    spacing: Theme.cardSpacing
    function current(key, fallback) {
        const values = host.config
        return values[key] === undefined ? String(fallback) : String(values[key])
    }
    readonly property var groups: [
        [
            {title: qsTr("主机名称"), detail: qsTr("在 Moonlight 的主机列表中显示"), key: "sunshine_name", fallback: "", type: "text"},
            {title: qsTr("日志级别"), detail: qsTr("通常使用“信息”，排查问题时可改为“调试”"), key: "min_log_level", fallback: "2", type: "choice", choices: [{text: qsTr("调试"), value: "1"}, {text: qsTr("信息"), value: "2"}, {text: qsTr("警告"), value: "3"}, {text: qsTr("错误"), value: "4"}]},
            {title: qsTr("最后会话结束时停止应用"), detail: qsTr("没有客户端串流时结束当前应用"), key: "stop_on_last_video_session", fallback: "disabled", type: "toggle"}
        ],
        [
            {title: qsTr("编码器"), detail: qsTr("自动选择适合此主机的编码后端"), key: "encoder", fallback: "", type: "choice", choices: encoderChoices()},
            {title: qsTr("显示器"), detail: qsTr("留空自动选择；可填写主机显示设备 ID"), key: "output_name", fallback: "", type: "text"},
            {title: qsTr("音频输出"), detail: qsTr("留空使用默认音频设备"), key: "audio_sink", fallback: "", type: "text"},
            {title: qsTr("最大码率"), detail: qsTr("单位 Kbps；0 表示由客户端决定"), key: "max_bitrate", fallback: "0", type: "number", min: 0, max: 1000000},
            {title: qsTr("HEVC 支持"), detail: qsTr("客户端仍需具备对应的解码能力"), key: "hevc_mode", fallback: "0", type: "choice", choices: codecChoices()},
            {title: qsTr("AV1 支持"), detail: qsTr("需要兼容的主机编码器与客户端"), key: "av1_mode", fallback: "0", type: "choice", choices: codecChoices()},
            {title: qsTr("鼠标光标采集"), detail: qsTr("将主机光标包含在串流画面中"), key: "capture_cursor", fallback: "true", type: "toggle", boolValue: true}
        ],
        [
            {title: qsTr("键盘输入"), detail: qsTr("允许客户端发送键盘事件"), key: "keyboard", fallback: "enabled", type: "toggle"},
            {title: qsTr("鼠标输入"), detail: qsTr("允许客户端发送鼠标事件"), key: "mouse", fallback: "enabled", type: "toggle"},
            {title: qsTr("手柄输入"), detail: qsTr("允许客户端连接手柄"), key: "controller", fallback: "enabled", type: "toggle"},
            {title: qsTr("手柄类型"), detail: qsTr("自动匹配客户端的手柄能力"), key: "gamepad", fallback: "auto", type: "choice", choices: [{text: qsTr("自动"), value: "auto"}, {text: "Xbox 360", value: "x360"}, {text: "DualShock 4", value: "ds4"}, {text: "DualSense", value: "ds5"}]},
            {title: qsTr("高精度滚动"), detail: qsTr("保留触控板和滚轮的细微滚动量"), key: "high_resolution_scrolling", fallback: "enabled", type: "toggle"}
        ],
        [
            {title: qsTr("UPnP 自动映射"), detail: qsTr("请求路由器自动映射串流所需端口"), key: "upnp", fallback: "disabled", type: "toggle"},
            {title: qsTr("串流基础端口"), detail: qsTr("重启生效；管理端口为此值 + 1"), key: "port", fallback: "47989", type: "number", min: 1029, max: 65514},
            {title: qsTr("地址协议"), detail: qsTr("选择监听 IPv4 或 IPv4 + IPv6"), key: "address_family", fallback: "ipv4", type: "choice", choices: [{text: "IPv4", value: "ipv4"}, {text: "IPv4 + IPv6", value: "both"}]},
            {title: qsTr("绑定地址"), detail: qsTr("留空监听所有网卡；可填写指定网卡 IP"), key: "bind_address", fallback: "", type: "text"},
            {title: qsTr("自动发现"), detail: qsTr("允许局域网 Moonlight 通过 mDNS 发现主机"), key: "mdns_broadcast", fallback: "enabled", type: "toggle"},
            {title: qsTr("前向纠错比例"), detail: qsTr("百分比；增大可改善丢包但会占用更多带宽"), key: "fec_percentage", fallback: "20", type: "number", min: 0, max: 100}
        ]
    ]
    function codecChoices() { return [{text: qsTr("自动"), value: "0"}, {text: qsTr("禁用"), value: "1"}, {text: qsTr("8-bit"), value: "2"}, {text: qsTr("8-bit + 10-bit HDR"), value: "3"}] }
    function encoderChoices() {
        const platform = host.config.platform || ""
        let choices = [{text: qsTr("自动"), value: ""}]
        if (platform === "macos") choices.push({text: "VideoToolbox", value: "videotoolbox"})
        else {
            choices.push({text: "NVIDIA NVENC", value: "nvenc"})
            if (platform === "windows") choices.push({text: "AMD AMF", value: "amdvce"}, {text: "Intel Quick Sync", value: "quicksync"})
            else choices.push({text: "VA-API", value: "vaapi"})
        }
        choices.push({text: qsTr("软件编码"), value: "software"})
        return choices
    }
    PanelText { visible: !host.connected; text: qsTr("连接主机后即可编辑设置。"); color: Theme.muted; Layout.bottomMargin: 12 }
    SettingGroup {
        visible: page.displayedOptions.length > 0
    Repeater {
        id: settingRows
        model: page.displayedOptions
        delegate: SettingCard {
            id: row
            required property var modelData
            required property int index
            grouped: true
            last: index === settingRows.count - 1
            readonly property bool valid: !fieldLoader.item || fieldLoader.item.acceptableInput === undefined || fieldLoader.item.acceptableInput
            title: modelData.title
            description: modelData.detail
            Loader {
                id: fieldLoader
                enabled: host.connected && !host.busy
                sourceComponent: row.modelData.type === "toggle" ? toggleEditor : row.modelData.type === "choice" ? choiceEditor : textEditor
            }
            Component {
                id: toggleEditor
                SegmentedControl {
                    model: [qsTr("关闭"), qsTr("开启")]
                    currentIndex: ["enabled", "true", "1"].includes(page.current(row.modelData.key, row.modelData.fallback)) ? 1 : 0
                    onActivated: function(index) { host.setValue(row.modelData.key, row.modelData.boolValue ? String(index === 1) : index === 1 ? "enabled" : "disabled") }
                }
            }
            Component {
                id: choiceEditor
                Choice {
                    model: row.modelData.choices
                    textRole: "text"; valueRole: "value"
                    currentIndex: {
                        const value = page.current(row.modelData.key, row.modelData.fallback)
                        return model.findIndex(function(option) { return option.value === value })
                    }
                    displayText: currentIndex < 0 ? page.current(row.modelData.key, row.modelData.fallback) : currentText
                    onActivated: host.setValue(row.modelData.key, currentValue)
                }
            }
            Component {
                id: textEditor
                Field {
                    text: page.current(row.modelData.key, row.modelData.fallback)
                    placeholderText: qsTr("默认")
                    validator: row.modelData.type === "number" ? numericValidator : null
                    IntValidator { id: numericValidator; bottom: row.modelData.min || 0; top: row.modelData.max || 2147483647 }
                    onEditingFinished: if (acceptableInput) host.setValue(row.modelData.key, text)
                    color: acceptableInput ? Theme.text : Theme.danger
                }
            }
        }
    }
    }
    PanelText { visible: page.displayedOptions.length === 0; text: qsTr("没有匹配的设置"); color: Theme.muted; font.pixelSize: 13; Layout.topMargin: 20 }
    RowLayout {
        Layout.fillWidth: true; Layout.topMargin: 20; spacing: 10
        PanelText { text: !page.inputsValid ? qsTr("请修正标红的数值") : host.dirty ? qsTr("有未保存的更改") : qsTr("设置与主机同步"); color: !page.inputsValid ? Theme.danger : host.dirty ? Theme.accent : Theme.muted; Layout.fillWidth: true; font.pixelSize: 12 }
        PanelButton { text: qsTr("放弃更改"); subtle: true; enabled: host.dirty && !host.busy; onClicked: host.discardChanges() }
        PanelButton { text: qsTr("保存设置"); primary: true; enabled: page.inputsValid && host.dirty && host.connected && !host.busy; onClicked: host.saveConfig() }
    }
    SectionTitle { title: qsTr("服务维护"); description: qsTr("大部分设置在重启主机后完整生效。请先结束正在进行的游戏。") }
    SettingCard {
        title: qsTr("重启 Sunshine"); description: qsTr("所有客户端的当前串流连接都会中断")
        PanelButton { text: qsTr("重启主机"); enabled: host.connected && !host.busy && !host.dirty; onClicked: restartDialog.open() }
    }
    PanelDialog {
        id: restartDialog
        width: 420
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true
        title: qsTr("重启 Sunshine 主机？")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label { text: qsTr("已保存的设置将生效，当前串流会话会被断开。"); wrapMode: Text.Wrap }
        onAccepted: host.restart()
    }
}
