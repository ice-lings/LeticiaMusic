import QtQuick

Item {
    implicitWidth: 200
    implicitHeight: 200

    property color iconColor: "#ffffff"
    property real iconOpacity: 1.0

    Image {
        anchors.fill: parent
        source: "qrc:/img/play-circle.svg"
        fillMode: Image.PreserveAspectFit
        opacity: iconOpacity
        mipmap: true
        smooth: true
    }
}
