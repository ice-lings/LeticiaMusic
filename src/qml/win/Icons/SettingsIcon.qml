import QtQuick
import QtQuick.Controls

Item {
    id: settingsBtn
    
    property color normalColor: "#FFFFFF"
    property color hoverColor: "#CCCCCC"
    property real rotationAngle: 0
    
    implicitWidth: 40
    implicitHeight: 40
    
    Rectangle {
        anchors.fill: parent
        radius: 4
        color: mouseArea.containsMouse ? hoverColor : "transparent"
        opacity: mouseArea.containsMouse ? 1 : 0
        
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
    
    Image {
        id: settingsImage
        anchors.centerIn: parent
        width: 20
        height: 20
        source: "qrc:/img/Setting-white.svg"
        fillMode: Image.PreserveAspectFit
        smooth: true
        
        transform: Rotation {
            origin.x: settingsImage.width / 2
            origin.y: settingsImage.height / 2
            angle: rotationAngle
        }
    }
    
    PropertyAnimation {
        id: spinAnimation
        target: settingsBtn
        property: "rotationAngle"
        from: rotationAngle
        to: rotationAngle + 360
        duration: 800
        loops: Animation.Infinite
        easing.type: Easing.Linear
    }
    
    PropertyAnimation {
        id: stopAnimation
        target: settingsBtn
        property: "rotationAngle"
        duration: 200
        easing.type: Easing.OutQuad
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        
        onEntered: {
            stopAnimation.stop()
            spinAnimation.from = rotationAngle
            spinAnimation.to = rotationAngle + 360
            spinAnimation.running = true
        }
        
        onExited: {
            spinAnimation.running = false
            stopAnimation.start()
        }
        
        onClicked: {
            console.log("Settings clicked")
            settingsBtn.clicked()
        }
    }
    
    signal clicked()
}