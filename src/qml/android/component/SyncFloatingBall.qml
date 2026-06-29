import QtQuick
import QtQuick.Shapes
import QtQuick.Window

/* 同步浮动进度球 — 右上角圆形悬停组件 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale
    readonly property real ballSize: 80 * s

    // 状态: hidden | syncing | cancelling | completed
    property string state_: "hidden"
    property real progress: 0.0
    property string phaseText: ""
    property int totalOps: 0
    property int conflicts: 0

    // 暴露给 Main.qml 定位
    anchors {
        right: parent ? parent.right : undefined
        top: parent ? parent.top : undefined
        rightMargin: 16 * s
        topMargin: 60 * s
    }
    width: ballSize
    height: ballSize
    z: 500

    visible: state_ !== "hidden"
    scale: state_ === "hidden" ? 0 : 1
    opacity: state_ === "hidden" ? 0 : 1

    Behavior on scale { NumberAnimation { duration: 280; easing.type: Easing.OutBack } }
    Behavior on opacity { NumberAnimation { duration: 280 } }

    // ═══════ 圆形主体 ═══════
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: {
            if (state_ === "cancelling") return "#cc4a1a1a"
            return "#cc1a1a27"
        }
        border.width: 0

        // ═══════ 环形进度 (Shape) ═══════
        Shape {
            anchors.fill: parent
            anchors.margins: 4
            visible: state_ === "syncing" || state_ === "cancelling"

            ShapePath {
                id: ringPath
                strokeColor: {
                    if (state_ === "cancelling") return Design.Colors.error
                    return Design.Colors.primary
                }
                strokeWidth: 4
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    id: ringArc
                    centerX: root.width / 2
                    centerY: root.height / 2
                    radiusX: root.width / 2 - 6
                    radiusY: root.height / 2 - 6
                    startAngle: -90
                    sweepAngle: Math.min(root.progress * 360, 360)
                }
            }
        }

        // ═══════ 完成满环 (绿色) ═══════
        Shape {
            anchors.fill: parent
            anchors.margins: 4
            visible: state_ === "completed"

            ShapePath {
                strokeColor: "#4caf50"
                strokeWidth: 4
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: root.width / 2
                    centerY: root.height / 2
                    radiusX: root.width / 2 - 6
                    radiusY: root.height / 2 - 6
                    startAngle: -90
                    sweepAngle: 360
                }
            }
        }

        // ═══════ 中心文字 ═══════
        Column {
            anchors.centerIn: parent
            spacing: 2 * s
            width: parent.width - 20 * s

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 14 * s
                font.weight: Font.Bold
                color: Design.Colors.textPrimary
                text: {
                    if (state_ === "cancelling") return "取消同步…"
                    if (state_ === "completed") return "完成"
                    return "同步中…"
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 11 * s
                color: "#aaa"
                text: phaseText
                visible: phaseText && state_ === "syncing"
                elide: Text.ElideRight
                maximumLineCount: 1
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // ═══════ 交互 ═══════
    // 长按: syncing → cancelling
    MouseArea {
        anchors.fill: parent
        z: 10

        property bool longPressTriggered: false

        onPressed: {
            longPressTriggered = false
            longPressTimer.start()
        }
        onReleased: {
            longPressTimer.stop()
            if (!longPressTriggered) {
                handleShortPress()
            }
        }
        onCanceled: {
            longPressTimer.stop()
        }
    }

    Timer {
        id: longPressTimer
        interval: 500
        onTriggered: {
            longPressTriggered = true
            handleLongPress()
        }
    }

    function handleShortPress() {
        if (state_ === "cancelling") {
            appContext.cancelSync()
            hide()
        }
    }

    function handleLongPress() {
        if (state_ === "syncing") {
            state_ = "cancelling"
        }
    }

    // ═══════ 同步信号连接 ═══════
    Connections {
        target: appContext

        function onSyncRunningChanged() {
            if (appContext.syncRunning) {
                root.state_ = "syncing"
            } else {
                root.hide()
            }
        }

        function onSyncProgressChanged(cur, total, file) {
            if (total > 0) {
                root.progress = cur / total
            }
        }

        function onSyncPhaseChanged(phase, name) {
            root.phaseText = name
        }

        function onSyncCompleted(totalOps, conflicts) {
            root.progress = 1.0
            root.state_ = "completed"
            root.totalOps = totalOps
            root.conflicts = conflicts
            completeTimer.start()
        }
    }

    Timer {
        id: completeTimer
        interval: 2000
        onTriggered: root.hide()
    }

    // ═══════ 消失动画 ═══════
    function hide() {
        scale = 0
        opacity = 0
        state_ = "hidden"
        progress = 0
        phaseText = ""
    }
}
