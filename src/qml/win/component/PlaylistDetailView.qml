import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: playlistDetailView
    color: Design.Colors.card

    property var playlistModel: appContext.playlistDetailViewModel
    property var player: appContext.playerController
    property bool isFavoritePlaylist: playlistModel ? playlistModel.playlistName === "我喜欢的音乐" : false
    property bool isCollectionPlaylist: playlistModel ? playlistModel.isCollection : false
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                ColumnLayout {
                    Layout.fillWidth: true

                    Text {
                        text: playlistModel.playlistName || "歌单详情"
                        color: Design.Colors.textPrimary
                        font.pixelSize: 24
                        font.bold: true
                    }

                    Text {
                        text: qsTr("%1 首歌曲").arg(playlistModel.count)
                        color: Design.Colors.textDisabled
                        font.pixelSize: 14
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Design.Colors.divider
        }

        ListView {
            id: musicListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: playlistModel
            currentIndex: -1

            highlight: null
            highlightFollowsCurrentItem: false

            delegate: Rectangle {
                id: delegateItem
                width: ListView.view.width
                height: 70
                color: "transparent"

                Rectangle {
                    id: background
                    anchors.fill: parent
                    anchors.margins: 5
                    radius: 8
                    color: {
                        if (delegateItem.ListView.isCurrentItem)
                            return Design.Colors.listItemSelected
                        else if (mouseArea.containsPress)
                            return Design.Colors.listItemPressed
                        else if (mouseArea.containsMouse)
                            return Design.Colors.listItemHovered
                        else
                            return "transparent"
                    }

                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 15

                    Image {
                        id: coverImage
                        source: {
                            var coverPath = appContext.getCoverPathByFileHash(model.musicHash)
                            return coverPath && coverPath !== "" ? coverPath : "qrc:/img/assets/images/default_music_cover.png"
                        }
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 50
                        Layout.alignment: Qt.AlignVCenter
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 4

                        Text {
                            text: model.title || "Unknown Title"
                            color: Design.Colors.textPrimary
                            font.bold: true
                            font.pixelSize: 16
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (model.artist || "Unknown Artist") + " • " + (model.duration || "0:00")
                            color: Design.Colors.textSecondary
                            font.pixelSize: 14
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    
                    FavoriteButton {
                        id: favoriteBtn
                        musicHash: model.musicHash || ""
                        showFavoriteButton: !playlistDetailView.isFavoritePlaylist
                        visible: showFavoriteButton
                        implicitWidth: showFavoriteButton ? 32 : 0
                        implicitHeight: showFavoriteButton ? 32 : 0
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: showFavoriteButton ? 10 : 0
                    }
                    
                    // 收藏夹专用：添加到本地音乐按钮
                    Rectangle {
                        visible: playlistDetailView.isCollectionPlaylist
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        width: addToLocalText.width + 12
                        height: 24
                        radius: 4
                        color: addToLocalMouse.containsMouse ? Design.Colors.primary : Design.Colors.primaryDark

                        Text {
                            id: addToLocalText
                            anchors.centerIn: parent
                            text: "+ 添加到本地"
                            color: Design.Colors.textPrimary
                            font.pixelSize: 12
                        }
                        
                        MouseArea {
                            id: addToLocalMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (model.musicHash) {
                                    appContext.addToLocalMusic(model.musicHash)
                                }
                            }
                        }
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton

                    onClicked: {
                        // 单击选中
                        musicListView.currentIndex = index
                    }
                    
                    onDoubleClicked: {
                        if (model.filePath && player) {
                            var currentPlaylistId = playlistModel.playlistId
                            if (player.currentPlaylistId === currentPlaylistId) {
                                // 当前已在该播放列表，直接播放
                                player.playAt(index)
                            } else {
                                // 切换到该播放列表，需要更新队列
                                var queueItems = playlistModel.getAllItems()
                                player.setQueueSourceType(isFavoritePlaylist ? 2 : 1)
                                player.setPlayQueue(currentPlaylistId, queueItems)
                                player.playAt(index)
                            }
                        } else {
                            console.warn("Could not play music: filePath or player is null")
                        }
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    
                    onClicked: {
                        songContextMenu.musicHash = model.musicHash || ""
                        songContextMenu.musicFilePath = model.filePath || ""
                        songContextMenu.musicTitle = model.title || ""
                        songContextMenu.popup()
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 8
                visible: musicListView.contentHeight > musicListView.height
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("歌单为空\n添加歌曲到歌单")
                visible: musicListView.count === 0
                horizontalAlignment: Text.AlignHCenter
                color: Design.Colors.textDisabled
                font.pixelSize: 18
                wrapMode: Text.WordWrap
            }
        }
    }
    
    SongContextMenu {
        id: songContextMenu
        showRemoveFromPlaylist: true
    }
}
