import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var host
    required property var updates
    signal themeSelected(bool dark)
    spacing: 12
    SettingGroup {
        SettingCard {
            grouped: true; title: qsTr("版本")
            description: updates.message || qsTr("Sunshine Qt 控制面板")
            descriptionColor: updates.state === "error" ? Theme.danger : Theme.secondary
            PanelText { text: updates.currentVersion; color: Theme.muted; font.pixelSize: 13 }
            PanelButton {
                visible: updates.state !== "ready" && updates.state !== "downloading"
                text: updates.state === "checking" ? qsTr("检查中…") : updates.available ? qsTr("下载更新") : qsTr("检查更新")
                subtle: !updates.available; primary: updates.available; enabled: !updates.busy
                onClicked: updates.available ? updates.download() : updates.check()
            }
            PanelButton {
                visible: updates.state === "ready"
                text: updates.canInstall ? qsTr("安装更新") : qsTr("打开下载目录"); primary: true
                onClicked: updates.canInstall ? installation.open() : updates.openDownloaded()
            }
            PanelButton { visible: updates.busy; text: qsTr("取消"); subtle: true; onClicked: updates.cancel() }
        }
        SettingCard {
            visible: updates.available
            grouped: true; title: updates.latestVersion
            description: updates.packageDescription
            PanelText { visible: updates.state === "downloading"; text: Math.round(updates.progress * 100) + "%"; color: Theme.secondary; font.pixelSize: 13 }
            ProgressBar {
                visible: updates.state === "downloading"
                value: updates.progress; implicitWidth: page.width < 800 ? 100 : 160
                background: Rectangle { implicitHeight: 4; radius: 2; color: Theme.control }
                contentItem: Item { implicitHeight: 4; Rectangle { width: parent.width * updates.progress; height: 4; radius: 2; color: Theme.accent } }
            }
        }
        SettingCard {
            grouped: true; title: qsTr("更新内容"); description: updates.lastChecked ? qsTr("上次检查：%1").arg(updates.lastChecked) : qsTr("查看新版本的更新说明")
            PanelButton { text: qsTr("打开"); subtle: true; onClicked: updates.releaseNotes ? notes.open() : updates.openRelease() }
        }
        SettingCard {
            grouped: true; title: qsTr("更新按钮"); description: qsTr("有新版本时显示在顶栏")
            SegmentedControl { model: [qsTr("关闭"), qsTr("开启")]; currentIndex: updates.showButton ? 1 : 0; onActivated: function(index) { updates.showButton = index === 1 } }
        }
        SettingCard {
            grouped: true; title: qsTr("自动检查更新"); description: qsTr("启动后及指定间隔检查新版本")
            SegmentedControl { model: [qsTr("关闭"), qsTr("开启")]; currentIndex: updates.autoCheck ? 1 : 0; onActivated: function(index) { updates.autoCheck = index === 1 } }
        }
        SettingCard {
            grouped: true; title: qsTr("检查间隔"); description: qsTr("多久检查一次新版本")
            SegmentedControl { enabled: updates.autoCheck; model: [qsTr("30 分钟"), qsTr("1 小时"), qsTr("6 小时"), qsTr("24 小时")]; currentIndex: [30, 60, 360, 1440].indexOf(updates.intervalMinutes); onActivated: function(index) { updates.intervalMinutes = [30, 60, 360, 1440][index] } }
        }
        SettingCard {
            grouped: true; title: qsTr("更新渠道"); description: qsTr("每个版本包含 Qt 预发布版本")
            SegmentedControl { enabled: !updates.busy; model: [qsTr("每个版本"), qsTr("稳定版")]; currentIndex: updates.includePrerelease ? 0 : 1; onActivated: function(index) { updates.includePrerelease = index === 0 } }
        }
        SettingCard {
            grouped: true; last: true; title: qsTr("自动下载更新"); description: qsTr("发现新版本后下载，安装前仍需手动确认")
            SegmentedControl { model: [qsTr("关闭"), qsTr("开启")]; currentIndex: updates.autoDownload ? 1 : 0; onActivated: function(index) { updates.autoDownload = index === 1 } }
        }
    }
    SettingGroup {
        SettingCard {
            grouped: true; title: qsTr("应用主题"); description: qsTr("选择控制面板的外观")
            SegmentedControl { model: [qsTr("浅色"), qsTr("深色")]; currentIndex: Theme.dark ? 1 : 0; onActivated: function(index) { page.themeSelected(index === 1) } }
        }
        SettingCard {
            grouped: true; title: qsTr("主机版本"); description: qsTr("当前连接的 Sunshine 主机")
            PanelText { text: String(host.config.version || qsTr("未连接")).replace(/(0\.0\.0\.)?([0-9a-f]{12})[0-9a-f]{28}/g, "$2"); color: Theme.muted; font.pixelSize: 13; Layout.maximumWidth: page.width * 0.45; elide: Text.ElideRight }
        }
        SettingCard { grouped: true; last: true; title: qsTr("开源许可"); description: "Sunshine · Qt 6.12"; PanelText { text: "GPL-3.0"; color: Theme.muted; font.pixelSize: 13 } }
    }
    PanelDialog {
        id: installation
        parent: Overlay.overlay; anchors.centerIn: parent; width: Math.min(480, parent.width - 40)
        implicitHeight: 232
        modal: true; title: qsTr("安装 Sunshine 更新？")
        contentHeight: installationText.implicitHeight
        contentItem: Item {
            PanelText {
                id: installationText
                width: installation.width - installation.leftPadding - installation.rightPadding
                text: qsTr("安装包已通过校验。继续后将打开系统安装程序并退出控制面板。覆盖安装保留配置与配对；正在运行的串流会在安装过程中中断。")
                color: Theme.secondary; font.pixelSize: 14; wrapMode: Text.Wrap
            }
        }
        footer: DialogActions {
            PanelButton { text: qsTr("稍后"); onClicked: installation.close() }
            PanelButton { text: qsTr("继续安装"); primary: true; onClicked: { installation.close(); updates.openDownloaded() } }
        }
    }
    PanelDialog {
        id: notes
        parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(680, parent.width - 40); height: Math.min(480, parent.height - 60)
        modal: true; title: qsTr("更新内容")
        contentItem: ScrollView {
            clip: true
            PanelText { width: notes.availableWidth; text: updates.releaseNotes; color: Theme.secondary; font.pixelSize: 14; wrapMode: Text.Wrap }
        }
        footer: DialogActions {
            PanelButton { text: qsTr("发布页"); subtle: true; onClicked: updates.openRelease() }
            PanelButton { text: qsTr("关闭"); onClicked: notes.close() }
        }
    }
}
