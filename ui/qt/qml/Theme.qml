pragma Singleton
import QtQuick

QtObject {
    property bool dark: true
    readonly property int pagePadding: 18
    readonly property int bodySize: 14
    readonly property int captionSize: 13
    readonly property int cardHeight: 66
    readonly property int cardSpacing: 12
    readonly property int groupSpacing: 24
    readonly property int controlHeight: 28
    readonly property int controlRadius: 8
    readonly property color background: dark ? "#1a1a1f" : "#f5f5f7"
    readonly property color card: dark ? "#232327" : "#ffffff"
    readonly property color border: dark ? "#33333a" : "#dedee4"
    readonly property color divider: dark ? "#2d2d34" : "#e9e9ed"
    readonly property color control: dark ? "#2d2d32" : "#eaeaee"
    readonly property color controlBorder: dark ? "#37373d" : "#dcdce2"
    readonly property color text: dark ? "#ededf1" : "#26262c"
    readonly property color muted: dark ? "#919199" : "#76767f"
    readonly property color secondary: dark ? "#b7b7c0" : "#60606b"
    readonly property color accent: dark ? "#aab6fa" : "#5969af"
    readonly property color accentFill: dark ? "#2b2d47" : "#e2e5f6"
    readonly property color accentText: accent
    readonly property color hover: dark ? "#303035" : "#e4e4e9"
    readonly property color selected: dark ? "#434347" : "#dcdce3"
    readonly property color popup: dark ? "#27272d" : "#ffffff"
    readonly property color success: dark ? "#72dca3" : "#27734a"
    readonly property color danger: dark ? "#f08d92" : "#b23b31"
}
