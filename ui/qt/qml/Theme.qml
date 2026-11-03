pragma Singleton
import QtQuick

QtObject {
    property bool dark: true
    readonly property string fontFamily: Qt.application.font.family
    readonly property bool windows: Qt.platform.os === "windows"
    readonly property int windowRadius: windows ? 8 : 14
    readonly property int pagePadding: 18
    readonly property int bodySize: 14
    readonly property int captionSize: 13
    readonly property int cardHeight: windows ? 54 : 57
    readonly property int cardSpacing: 12
    readonly property int groupSpacing: 24
    readonly property int controlHeight: 28
    readonly property int controlRadius: 8
    readonly property color background: dark ? "#1a1a1e" : "#f4f4f6"
    readonly property color card: dark ? "#232327" : "#ffffff"
    readonly property color border: dark ? "#333339" : "#e3e3e8"
    readonly property color divider: dark ? "#2c2c32" : "#ececf0"
    readonly property color control: dark ? "#2d2d33" : "#f1f1f4"
    readonly property color controlBorder: dark ? "#333339" : "#e3e3e8"
    readonly property color text: dark ? "#ededf1" : "#1c1c21"
    readonly property color muted: dark ? "#8b8b94" : "#85858d"
    readonly property color secondary: dark ? "#b9b9c1" : "#4b4b53"
    readonly property color accent: dark ? "#a5b4fc" : "#4f46e5"
    readonly property color accentFill: dark ? "#2b2c47" : "#eeedfd"
    readonly property color accentText: accent
    readonly property color hover: dark ? "#36363d" : "#e9e9ee"
    readonly property color selected: dark ? "#3c3c40" : "#ffffff"
    readonly property color popup: dark ? "#26262b" : "#ffffff"
    readonly property color success: dark ? "#72dca3" : "#27734a"
    readonly property color danger: dark ? "#f08d92" : "#b23b31"
}
