/**
 * 歌单右键菜单 - 自定义黑色主题
 * 
 * 特性：
 * - 黑色背景（与程序主题一致）
 * - 白色文字
 * - 包含重命名和删除选项
 * - 可扩展更多操作
 */

import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

Popup {
    id: root
    
    // 属性
    property int playlistId: -1
    property string playlistName: ""
    
    // 信号
    signal renameClicked()
    signal deleteClicked()
    
    // 基础配置
    width: 120
    height: contentColumn.height + 8
    padding: 4
    
    // 背景 - 黑色主题
    background: Rectangle {
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1
        radius: 4
    }
    
    // 内容列
    Column {
        id: contentColumn
        width: parent.width - 8
        anchors.centerIn: parent
        spacing: 2
        
        // 重命名选项
        Rectangle {
            id: renameItem
            width: parent.width
            height: 32
            color: renameMouseArea.containsMouse ? 
                   Design.Colors.withAlpha(Design.Colors.primary, 0.2) : 
                   "transparent"
            radius: 4
            
            Text {
                anchors.centerIn: parent
                text: qsTr("重命名")
                color: Design.Colors.textPrimary
                font.pixelSize: 13
            }
            
            MouseArea {
                id: renameMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.close()
                    root.renameClicked()
                }
            }
        }
        
        // 分隔线
        Rectangle {
            width: parent.width - 8
            height: 1
            color: Design.Colors.divider
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        // 删除选项
        Rectangle {
            id: deleteItem
            width: parent.width
            height: 32
            color: deleteMouseArea.containsMouse ? 
                   Design.Colors.withAlpha(Design.Colors.error, 0.2) : 
                   "transparent"
            radius: 4
            
            Text {
                anchors.centerIn: parent
                text: qsTr("删除")
                color: Design.Colors.textPrimary
                font.pixelSize: 13
            }
            
            MouseArea {
                id: deleteMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.close()
                    root.deleteClicked()
                }
            }
        }
    }
    
    // 动画效果
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 150
        }
    }
    
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
    }
}