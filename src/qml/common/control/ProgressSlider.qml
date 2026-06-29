import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

RowLayout {
    id: root
    spacing: Design.Spacing.spaceXs

    property var playerController: null
    property real scaleFactor: 1.0

    function formatMs(ms) {
        return Tools.formatProgress(ms)
    }

    Text {
        text: formatMs(playerController ? playerController.position : 0)
        color: Design.Colors.textSecondary
        font.pixelSize: Design.Fonts.sizeSm * scaleFactor
        Layout.minimumWidth: 36 * scaleFactor
        horizontalAlignment: Text.AlignRight
    }

    Slider {
        id: seekSlider
        Layout.fillWidth: true
        from: 0
        to: Math.max(playerController ? playerController.duration : 0, 1)
        enabled: playerController && playerController.duration > 0
        live: false
        hoverEnabled: true
        implicitHeight: 28 * scaleFactor

        property bool isUserInteracting: false
        property int lastPlayerPosition: playerController ? playerController.position : 0

        value: isUserInteracting ? value : lastPlayerPosition

        onLastPlayerPositionChanged: {
            if (!isUserInteracting) {
                value = lastPlayerPosition
            }
        }

        onPressedChanged: {
            if (pressed) {
                isUserInteracting = true
            } else {
                if (playerController) {
                    playerController.seek(Math.round(value))
                }
                isUserInteracting = false
            }
        }

        background: Item {
            x: seekSlider.leftPadding
            y: (seekSlider.height - implicitHeight) / 2
            implicitWidth: 200
            implicitHeight: 8 * scaleFactor
            width: seekSlider.availableWidth
            height: implicitHeight

            Rectangle {
                id: trackBg
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: seekSlider.hovered || seekSlider.pressed ? 8 * scaleFactor : 6 * scaleFactor
                radius: height / 2
                color: Design.Colors.progressTrack

                Behavior on height {
                    NumberAnimation {
                        duration: Design.Spacing.durationHover
                        easing.type: Easing.OutCubic
                    }
                }
            }

            Rectangle {
                id: trackFill
                anchors.verticalCenter: parent.verticalCenter
                width: seekSlider.visualPosition * parent.width
                height: seekSlider.hovered || seekSlider.pressed ? 8 * scaleFactor : 6 * scaleFactor
                radius: height / 2
                color: seekSlider.hovered || seekSlider.pressed ? Design.Colors.primaryLight : Design.Colors.progressFill

                Behavior on width {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.InOutCubic
                    }
                }
                Behavior on height {
                    NumberAnimation {
                        duration: Design.Spacing.durationHover
                        easing.type: Easing.OutCubic
                    }
                }
                Behavior on color {
                    ColorAnimation {
                        duration: Design.Spacing.durationHover
                    }
                }
            }
        }

        handle: Item {
            x: seekSlider.leftPadding + seekSlider.visualPosition * (seekSlider.availableWidth - 12 * scaleFactor)
            y: (seekSlider.height - 12 * scaleFactor) / 2
            width: 12 * scaleFactor
            height: 12 * scaleFactor

            Behavior on x {
                NumberAnimation {
                    duration: 180
                    easing.type: Easing.InOutCubic
                }
            }

            Rectangle {
                id: handleGlow
                anchors.centerIn: parent
                width: handleDot.width + 10 * scaleFactor
                height: handleDot.height + 10 * scaleFactor
                radius: width / 2
                color: Design.Colors.withAlpha(Design.Colors.progressFill, 0.12)
                visible: seekSlider.hovered || seekSlider.pressed
                opacity: visible ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 100 }
                }
            }

            Rectangle {
                id: handleDot
                anchors.centerIn: parent
                width: 12 * scaleFactor
                height: 12 * scaleFactor
                radius: 6 * scaleFactor
                color: Design.Colors.progressFill
                border.color: Qt.rgba(1, 1, 1, seekSlider.hovered || seekSlider.pressed ? 0.4 : 0.2)
                border.width: 1

                Behavior on width {
                    NumberAnimation {
                        duration: Design.Spacing.durationHover
                        easing.type: Easing.OutBack
                    }
                }
                Behavior on height {
                    NumberAnimation {
                        duration: Design.Spacing.durationHover
                        easing.type: Easing.OutBack
                    }
                }
                Behavior on color {
                    ColorAnimation {
                        duration: Design.Spacing.durationHover
                    }
                }
            }

            states: [
                State {
                    name: "hovered"
                    when: seekSlider.hovered && !seekSlider.pressed
                    PropertyChanges {
                        target: handleDot
                        width: 16 * scaleFactor
                        height: 16 * scaleFactor
                        radius: 8 * scaleFactor
                        color: Design.Colors.primaryLight
                    }
                },
                State {
                    name: "pressed"
                    when: seekSlider.pressed
                    PropertyChanges {
                        target: handleDot
                        width: 19 * scaleFactor
                        height: 19 * scaleFactor
                        radius: 9.5 * scaleFactor
                        color: "#6ab0ff"
                        border.color: Qt.rgba(1, 1, 1, 0.5)
                    }
                }
            ]
        }
    }

    Text {
        text: formatMs(playerController ? playerController.duration : 0)
        color: Design.Colors.textSecondary
        font.pixelSize: Design.Fonts.sizeSm * scaleFactor
        Layout.minimumWidth: 36 * scaleFactor
    }
}
