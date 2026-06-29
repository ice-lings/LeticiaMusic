import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: playQueueView
    width: 320
    height: 400
    parent: Overlay.overlay
    z: 10001
    modal: false
    focus: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    
    property var player: appContext.playerController
    
    x: parent.width - width - 20
    y: parent.height - height - 100
    
    background: Rectangle {
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1
        radius: 8
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0
        
        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: Design.Colors.background
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 8
                
                Text {
                    text: qsTr("播放队列")
                    color: Design.Colors.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                }
                
                Item { Layout.fillWidth: true }
                
                Text {
                    text: (playQueueView.player ? playQueueView.player.queueCount : 0) + " 首歌曲"
                    color: Design.Colors.textSecondary
                    font.pixelSize: 12
                }
                
                ToolButton {
                    text: "×"
                    onClicked: playQueueView.close()
                }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Design.Colors.divider
        }
        
        // 播放队列列表
        ListView {
            id: queueListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            // 监听队列变化
            Connections {
                target: playQueueView.player
                function onQueueChanged() {
                    queueListView.model = playQueueView.player ? playQueueView.player.getPlayQueue() : []
                }
            }
            
            model: playQueueView.player ? playQueueView.player.getPlayQueue() : []
            
            delegate: Rectangle {
                width: queueListView.width
                height: 52
                color: index === playQueueView.player.currentIndex ? 
                       Design.Colors.withAlpha(Design.Colors.primary, 0.15) : 
                       "transparent"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10
                    
                    // 正在播放指示器
                    Text {
                        text: index === playQueueView.player.currentIndex ? "▶" : (index + 1)
                        color: index === playQueueView.player.currentIndex ? 
                               Design.Colors.primary : Design.Colors.textSecondary
                        font.pixelSize: index === playQueueView.player.currentIndex ? 12 : 11
                        Layout.preferredWidth: 24
                    }
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Text {
                            text: modelData.title || "未知标题"
                            color: index === playQueueView.player.currentIndex ? 
                                   Design.Colors.primary : Design.Colors.textPrimary
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        
                        Text {
                            text: modelData.artist || "未知艺术家"
                            color: Design.Colors.textSecondary
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        playQueueView.player.playAt(index)
                    }
                }
            }
            
            Rectangle {
                anchors.centerIn: parent
                visible: queueListView.count === 0
                
                Text {
                    text: qsTr("播放队列为空")
                    color: Design.Colors.textSecondary
                    font.pixelSize: 14
                }
            }
        }
    }
}
