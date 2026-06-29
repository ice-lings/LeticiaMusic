import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

Item {
    id: root

    width: 340
    implicitHeight: card.implicitHeight

    property string state_: "hidden"
    property string phaseName: ""
    property int current: 0
    property int total: 0
    property string currentFile: ""
    property bool syncRunning: false

    visible: state_ !== "hidden"
    opacity: state_ === "hidden" ? 0 : 1.0

    onState_Changed: {
        // console.log("[SyncCard] state:", state_, "syncRunning:", syncRunning)
        if (state_ === "completed") {
            fadeOutTimer.restart()
        }
    }

    Component.onCompleted: {
        // console.log("[SyncCard] loaded, appContext.syncRunning:", appContext ? appContext.syncRunning : "undefined")
        if (appContext && appContext.syncRunning) {
            root.state_ = "syncing"
        }
    }

    Timer {
        id: fadeOutTimer
        interval: 2500
        repeat: false
        onTriggered: { root.state_ = "hidden" }
    }

    Connections {
        target: appContext
        function onSyncRunningChanged() {
            syncRunning = appContext.syncRunning
            if (syncRunning) {
                root.state_ = "syncing"
            }
        }
        function onSyncPhaseChanged(ph, name) {
            phaseName = name
        }
        function onSyncProgressChanged(cur, tot, file) {
            current = cur
            total = tot
            currentFile = file
        }
        function onSyncCompleted(totalOps, conflicts) {
            root.state_ = "completed"
        }
        function onSyncError(message) {
            root.state_ = "hidden"
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
                    text: state_ === "completed"
                          ? qsTr("同步完成")
                          : qsTr("NAS 同步中")
                    font.pixelSize: 14
                    font.bold: true
                    color: Design.Colors.textPrimary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Item {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    visible: state_ === "syncing"

                    Rectangle {
                        anchors.fill: parent
                        radius: 10
                        color: cancelMouse.containsMouse
                               ? Qt.rgba(1, 1, 1, 0.15)
                               : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "\u00D7"
                            font.pixelSize: 18
                            color: Design.Colors.textSecondary
                        }
                    }

                    MouseArea {
                        id: cancelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: appContext.cancelSync()
                    }
                }
            }

            Text {
                text: state_ === "completed"
                      ? qsTr("同步完成")
                      : qsTr("阶段: ") + phaseName
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
                visible: state_ === "syncing"
                indeterminate: total === 0
                value: total > 0 ? (current / total * 100) : 0

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
                text: currentFile
                font.pixelSize: 10
                color: Qt.rgba(1, 1, 1, 0.45)
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? implicitHeight : 0
                elide: Text.ElideMiddle
                visible: state_ === "syncing" && text !== ""
            }

            Text {
                text: qsTr("总计 %1 项，已完成 %2 项").arg(total).arg(current)
                font.pixelSize: 11
                color: Qt.rgba(1, 1, 1, 0.45)
                Layout.fillWidth: true
                visible: state_ === "syncing" && total > 0
            }
        }
    }
}
