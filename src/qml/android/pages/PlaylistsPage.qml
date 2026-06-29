import QtQuick
import QtQuick.Controls
import QtQuick.Window

import "../component"

/* 歌单页面 */
Item {
    id: page

    readonly property real s: Design.Responsive.guiScale


    // 0 = 列表模式, >0 = 歌单详情模式
    property int viewingPlaylistId: 0

    property string detailName: ""

    // 当前标签: 0=歌单, 1=收藏歌单
    property int currentTabIndex: 0
    property int previousTabIndex: 0

    function showPlaylistDetail(id, name) {
        previousTabIndex = currentTabIndex
        viewingPlaylistId = id
        detailName = name
        appContext.playlistDetailViewModel.loadPlaylist(id)
    }

    function backToList() {
        viewingPlaylistId = 0
        detailName = ""
        currentTabIndex = previousTabIndex
        mainSwipeView.setCurrentIndex(previousTabIndex)
    }

    Rectangle { anchors.fill: parent; color: Design.Colors.background }

    // ==================== 歌单列表区 ====================
    Item {
        anchors.fill: parent
        visible: viewingPlaylistId === 0

        // 顶部 Tab 栏
        Rectangle {
            id: tabBar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 50 * s
            color: Design.Colors.surface

            Row {
                anchors { left: parent.left; leftMargin: 16 * s; verticalCenter: parent.verticalCenter }
                spacing: 20 * s

                // Tab 1: 歌单
                Rectangle {
                    width: tabLabel1.implicitWidth + 12 * s
                    height: 30 * s
                    radius: 15 * s
                    color: currentTabIndex === 0 ? Design.Colors.primary : "transparent"

                    Text {
                        id: tabLabel1
                        anchors.centerIn: parent
                        text: "歌单"
                        font.pixelSize: 15 * s
                        font.weight: currentTabIndex === 0 ? Font.Bold : Font.Normal
                        color: currentTabIndex === 0 ? Design.Colors.textPrimary : Design.Colors.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -4 * s
                        onClicked: { currentTabIndex = 0; mainSwipeView.setCurrentIndex(0) }
                    }
                }

                // Tab 2: 收藏歌单
                Rectangle {
                    width: tabLabel2.implicitWidth + 12 * s
                    height: 30 * s
                    radius: 15 * s
                    color: currentTabIndex === 1 ? Design.Colors.primary : "transparent"

                    Text {
                        id: tabLabel2
                        anchors.centerIn: parent
                        text: "收藏歌单"
                        font.pixelSize: 15 * s
                        font.weight: currentTabIndex === 1 ? Font.Bold : Font.Normal
                        color: currentTabIndex === 1 ? Design.Colors.textPrimary : Design.Colors.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -4 * s
                        onClicked: { currentTabIndex = 1; mainSwipeView.setCurrentIndex(1) }
                    }
                }
            }

            // "+" 按钮
            Rectangle {
                anchors { right: parent.right; rightMargin: 12 * s; verticalCenter: parent.verticalCenter }
                width: 32 * s
                height: 32 * s
                radius: 16 * s
                color: "#15ffffff"

                Image {
                    anchors.centerIn: parent
                    width: 20 * s; height: 20 * s
                    source: "qrc:/img/plus.svg"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6 * s
                    onClicked: {
                        if (currentTabIndex === 0) {
                            appContext.playlistViewModel.createPlaylist("新建歌单", "")
                            appContext.playlistViewModel.refresh()
                        } else {
                            appContext.collectionPlaylistViewModel.createPlaylist("新建收藏歌单")
                        }
                    }
                }
            }
        }

        // SwipeView
        SwipeView {
            id: mainSwipeView
            anchors { top: tabBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            interactive: true

            onCurrentIndexChanged: {
                if (currentIndex !== currentTabIndex) {
                    currentTabIndex = currentIndex
                }
            }

            // 页面0: 普通歌单
            Item {
                id: normalListPage
                ListView {
                    id: playlistListView
                    anchors.fill: parent
                    clip: true
                    model: appContext.playlistViewModel
                    spacing: 0
                    reuseItems: true

                    Label {
                        anchors.centerIn: parent
                        text: "点击 + 创建歌单"
                        color: Design.Colors.textSecondary
                        font.pixelSize: 15 * s
                        visible: playlistListView.count === 0
                    }

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 64 * s
                        color: "transparent"

                        Row {
                            anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s }
                            spacing: 14 * s

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 42 * s
                                height: 42 * s
                                radius: 8 * s
                                color: Design.Colors.listItemHovered

                                Image {
                                    anchors.centerIn: parent
                                    width: 22 * s; height: 22 * s
                                    source: "qrc:/img/playlist.svg"
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 56 * s - 40 * s
                                spacing: 3 * s

                                Text {
                                    text: name || "未命名歌单"
                                    font.pixelSize: 15 * s
                                    font.weight: Font.Medium
                                    color: Design.Colors.textPrimary
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                    width: parent.width
                                }

                                Text {
                                    text: qsTr("%1 首").arg(musicCount || 0)
                                    font.pixelSize: 12 * s
                                    color: Design.Colors.textSecondary
                                    visible: (musicCount || 0) > 0
                                }
                            }
                        }

                        Rectangle {
                            anchors { left: parent.left; leftMargin: 72 * s; right: parent.right; bottom: parent.bottom }
                            height: 0.5
                            color: Design.Colors.divider
                        }

                        MouseArea {
                            anchors.fill: parent
                            anchors.rightMargin: 40 * s
                            onClicked: page.showPlaylistDetail(id, name)
                        }

                        Image {
                            anchors { right: parent.right; rightMargin: 12 * s; verticalCenter: parent.verticalCenter }
                            width: 18 * s; height: 18 * s
                            source: "qrc:/img/more-vertical.svg"
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -8 * s
                                onClicked: page.showPlaylistMenu(id, name)
                            }
                        }
                    }
                }
            }

            // 页面1: 收藏歌单
            Item {
                id: collectionListPage
                ListView {
                    id: collectionListView
                    anchors.fill: parent
                    clip: true
                    model: appContext.collectionPlaylistViewModel
                    spacing: 0
                    reuseItems: true

                    Label {
                        anchors.centerIn: parent
                        text: "点击 + 创建收藏歌单\n导入本地文件夹成为收藏歌单"
                        color: Design.Colors.textSecondary
                        font.pixelSize: 14 * s
                        visible: collectionListView.count === 0
                        horizontalAlignment: Text.AlignHCenter
                        lineHeight: 1.5
                    }

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 64 * s
                        color: "transparent"

                        Row {
                            anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s }
                            spacing: 14 * s

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 42 * s
                                height: 42 * s
                                radius: 8 * s
                                color: "#19ff9500"

                                Image {
                                    anchors.centerIn: parent
                                    width: 20 * s; height: 20 * s
                                    source: "qrc:/img/playlist.svg"
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 56 * s - 40 * s
                                spacing: 3 * s

                                Text {
                                    text: name || "未命名收藏"
                                    font.pixelSize: 15 * s
                                    font.weight: Font.Medium
                                    color: Design.Colors.textPrimary
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                    width: parent.width
                                }

                                Text {
                                    text: qsTr("%1 首").arg(musicCount || 0)
                                    font.pixelSize: 12 * s
                                    color: Design.Colors.textSecondary
                                    visible: (musicCount || 0) > 0
                                }
                            }
                        }

                        Rectangle {
                            anchors { left: parent.left; leftMargin: 72 * s; right: parent.right; bottom: parent.bottom }
                            height: 0.5
                            color: Design.Colors.divider
                        }

                        MouseArea {
                            anchors.fill: parent
                            anchors.rightMargin: 40 * s
                            onClicked: page.showPlaylistDetail(id, name)
                        }

                        Image {
                            anchors { right: parent.right; rightMargin: 12 * s; verticalCenter: parent.verticalCenter }
                            width: 18 * s; height: 18 * s
                            source: "qrc:/img/more-vertical.svg"
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -8 * s
                                onClicked: page.showPlaylistMenu(id, name)
                            }
                        }
                    }
                }
            }
        }
    }

    // ==================== 歌单详情 ====================
    Item {
        anchors.fill: parent
        visible: viewingPlaylistId > 0

        // 顶部栏
        Rectangle {
            id: detailTopBar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 50 * s
            color: Design.Colors.surface

            // 返回
            Text {
                anchors { left: parent.left; leftMargin: 12 * s; verticalCenter: parent.verticalCenter }
                text: "← 返回"
                font.pixelSize: 15 * s
                color: Design.Colors.primary

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8 * s
                    onClicked: page.backToList()
                }
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width * 0.5

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: detailName || "歌单"
                    font.pixelSize: 17 * s
                    font.weight: Font.Bold
                    color: Design.Colors.textPrimary
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("%1 首").arg(detailListView.count)
                    font.pixelSize: 12 * s
                    color: Design.Colors.textSecondary
                    visible: detailListView.count > 0
                }
            }

            // ⋮ 按钮
            Image {
                anchors { right: parent.right; rightMargin: 16 * s; verticalCenter: parent.verticalCenter }
                width: 18 * s; height: 18 * s
                source: "qrc:/img/more-vertical.svg"
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8 * s
                    onClicked: page.showPlaylistMenu(viewingPlaylistId, detailName)
                }
            }
        }

        // 歌曲列表
        ListView {
            id: detailListView
            anchors { top: detailTopBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            model: appContext.playlistDetailViewModel
            spacing: 0
            reuseItems: true

            Label {
                anchors.centerIn: parent
                text: "歌单为空\n从本地音乐中添加歌曲"
                color: Design.Colors.textSecondary
                font.pixelSize: 14 * s
                visible: detailListView.count === 0
                horizontalAlignment: Text.AlignHCenter
                lineHeight: 1.5
            }

            delegate: MusicListItem {
                musicTitle: title || "未知歌曲"
                musicArtist: artist || "佚名"
                musicDuration: duration || ""
                coverSource: coverPath || ""
                musicHash: musicHash || ""
                musicFilePath: filePath || ""

                onClicked: {
                    if (musicFilePath && player) {
                        var q = getCurrentQueue()
                        player.setQueueSourceType(1)
                        player.setPlayQueue(viewingPlaylistId, q)
                        player.playAt(index)
                    }
                }

                onShowActions: {
                    detailActionSheet.open({
                        musicHash: musicHash || "",
                        musicFilePath: musicFilePath || "",
                        musicTitle: musicTitle || "",
                        musicArtist: musicArtist || ""
                    })
                }
            }
        }
    }

    // 获取当前歌单队列
    function getCurrentQueue() {
        var q = []
        var count = appContext.playlistDetailViewModel.rowCount()
        for (var i = 0; i < count; i++) {
            var idx = appContext.playlistDetailViewModel.index(i, 0)
            q.push({
                id: appContext.playlistDetailViewModel.data(idx, 0x0101),
                filePath: appContext.playlistDetailViewModel.data(idx, 0x0106),
                title: appContext.playlistDetailViewModel.data(idx, 0x0102),
                artist: appContext.playlistDetailViewModel.data(idx, 0x0103)
            })
        }
        return q
    }

    // ===== 歌单操作弹窗 =====
    function showPlaylistMenu(pid, pname) {
        menuPlaylistId = pid
        menuPlaylistName = pname
        menuSheet.visible = true
    }

    property int menuPlaylistId: -1
    property string menuPlaylistName: ""

    Rectangle {
        id: menuSheet
        anchors.fill: parent
        z: 1000
        visible: false
        color: "#80000000"

        MouseArea { anchors.fill: parent; onClicked: menuSheet.visible = false }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 180 * s
            radius: 16 * s
            color: Design.Colors.surface

            Rectangle {
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
                height: 16 * s
                color: parent.color
            }

            Column {
                anchors { fill: parent; leftMargin: 20 * s; rightMargin: 20 * s; topMargin: 20 * s }
                spacing: 0

                Text { text: menuPlaylistName || ""; font.pixelSize: 16 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }

                Item { width: 1; height: 16 * s }

                // 重命名
                Rectangle {
                    width: parent.width; height: 46 * s; color: "transparent"
                    Text {
                        anchors {
                            left: parent.left
                            verticalCenter: parent.verticalCenter
                        }
                        text: "重命名"
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                    }
                    MouseArea { anchors.fill: parent; onClicked: { menuSheet.visible = false; renameDialog.visible = true } }
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    height: 0.5
                    color: Design.Colors.divider
                }

                // 删除
                Rectangle {
                    width: parent.width; height: 46 * s; color: "transparent"
                    Text {
                        anchors {
                            left: parent.left
                            verticalCenter: parent.verticalCenter
                        }
                        text: "删除"
                        font.pixelSize: 15 * s
                        color: Design.Colors.error
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            menuSheet.visible = false
                            confirmDeletePlaylist.visible = true
                        }
                    }
                }
            }
        }
    }

    // ===== 重命名弹窗 =====
    Rectangle {
        id: renameDialog
        anchors.fill: parent
        z: 1001
        visible: false
        color: "#80000000"

        MouseArea { anchors.fill: parent; onClicked: renameDialog.visible = false }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.8
            height: 180 * s
            radius: 14 * s
            color: Design.Colors.surface

            Column {
                anchors { fill: parent; leftMargin: 20 * s; rightMargin: 20 * s; topMargin: 20 * s }
                spacing: 16 * s

                Text { text: "重命名歌单"; font.pixelSize: 16 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }

                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 8 * s
                    color: "#14ffffff"

                    TextInput {
                        id: renameField
                        anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        text: menuPlaylistName
                        clip: true
                    }
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 20 * s

                    Rectangle {
                        width: 90 * s; height: 38 * s; radius: 19 * s; color: Design.Colors.divider
                        Text { anchors.centerIn: parent; text: "取消"; font.pixelSize: 14 * s; color: Design.Colors.textPrimary }
                        MouseArea { anchors.fill: parent; onClicked: renameDialog.visible = false }
                    }

                    Rectangle {
                        width: 90 * s; height: 38 * s; radius: 19 * s; color: Design.Colors.primary
                        Text { anchors.centerIn: parent; text: "确认"; font.pixelSize: 14 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                var newName = renameField.text.trim()
                                if (newName && menuPlaylistId > 0) {
                                    appContext.playlistViewModel.renamePlaylist(menuPlaylistId, newName)
                                    appContext.playlistViewModel.refresh()
                                    if (detailName === menuPlaylistName) detailName = newName
                                }
                                renameDialog.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    // ===== 删除确认弹窗 =====
    Rectangle {
        id: confirmDeletePlaylist
        anchors.fill: parent
        z: 1001
        visible: false
        color: "#80000000"

        MouseArea { anchors.fill: parent; onClicked: confirmDeletePlaylist.visible = false }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.75
            height: 160 * s
            radius: 14 * s
            color: Design.Colors.surface

            Column {
                anchors { fill: parent; leftMargin: 20 * s; rightMargin: 20 * s; topMargin: 20 * s }
                spacing: 16 * s

                Text { text: qsTr("删除歌单「%1」?").arg(menuPlaylistName); font.pixelSize: 15 * s; color: Design.Colors.textPrimary; width: parent.width; wrapMode: Text.Wrap }
                Text { text: "歌单中的歌曲不会被删除"; font.pixelSize: 13 * s; color: Design.Colors.textSecondary }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 20 * s

                    Rectangle {
                        width: 90 * s; height: 38 * s; radius: 19 * s; color: Design.Colors.divider
                        Text { anchors.centerIn: parent; text: "取消"; font.pixelSize: 14 * s; color: Design.Colors.textPrimary }
                        MouseArea { anchors.fill: parent; onClicked: confirmDeletePlaylist.visible = false }
                    }

                    Rectangle {
                        width: 90 * s; height: 38 * s; radius: 19 * s; color: Design.Colors.error
                        Text { anchors.centerIn: parent; text: "删除"; font.pixelSize: 14 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (menuPlaylistId > 0) {
                                    appContext.playlistViewModel.deletePlaylist(menuPlaylistId)
                                    appContext.playlistViewModel.refresh()
                                    if (viewingPlaylistId === menuPlaylistId) backToList()
                                }
                                confirmDeletePlaylist.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    // ===== 歌单内歌曲操作面板 =====
    MusicActionSheet {
        id: detailActionSheet
        onEditMetadata: {
            detailMetadataSheet.open({
                musicHash: detailActionSheet.musicHash,
                musicTitle: detailActionSheet.musicTitle,
                musicArtist: detailActionSheet.musicArtist
            })
        }
    }

    MetadataEditSheet {
        id: detailMetadataSheet
    }

    property var player: appContext.playerController

    // 添加歌曲后刷新
    Connections {
        target: appContext.playlistManager
        function onMusicAddedToPlaylist(playlistId, musicHash) {
            appContext.playlistViewModel.refresh()
            appContext.collectionPlaylistViewModel.refresh()
            if (viewingPlaylistId === playlistId) {
                appContext.playlistDetailViewModel.loadPlaylist(playlistId)
            }
        }
    }

    // 删除歌曲后刷新
    Connections {
        target: appContext.playlistManager
        function onMusicRemovedFromPlaylist(playlistId, musicHash) {
            appContext.playlistViewModel.refresh()
            appContext.collectionPlaylistViewModel.refresh()
            if (viewingPlaylistId === playlistId) {
                appContext.playlistDetailViewModel.loadPlaylist(playlistId)
            }
        }
    }

    // 从其他页面切回时刷新
    Connections {
        target: page
        function onVisibleChanged() {
            if (page.visible) {
                appContext.playlistViewModel.refresh()
                appContext.collectionPlaylistViewModel.refresh()
                if (viewingPlaylistId > 0) {
                    appContext.playlistDetailViewModel.loadPlaylist(viewingPlaylistId)
                }
            }
        }
    }
}
