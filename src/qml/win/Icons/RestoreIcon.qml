// Generated from SVG file restoreIcon.svg
import QtQuick
import QtQuick.Shapes

Item {
    id: restoreIcon
    implicitWidth: 36
    implicitHeight: 36

    property color normalColor: "#FFFFFF"
    property color hoverColor: "#CCCCCC"

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: mouseArea.containsMouse ? hoverColor : "transparent"
        opacity: mouseArea.containsMouse ? 1 : 0

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    Canvas {
        id: canvas
        anchors.centerIn: parent
        width: 11
        height: 11

        onPaint: {
            var ctx = canvas.getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = restoreIcon.normalColor
            ctx.lineWidth = 1.5

            ctx.beginPath()
            ctx.moveTo(5, 0.5)
            ctx.lineTo(0.5, 0.5)
            ctx.lineTo(0.5, 5)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(5.5, 0.5)
            ctx.lineTo(5.5, 5.5)
            ctx.lineTo(0.5, 5.5)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(5.5, 5.5)
            ctx.lineTo(10.5, 5.5)
            ctx.lineTo(10.5, 10.5)
            ctx.lineTo(5.5, 10.5)
            ctx.stroke()
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: window.showNormal()
    }
}
