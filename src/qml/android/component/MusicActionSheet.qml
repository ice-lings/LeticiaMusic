import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 音乐操作面板 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale

    property string musicHash: ""
    property string musicFilePath: ""
    property string musicTitle: ""
    property string musicArtist: ""
    property bool isOpen: false


    anchors.fill: parent
    z: 1000
    visible: isOpen

    function open(props) {
        isOpen = true
        panel.y = panelHomeY
        musicHash = props.musicHash || ""
        musicFilePath = props.musicFilePath || ""
        musicTitle = props.musicTitle || ""
        musicArtist = props.musicArtist || ""
        playlistMode = false
        deleteMode = false
    }

    function close() {
        isOpen = false
        playlistMode = false
        deleteMode = false
    }

    property bool playlistMode: false
    property bool deleteMode: false
    property real panelHomeY: root.parent ? root.parent.height * 0.4 : 0

    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        MouseArea { anchors.fill: parent; onClicked: root.close() }
    }

    Rectangle {
        id: panel
        width: parent.width
        height: parent.height * 0.65
        x: 0
        y: parent.height

        Behavior on y {
            enabled: !handleArea.dragActive
            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
        }

        color: Design.Colors.background
        radius: 16 * s

        Rectangle {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: 16 * s
            color: parent.color
        }

        Rectangle {
            anchors { horizontalCenter: parent.horizontalCenter }
            y: 8 * s
            width: 36 * s
            height: 5 * s
            radius: 2.5 * s
            color: Design.Colors.textHint

            MouseArea {
                id: handleArea
                anchors {
                    fill: parent
                    margins: -14 * s
                }
                property real startY: 0
                property real startPanelY: 0
                property bool dragActive: false

                onPressed: {
                    startY = mouseY
                    startPanelY = panel.y
                    dragActive = false
                }
                onPositionChanged: {
                    var d = mouseY - startY
                    if (Math.abs(d) > 6) dragActive = true
                    if (dragActive && d > 0) panel.y = startPanelY + d
                }
                onReleased: {
                    if (dragActive) {
                        if (panel.y - startPanelY > 80) root.close()
                        else panel.y = panelHomeY
                    }
                    dragActive = false
                }
                onClicked: {
                    if (!dragActive) root.close()
                }
            }
        }

        Column {
            anchors {
                top: parent.top; topMargin: 26 * s
                left: parent.left; leftMargin: 20 * s
                right: parent.right; rightMargin: 20 * s
            }
            spacing: 2 * s

            Text {
                text: musicTitle || "未知歌曲"
                font.pixelSize: 16 * s
                font.weight: Font.Medium
                color: Design.Colors.textPrimary
                elide: Text.ElideRight
                width: parent.width
                maximumLineCount: 1
            }
            Text {
                text: musicArtist || "佚名"
                font.pixelSize: 13 * s
                color: Design.Colors.textSecondary
                width: parent.width
            }
        }

        Rectangle {
            anchors {
                top: parent.top; topMargin: 68 * s
                left: parent.left; right: parent.right
                leftMargin: 20 * s; rightMargin: 20 * s
            }
            height: 0.5
            color: Design.Colors.divider
        }

        // 正常操作列表
        Column {
            anchors {
                top: parent.top; topMargin: 74 * s
                left: parent.left; right: parent.right
            }
            visible: !root.playlistMode && !root.deleteMode

            Rectangle {
                width: parent.width
                height: 50 * s
                color: "transparent"

                Text {
                    anchors {
                        left: parent.left; leftMargin: 20 * s
                        verticalCenter: parent.verticalCenter
                    }
                    text: "添加到歌单"
                    font.pixelSize: 16 * s
                    color: Design.Colors.textPrimary
                }
                Image {
                    anchors {
                        right: parent.right; rightMargin: 20 * s
                        verticalCenter: parent.verticalCenter
                    }
                    width: 18 * s; height: 18 * s
                    source: "qrc:/img/chevron-right.svg"
                    fillMode: Image.PreserveAspectFit
                }
                MouseArea { anchors.fill: parent; onClicked: root.playlistMode = true }
            }

            Rectangle {
                anchors { left: parent.left; right: parent.right; leftMargin: 20 * s; rightMargin: 20 * s }
                height: 0.5
                color: Design.Colors.divider
            }

            Rectangle {
                width: parent.width
                height: 50 * s
                color: "transparent"

                Text {
                    anchors {
                        left: parent.left; leftMargin: 20 * s
                        verticalCenter: parent.verticalCenter
                    }
                    text: "编辑元数据"
                    font.pixelSize: 16 * s
                    color: Design.Colors.textPrimary
                }
                MouseArea { anchors.fill: parent; onClicked: { root.close(); root.editMetadata() } }
            }

            Rectangle {
                anchors { left: parent.left; right: parent.right; leftMargin: 20 * s; rightMargin: 20 * s }
                height: 0.5
                color: Design.Colors.divider
            }

            Rectangle {
                width: parent.width
                height: 50 * s
                color: "transparent"

                Text {
                    anchors {
                        left: parent.left; leftMargin: 20 * s
                        verticalCenter: parent.verticalCenter
                    }
                    text: "删除"
                    font.pixelSize: 16 * s
                    color: Design.Colors.error
                }
                MouseArea { anchors.fill: parent; onClicked: root.deleteMode = true }
            }
        }

        // 删除确认
        Column {
            anchors {
                top: parent.top; topMargin: 90 * s
                left: parent.left; right: parent.right
            }
            visible: root.deleteMode
            spacing: 20 * s

            Text {
                anchors { left: parent.left; leftMargin: 20 * s; right: parent.right; rightMargin: 20 * s }
                text: qsTr("确认删除「%1」?").arg(musicTitle || "未知歌曲")
                font.pixelSize: 16 * s
                color: Design.Colors.textPrimary
                wrapMode: Text.Wrap
                width: parent.width - 40 * s
            }

            Text {
                anchors { left: parent.left; leftMargin: 20 * s }
                text: "此操作不可撤销"
                font.pixelSize: 13 * s
                color: Design.Colors.textSecondary
            }

            Row {
                anchors { horizontalCenter: parent.horizontalCenter }
                spacing: 24 * s

                Rectangle {
                    width: 100 * s
                    height: 40 * s
                    radius: 20 * s
                    color: Design.Colors.divider

                    Text {
                        anchors.centerIn: parent
                        text: "取消"
                        font.pixelSize: 14 * s
                        color: Design.Colors.textPrimary
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.deleteMode = false }
                }

                Rectangle {
                    width: 100 * s
                    height: 40 * s
                    radius: 20 * s
                    color: Design.Colors.error

                    Text {
                        anchors.centerIn: parent
                        text: "确认删除"
                        font.pixelSize: 14 * s
                        font.weight: Font.Bold
                        color: Design.Colors.textPrimary
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (root.musicHash) appContext.deleteMusic(root.musicHash, true)
                            root.close()
                        }
                    }
                }
            }
        }

        // 歌单选择
        Column {
            anchors {
                top: parent.top; topMargin: 74 * s
                left: parent.left; right: parent.right; bottom: parent.bottom
            }
            visible: root.playlistMode

            Rectangle {
                width: parent.width
                height: 50 * s
                color: "transparent"

                Text {
                    anchors {
                        left: parent.left; leftMargin: 20 * s
                        verticalCenter: parent.verticalCenter
                    }
                    text: "← 返回"
                    font.pixelSize: 16 * s
                    color: Design.Colors.primary
                }
                MouseArea { anchors.fill: parent; onClicked: root.playlistMode = false }
            }

            Rectangle {
                anchors { left: parent.left; right: parent.right; leftMargin: 20 * s; rightMargin: 20 * s }
                height: 0.5
                color: Design.Colors.divider
            }

            ListView {
                anchors {
                    left: parent.left; right: parent.right
                    top: parent.top; topMargin: 52 * s
                    bottom: parent.bottom
                }
                clip: true
                spacing: 0
                reuseItems: true
                model: appContext ? appContext.playlistViewModel : null

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 46 * s
                    color: "transparent"

                    Text {
                        anchors {
                            left: parent.left; leftMargin: 20 * s
                            verticalCenter: parent.verticalCenter
                        }
                        text: model.name || ""
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        elide: Text.ElideRight
                        width: parent.width * 0.65
                    }
                    Text {
                        anchors {
                            right: parent.right; rightMargin: 20 * s
                            verticalCenter: parent.verticalCenter
                        }
                        text: model.musicCount || 0
                        font.pixelSize: 12 * s
                        color: Design.Colors.textSecondary
                    }

                    Rectangle {
                        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 20 * s }
                        height: 0.5
                        color: Design.Colors.divider
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (root.musicHash && appContext && appContext.playlistManager) {
                                appContext.playlistManager.addMusicToPlaylist(model.id, root.musicHash)
                            }
                            root.close()
                        }
                    }
                }
            }
        }
    }

    signal editMetadata()
}
