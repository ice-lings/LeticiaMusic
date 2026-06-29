import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    width: 420
    height: contentCol.implicitHeight + 48

    property string musicHash: ""
    property string title: ""
    property string artist: ""
    property string album: ""
    property string genre: ""
    property int year: 0
    property string coverPath: ""
    property string pendingCoverPath: ""

    background: Rectangle {
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1
        radius: 8
    }

    Overlay.modal: Rectangle {
        color: "#80000000"
    }

    onOpened: {
        titleInput.text = root.title
        artistInput.text = root.artist
        albumInput.text = root.album
        genreInput.text = root.genre
        yearInput.text = root.year > 0 ? String(root.year) : ""
    }

    Column {
        id: contentCol
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 20
        }
        spacing: 12

        Text {
            text: qsTr("编辑歌曲信息")
            color: Design.Colors.textPrimary
            font.pixelSize: 16
            font.bold: true
        }

        RowLayout {
            width: parent.width
            spacing: 16

            Rectangle {
                width: 80; height: 80; radius: 6
                color: Design.Colors.buttonNormal

                Image {
                    anchors.fill: parent; anchors.margins: 2
                    source: root.coverPath ? root.coverPath
                            : "qrc:/img/assets/images/default_music_cover.png"
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }
            }

            ColumnLayout {
                Text {
                    text: qsTr("更换封面")
                    color: Design.Colors.primary; font.pixelSize: 13
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: coverFileDialog.open()
                    }
                }
                Text {
                    text: qsTr("点击上方文字选择图片")
                    color: Design.Colors.textSecondary; font.pixelSize: 11
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: Design.Colors.divider }

        Text { text: qsTr("标题"); color: Design.Colors.textSecondary; font.pixelSize: 11 }
        Rectangle {
            width: parent.width; height: 36; radius: 5
            color: Design.Colors.buttonPressed
            border.color: Design.Colors.divider; border.width: 1
            TextInput {
                id: titleInput
                anchors.fill: parent; anchors.margins: 8
                color: Design.Colors.textPrimary; font.pixelSize: 13
                verticalAlignment: TextInput.AlignVCenter; clip: true
            }
        }

        Text { text: qsTr("艺术家"); color: Design.Colors.textSecondary; font.pixelSize: 11 }
        Rectangle {
            width: parent.width; height: 36; radius: 5
            color: Design.Colors.buttonPressed
            border.color: Design.Colors.divider; border.width: 1
            TextInput {
                id: artistInput
                anchors.fill: parent; anchors.margins: 8
                color: Design.Colors.textPrimary; font.pixelSize: 13
                verticalAlignment: TextInput.AlignVCenter; clip: true
            }
        }

        Text { text: qsTr("专辑"); color: Design.Colors.textSecondary; font.pixelSize: 11 }
        Rectangle {
            width: parent.width; height: 36; radius: 5
            color: Design.Colors.buttonPressed
            border.color: Design.Colors.divider; border.width: 1
            TextInput {
                id: albumInput
                anchors.fill: parent; anchors.margins: 8
                color: Design.Colors.textPrimary; font.pixelSize: 13
                verticalAlignment: TextInput.AlignVCenter; clip: true
            }
        }

        Text { text: qsTr("流派"); color: Design.Colors.textSecondary; font.pixelSize: 11 }
        Rectangle {
            width: parent.width; height: 36; radius: 5
            color: Design.Colors.buttonPressed
            border.color: Design.Colors.divider; border.width: 1
            TextInput {
                id: genreInput
                anchors.fill: parent; anchors.margins: 8
                color: Design.Colors.textPrimary; font.pixelSize: 13
                verticalAlignment: TextInput.AlignVCenter; clip: true
            }
        }

        Text { text: qsTr("年份"); color: Design.Colors.textSecondary; font.pixelSize: 11 }
        Rectangle {
            width: parent.width; height: 36; radius: 5
            color: Design.Colors.buttonPressed
            border.color: Design.Colors.divider; border.width: 1
            TextInput {
                id: yearInput
                anchors.fill: parent; anchors.margins: 8
                color: Design.Colors.textPrimary; font.pixelSize: 13
                verticalAlignment: TextInput.AlignVCenter; clip: true
                validator: IntValidator {}
            }
        }

        Text {
            text: qsTr("留空的字段将使用原始元数据")
            color: Design.Colors.textSecondary; font.pixelSize: 11
            width: parent.width; horizontalAlignment: Text.AlignRight
        }

        Item { width: 1; height: 4 }

        Row {
            anchors.right: parent.right; spacing: 10

            Rectangle {
                width: 100; height: 34; radius: 5
                color: resetMouse.containsMouse
                       ? Design.Colors.buttonHovered : Design.Colors.buttonNormal
                border.color: Design.Colors.divider; border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: qsTr("恢复原始")
                    color: Design.Colors.textPrimary; font.pixelSize: 13
                }
                MouseArea {
                    id: resetMouse
                    anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        appContext.resetSongMetadata(root.musicHash)
                        root.close()
                    }
                }
            }

            Rectangle {
                width: 80; height: 34; radius: 5
                color: cancelMouse.containsMouse
                       ? Design.Colors.buttonHovered : Design.Colors.buttonNormal
                border.color: Design.Colors.divider; border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: qsTr("取消")
                    color: Design.Colors.textPrimary; font.pixelSize: 13
                }
                MouseArea {
                    id: cancelMouse
                    anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }

            Rectangle {
                width: 80; height: 34; radius: 5
                color: saveMouse.containsMouse
                       ? Design.Colors.withAlpha(Design.Colors.primary, 0.85)
                       : Design.Colors.primary
                Text {
                    anchors.centerIn: parent
                    text: qsTr("保存")
                    color: Design.Colors.textPrimary; font.pixelSize: 13; font.bold: true
                }
                MouseArea {
                    id: saveMouse
                    anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var yearVal = parseInt(yearInput.text) || 0
                        if (root.pendingCoverPath) {
                            appContext.setSongCover(root.musicHash, root.pendingCoverPath)
                        }
                        appContext.setSongMetadata(
                            root.musicHash,
                            titleInput.text, artistInput.text,
                            albumInput.text, genreInput.text, yearVal
                        )
                        globalToast.showSuccess(qsTr("元数据已保存"))
                        root.close()
                    }
                }
            }
        }
    }

    Component {
        id: coverCropComponent
        CoverCropDialog {}
    }

    FileDialog {
        id: coverFileDialog
        title: qsTr("选择封面图片")
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.bmp)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (!path) return

            var parentItem = ApplicationWindow.window
                             ? ApplicationWindow.window.contentItem : root.parent

            var cropDialog = coverCropComponent.createObject(parentItem)
            cropDialog.sourcePath = path
            cropDialog.x = (parentItem.width - cropDialog.width) / 2
            cropDialog.y = (parentItem.height - cropDialog.height) / 2

            cropDialog.cropConfirmed.connect(function(croppedPath) {
                root.pendingCoverPath = croppedPath
                root.coverPath = "file:///" + croppedPath
            })

            cropDialog.closed.connect(function() {
                cropDialog.destroy()
            })

            cropDialog.open()
        }
    }
}
