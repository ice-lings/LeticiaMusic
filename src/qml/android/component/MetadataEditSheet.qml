import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 元数据编辑器 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale

    property string musicHash: ""
    property string musicTitle: ""
    property string musicArtist: ""
    property string musicAlbum: ""
    property string musicGenre: ""
    property int musicYear: 0
    property bool isOpen: false


    anchors.fill: parent
    z: 1000
    visible: isOpen

    function open(props) {
        isOpen = true
        panel.y = panelHomeY
        musicHash = props.musicHash || ""
        musicTitle = props.musicTitle || ""
        musicArtist = props.musicArtist || ""
        musicAlbum = props.musicAlbum || ""
        musicGenre = props.musicGenre || ""
        musicYear = props.musicYear || 0

        var overrides = appContext.getSongOverrides(musicHash)
        titleField.text = (overrides && overrides.title) ? overrides.title : musicTitle
        artistField.text = (overrides && overrides.artist) ? overrides.artist : musicArtist
        albumField.text = (overrides && overrides.album) ? overrides.album : musicAlbum
        genreField.text = (overrides && overrides.genre) ? overrides.genre : musicGenre
        yearField.text = (overrides && overrides.year && overrides.year > 0) ? String(overrides.year) : (musicYear > 0 ? String(musicYear) : "")
    }

    function close() { isOpen = false }

    function save() {
        var yearVal = parseInt(yearField.text) || 0
        appContext.setSongMetadata(musicHash, titleField.text, artistField.text, albumField.text, genreField.text, yearVal)
        close()
    }

    property real panelHomeY: 0

    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        MouseArea { anchors.fill: parent; onClicked: root.close() }
    }

    Rectangle {
        id: panel
        width: parent.width
        height: parent.height * 0.7
        x: 0
        y: parent.height

        Behavior on y {
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

        // 标题栏
        Rectangle {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: 52 * s
            color: "transparent"

            Rectangle {
                anchors {
                    top: parent.top; topMargin: 8 * s
                    horizontalCenter: parent.horizontalCenter
                }
                width: 36 * s
                height: 5 * s
                radius: 2.5 * s
                color: Design.Colors.textHint
            }

            Text {
                anchors {
                    left: parent.left; leftMargin: 20 * s
                    verticalCenter: parent.verticalCenter
                }
                text: "取消"
                font.pixelSize: 15 * s
                color: Design.Colors.textSecondary
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8 * s
                    onClicked: root.close()
                }
            }

            Text {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                text: "编辑元数据"
                font.pixelSize: 16 * s
                font.weight: Font.Bold
                color: Design.Colors.textPrimary
            }

            Text {
                anchors {
                    right: parent.right; rightMargin: 20 * s
                    verticalCenter: parent.verticalCenter
                }
                text: "保存"
                font.pixelSize: 15 * s
                font.weight: Font.Medium
                color: Design.Colors.primary
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8 * s
                    onClicked: root.save()
                }
            }
        }

        // 表单
        Flickable {
            anchors {
                top: parent.top; topMargin: 56 * s
                left: parent.left; right: parent.right; bottom: parent.bottom
            }
            contentHeight: formCol.height + 40 * s
            clip: true

            Column {
                id: formCol
                anchors {
                    left: parent.left; right: parent.right
                    leftMargin: 20 * s; rightMargin: 20 * s
                }
                spacing: 16 * s

                Text { text: "歌曲名"; font.pixelSize: 13 * s; color: Design.Colors.textSecondary }
                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: titleField
                        anchors {
                            fill: parent
                            leftMargin: 12 * s
                            rightMargin: 12 * s
                        }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                    }
                }

                Text { text: "歌手"; font.pixelSize: 13 * s; color: Design.Colors.textSecondary }
                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: artistField
                        anchors {
                            fill: parent
                            leftMargin: 12 * s
                            rightMargin: 12 * s
                        }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                    }
                }

                Text { text: "专辑"; font.pixelSize: 13 * s; color: Design.Colors.textSecondary }
                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: albumField
                        anchors {
                            fill: parent
                            leftMargin: 12 * s
                            rightMargin: 12 * s
                        }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                    }
                }

                Text { text: "风格"; font.pixelSize: 13 * s; color: Design.Colors.textSecondary }
                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: genreField
                        anchors {
                            fill: parent
                            leftMargin: 12 * s
                            rightMargin: 12 * s
                        }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                    }
                }

                Text { text: "年份"; font.pixelSize: 13 * s; color: Design.Colors.textSecondary }
                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: yearField
                        anchors {
                            fill: parent
                            leftMargin: 12 * s
                            rightMargin: 12 * s
                        }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        inputMethodHints: Qt.ImhDigitsOnly
                        clip: true
                    }
                }
            }
        }
    }
}
