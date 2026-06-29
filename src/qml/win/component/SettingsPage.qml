import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

Rectangle {
    id: settingsPage
    color: Design.Colors.card
    clip: true
    
    property int currentTabIndex: 0
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Design.Spacing.spaceXl
        spacing: Design.Spacing.spaceLg
        
        // Header
        Text {
            text: qsTr("设置")
            font.pixelSize: 28
            font.bold: true
            color: Design.Colors.textPrimary
        }
        
        // Tab Bar
        TabBar {
            id: settingsTabBar
            Layout.fillWidth: true
            implicitHeight: 48

            background: Rectangle {
                color: "transparent"
            }
            
            TabButton {
                id: musicTabBtn
                text: qsTr("音乐")
                implicitWidth: 100
                
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: settingsTabBar.currentIndex === 0 ? Qt.rgba(0.25, 0.25, 0.28, 0.8) : "transparent"
                }
                
                contentItem: Text {
                    text: musicTabBtn.text
                    font.pixelSize: 16
                    color: settingsTabBar.currentIndex === 0 ? Design.Colors.accent : Design.Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            TabButton {
                id: playbackTabBtn
                text: qsTr("播放")
                implicitWidth: 100
                
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: settingsTabBar.currentIndex === 1 ? Qt.rgba(0.25, 0.25, 0.28, 0.8) : "transparent"
                }
                
                contentItem: Text {
                    text: playbackTabBtn.text
                    font.pixelSize: 16
                    color: settingsTabBar.currentIndex === 1 ? Design.Colors.accent : Design.Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            TabButton {
                id: aboutTabBtn
                text: qsTr("关于")
                implicitWidth: 100
                
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: settingsTabBar.currentIndex === 2 ? Qt.rgba(0.25, 0.25, 0.28, 0.8) : "transparent"
                }
                
                contentItem: Text {
                    text: aboutTabBtn.text
                    font.pixelSize: 16
                    color: settingsTabBar.currentIndex === 2 ? Design.Colors.accent : Design.Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            TabButton {
                id: syncTabBtn
                text: qsTr("同步")
                implicitWidth: 100
                
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: settingsTabBar.currentIndex === 3 ? Qt.rgba(0.25, 0.25, 0.28, 0.8) : "transparent"
                }
                
                contentItem: Text {
                    text: syncTabBtn.text
                    font.pixelSize: 16
                    color: settingsTabBar.currentIndex === 3 ? Design.Colors.accent : Design.Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        
        // Content Stack
        StackLayout {
            id: settingsStack
            currentIndex: settingsTabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            MusicSettingsView {
                id: musicSettingsView
            }
            
            PlaybackSettingsView {
                id: playbackSettingsView
            }
            
            AboutSettingsView {
                id: aboutSettingsView
            }

            SyncSettingsView {
                id: syncSettingsView
            }
        }
    }
}