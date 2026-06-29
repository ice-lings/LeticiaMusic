import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Menu {
    id: root
    
    // 输入属性
    property string musicHash: ""
    property string musicFilePath: ""
    property string musicTitle: ""
    property bool showRemoveFromPlaylist: false
    
    // 删除确认弹窗
    Component {
        id: deleteConfirmDialog
        Popup {
            id: confirmPopup
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape
            width: 360
            height: contentCol.implicitHeight + 48

            property string targetTitle: ""
            property bool deleteFile: false

            background: Rectangle {
                color: Design.Colors.surface
                border.color: Design.Colors.divider
                border.width: 1
                radius: 8
            }

            // 半透明遮罩
            Overlay.modal: Rectangle {
                color: Design.Colors.withAlpha("#000000", 0.5)
            }

            Column {
                id: contentCol
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: 20
                }
                spacing: 0

                // 标题行
                Text {
                    text: qsTr("删除歌曲")
                    color: Design.Colors.textPrimary
                    font.pixelSize: 15
                    font.bold: true
                }

                Item { width: 1; height: 14 }

                // 分隔线
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Design.Colors.divider
                }

                Item { width: 1; height: 14 }

                // 正文
                Text {
                    width: parent.width
                    text: qsTr("是否删除《%1》？").arg(confirmPopup.targetTitle)
                    color: Design.Colors.textPrimary
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                }

                Item { width: 1; height: 12 }

                // 复选框
                Row {
                    spacing: 8

                    Rectangle {
                        id: checkBoxRect
                        width: 16
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 3
                        color: confirmPopup.deleteFile
                               ? Design.Colors.primary
                               : "transparent"
                        border.color: confirmPopup.deleteFile
                                      ? Design.Colors.primary
                                      : Design.Colors.textSecondary
                        border.width: 1.5

                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: Design.Colors.textPrimary
                            font.pixelSize: 11
                            font.bold: true
                            visible: confirmPopup.deleteFile
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: confirmPopup.deleteFile = !confirmPopup.deleteFile
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("同时删除本地音乐文件")
                        color: Design.Colors.textSecondary
                        font.pixelSize: 13

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: confirmPopup.deleteFile = !confirmPopup.deleteFile
                        }
                    }
                }

                Item { width: 1; height: 18 }

                // 按钮行
                Row {
                    anchors.right: parent.right
                    spacing: 10

                    // 取消按钮
                    Rectangle {
                        width: 80
                        height: 32
                        radius: 5
                        color: cancelBtn.containsMouse
                               ? Design.Colors.buttonHovered
                               : Design.Colors.buttonNormal
                        border.color: Design.Colors.divider
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("取消")
                            color: Design.Colors.textPrimary
                            font.pixelSize: 13
                        }

                        MouseArea {
                            id: cancelBtn
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: confirmPopup.close()
                        }
                    }

                    // 确认按钮
                    Rectangle {
                        width: 80
                        height: 32
                        radius: 5
                        color: confirmBtn.containsMouse
                               ? Design.Colors.withAlpha(Design.Colors.error, 0.85)
                               : Design.Colors.error

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("删除")
                            color: Design.Colors.textPrimary
                            font.pixelSize: 13
                            font.bold: true
                        }

                        MouseArea {
                            id: confirmBtn
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var success = appContext.deleteMusic(root.musicHash, confirmPopup.deleteFile)
                                if (success) {
                                    globalToast.showSuccess(qsTr("已删除歌曲：%1").arg(confirmPopup.targetTitle))
                                } else {
                                    globalToast.showError(qsTr("删除歌曲失败"))
                                }
                                confirmPopup.close()
                            }
                        }
                    }
                }

                Item { width: 1; height: 4 }
            }
        }
    }
    
    Component {
        id: metadataEditorComponent
        MetadataEditorDialog {}
    }

    // 子菜单组件
    Component {
        id: subMenuComponent
        Popup {
            id: subMenuPopup
            
            property string musicHash: ""
            
            width: 200
            height: 180
            
            modal: false
            focus: false
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            
            // 使用 fixed parent instead of binding to avoid binding loop
            y: parent ? parent.y + addToPlaylistItem.y + 40 : 0
            x: parent ? parent.x + addToPlaylistItem.width + 5 : 0
            
            background: Rectangle {
                color: Design.Colors.surface
                border.color: Design.Colors.divider
                border.width: 1
                radius: 6
            }
            
            ColumnLayout {
                id: subContentLayout
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2
                
                Text {
                    text: qsTr("选择歌单")
                    color: Design.Colors.textSecondary
                    font.pixelSize: 12
                    Layout.leftMargin: 8
                    Layout.topMargin: 4
                }
                
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Design.Colors.divider
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                }
                
                ListView {
                    id: playlistListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: appContext ? appContext.playlistViewModel : null
                    spacing: 2
                    
                    Component.onCompleted: {
                        console.log("[SongContextMenu] ListView completed, count:", count)
                    }
                    
                    onCountChanged: {
                        console.log("[SongContextMenu] ListView count changed to:", count)
                    }
                    
                    delegate: Rectangle {
                        width: playlistListView.width
                        height: 36
                        radius: 4
                        color: subMouseArea.containsMouse ? 
                               Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                               "transparent"
                        
                        property var playlistData: model
                        
                        Component.onCompleted: {
                            console.log("[SongContextMenu] delegate created, name:", playlistData ? playlistData.name : "null")
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8
                            
                            Text {
                                Layout.fillWidth: true
                                text: playlistData ? playlistData.name : ""
                                color: Design.Colors.textPrimary
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            Text {
                                text: playlistData ? playlistData.musicCount : 0
                                color: Design.Colors.textSecondary
                                font.pixelSize: 11
                            }
                        }
                        
                        MouseArea {
                            id: subMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            
                            onClicked: {
                                if (!playlistData) return
                                
                                var items = appContext.playlistManager.getItems(playlistData.id)
                                var alreadyExists = false
                                
                                for (var i = 0; i < items.length; i++) {
                                    if (items[i].musicHash === subMenuPopup.musicHash) {
                                        alreadyExists = true
                                        break
                                    }
                                }
                                
                                if (alreadyExists) {
                                    globalToast.showWarning(qsTr("歌曲已在歌单：%1").arg(playlistData.name))
                                } else {
                                    var success = appContext.playlistManager.addMusicToPlaylist(
                                        playlistData.id, 
                                        subMenuPopup.musicHash
                                    )
                                    
                                    if (success) {
                                        globalToast.showSuccess(qsTr("已添加到歌单：%1").arg(playlistData.name))
                                        if (appContext && appContext.playlistViewModel) {
                                            appContext.playlistViewModel.refresh()
                                        }
                                    }
                                }
                                
                                subMenuPopup.close()
                                root.close()
                            }
                        }
                    }
                    
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("暂无自定义歌单")
                        color: Design.Colors.textSecondary
                        font.pixelSize: 12
                        visible: playlistListView.count === 0
                    }
                }
            }
        }
    }
    
    // 子菜单实例
    property var subMenuInstance: null
    
    // 菜单样式
    background: Rectangle {
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1
        radius: 6
    }
    
    padding: 6
    width: 180
    
    // 菜单项：添加到歌单
    MenuItem {
        id: addToPlaylistItem
        text: qsTr("添加到歌单")
        height: 36
        
        background: Rectangle {
            color: parent.hovered ? 
                   Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                   "transparent"
            radius: 4
        }
        
        contentItem: RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8
            
            Text {
                text: addToPlaylistItem.text
                color: Design.Colors.textPrimary
                font.pixelSize: 13
                Layout.fillWidth: true
            }
            
            Text {
                text: "▶"
                color: Design.Colors.textSecondary
                font.pixelSize: 10
            }
        }
        
        // 悬停时显示子菜单
        onHoveredChanged: {
            if (hovered) {
                // 延迟显示，避免快速移动时闪烁
                showSubMenuTimer.start()
            } else {
                showSubMenuTimer.stop()
            }
        }
    }
    
    // 菜单项：添加到本地音乐（仅收藏夹显示）
    MenuItem {
        id: addToLocalMusicItem
        text: qsTr("添加到本地音乐")
        visible: appContext && appContext.playlistDetailViewModel && appContext.playlistDetailViewModel.isCollection
        height: visible ? 36 : 0
        implicitHeight: height
        
        background: Rectangle {
            color: parent.hovered ? 
                   Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                   "transparent"
            radius: 4
        }
        
        contentItem: Text {
            text: parent.text
            color: Design.Colors.textPrimary
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }
        
        onTriggered: {
            if (root.musicHash) {
                appContext.addToLocalMusic(root.musicHash)
            }
            root.close()
        }
    }
    
    // 菜单项：从歌单移除
    MenuItem {
        id: removeFromPlaylistItem
        text: qsTr("从歌单移除")
        visible: showRemoveFromPlaylist
        height: showRemoveFromPlaylist ? 36 : 0
        implicitHeight: height
        
        background: Rectangle {
            color: parent.hovered ? 
                   Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                   "transparent"
            radius: 4
        }
        
        contentItem: Text {
            text: parent.text
            color: Design.Colors.textPrimary
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }
        
        onTriggered: {
            console.log("[SongContextMenu] removeFromPlaylist clicked, musicHash:", root.musicHash)
            console.log("[SongContextMenu] playlistDetailViewModel:", appContext.playlistDetailViewModel)
            console.log("[SongContextMenu] playlistId:", appContext.playlistDetailViewModel ? appContext.playlistDetailViewModel.playlistId : "null")
            if (appContext && appContext.playlistManager && root.musicHash) {
                var playlistId = appContext.playlistDetailViewModel ? appContext.playlistDetailViewModel.playlistId : -1
                console.log("[SongContextMenu] using playlistId:", playlistId)
                var success = appContext.playlistManager.removeMusicFromPlaylist(
                    playlistId,
                    root.musicHash
                )
                console.log("[SongContextMenu] remove result:", success)
                if (success) {
                    appContext.playlistDetailViewModel.refresh()
                }
            }
            root.close()
        }
    }
    
    // 菜单项：编辑元数据
    MenuItem {
        text: qsTr("编辑元数据")
        height: 36

        background: Rectangle {
            color: parent.hovered
                   ? Design.Colors.withAlpha(Design.Colors.primary, 0.2)
                   : "transparent"
            radius: 4
        }

        contentItem: Text {
            text: parent.text
            color: Design.Colors.textPrimary
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }

        onTriggered: {
            var parentItem = ApplicationWindow.window
                              ? ApplicationWindow.window.contentItem
                              : root.parent
            var dialog = metadataEditorComponent.createObject(parentItem)
            if (dialog) {
                dialog.musicHash = root.musicHash
                var overrides = appContext.getSongOverrides(root.musicHash)
                dialog.title  = (overrides && overrides.title)  ? overrides.title  : root.musicTitle
                dialog.artist = (overrides && overrides.artist) ? overrides.artist : ""
                dialog.album  = (overrides && overrides.album)  ? overrides.album  : ""
                dialog.genre  = (overrides && overrides.genre)  ? overrides.genre  : ""
                dialog.year   = (overrides && overrides.year)   ? overrides.year   : 0
                dialog.coverPath = appContext.getCoverPathByFileHash(root.musicHash)
                dialog.x = (parentItem.width - dialog.width) / 2
                dialog.y = (parentItem.height - dialog.height) / 2
                dialog.open()
            }
            root.close()
        }
    }

    // 菜单项：在文件夹中显示
    MenuItem {
        text: qsTr("在文件夹中显示")
        height: 36
        
        background: Rectangle {
            color: parent.hovered ? 
                   Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                   "transparent"
            radius: 4
        }
        
        contentItem: Text {
            text: parent.text
            color: Design.Colors.textPrimary
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }
        
        onTriggered: {
            if (root.musicFilePath) {
                var folderPath = root.musicFilePath.substring(0, root.musicFilePath.lastIndexOf("/"))
                Qt.openUrlExternally("file:///" + folderPath)
            }
            root.close()
        }
    }
    
    // 菜单项：删除
    MenuItem {
        text: qsTr("删除")
        height: 36
        
        background: Rectangle {
            color: parent.hovered ? 
                   Design.Colors.withAlpha(Design.Colors.error, 0.2) : 
                   "transparent"
            radius: 4
        }
        
        contentItem: Text {
            text: parent.text
            color: Design.Colors.error
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }
        
        onTriggered: {
            var parentItem = ApplicationWindow.window ? ApplicationWindow.window.contentItem : root.parent
            var dialogInstance = deleteConfirmDialog.createObject(parentItem)
            if (dialogInstance) {
                dialogInstance.targetTitle = root.musicTitle
                dialogInstance.x = (parentItem.width - dialogInstance.width) / 2
                dialogInstance.y = (parentItem.height - dialogInstance.height) / 2
                dialogInstance.open()
            }
            root.close()
        }
    }
    
    // 显示子菜单的定时器
    Timer {
        id: showSubMenuTimer
        interval: 100
        onTriggered: {
            showSubMenu()
        }
    }
    
    // 显示子菜单函数
    function showSubMenu() {
        // 关闭已有的子菜单
        if (subMenuInstance) {
            subMenuInstance.close()
            if (subMenuInstance instanceof QtObject && subMenuInstance !== null) {
                subMenuInstance.destroy()
            }
            subMenuInstance = null
        }
        
        console.log("[SongContextMenu] showSubMenu called")
        console.log("[SongContextMenu] appContext:", appContext)
        console.log("[SongContextMenu] playlistViewModel:", appContext ? appContext.playlistViewModel : "null")
        if (appContext && appContext.playlistViewModel) {
            console.log("[SongContextMenu] rowCount:", appContext.playlistViewModel.rowCount())
        }
        
        // 创建新的子菜单
        var parentItem = ApplicationWindow.window ? ApplicationWindow.window.contentItem : root.parent
        subMenuInstance = subMenuComponent.createObject(parentItem)
        if (subMenuInstance) {
            subMenuInstance.musicHash = root.musicHash
            
            // 计算菜单位置
            var menuPos = addToPlaylistItem.mapToItem(parentItem, addToPlaylistItem.width, 0)
            subMenuInstance.x = menuPos.x + 5
            subMenuInstance.y = menuPos.y + 5
            
            console.log("[SongContextMenu] opening submenu, implicitHeight:", subMenuInstance.subContentLayout ? subMenuInstance.subContentLayout.implicitHeight : "N/A")
            subMenuInstance.open()
        }
    }
    
    // 关闭时清理子菜单
    onClosed: {
        if (subMenuInstance) {
            subMenuInstance.close()
            if (subMenuInstance instanceof QtObject && subMenuInstance !== null) {
                subMenuInstance.destroy()
            }
            subMenuInstance = null
        }
    }
}
