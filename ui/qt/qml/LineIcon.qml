import QtQuick

Canvas {
    id: icon
    property string name: "home"
    property color ink: Theme.text
    implicitWidth: 24
    implicitHeight: 24
    onNameChanged: requestPaint()
    onInkChanged: requestPaint()
    onPaint: {
        const c = getContext("2d")
        c.reset()
        c.scale(width / 24, height / 24)
        c.strokeStyle = ink
        c.lineWidth = 1.45
        c.lineCap = "round"
        c.lineJoin = "round"
        function line(x1, y1, x2, y2) { c.moveTo(x1, y1); c.lineTo(x2, y2) }
        c.beginPath()
        if (name === "sun") {
            c.arc(12, 12, 5, 0, Math.PI * 2)
            for (let i = 0; i < 8; ++i) { const a = i * Math.PI / 4; line(12 + Math.cos(a) * 8, 12 + Math.sin(a) * 8, 12 + Math.cos(a) * 10, 12 + Math.sin(a) * 10) }
        } else if (name === "panel") {
            c.rect(3, 4, 18, 16); line(9, 4, 9, 20)
        } else if (name === "trash") {
            line(4, 6, 20, 6); c.rect(9, 3, 6, 3); c.moveTo(6, 6); c.lineTo(7, 21); c.lineTo(17, 21); c.lineTo(18, 6); line(10, 10, 10, 17); line(14, 10, 14, 17)
        } else if (name === "chevron") {
            c.moveTo(5, 9); c.lineTo(12, 16); c.lineTo(19, 9)
        } else if (name === "close") {
            line(4, 4, 20, 20); line(20, 4, 4, 20)
        } else if (name === "minimize") {
            line(3, 12, 21, 12)
        } else if (name === "maximize") {
            c.rect(4, 4, 16, 16)
        } else if (name === "restore") {
            c.rect(3, 7, 14, 14); c.moveTo(7, 7); c.lineTo(7, 3); c.lineTo(21, 3); c.lineTo(21, 17); c.lineTo(17, 17)
        } else if (name === "menu") {
            line(3, 6, 21, 6); line(3, 12, 21, 12); line(3, 18, 21, 18)
        } else if (name === "refresh") {
            c.arc(12, 12, 8, -Math.PI * 0.8, Math.PI * 0.9)
            c.moveTo(4, 4); c.lineTo(4, 10); c.lineTo(10, 10)
        } else if (name === "update") {
            line(12, 20, 12, 4); line(12, 4, 6, 10); line(12, 4, 18, 10)
        } else if (name === "add") {
            line(12, 3, 12, 21); line(3, 12, 21, 12)
        } else if (name === "folder") {
            c.moveTo(3, 6); c.lineTo(3, 20); c.lineTo(21, 20); c.lineTo(21, 7); c.lineTo(12, 7); c.lineTo(10, 4); c.lineTo(3, 4); c.closePath()
        } else if (name === "theme") {
            c.arc(12, 12, 9, 0, Math.PI * 2); c.moveTo(12, 3); c.lineTo(12, 21)
            c.stroke(); c.beginPath(); c.arc(12, 12, 6, -Math.PI / 2, Math.PI / 2); c.closePath(); c.fillStyle = ink; c.fill()
        } else if (name === "sessions") {
            c.arc(12, 12, 9, 0, Math.PI * 2); c.rect(9, 9, 6, 6)
        } else if (name === "audio") {
            c.moveTo(3, 9); c.lineTo(7, 9); c.lineTo(12, 4); c.lineTo(12, 20); c.lineTo(7, 15); c.lineTo(3, 15); c.closePath()
            c.moveTo(16, 7); c.bezierCurveTo(20, 9, 20, 15, 16, 17)
        } else if (name === "keyboard") {
            c.rect(2, 5, 20, 14)
            for (let j = 8; j <= 11; j += 3) for (let i = 5; i <= 19; i += 3) line(i, j, i + 0.5, j)
            line(7, 16, 17, 16)
        } else if (name === "mouse") {
            c.moveTo(5, 9); c.bezierCurveTo(5, 0, 19, 0, 19, 9); c.lineTo(19, 15); c.bezierCurveTo(19, 24, 5, 24, 5, 15); c.closePath(); line(12, 5, 12, 9)
        } else if (name === "gamepad") {
            c.moveTo(7, 6); c.lineTo(17, 6); c.bezierCurveTo(21, 6, 24, 20, 19, 20); c.lineTo(15, 16); c.lineTo(9, 16); c.lineTo(5, 20); c.bezierCurveTo(0, 20, 3, 6, 7, 6)
            line(6, 11, 10, 11); line(8, 9, 8, 13); line(16, 10, 16.2, 10); line(19, 12, 19.2, 12)
        } else if (name === "network") {
            c.arc(12, 12, 9, 0, Math.PI * 2); c.moveTo(3, 12); c.lineTo(21, 12)
            c.moveTo(12, 3); c.bezierCurveTo(5, 8, 5, 16, 12, 21); c.bezierCurveTo(19, 16, 19, 8, 12, 3)
        } else if (name === "home") {
            c.moveTo(3, 10); c.lineTo(12, 3); c.lineTo(21, 10)
            c.moveTo(5, 9); c.lineTo(5, 21); c.lineTo(10, 21); c.lineTo(10, 14)
            c.lineTo(14, 14); c.lineTo(14, 21); c.lineTo(19, 21); c.lineTo(19, 9)
        } else if (name === "apps" || name === "▦") {
            c.rect(3, 3, 7, 7); c.rect(14, 3, 7, 7); c.rect(3, 14, 7, 7); c.rect(14, 14, 7, 7)
        } else if (name === "clients" || name === "▣") {
            c.rect(3, 4, 18, 13); line(8, 21, 16, 21); line(12, 17, 12, 21)
        } else if (name === "settings") {
            line(3, 6, 21, 6); line(3, 12, 21, 12); line(3, 18, 21, 18)
            line(8, 3, 8, 9); line(16, 9, 16, 15); line(10, 15, 10, 21)
        } else if (name === "logs") {
            c.rect(5, 2, 14, 20); line(8, 7, 16, 7); line(8, 12, 16, 12); line(8, 17, 13, 17)
        } else if (name === "about") {
            c.arc(12, 12, 9, 0, Math.PI * 2); line(12, 11, 12, 17); line(12, 7, 12, 7.2)
        } else if (name === "connection") {
            c.arc(12, 12, 4, 0, Math.PI * 2)
            for (let i = 0; i < 8; ++i) {
                const a = i * Math.PI / 4
                line(12 + Math.cos(a) * 8, 12 + Math.sin(a) * 8, 12 + Math.cos(a) * 10, 12 + Math.sin(a) * 10)
            }
            c.moveTo(20, 12); c.arc(12, 12, 8, 0, Math.PI * 2)
        } else {
            c.rect(6, 6, 12, 12)
            for (let i = 8; i <= 16; i += 4) { line(i, 2, i, 6); line(i, 18, i, 22); line(2, i, 6, i); line(18, i, 22, i) }
        }
        c.stroke()
    }
}
