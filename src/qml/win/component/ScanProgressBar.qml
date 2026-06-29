import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

Item {
    id: root

    // 外部可定位本组件，内部只占用 card 的尺寸
    width: 340
    implicitHeight: card.implicitHeight

    // 状态: "hidden" | "scanning" | "completed"
    property string displayState: appContext.scanning ? "scanning" : "hidden"
    property string completionText: ""

    property string dynamicPhaseText: {
        if (appContext.scanPhase === 2 && appContext.scanTotal > 0) {
            return qsTr("正在提取元数据 %1 / %2").arg(appContext.scanCurrent).arg(appContext.scanTotal)
        }
        return appContext.scanPhaseText
    }

    visible: displayState !== "hidden"
    opacity: 0
    Behavior on opacity { NumberAnimation { duration: 250 } }

    // 进入时淡入；进入 completed 态后延迟淡出
    onDisplayStateChanged: {
        if (displayState === "scanning" || displayState === "completed") {
            opacity = 1.0
        } else {
            opacity = 0
        }

        if (displayState === "completed") {
            fadeOutTimer.restart()
        } else {
            fadeOutTimer.stop()
        }
    }

    Component.onCompleted: {
        if (displayState !== "hidden") opacity = 1.0
    }

    Timer {
        id: fadeOutTimer
        interval: 2500
        repeat: false
        onTriggered: {
            root.displayState = "hidden"
        }
    }

    Connections {
        target: appContext
        function onScanningChanged() {
            if (appContext.scanning) {
                root.displayState = "scanning"
            }
        }
        function onScanCompleted(totalAdded) {
            if (totalAdded > 0) {
                root.completionText = qsTr("扫描完成，新增 %1 首").arg(totalAdded)
            } else {
                root.completionText = qsTr("扫描完成，没有新歌曲")
            }
            root.displayState = "completed"
        }
    }

    Rectangle {
        id: card
        anchors.fill: parent
        implicitHeight: columnLayout.implicitHeight + 28
        radius: 12
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1

        // 轻微阴影
        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.margins: -2
            radius: parent.radius + 2
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.25)
            border.width: 1
        }

        ColumnLayout {
            id: columnLayout
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    id: titleText
                    text: root.displayState === "completed"
                          ? qsTr("扫描完成")
                          : qsTr("音乐库扫描")
                    font.pixelSize: 14
                    font.bold: true
                    color: Design.Colors.textPrimary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                // 取消按钮（只在扫描中显示）
                Item {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    visible: root.displayState === "scanning"

                    Rectangle {
                        anchors.fill: parent
                        radius: 10
                        color: cancelMouseArea.containsMouse
                               ? Qt.rgba(1, 1, 1, 0.15)
                               : "transparent"

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/img/close-x.svg"
                            width: 18
                            height: 18
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    MouseArea {
                        id: cancelMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: appContext.cancelScan()
                    }
                }
            }

            Text {
                id: phaseText
                text: root.displayState === "completed"
                      ? root.completionText
                      : root.dynamicPhaseText
                font.pixelSize: 12
                color: Design.Colors.textSecondary
                Layout.fillWidth: true
                elide: Text.ElideRight
                visible: text !== ""
            }

            ProgressBar {
                id: progressBar
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                from: 0
                to: 100
                visible: root.displayState === "scanning"
                indeterminate: appContext.scanning && appContext.scanTotal === 0
                value: appContext.scanTotal > 0
                       ? (appContext.scanCurrent / appContext.scanTotal * 100)
                       : 0

                background: Rectangle {
                    implicitHeight: 4
                    color: Qt.rgba(1, 1, 1, 0.1)
                    radius: 2
                }

                contentItem: Item {
                    implicitHeight: 4
                    Rectangle {
                        width: progressBar.indeterminate
                               ? parent.width * 0.3
                               : progressBar.visualPosition * parent.width
                        height: parent.height
                        color: Design.Colors.primary
                        radius: 2

                        SequentialAnimation on x {
                            running: progressBar.indeterminate && progressBar.visible
                            loops: Animation.Infinite
                            NumberAnimation { from: 0; to: progressBar.width * 0.7; duration: 900; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: progressBar.width * 0.7; to: 0; duration: 900; easing.type: Easing.InOutQuad }
                        }
                    }
                }
            }

            Text {
                id: currentFileText
                text: appContext.scanCurrentFile
                font.pixelSize: 10
                color: Qt.rgba(1, 1, 1, 0.45)
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? implicitHeight : 0
                elide: Text.ElideMiddle
                visible: root.displayState === "scanning" && text !== ""
            }
        }
    }
}
