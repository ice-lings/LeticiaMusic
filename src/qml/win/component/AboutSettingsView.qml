import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

ScrollView {
    id: aboutSettingsScroll
    Layout.fillWidth: true
    Layout.fillHeight: true
    
    ColumnLayout {
        width: parent.width
        spacing: Design.Spacing.spaceMd
        
        // App Icon and Name
        RowLayout {
            spacing: Design.Spacing.spaceMd
            
            Image {
                source: "qrc:/img/heart-fill.svg"
                width: 64
                height: 64
            }
            
            ColumnLayout {
                spacing: Design.Spacing.spaceXs
                
                Text {
                    text: "LeticiaMusic"
                    font.pixelSize: 24
                    font.bold: true
                    color: Design.Colors.textPrimary
                }
                
                Text {
                    text: qsTr("版本 ") + appContext.getAppVersion()
                    color: Design.Colors.textSecondary
                }
            }
        }
        
        // Description
        Text {
            text: qsTr("一款优雅的Qt6音乐播放器，支持多路径扫描、播放列表管理等功能。")
            color: Design.Colors.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        
        // Developer Info
        GroupBox {
            title: qsTr("开发者信息")
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: Design.Spacing.spaceSm
                
                RowLayout {
                    spacing: Design.Spacing.spaceSm
                    Text { 
                        text: qsTr("开发者:"); 
                        color: Design.Colors.textSecondary
                    }
                    Text { 
                        text: "LeticiaMusic Team"; 
                        color: Design.Colors.textPrimary
                    }
                }
                
                RowLayout {
                    spacing: Design.Spacing.spaceSm
                    Text { 
                        text: qsTr("主页:"); 
                        color: Design.Colors.textSecondary
                    }
                    Text { 
                        text: "https://github.com/leticiamusic"; 
                        color: Design.Colors.primary
                    }
                }
            }
        }
    }
}