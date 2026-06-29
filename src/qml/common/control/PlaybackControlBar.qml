import QtQuick 6.7
import QtQuick.Controls 6.7
import "../icon"

Row {
    id: root
    spacing: buttonSpacing
    layoutDirection: Qt.LeftToRight

    property var playerController: null

    property int controlButtonSize: Design.Spacing.iconSizeLg
    property int playButtonSize: Design.Spacing.iconSizeXl
    property int buttonSpacing: Design.Spacing.spaceMd
    readonly property int rowHeight: playButtonSize

    property color iconNormalColor: Design.Colors.iconNormal
    property color iconHoveredColor: Design.Colors.iconHovered
    property color iconPressedColor: Design.Colors.iconNormal

    property color buttonNormalColor: "transparent"
    property color buttonHoveredColor: Design.Colors.withAlpha(Design.Colors.iconNormal, 0.1)
    property color buttonPressedColor: Design.Colors.withAlpha(Design.Colors.iconNormal, 0.2)

    signal prevClicked()
    signal playClicked()
    signal pauseClicked()
    signal nextClicked()

    Item {
        width: controlButtonSize
        height: rowHeight

        ControlButton {
            anchors.centerIn: parent
            id: prevButton
            iconComponent: PrevIcon {
                width: prevButton.iconSize
                height: prevButton.iconSize
                iconOpacity: prevButton.iconOpacity
            }
            buttonSize: controlButtonSize
            tooltipText: qsTr("上一曲")

            onClicked: {
                root.prevClicked()
                if (playerController)
                    playerController.playPrev()
            }
        }
    }

    Item {
        width: playButtonSize
        height: rowHeight

        ControlButton {
            anchors.centerIn: parent
            id: playPauseButton
            iconComponent: Item {
                id: playPauseIconContainer
                anchors.fill: parent

                property bool isPlaying: playerController && playerController.isPlaying
                property real playIconRotation: isPlaying ? 90 : 0
                property real pauseIconRotation: isPlaying ? 0 : -90

                PlayIcon {
                    width: parent.width
                    height: parent.height
                    iconOpacity: parent.isPlaying ? 0.0 : 1.0
                    rotation: parent.playIconRotation
                    transformOrigin: Item.Center
                    Behavior on opacity {
                        NumberAnimation {
                            duration: Design.Spacing.durationFast
                            easing.type: Design.Spacing.getEasingCurve("easeOut")
                        }
                    }
                    Behavior on rotation {
                        NumberAnimation {
                            duration: Design.Spacing.durationNormal
                            easing.type: Design.Spacing.getEasingCurve("easeOut")
                        }
                    }
                }

                PauseIcon {
                    width: parent.width
                    height: parent.height
                    smooth: true
                    opacity: parent.isPlaying ? 1.0 : 0.0
                    rotation: parent.pauseIconRotation
                    transformOrigin: Item.Center
                    Behavior on opacity {
                        NumberAnimation {
                            duration: Design.Spacing.durationFast
                            easing.type: Design.Spacing.getEasingCurve("easeOut")
                        }
                    }
                    Behavior on rotation {
                        NumberAnimation {
                            duration: Design.Spacing.durationNormal
                            easing.type: Design.Spacing.getEasingCurve("easeOut")
                        }
                    }
                }
            }
            buttonSize: playButtonSize
            tooltipText: playerController && playerController.isPlaying ? qsTr("暂停") : qsTr("播放")

            iconColor: {
                if (playPauseButton.mouseArea.pressed)
                    return iconPressedColor
                if (playPauseButton.mouseArea.containsMouse)
                    return iconHoveredColor
                return iconNormalColor
            }

            onClicked: {
                if (playerController) {
                    var wasPlaying = playerController.isPlaying
                    playerController.playOrPause()
                    if (wasPlaying)
                        root.pauseClicked()
                    else
                        root.playClicked()
                }
            }
        }
    }

    Item {
        width: controlButtonSize
        height: rowHeight

        ControlButton {
            anchors.centerIn: parent
            id: nextButton
            iconComponent: NextIcon {
                width: nextButton.iconSize
                height: nextButton.iconSize
                iconOpacity: nextButton.iconOpacity
            }
            buttonSize: controlButtonSize
            tooltipText: qsTr("下一曲")

            onClicked: {
                root.nextClicked()
                if (playerController)
                    playerController.playNext()
            }
        }
    }

    component ControlButton: Rectangle {
        id: controlButton

        property alias iconComponent: iconLoader.sourceComponent
        property alias mouseArea: mouseArea
        property int buttonSize: Design.Spacing.iconSizeLg
        property string tooltipText: ""
        property color iconColor: root.iconNormalColor
        property real iconOpacity: 1.0
        property int iconSize: Math.round(buttonSize * 0.75)

        signal clicked()

        width: buttonSize
        height: buttonSize
        radius: Design.Spacing.buttonRadius

        color: {
            if (mouseArea.pressed)
                return buttonPressedColor
            if (mouseArea.containsMouse)
                return buttonHoveredColor
            return buttonNormalColor
        }

        Loader {
            id: iconLoader
            anchors.centerIn: parent
            width: iconSize
            height: iconSize
            z: 0
        }

        MouseArea {
            id: mouseArea
            z: 10
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton

            onClicked: {
                controlButton.clicked()
            }
        }

        ToolTip {
            visible: mouseArea.containsMouse && tooltipText
            text: tooltipText
            delay: 500
            timeout: 1500
        }

        Behavior on color {
            ColorAnimation {
                duration: Design.Spacing.durationHover
                easing.type: Design.Spacing.getEasingCurve("easeOut")
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: Design.Spacing.durationHover
                easing.type: Design.Spacing.getEasingCurve("easeOut")
            }
        }

        scale: mouseArea.pressed ? 0.95 : (mouseArea.containsMouse ? 1.05 : 1.0)
    }
}
