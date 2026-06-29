import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: playlistView
    color: Design.Colors.surface

    property var currentPlaylistId: -1

    function refreshPlaylist() {
        playlistListView.model = appContext.playlistManager.getPlaylists()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Text {
            text: "我的歌单"
            color: Design.Colors.textPrimary
            font.pixelSize: 24
            font.bold: true
            leftPadding: 20
            topPadding: 10
        }

        ListView {
            id: playlistListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: appContext.playlistManager.getPlaylists()
            delegate: Rectangle {
                width: playlistListView.width
                height: 60
                color: mouseArea.containsMouse ? Design.Colors.buttonHovered : Design.Colors.background

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    Rectangle {
                        width: 40
                        height: 40
                        color: Design.Colors.divider
                        radius: 5

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/img/music-note.svg"
                            width: 20
                            height: 20
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData.name
                            color: Design.Colors.textPrimary
                            font.pixelSize: 16
                        }
                        Text {
                            text: modelData.musicCount + " 首音乐"
                            color: Design.Colors.textDisabled
                            font.pixelSize: 12
                        }
                    }

                    Image {
                        width: 20
                        height: 20
                        source: "qrc:/img/heart-fill.svg"
                        visible: modelData.isFavorite
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: {
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.playlistId = modelData.id
                            contextMenu.playlistName = modelData.name
                            contextMenu.popup()
                        } else if (mouse.button === Qt.LeftButton) {
                            console.log("Clicked playlist:", modelData.id, modelData.name)
                        }
                    }
                }
            }
        }
    }

    Menu {
        id: contextMenu
        property int playlistId: -1
        property string playlistName: ""

        MenuItem {
            text: "重命名"
            onClicked: {
                renameDialog.playlistId = contextMenu.playlistId
                renameDialog.playlistName = contextMenu.playlistName
                renameDialog.open()
            }
        }
        MenuItem {
            text: "删除"
            onClicked: {
                appContext.playlistViewModel.deletePlaylist(contextMenu.playlistId)
                refreshPlaylist()
            }
        }
    }

    Dialog {
        id: renameDialog
        property int playlistId: -1
        property string playlistName: ""
        title: "重命名歌单"
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            Text {
                text: "请输入新名称:"
                color: Design.Colors.textPrimary
            }
            TextField {
                id: renameTextField
                placeholderText: "歌单名称"
                text: renameDialog.playlistName
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            if (renameTextField.text.trim() !== "") {
                appContext.playlistViewModel.renamePlaylist(renameDialog.playlistId, renameTextField.text.trim())
                refreshPlaylist()
            }
        }
    }
}
