import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property int highlightIndex: -1

    function highlightAndScrollTo(idx) {
        if (idx < 0 || idx >= localMusicListView.count) return
        highlightIndex = idx
        localMusicListView.positionViewAtIndex(idx, ListView.Center)
        highlightClearTimer.restart()
    }

    Timer {
        id: highlightClearTimer
        interval: 3000
        onTriggered: root.highlightIndex = -1
    }

    // 歌曲总数显示标签
    Text {
        id: countLabel
        text: qsTr("%1 首歌曲").arg(localMusicModel.count)
        color: Design.Colors.textSecondary
        font.pixelSize: 12
        anchors.horizontalCenter: parent.horizontalCenter
        // 动态定位：根据列表滚动位置实现隐藏/显示动画
        y: localMusicListView.contentY <= 0 ? (10 + localMusicListView.contentY * 0.3) : -25
        opacity: localMusicListView.contentY <= 0 ? 1.0 : 0.0
        // 添加平滑动画效果
        Behavior on y { NumberAnimation { duration: 200 } }
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    ListView {
        id: localMusicListView
        anchors.fill: parent
        anchors.rightMargin: 28
        anchors.top: countLabel.bottom  // 列表从标签下方开始
        clip: true
        model: localMusicModel
        currentIndex: -1

        cacheBuffer: Math.max(height * 2, 1000)
        boundsBehavior: Flickable.StopAtBounds
        maximumFlickVelocity: 2500
        flickDeceleration: 1500

        displayMarginBeginning: 200
        displayMarginEnd: 200

        property var player: appContext.playerController
        highlight: null
        highlightFollowsCurrentItem: false

        delegate: Rectangle {
            id: delegateItem
            width: localMusicListView.width
            height: 70
            color: !model.fileExists ? "#18E53935" : "transparent"

            Rectangle {
                id: background
                anchors.fill: parent
                anchors.margins: 5
                radius: 8
                color: {
                    if (index === root.highlightIndex)
                        return Design.Colors.withAlpha(Design.Colors.primary, 0.35)
                    else if (delegateItem.ListView.isCurrentItem)
                        return "#4DFFFFFF"
                    else if (mainMouseArea.containsPress)
                        return "#4DFFFFFF"
                    else if (mainMouseArea.containsMouse)
                        return "#19FFFFFF"
                    else
                        return "transparent"
                }

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }

            MouseArea {
                id: mainMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                z: 0

                onClicked: {
                    localMusicListView.currentIndex = index
                }

                onDoubleClicked: {
                    if (model.filePath && localMusicListView.player) {
                        if (localMusicListView.player.currentPlaylistId === 0) {
                            localMusicListView.player.playAt(index)
                        } else {
                            var queueItems = localMusicModel.getAllItems()
                            localMusicListView.player.setQueueSourceType(0)
                            localMusicListView.player.setPlayQueue(0, queueItems)
                            localMusicListView.player.playAt(index)
                        }
                    }
                }
            }

            MouseArea {
                id: rightMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                z: 0

                onClicked: {
                    songContextMenu.musicHash = model.musicHash || ""
                    songContextMenu.musicFilePath = model.filePath || ""
                    songContextMenu.musicTitle = model.title || ""
                    songContextMenu.popup()
                }
            }

            RowLayout {
                id: contentRow
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 15
                z: 1

                Image {
                    id: coverImage
                    source: model.coverArt || "qrc:/img/assets/images/default_music_cover.png"
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 50
                    Layout.alignment: Qt.AlignVCenter
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: true
                    smooth: true
                    opacity: status === Image.Ready ? 1.0 : 0.3

                    Behavior on opacity {
                        NumberAnimation { duration: 200 }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    Text {
                        text: model.title || "Unknown Title"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 16
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Rectangle {
                            visible: model.quality !== ""
                            Layout.alignment: Qt.AlignVCenter
                            width: qualityTag.implicitWidth + 8
                            height: 16
                            radius: 3
                            color: model.quality !== "LQ" ? (model.qualityColor || "transparent") : "transparent"
                            border.width: model.quality === "LQ" ? 1 : 0
                            border.color: "#666666"

                            Text {
                                id: qualityTag
                                anchors.centerIn: parent
                                text: model.quality || ""
                                color: model.quality === "LQ" ? "#888888" : "#FFFFFF"
                                font.pixelSize: 10
                                font.bold: true
                            }
                        }

                        Text {
                            text: (model.artist || "Unknown Artist") + " • " +
                                  (model.duration || "0:00")
                            color: "#B3AAAAEE"
                            font.pixelSize: 14
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                FavoriteButton {
                    id: favoriteBtn
                    musicHash: model.musicHash || ""
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: 10
                    Layout.minimumWidth: 32
                    Layout.minimumHeight: 32
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    Layout.maximumWidth: 32
                    Layout.maximumHeight: 32
                    z: 100
                }
            }
        }

        Label {
            id: emptyLabel
            anchors.centerIn: parent
            text: qsTr("No music found\nClick to refresh")
            visible: localMusicListView.count === 0
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.pixelSize: 18
            wrapMode: Text.WordWrap

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    if (typeof localMusicModel.refresh === "function") {
                        localMusicModel.refresh()
                    }
                }
            }
        }
    }

    AlphabetIndex {
        id: alphabetIndex
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 28
        listView: localMusicListView
        visible: localMusicListView.count > 0
    }

    SongContextMenu {
        id: songContextMenu
    }
}