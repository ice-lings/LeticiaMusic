import QtQuick
import QtQuick.Controls
import QtQuick.Window
import "../../common/control"

/* 播放详情页 — 全屏，全区域下滑关闭 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale
    property var player: appContext.playerController
    property bool isOpen: false
    property bool closing: false

    readonly property color clrFavorite: Design.Colors.favorite

    anchors.fill: parent
    z: 1000
    visible: isOpen

    property var currentItem: null
    property string musicHash: ""
    property bool isFavorite: false

    // 按钮行自动缩放
    readonly property real btnRowUsable: root.width - 40 * s
    readonly property real btnRowIdeal: (4 * 48 + 70 + 4 * 20) * s
    readonly property real btnScale: Math.min(1.0, btnRowUsable / btnRowIdeal)
    readonly property real panelHomeY: 0
    readonly property real dismissThreshold: panel.height * 0.28   // 下滑超过 28% 即关闭
    readonly property real longPressDuration: 150
    readonly property real dragActivateDistance: 25
    readonly property real dragFastDistance: 60

    function open() {
        isOpen = true
        closing = false
        openAnim.from = panel.parent.height
        openAnim.to = panelHomeY
        openAnim.duration = 350
        openAnim.start()
        refreshInfo()
    }

    function close() { dismiss() }

    function dismiss() {
        closing = true
        closeAnim.from = panel.y
        closeAnim.to = panel.parent.height
        closeAnim.duration = Math.max(150, 300 * (1 - (panel.y - panelHomeY) / panel.parent.height))
        closeAnim.start()
    }

    function snapBack() {
        snapAnim.from = panel.y
        snapAnim.to = panelHomeY
        snapAnim.duration = Math.max(150, 300 * (panel.y - panelHomeY) / panel.parent.height)
        snapAnim.start()
    }

    function refreshInfo() {
        if (!player || !player.currentFile) {
            currentItem = null; musicHash = ""; isFavorite = false; return
        }
        currentItem = appContext.getMusicItemForFile(player.currentFile)
        musicHash = ""
        var model = appContext.getViewModel("local_music")
        if (model && player.currentFile) {
            var idx = model.indexForFilePath(player.currentFile)
            if (idx >= 0) {
                var item = model.get(idx)
                musicHash = item.musicHash || ""
            }
        }
        isFavorite = musicHash && appContext.favoriteManager ? appContext.favoriteManager.checkIsFavorite(musicHash) : false
    }

    function toggleFavorite() {
        if (musicHash && appContext.favoriteManager) {
            appContext.favoriteManager.toggleFavorite(musicHash)
            isFavorite = appContext.favoriteManager.checkIsFavorite(musicHash)
        }
    }

    function modeSvg(mode) {
        return Tools.modeSvg(mode)
    }

    // === 展开动画 ===
    NumberAnimation {
        id: openAnim
        target: panel
        property: "y"
        easing.type: Easing.OutCubic
    }

    // === 关闭动画 ===
    NumberAnimation {
        id: closeAnim
        target: panel
        property: "y"
        easing.type: Easing.InCubic
        onStopped: {
            if (closing) {
                isOpen = false
                closing = false
            }
        }
    }

    // === 回弹动画 ===
    NumberAnimation {
        id: snapAnim
        target: panel
        property: "y"
        easing.type: Easing.OutCubic
    }

    // === 面板 ===
    Rectangle {
        id: panel
        width: parent.width
        height: parent.height
        x: 0
        y: parent.height

        clip: true
        color: Design.Colors.background

        // === 全区域拖拽 ===
        MouseArea {
            id: dragMouse
            anchors.fill: parent

            property real pressY: 0
            property real pressPanelY: 0
            property bool dragging: false
            property real targetY: panelHomeY
            property real lastMouseY: 0
            property real lastTime: 0
            property real currentSpeed: 0
            property bool longPressReady: false
            property bool blockDrag: false

            onPressed: function(mouse) {
                if (blockDrag) {
                    mouse.accepted = false
                    return
                }
                if (closeAnim.running) {
                    closeAnim.stop()
                    closing = false
                } else {
                    closeAnim.stop()
                }
                openAnim.stop()
                snapAnim.stop()
                pressY = mouse.y
                pressPanelY = panel.y
                targetY = panel.y
                dragging = false
                longPressReady = false
                currentSpeed = 0
                lastMouseY = mouse.y
                lastTime = Date.now()
                longPressTimer.start()
            }

            onPositionChanged: function(mouse) {
                if (blockDrag) return
                var now = Date.now()
                var dt = now - lastTime
                if (dt > 0) {
                    currentSpeed = Math.abs(mouse.y - lastMouseY) / dt
                }
                lastMouseY = mouse.y
                lastTime = now

                var delta = Math.abs(mouse.y - pressY)
                if (!dragging) {
                    if (delta > 60) {
                        dragging = true
                    } else if (delta > 25 && longPressReady) {
                        dragging = true
                    }
                }
                if (dragging) {
                    targetY = pressPanelY + mouse.y - pressY
                    targetY = Math.max(panelHomeY, Math.min(panel.parent.height, targetY))
                    if (!interpTimer.running) {
                        interpTimer.lastFrameTime = Date.now()
                        interpTimer.start()
                    }
                }
            }

            onReleased: function(mouse) {
                longPressTimer.stop()
                interpTimer.stop()
                blockDrag = false
                if (!dragging) return
                dragging = false
                if (panel.y - pressPanelY > dismissThreshold
                        || (currentSpeed > 0.5 && panel.y - pressPanelY > dismissThreshold * 0.6)) {
                    dismiss()
                } else {
                    snapBack()
                }
            }

            onCanceled: {
                longPressTimer.stop()
                interpTimer.stop()
                blockDrag = false
                if (dragging) snapBack()
                dragging = false
            }
        }

        Timer {
            id: longPressTimer
            interval: 150
            repeat: false
            onTriggered: dragMouse.longPressReady = true
        }

        Timer {
            id: interpTimer
            interval: 1
            repeat: true
            property real lastFrameTime: 0

            onTriggered: {
                var now = Date.now()
                var dt = (now - lastFrameTime) / 1000.0
                lastFrameTime = now
                if (dt <= 0 || dt > 0.1) return

                if (!dragMouse.dragging) {
                    stop()
                    return
                }

                var speedRatio = Math.min(1.0, dragMouse.currentSpeed / 2.0)
                var baseFactor = 0.15 + speedRatio * 0.70
                var factor = 1.0 - Math.pow(1.0 - baseFactor, dt * 60.0)
                panel.y += (dragMouse.targetY - panel.y) * factor
            }
        }

        // 拖拽指示条（纯视觉）
        Rectangle {
            anchors {
                top: parent.top
                topMargin: 10 * s
                horizontalCenter: parent.horizontalCenter
            }
            width: 36 * s
            height: 5 * s
            radius: 2.5 * s
            color: Design.Colors.textHint
        }

        // === 上部内容区（封面）===
        Item {
            id: middleArea
            anchors {
                top: parent.top
                topMargin: 36 * s
                left: parent.left
                right: parent.right
                bottom: bottomArea.top
            }

            Column {
                anchors.centerIn: parent
                width: parent.width - 40 * s
                spacing: 0

                // 封面
                Rectangle {
                    id: coverBox
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width * 0.68, Math.min(280 * s, middleArea.height * 0.58))
                    height: width
                    radius: 14 * s
                    color: Design.Colors.listItemHovered
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: {
                            if (!currentItem) return ""
                            var cp = currentItem.coverPath !== undefined ? currentItem.coverPath : currentItem["coverPath"]
                            return cp || ""
                        }
                        fillMode: Image.PreserveAspectCrop
                        visible: source.toString() !== ""
                        asynchronous: true
                        cache: true
                        smooth: true
                    }
                    Image {
                        anchors.centerIn: parent
                        width: 72 * s; height: 72 * s
                        source: "qrc:/img/music-note.svg"
                        fillMode: Image.PreserveAspectFit
                        visible: !parent.children[0].visible
                    }

                    MouseArea {
                        anchors.fill: parent
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: {
                            console.log("[TODO] Cover clicked — 预留封面点击功能")
                        }
                    }
                }
            }
        }

        // === 底部固定区（信息+进度条+时间+控制按钮）===
        Column {
            id: bottomArea
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                bottomMargin: 16 * s
            }
            width: parent.width - 40 * s
            anchors.leftMargin: 20 * s
            anchors.rightMargin: 20 * s
            spacing: 0

            // 信息栏：歌名/歌手（左）+ 收藏（右）
            Row {
                anchors { left: parent.left; right: parent.right }
                height: Math.max(songInfoCol.implicitHeight, 40 * s)

                Column {
                    id: songInfoCol
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 48 * s
                    spacing: 2 * s

                    Text {
                        width: parent.width
                        text: currentItem ? (currentItem.title || "未知歌曲") : "未播放"
                        font.pixelSize: 18 * s
                        font.weight: Font.Bold
                        color: Design.Colors.textPrimary
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                    Item {
                        width: parent.width
                        height: 16 * s
                        clip: true

                        Text {
                            id: artistText
                            text: currentItem ? (currentItem.artist || "佚名") : ""
                            font.pixelSize: 14 * s
                            color: Design.Colors.textSecondary
                            anchors.verticalCenter: parent.verticalCenter

                            SequentialAnimation on x {
                                id: artistMarquee
                                loops: Animation.Infinite
                                running: artistText.implicitWidth > parent.width
                                PauseAnimation { duration: 2000 }
                                NumberAnimation {
                                    from: 0
                                    to: parent.width - artistText.implicitWidth
                                    duration: Math.max(2500, (artistText.implicitWidth - parent.width) * 20)
                                }
                                PauseAnimation { duration: 2000 }
                                NumberAnimation {
                                    to: 0
                                    duration: 500
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: 48 * s; height: 48 * s
                    color: "transparent"
                    anchors.verticalCenter: parent.verticalCenter

                    Image {
                        anchors.centerIn: parent
                        width: 24 * s; height: 24 * s
                        source: isFavorite ? "qrc:/img/heart-fill.svg" : "qrc:/img/heart-line.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: root.toggleFavorite()
                    }
                }
            }

            Item { width: 1; height: 12 * s }

            ProgressSlider {
                anchors { left: parent.left; right: parent.right }
                playerController: player
                scaleFactor: s
            }

            Item { width: 1; height: 20 * s }

            // 控制按钮（播放列表 | 上一首 | 播放/暂停 | 下一首 | 播放模式）
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 20 * s
                scale: btnScale
                transformOrigin: Item.Center

                // 播放列表
                Rectangle {
                    width: 48 * s; height: 70 * s
                    color: "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 22 * s; height: 22 * s
                        source: "qrc:/img/queue.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: { root.close(); root.showQueue() }
                    }
                }

                // 上一首
                Rectangle {
                    width: 48 * s; height: 70 * s
                    color: "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 28 * s; height: 28 * s
                        source: "qrc:/img/prev.svg"
                        fillMode: Image.PreserveAspectFit
                        opacity: (player && player.queueCount > 1) ? 1.0 : 0.4
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: player && player.queueCount > 1
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: { if (player) player.playPrev() }
                    }
                }

                // 播放/暂停
                Rectangle {
                    width: 70 * s; height: 70 * s; radius: 35 * s
                    color: Design.Colors.primary

                    Image {
                        anchors.centerIn: parent
                        width: 36 * s; height: 36 * s
                        source: (player && player.isPlaying) ? "qrc:/img/pause-circle.svg" : "qrc:/img/play-circle.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: { if (player) player.playOrPause() }
                    }
                }

                // 下一首
                Rectangle {
                    width: 48 * s; height: 70 * s
                    color: "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 28 * s; height: 28 * s
                        source: "qrc:/img/next.svg"
                        fillMode: Image.PreserveAspectFit
                        opacity: (player && player.queueCount > 1) ? 1.0 : 0.4
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: player && player.queueCount > 1
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: { if (player) player.playNext() }
                    }
                }

                // 播放模式
                Rectangle {
                    width: 48 * s; height: 70 * s
                    color: "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 24 * s; height: 24 * s
                        source: root.modeSvg(player ? player.playMode : 0)
                        fillMode: Image.PreserveAspectFit
                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed: dragMouse.blockDrag = true
                        onReleased: dragMouse.blockDrag = false
                        onClicked: { if (player) player.setPlayMode((player.playMode + 1) % 3) }
                    }
                }
            }
        }
    }

    Connections {
        target: player
        function onCurrentFileChanged() { root.refreshInfo() }
    }

    signal showQueue()
}
