import QtQuick
import QtQuick.Controls
import QtQuick.Window

import "../component"

/* 收藏页面 */
Item {
    id: page

    readonly property real s: Design.Responsive.guiScale


    Rectangle { anchors.fill: parent; color: Design.Colors.background }

    Rectangle {
        id: topBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 50 * s
        color: Design.Colors.surface

        Row {
            anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "收藏"
                font.pixelSize: 20 * s
                font.bold: true
                color: Design.Colors.textPrimary
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                x: parent.width - width - 16 * s
                text: qsTr("共 %1 首").arg(listView.count)
                font.pixelSize: 13 * s
                color: Design.Colors.textSecondary
                visible: listView.count > 0
            }
        }
    }

    ListView {
        id: listView
        anchors { top: topBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        clip: true
        model: ListModel { id: favModel }
        spacing: 0
        reuseItems: true

        Label {
            anchors.centerIn: parent
            text: "点击歌曲右侧 ♥ 添加到收藏"
            color: Design.Colors.textSecondary
            font.pixelSize: 15 * s
            visible: listView.count === 0
            horizontalAlignment: Text.AlignHCenter
        }

        delegate: MusicListItem {
            musicTitle: title || "未知歌曲"; musicArtist: artist || "佚名"; musicDuration: duration || ""
            coverSource: cover || ""; musicHash: hash || ""; musicFilePath: filePath || ""; isFavorite: true

            onClicked: {
                if (musicFilePath && page.player) { var queue = page.buildQueue(); page.player.setQueueSourceType(2); page.player.setPlayQueue(0, queue); page.player.playAt(index) }
            }

            onShowActions: {
                favActionSheet.open({
                    musicHash: hash || "",
                    musicFilePath: filePath || "",
                    musicTitle: title || "",
                    musicArtist: artist || ""
                })
            }
        }
    }

    property var player: appContext.playerController
    property int favPlaylistId: -1

    function buildQueue() {
        var queue = []
        for (var i = 0; i < favModel.count; i++) {
            var item = favModel.get(i)
            queue.push({ id: item.hash, filePath: item.filePath, title: item.title, artist: item.artist })
        }
        return queue
    }

    function refreshFavorites() {
        favModel.clear()
        if (!appContext || !appContext.playlistManager) return
        if (favPlaylistId < 0) favPlaylistId = appContext.playlistManager.getFavoritePlaylistId()
        if (favPlaylistId < 0) return
        var items = appContext.playlistManager.getMusicItems(favPlaylistId)
        if (!items || items.length === 0) return
        for (var i = 0; i < items.length; i++) {
            var item = items[i]
            favModel.append({
                title: item.title,
                artist: item.artist,
                duration: item.duration,
                cover: item.coverPath,
                hash: item.musicHash,
                filePath: item.filePath
            })
        }
    }

    Component.onCompleted: refreshFavorites()

    Connections {
        target: appContext.favoriteManager
        function onFavoriteAdded() { console.log("[FavPage] favoriteAdded"); refreshFavorites() }
        function onFavoriteRemoved() { console.log("[FavPage] favoriteRemoved"); refreshFavorites() }
    }

    Connections {
        target: page
        function onVisibleChanged() {
            if (page.visible) refreshFavorites()
        }
    }

    MusicActionSheet {
        id: favActionSheet
        onEditMetadata: {
            favMetadataSheet.open({
                musicHash: favActionSheet.musicHash,
                musicTitle: favActionSheet.musicTitle,
                musicArtist: favActionSheet.musicArtist
            })
        }
    }

    MetadataEditSheet {
        id: favMetadataSheet
    }
}
