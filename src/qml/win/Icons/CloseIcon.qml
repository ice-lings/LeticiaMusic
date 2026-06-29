import QtQuick
import QtQuick.Controls

Item {
    id: closeBtn
    
    property color normalColor: "#FFFFFF"
    property color hoverColor: "#E81123"
    
    implicitWidth: 46
    implicitHeight: 36
    
    Rectangle {
        anchors.fill: parent
        color: mouseArea.containsMouse ? hoverColor : "transparent"
        
        Behavior on color { ColorAnimation { duration: 100 } }
    }
    
    Canvas {
        id: canvas
        anchors.centerIn: parent
        width: 10
        height: 10
        
        onPaint: {
            var ctx = canvas.getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = parent.normalColor
            ctx.lineWidth = 1.2
            ctx.beginPath()
            ctx.moveTo(0, 0)
            ctx.lineTo(width, height)
            ctx.moveTo(width, 0)
            ctx.lineTo(0, height)
            ctx.stroke()
        }
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            appContext.savePlaybackState()
            appContext.playerController.stop()
            Qt.quit()
        }
    }
}