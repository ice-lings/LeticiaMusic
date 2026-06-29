import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 迷你播放控制条 */
Rectangle {
    id: root

    readonly property real s: Design.Responsive.guiScale
    property var player: appContext.playerController


    height: 62 * s
    color: Design.Colors.background
    radius: 12 * s

    Rectangle {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 0.5
        color: Design.Colors.divider
    }

    property var currentItem: null
    property string currentFile: ""

    function modeSvg(mode) {
        return Tools.modeSvg(mode)
    }

    function refreshInfo() {
        if (!player || !player.currentFile) {
            currentItem = null
            currentFile = ""
            return
        }
        if (player.currentFile !== currentFile) {
            currentFile = player.currentFile
            currentItem = appContext.getMusicItemForFile(currentFile)
            if (currentItem && currentItem.coverPath) {
                appContext.config.lastCoverPath = currentItem.coverPath
            }
        }
    }

    Connections {
        target: player
        function onCurrentFileChanged() { root.refreshInfo() }
    }

    Component.onCompleted: refreshInfo()

    Row {
        anchors.fill: parent
        anchors.leftMargin: 4 * s
        anchors.rightMargin: 2 * s
        spacing: 4 * s

        // 封面 + 歌名
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 200 * s
            height: parent.height - 8 * s
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                onClicked: root.showNowPlaying()
            }

            Row {
                anchors { fill: parent; leftMargin: 2 }
                spacing: 7 * s

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40 * s; height: 40 * s; radius: 20 * s
                    color: Design.Colors.listItemHovered
                    clip: true

                    Image {
                        id: coverImage
                        anchors.fill: parent
                        asynchronous: true
                        cache: true
                        source: {
                            if (currentItem && currentItem.coverPath) return currentItem.coverPath
                            return appContext.config.lastCoverPath || ""
                        }
                        fillMode: Image.PreserveAspectCrop
                        visible: status === Image.Ready
                    }
                    Image {
                        anchors.centerIn: parent
                        width: 24 * s; height: 24 * s
                        source: "qrc:/img/music-note.svg"
                        fillMode: Image.PreserveAspectFit
                        visible: !currentItem || coverImage.status !== Image.Ready
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 38 * s
                    spacing: 2 * s

                    Text {
                        width: parent.width
                        text: currentItem ? (currentItem.title || "未知歌曲") : "未播放"
                        font.pixelSize: 15 * s
                        font.weight: Font.Medium
                        color: Design.Colors.textPrimary
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                    Text {
                        width: parent.width
                        text: currentItem ? (currentItem.artist || "佚名")
                                          : "点击歌曲开始播放"
                        font.pixelSize: 13 * s
                        color: Design.Colors.textSecondary
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                }
            }
        }

        // 控制按钮
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1 * s

            // 上一首
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 36 * s; height: 36 * s; radius: 18 * s
                color: "transparent"
                opacity: (player && player.queueCount > 1) ? 1.0 : 0.4

                Image {
                    anchors.centerIn: parent
                    width: 22 * s; height: 22 * s
                    source: "qrc:/img/prev.svg"
                    fillMode: Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { if (player && player.queueCount > 1) player.playPrev() }
                }
            }

            // 播放/暂停
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 40 * s; height: 40 * s; radius: 20 * s
                color: Design.Colors.primary

                Image {
                    anchors.centerIn: parent
                    width: 22 * s; height: 22 * s
                    source: (player && player.isPlaying) ? "qrc:/img/pause-circle.svg" : "qrc:/img/play-circle.svg"
                    fillMode: Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { if (player) player.playOrPause() }
                }
            }

            // 下一首
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 36 * s; height: 36 * s; radius: 18 * s
                color: "transparent"
                opacity: (player && player.queueCount > 1) ? 1.0 : 0.4

                Image {
                    anchors.centerIn: parent
                    width: 22 * s; height: 22 * s
                    source: "qrc:/img/next.svg"
                    fillMode: Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { if (player && player.queueCount > 1) player.playNext() }
                }
            }

            // 歌单按钮
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 32 * s; height: 32 * s; radius: 16 * s
                color: "transparent"

                Image {
                    anchors.centerIn: parent
                    width: 20 * s; height: 20 * s
                    source: "qrc:/img/queue.svg"
                    fillMode: Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.showQueue()
                }
            }

            // 播放模式切换
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 32 * s; height: 32 * s; radius: 16 * s
                color: "transparent"

                Image {
                    anchors.centerIn: parent
                    width: 20 * s; height: 20 * s
                    source: root.modeSvg(player ? player.playMode : 0)
                    fillMode: Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { if (player) player.setPlayMode((player.playMode + 1) % 3) }
                }
            }
        }
    }

    signal showNowPlaying()
    signal showQueue()
}

