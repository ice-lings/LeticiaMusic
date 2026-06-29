import QtQuick
import QtQuick.Layouts
import QtQuick.Controls 6.7
import QtQuick.Dialogs

Rectangle  {
    id: sideNavigation
    color: Design.Colors.surface
    implicitWidth: 200
    implicitHeight: 400

    property var playlistModel: appContext.playlistViewModel
    property int currentPlaylistId: -1
    
    property ListModel collectionListModel: ListModel {}
    
    function loadCollections() {
        collectionListModel.clear()
        var list = appContext ? appContext.playlistViewModel.getCollectionList() : []
        for (var i = 0; i < list.length; i++) {
            collectionListModel.append(list[i])
        }
    }
    
    // 歌单重命名编辑状态管理
    property int editingPlaylistId: -1
    property string editingOriginalName: ""
    
    // 背景点击处理：点击空白区域完成编辑
    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: {
            if (sideNavigation.editingPlaylistId >= 0) {
                // 找到当前编辑的TextField并完成编辑
                for (var i = 0; i < playlistListView.count; i++) {
                    var item = playlistListView.itemAtIndex(i)
                    if (item && item.children[1] && item.children[1].children[1]) {
                        var row = item.children[1].children[1]
                        if (row.visible && row.children[0]) {
                            sideNavigation.finishEditing(row.children[0].text)
                            return
                        }
                    }
                }
                sideNavigation.cancelEditing()
            }
        }
    }
    
    function refreshPlaylists() {
        if (playlistModel && playlistModel.refresh)
            playlistModel.refresh()
    }
    
    // 监听歌单变化信号，自动刷新
    Connections {
        target: appContext ? appContext.playlistManager : null
        ignoreUnknownSignals: true
        
        function onMusicAddedToPlaylist(playlistId, musicHash) {
            // 延迟刷新确保数据库更新完成
            Qt.callLater(function() {
                refreshPlaylists()
            })
        }
        
        function onMusicRemovedFromPlaylist(playlistId, musicHash) {
            Qt.callLater(function() {
                refreshPlaylists()
            })
        }
    }
    
    // 监听收藏夹导入完成
    Connections {
        target: appContext
        function onCollectionImported() {
            Qt.callLater(function() {
                loadCollections()
            })
        }
    }
    
    // 开始编辑歌单名称
    function startEditing(playlistId, currentName) {
        editingPlaylistId = playlistId
        editingOriginalName = currentName
    }
    
    // 完成编辑并保存
    function finishEditing(newName) {
        // 防止重复调用
        if (editingPlaylistId < 0) return
        
        if (newName.trim() !== "" && newName !== editingOriginalName) {
            appContext.playlistViewModel.renamePlaylist(editingPlaylistId, newName.trim())
            refreshPlaylists()
        }
        cancelEditing()
    }
    
    // 取消编辑
    function cancelEditing() {
        editingPlaylistId = -1
        editingOriginalName = ""
    }

    signal playlistSelected(int playlistId)
    signal favoriteSelected()
    signal localMusicSelected()

    // 自定义黑色主题右键菜单
    PlaylistContextMenu {
        id: playlistContextMenu
        onRenameClicked: {
            sideNavigation.startEditing(playlistContextMenu.playlistId, 
                                         playlistContextMenu.playlistName)
        }
        onDeleteClicked: {
            if (playlistContextMenu.playlistId < 0) return
            
            appContext.playlistViewModel.deletePlaylist(playlistContextMenu.playlistId)
            refreshPlaylists()
            
            if (playlistContextMenu.playlistId === sideNavigation.currentPlaylistId) {
                sideNavigation.currentPlaylistId = -1
                appContext.playlistDetailViewModel.clear()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5

        // 本地音乐
        Item {
            Layout.fillWidth: true
            implicitHeight: 36
            
            Rectangle {
                id: localMusicBg
                anchors.fill: parent
                radius: 6
                color: sideNavigation.currentPlaylistId === -1 ? "#404050" : "transparent"
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                spacing: 8

                Item {
                    width: 25
                    height: 25
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    color: sideNavigation.currentPlaylistId === -1 ? "#ff4757" : "white"
                    text: qsTr("本地音乐")
                    font.pixelSize: 14
                }
                Item { Layout.fillWidth: true }
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    currentPlaylistId = -1
                    localMusicSelected()
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3a4a"
            Layout.topMargin: 10
            Layout.bottomMargin: 10
        }

        // 我喜欢的音乐
        Item {
            Layout.fillWidth: true
            implicitHeight: 36
            
            Rectangle {
                id: favoriteMusicBg
                anchors.fill: parent
                radius: 6
                color: sideNavigation.currentPlaylistId === 1 ? "#404050" : "transparent"
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                spacing: 8

                Item {
                    width: 25
                    height: 25
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    color: sideNavigation.currentPlaylistId === 1 ? "#ff4757" : "white"
                    text: qsTr("我喜欢的音乐")
                    font.pixelSize: 14
                }
                Item { Layout.fillWidth: true }
            }
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var favPlaylistId = appContext.playlistManager.getFavoritePlaylistId()
                    currentPlaylistId = favPlaylistId
                    favoriteSelected()
                    appContext.playlistDetailViewModel.loadPlaylist(favPlaylistId)
                    console.log("Clicked Faverite Music, playlistId:", favPlaylistId)
                }
            }
        }

        // 歌单列表标题
        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            Layout.topMargin: 15

            Text {
                color: "#888888"
                text: qsTr("我的歌单")
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }
            
            // 添加歌单按钮 (+)
            Item {
                width: 20
                height: 20
                
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    color: addPlaylistMouseArea.containsMouse ? "#aaaaaa" : "#888888"
                    font.pixelSize: 16
                    font.bold: true
                }
                
                MouseArea {
                    id: addPlaylistMouseArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var name = "新建歌单"
                        var result = appContext.playlistViewModel.createPlaylist(name, "")
                        console.log("Create playlist result:", result)
                        playlistModel.refresh()
                    }
                }
            }
        }

        // 歌单列表
        ListView {
            id: playlistListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 200
            clip: true
            model: playlistModel
            delegate: Rectangle {
                width: playlistListView.width
                height: 36
                property bool isSelected: model.id === sideNavigation.currentPlaylistId
                property bool isEditing: model.id === sideNavigation.editingPlaylistId
                color: isSelected ? "#4DFFFFFF" : (mouseArea.containsMouse ? "#2d2d37" : "transparent")
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Item {
                        width: 20
                        height: 20
                        // 正常显示时显示图标，编辑时隐藏
                        Text {
                            anchors.centerIn: parent
                            text: "▶"
                            color: isSelected ? "#1a1a27" : "white"
                            font.pixelSize: 10
                            visible: !isEditing
                        }
                    }

                    // 正常显示模式：Text
                    Text {
                        id: playlistNameText
                        Layout.fillWidth: true
                        color: isSelected ? "#1a1a27" : "white"
                        text: model.name
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        visible: !isEditing
                    }

                    // 编辑模式：TextField + ✓按钮
                    Row {
                        id: editingRow
                        Layout.fillWidth: true
                        spacing: 8
                        visible: isEditing

                        TextField {
                            id: editField
                            width: parent.width - confirmButton.width - 8
                            text: sideNavigation.editingOriginalName
                            color: "#1a1a27"
                            font.pixelSize: 13

                            background: Rectangle {
                                color: "white"
                                border.color: "#4a90e2"
                                border.width: 1
                                radius: 4
                            }

                            // 当变为可见时自动获取焦点
                            onVisibleChanged: {
                                if (visible) {
                                    forceActiveFocus()
                                    selectAll()
                                }
                            }

                            // 失去焦点自动保存（使用Timer延迟，避免重复触发）
                            onActiveFocusChanged: {
                                if (!activeFocus && sideNavigation.editingPlaylistId === model.id) {
                                    // 延迟执行，避免与按钮点击冲突
                                    Qt.callLater(function() {
                                        if (sideNavigation.editingPlaylistId === model.id) {
                                            sideNavigation.finishEditing(text)
                                        }
                                    })
                                }
                            }

                            // Enter保存，Esc取消
                            Keys.onReturnPressed: sideNavigation.finishEditing(text)
                            Keys.onEnterPressed: sideNavigation.finishEditing(text)
                            Keys.onEscapePressed: sideNavigation.cancelEditing()
                        }

                        // 确认按钮 ✓
                        Text {
                            id: confirmButton
                            text: "✓"
                            color: "#2ecc71"
                            font.pixelSize: 16
                            font.bold: true

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    sideNavigation.finishEditing(editField.text)
                                }
        }
    }
    
    Connections {
        target: appContext
        function onCollectionImported() {
            Qt.callLater(function() {
                loadCollections()
            })
        }
    }
                    }

                    Text {
                        color: "#666666"
                        text: model.musicCount
                        font.pixelSize: 11
                        visible: !isEditing
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: isEditing ? Qt.ArrowCursor : Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    // 编辑时禁用MouseArea，让事件传递给子元素（TextField）
                    enabled: !isEditing
                    
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            // 我喜欢的音乐（isFavorite=true）不能重命名
                            if (model.isFavorite) return

                            playlistContextMenu.playlistId = model.id
                            playlistContextMenu.playlistName = model.name
                            var p = mouseArea.mapToItem(sideNavigation, mouse.x, mouse.y)
                            playlistContextMenu.x = p.x
                            playlistContextMenu.y = p.y
                            playlistContextMenu.open()
                            return
                        }

                        currentPlaylistId = model.id
                        playlistSelected(model.id)
                        appContext.playlistDetailViewModel.loadPlaylist(model.id)
                    }
                }
            }
        }

        // 收藏夹列表标题
        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            Layout.topMargin: 15

            Text {
                color: "#888888"
                text: qsTr("我的收藏夹")
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }
            
            Item {
                width: 20
                height: 20
                
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    color: importCollectionMouseArea.containsMouse ? "#aaaaaa" : "#888888"
                    font.pixelSize: 16
                    font.bold: true
                }
                
                MouseArea {
                    id: importCollectionMouseArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        collectionFolderDialog.open()
                    }
                }
            }
        }

        // 收藏夹列表
        ListView {
            id: collectionListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 100
            clip: true
            model: sideNavigation.collectionListModel
            delegate: Rectangle {
                width: collectionListView.width
                height: 36
                property bool isSelected: model.id === sideNavigation.currentPlaylistId
                color: isSelected ? "#4DFFFFFF" : (collMouseArea.containsMouse ? "#2d2d37" : "transparent")
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Text {
                        text: "📁"
                        color: isSelected ? "#1a1a27" : "white"
                        font.pixelSize: 12
                    }

                    Text {
                        Layout.fillWidth: true
                        color: isSelected ? "#1a1a27" : "white"
                        text: model.name
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }

                    Text {
                        color: "#666666"
                        text: model.musicCount
                        font.pixelSize: 11
                    }
                }

                MouseArea {
                    id: collMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        sideNavigation.currentPlaylistId = model.id
                        sideNavigation.playlistSelected(model.id)
                        appContext.playlistDetailViewModel.loadPlaylist(model.id)
                    }
                }
            }
        }

    }

    // 收藏夹导入文件夹对话框
    FolderDialog {
        id: collectionFolderDialog
        title: qsTr("选择要导入的文件夹")
        
        onAccepted: {
            var folder = collectionFolderDialog.selectedFolder.toString()
            if (folder.startsWith("file:///")) {
                folder = folder.substring(8)
            } else if (folder.startsWith("file://")) {
                folder = folder.substring(7)
            }
            if (folder.indexOf("/") !== -1) {
                folder = folder.replace(/\//g, "\\")
            }
            if (folder.endsWith("\\")) {
                folder = folder.substring(0, folder.length - 1)
            }
            
            var parts = folder.split("\\")
            var folderName = parts[parts.length - 1]
            
            appContext.importCollectionFolder(folder, folderName)
            Qt.callLater(function() {
                loadCollections()
            })
        }
    }
    
    Component.onCompleted: {
        loadCollections()
    }

}
