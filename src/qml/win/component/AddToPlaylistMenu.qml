import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    
    property string musicHash: ""
    property string musicFilePath: ""
    property string musicTitle: ""
    
    signal songAddedToPlaylist(int playlistId, string playlistName)
    signal songAlreadyExists(int playlistId, string playlistName)
    
    width: 200
    height: Math.min(contentLayout.implicitHeight + 16, 300)
    
    modal: false
    focus: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    
    parent: Overlay.overlay
    z: 10000
    
    background: Rectangle {
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1
        radius: 6
    }
    
    ColumnLayout {
        id: contentLayout
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
            
            delegate: Rectangle {
                width: playlistListView.width
                height: 36
                radius: 4
                color: mouseArea.containsMouse ? 
                       Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                       "transparent"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8
                    
                    Text {
                        Layout.fillWidth: true
                        text: model.name
                        color: Design.Colors.textPrimary
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    Text {
                        text: model.musicCount
                        color: Design.Colors.textSecondary
                        font.pixelSize: 11
                    }
                }
                
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        var items = appContext.playlistManager.getItems(model.id)
                        var alreadyExists = false
                        
                        for (var i = 0; i < items.length; i++) {
                            if (items[i].musicHash === root.musicHash) {
                                alreadyExists = true
                                break
                            }
                        }
                        
                        if (alreadyExists) {
                            root.songAlreadyExists(model.id, model.name)
                        } else {
                            var success = appContext.playlistManager.addMusicToPlaylist(
                                model.id, 
                                root.musicHash
                            )
                            
                            if (success) {
                                root.songAddedToPlaylist(model.id, model.name)
                            }
                        }
                        
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
    
    onOpened: {
        if (appContext && appContext.playlistViewModel) {
            appContext.playlistViewModel.refresh()
        }
    }
}
