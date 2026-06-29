import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/* 扫描进度卡片 — 浮在列表底部 */
Rectangle {
    id: root
    visible: appContext.scanning

    anchors {
        left: parent.left
        right: parent.right
        bottom: parent.bottom
        leftMargin: 8
        rightMargin: 8
        bottomMargin: 8
    }
    height: 56
    radius: 12
    color: Design.Colors.surface
    opacity: 0.95

    Behavior on opacity {
        NumberAnimation { duration: 300 }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        // 旋转动画指示器
        Rectangle {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            radius: 10
            color: Design.Colors.primary
            opacity: 0.8

            RotationAnimator on rotation {
                from: 0; to: 360
                duration: 1000
                loops: Animation.Infinite
                running: root.visible
            }
        }

        Column {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: appContext.scanPhaseText || "正在扫描..."
                font.pixelSize: 13
                font.weight: Font.Medium
                color: Design.Colors.textPrimary
                elide: Text.ElideRight
                width: parent.width
            }

            Text {
                text: appContext.scanTotal > 0
                    ? qsTr("%1 / %2").arg(appContext.scanCurrent).arg(appContext.scanTotal)
                    : ""
                font.pixelSize: 11
                color: Design.Colors.textSecondary
                visible: appContext.scanTotal > 0
            }
        }

        ProgressBar {
            Layout.preferredWidth: 60
            Layout.preferredHeight: 4
            value: appContext.scanTotal > 0 ? appContext.scanCurrent / appContext.scanTotal : 0

            background: Rectangle {
                color: Design.Colors.divider
                radius: 2
            }
            contentItem: Rectangle {
                color: Design.Colors.primary
                radius: 2
                width: parent.visualPosition * parent.width
            }
        }
    }
}
