import QtQuick
import QtQuick.Controls

Item {
    id: minimizeBtn
    
    property color normalColor: "#FFFFFF"
    property color hoverColor: "#CCCCCC"
    
    implicitWidth: 36
    implicitHeight: 36
    
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
        width: 12
        height: 12
        
        onPaint: {
            var ctx = canvas.getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = parent.normalColor
            ctx.lineWidth = 1.5
            ctx.beginPath()
            ctx.moveTo(0, height / 2)
            ctx.lineTo(width, height / 2)
            ctx.stroke()
        }
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: window.showMinimized()
    }
}