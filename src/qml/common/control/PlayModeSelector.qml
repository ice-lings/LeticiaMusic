import QtQuick 6.7
import QtQuick.Controls 6.7
import "../icon"

Rectangle {
    id: root
    property real size: 36
    width: size
    height: size
    radius: Math.round(size * 0.22)
    color: mouseArea.containsMouse ? Design.Colors.withAlpha(Design.Colors.primary, 0.15) : "transparent"

    Behavior on color {
        ColorAnimation { duration: Design.Spacing.durationHover }
    }

    property var playerController: null

    readonly property var modeNames: [qsTr("列表循环"), qsTr("单曲循环"), qsTr("随机播放")]

    readonly property real iconSize: Math.round(size * 0.55)

    ListLoopIcon {
        anchors.centerIn: parent
        width: iconSize
        height: iconSize
        visible: playerController && playerController.playMode === 0
    }

    SingleLoopIcon {
        anchors.centerIn: parent
        width: iconSize
        height: iconSize
        visible: playerController && playerController.playMode === 1
    }

    ShuffleIcon {
        anchors.centerIn: parent
        width: iconSize
        height: iconSize
        visible: playerController && playerController.playMode === 2
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            if (!playerController)
                return
            playerController.setPlayMode((playerController.playMode + 1) % 3)
        }
    }

    ToolTip {
        visible: mouseArea.containsMouse
        text: playerController ? modeNames[playerController.playMode] : ""
        delay: 500
        timeout: 1500
    }
}
