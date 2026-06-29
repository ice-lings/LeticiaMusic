import QtQuick
import QtQuick.Shapes
import QtQuick.Window

/* 通用浮动进度球 — 支持同步/扫描双模式 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale
    readonly property real ballSize: 80 * s

    // 模式: "hidden" | "sync" | "scan"
    property string mode: "hidden"
    property real progress: 0.0
    property string phaseText: ""

    anchors {
        right: parent ? parent.right : undefined
        top: parent ? parent.top : undefined
        rightMargin: 16 * s
        topMargin: 60 * s
    }
    width: ballSize
    height: ballSize
    z: 500

    visible: mode !== "hidden"
    scale: mode === "hidden" ? 0 : 1
    opacity: mode === "hidden" ? 0 : 1

    Behavior on scale { NumberAnimation { duration: 280; easing.type: Easing.OutBack } }
    Behavior on opacity { NumberAnimation { duration: 280 } }

    // ═══════ 圆形主体 ═══════
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: {
            if (mode === "sync" && syncCancelling) return "#cc4a1a1a"
            if (mode === "scan") return "#cc0a2a1a"
            return "#cc1a1a27"
        }

        // ═══════ 环形进度 (Shape) ═══════
        Shape {
            anchors.fill: parent
            anchors.margins: 4
            visible: mode !== "hidden"

            ShapePath {
                id: ringPath
                strokeColor: {
                    if (mode === "sync" && syncCancelling) return Design.Colors.error
                    if (mode === "sync") return Design.Colors.primary
                    return "#4caf50"  // scan mode
                }
                strokeWidth: 4
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: root.width / 2
                    centerY: root.height / 2
                    radiusX: root.width / 2 - 6
                    radiusY: root.height / 2 - 6
                    startAngle: -90
                    sweepAngle: Math.min(root.progress * 360, 360)
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
                    if (mode === "sync" && syncCancelling) return "取消同步…"
                    if (mode === "scan") return "扫描中…"
                    return "同步中…"
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 11 * s
                color: "#aaa"
                text: phaseText
                visible: phaseText && (mode === "sync" || mode === "scan")
                elide: Text.ElideRight
                maximumLineCount: 1
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // ═══════ 同步模式交互 (仅 sync 模式) ═══════
    property bool syncCancelling: false

    MouseArea {
        anchors.fill: parent
        z: 10
        enabled: mode === "sync"

        property bool longPressTriggered: false

        onPressed: {
            if (mode !== "sync") return
            longPressTriggered = false
            longPressTimer.start()
        }
        onReleased: {
            if (mode !== "sync") return
            longPressTimer.stop()
            if (!longPressTriggered) handleShortPress()
        }
        onCanceled: { longPressTimer.stop() }
    }

    Timer {
        id: longPressTimer
        interval: 500
        onTriggered: {
            longPressTriggered = true
            if (mode === "sync" && !syncCancelling) {
                syncCancelling = true
            }
        }
    }

    function handleShortPress() {
        if (mode !== "sync") return
        if (syncCancelling) {
            appContext.cancelSync()
            hide()
        }
    }

    // ═══════ 信号监听 ═══════
    Connections {
        target: appContext

        function onScanningChanged() { updateMode() }
        function onSyncRunningChanged() {
            syncCancelling = false
            updateMode()
        }

        function onSyncProgressChanged(cur, total, file) {
            if (mode === "sync" && total > 0) progress = cur / total
        }
        function onSyncPhaseChanged(phase, name) {
            if (mode === "sync") phaseText = name
        }
        function onSyncCompleted(totalOps, conflicts) {
            progress = 1.0
            completeTimer.start()
        }

        function onScanProgressChanged() {
            if (mode === "scan") {
                var cur = appContext.scanCurrent
                var total = appContext.scanTotal
                if (total > 0) progress = cur / total
            }
        }
        function onScanPhaseChanged() {
            if (mode === "scan") phaseText = appContext.scanPhaseText
        }
    }

    function updateMode() {
        if (appContext.syncRunning) mode = "sync"
        else if (appContext.scanning) mode = "scan"
        else { mode = "hidden"; hide() }
    }

    Timer {
        id: completeTimer
        interval: 2000
        onTriggered: hide()
    }

    function hide() {
        progress = 0
        phaseText = ""
        syncCancelling = false
    }
}
