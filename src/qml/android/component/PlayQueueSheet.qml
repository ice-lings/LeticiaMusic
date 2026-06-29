import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 播放队列底部抽屉 — 支持下滑关闭手势 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale
    property var player: appContext.playerController
    property bool isOpen: false

    readonly property color clrFavorite: Design.Colors.favorite

    anchors.fill: parent
    z: 999
    visible: isOpen

    function open() {
        isOpen = true
        panel.y = panelHomeY
        refreshQueue()
    }

    function close() {
        isOpen = false
    }

    function removeFromQueue(idx) {
        if (!player) return
        var q = player.getPlayQueue()
        if (!q || idx < 0 || idx >= q.length) return
        var newQ = []
        for (var i = 0; i < q.length; i++) {
            if (i !== idx) newQ.push(q[i])
        }
        if (newQ.length === 0) {
            player.clearPlayQueue()
            queueModel.clear()
            return
        }
        player.setPlayQueue(0, newQ)
        refreshTimer.restart()
    }

    property real panelHomeY: root.parent ? root.parent.height * 0.28 : 0

    // 遮罩
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        Behavior on opacity { NumberAnimation { duration: 200 } }
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // 抽屉面板
    Rectangle {
        id: panel
        width: parent.width
        height: parent.height * 0.72
        x: 0
        y: parent.height

        Behavior on y {
            enabled: !handleArea.dragActive
            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
        }

        color: Design.Colors.background
        radius: 16 * s

        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 16 * s
            color: parent.color
        }

        // 拖动手柄
        Rectangle {
            id: handleBar
            anchors { horizontalCenter: parent.horizontalCenter }
            y: 8 * s
            width: 36 * s
            height: 5 * s
            radius: 2.5 * s
            color: Design.Colors.textHint

            MouseArea {
                id: handleArea
                anchors { fill: parent; margins: -14 * s }
                property real startY: 0
                property real startPanelY: 0
                property bool dragActive: false

                onPressed: {
                    startY = mouseY
                    startPanelY = panel.y
                    dragActive = false
                }
                onPositionChanged: {
                    var d = mouseY - startY
                    if (Math.abs(d) > 6) dragActive = true
                    if (dragActive && d > 0) panel.y = startPanelY + d
                }
                onReleased: {
                    if (dragActive) {
                        if (panel.y - startPanelY > 80) root.close()
                        else panel.y = panelHomeY
                    }
                    dragActive = false
                }
                onClicked: {
                    if (!dragActive) root.close()
                }
            }
        }

        // 播放模式行
        Row {
            id: modeRow
            anchors { top: parent.top; topMargin: 26 * s; left: parent.left; leftMargin: 16 * s }
            height: 32 * s
            spacing: 14 * s

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 20 * s; height: 20 * s
                source: "qrc:/img/list-loop.svg"
                fillMode: Image.PreserveAspectFit
                opacity: player && player.playMode === 0 ? 1.0 : 0.4
                MouseArea {
                    anchors.fill: parent; anchors.margins: -6 * s
                    onClicked: { if (player) player.setPlayMode(0) }
                }
            }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 20 * s; height: 20 * s
                source: "qrc:/img/single-loop.svg"
                fillMode: Image.PreserveAspectFit
                opacity: player && player.playMode === 1 ? 1.0 : 0.4
                MouseArea {
                    anchors.fill: parent; anchors.margins: -6 * s
                    onClicked: { if (player) player.setPlayMode(1) }
                }
            }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 20 * s; height: 20 * s
                source: "qrc:/img/shuffle.svg"
                fillMode: Image.PreserveAspectFit
                opacity: player && player.playMode === 2 ? 1.0 : 0.4
                MouseArea {
                    anchors.fill: parent; anchors.margins: -6 * s
                    onClicked: { if (player) player.setPlayMode(2) }
                }
            }
        }

        // 清空
        Image {
            anchors { right: parent.right; rightMargin: 20 * s; verticalCenter: modeRow.verticalCenter }
            width: 20 * s; height: 20 * s
            source: "qrc:/img/trash.svg"
            fillMode: Image.PreserveAspectFit
            visible: queueList.count > 0
            MouseArea {
                anchors.fill: parent; anchors.margins: -6 * s
                onClicked: {
                    if (player) player.clearPlayQueue()
                    queueModel.clear()
                }
            }
        }

        // 分割线
        Rectangle {
            anchors {
                top: modeRow.bottom
                topMargin: 8 * s
                left: parent.left; right: parent.right
                leftMargin: 16 * s; rightMargin: 16 * s
            }
            height: 0.5
            color: Design.Colors.divider
        }

        // 歌曲列表
        ListView {
            id: queueList
            anchors {
                top: modeRow.bottom
                topMargin: 16 * s
                left: parent.left; right: parent.right
                bottom: parent.bottom; bottomMargin: 8 * s
            }
            clip: true
            model: ListModel { id: queueModel }
            spacing: 0
            reuseItems: true

            Label {
                anchors.centerIn: parent
                text: "播放列表为空"
                color: Design.Colors.textSecondary
                font.pixelSize: 15 * s
                visible: queueList.count === 0
            }

            delegate: Rectangle {
                width: ListView.view.width
                height: 54 * s
                color: isCurrent ? "#14ffffff" : "transparent"

                Text {
                    anchors { left: parent.left; leftMargin: 20 * s; verticalCenter: parent.verticalCenter }
                    width: parent.width - 52 * s
                    text: (title || "未知") + " - " + (artist || "佚名")
                    font.pixelSize: 15 * s
                    font.weight: isCurrent ? Font.Medium : Font.Normal
                    color: isCurrent ? clrFavorite : Design.Colors.textPrimary
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Image {
                    anchors { right: parent.right; rightMargin: 16 * s; verticalCenter: parent.verticalCenter }
                    width: 15 * s; height: 15 * s
                    source: "qrc:/img/close-x.svg"
                    fillMode: Image.PreserveAspectFit
                    MouseArea {
                        anchors.fill: parent; anchors.margins: -8 * s
                        onClicked: root.removeFromQueue(index)
                    }
                }

                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                    anchors.leftMargin: 20 * s
                    height: 0.5
                    color: Design.Colors.divider
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 42 * s
                    onClicked: { if (player) player.playAt(index) }
                }
            }
        }
    }

    function refreshQueue() {
        queueModel.clear()
        if (!player) return
        var q = player.getPlayQueue()
        if (!q || q.length === 0) return
        var curIdx = player.currentIndex
        for (var i = 0; i < q.length; i++) {
            var item = q[i]
            queueModel.append({
                title: item.title || "未知歌曲",
                artist: item.artist || "佚名",
                isCurrent: i === curIdx
            })
        }
    }

    Timer { id: refreshTimer; interval: 150; onTriggered: refreshQueue() }

    Connections {
        target: player
        function onQueueChanged() { refreshTimer.restart() }
        function onCurrentIndexChanged() { refreshTimer.restart() }
    }
}
