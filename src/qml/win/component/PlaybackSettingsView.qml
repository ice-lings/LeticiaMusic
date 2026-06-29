import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

ScrollView {
    id: playbackSettingsScroll
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: Design.Spacing.spaceLg
        
        // Section: Playback Settings Header
        Text {
            text: qsTr("播放设置")
            font.pixelSize: 20
            font.bold: true
            color: Design.Colors.textPrimary
        }
        
        // Resume playback checkbox
        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignLeft
            
            CheckBox {
                id: resumePlaybackCheckbox
                checked: appContext.getResumePlaybackOnStartup()
                onCheckedChanged: appContext.setResumePlaybackOnStartup(checked)
                spacing: 0
                
                indicator: Rectangle {
                    width: 20
                    height: 20
                    radius: 4
                    border.width: 2
                    border.color: resumePlaybackCheckbox.checked ? Design.Colors.primary : Design.Colors.textSecondary
                    color: resumePlaybackCheckbox.checked ? Design.Colors.primary : "transparent"
                    
                    Image {
                        source: "qrc:/img/checkmark.svg"
                        width: 12
                        height: 12
                        anchors.centerIn: parent
                        visible: resumePlaybackCheckbox.checked
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }
            
            Column {
                spacing: 2
                Layout.alignment: Qt.AlignLeft
                
                Text {
                    text: qsTr("启动时恢复播放")
                    font.pixelSize: 16
                    color: Design.Colors.textPrimary
                }
                
                Text {
                    text: qsTr("程序启动时自动播放上次曲目")
                    font.pixelSize: 12
                    color: Design.Colors.textSecondary
                }
            }
        }
        
        // Section: Volume Settings Header
        Text {
            text: qsTr("音量设置")
            font.pixelSize: 20
            font.bold: true
            color: Design.Colors.textPrimary
            Layout.topMargin: Design.Spacing.spaceLg
        }
        
        // Volume slider
        RowLayout {
            spacing: Design.Spacing.spaceMd
            
            Text {
                text: qsTr("默认音量")
                font.pixelSize: 16
                color: Design.Colors.textPrimary
                Layout.alignment: Qt.AlignVCenter
            }
            
            Slider {
                id: volumeSlider
                from: 0
                to: 100
                stepSize: 1
                value: appContext.getLastVolume()
                Layout.fillWidth: true
                
                background: Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: 6
                    radius: 3
                    color: Qt.rgba(255, 255, 255, 0.1)
                    
                    Rectangle {
                        width: volumeSlider.position * parent.width
                        height: parent.height
                        radius: 3
                        color: Design.Colors.primary
                    }
                }
                
                handle: Rectangle {
                    anchors.centerIn: parent
                    width: 18
                    height: 18
                    radius: 9
                    color: Design.Colors.primary
                    border.width: 2
                    border.color: "#FFFFFF"
                }
                
                onValueChanged: {
                    appContext.setLastVolume(value)
                    volumeText.text = Math.round(value) + "%"
                }
            }
            
            Text {
                id: volumeText
                text: volumeSlider.value + "%"
                font.pixelSize: 16
                color: Design.Colors.textPrimary
                Layout.preferredWidth: 50
                horizontalAlignment: Text.AlignRight
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}