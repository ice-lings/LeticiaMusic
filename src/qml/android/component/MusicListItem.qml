import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 音乐列表项 */
Rectangle {
    id: root

    readonly property real s: Design.Responsive.guiScale

    property string musicTitle: ""
    property string musicArtist: ""
    property string musicDuration: ""
    property string coverSource: ""
    property string musicQuality: ""
    property string qualityColor: ""
    property string musicHash: ""
    property string musicFilePath: ""
    property bool fileMissing: false
    property bool isActive: false
    property bool isFavorite: false

    signal clicked()
    signal longPressed()
    signal showActions()

    width: parent ? parent.width : 0
    height: 80 * s
    color: fileMissing ? "#18E53935" : (isActive ? "#1affffff" : "transparent")

    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        anchors.leftMargin: 70 * s
        height: 0.5
        color: Design.Colors.divider
    }

    // 封面
    Image {
        id: cover
        anchors { left: parent.left; leftMargin: 14 * s; verticalCenter: parent.verticalCenter }
        width: 52 * s; height: 52 * s
        source: coverSource || "qrc:/img/assets/images/default_music_cover.png"
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true
        smooth: true
        Rectangle {
            anchors.fill: parent; radius: 5 * s; color: "transparent"
            border.width: 0.5; border.color: Design.Colors.divider
        }
    }

    // 更多操作
    Item {
        id: moreBtn
        anchors { right: parent.right; rightMargin: 4 * s; verticalCenter: parent.verticalCenter }
        width: 30 * s; height: 30 * s

        Image {
            anchors.centerIn: parent
            width: 16 * s; height: 16 * s
            source: "qrc:/img/more-vertical.svg"
            fillMode: Image.PreserveAspectFit
        }
        MouseArea {
            anchors.fill: parent; anchors.margins: -4 * s
            onClicked: root.showActions()
        }
    }

    // 收藏
    Item {
        id: heartBtn
        anchors { right: moreBtn.left; rightMargin: 2 * s; verticalCenter: parent.verticalCenter }
        width: 36 * s; height: 36 * s

        Image {
            id: heartIcon; anchors.centerIn: parent
            width: 22 * s; height: 22 * s
            source: isFavorite ? "qrc:/img/heart-fill.svg" : "qrc:/img/heart-line.svg"
            fillMode: Image.PreserveAspectFit
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (musicHash && appContext && appContext.favoriteManager) {
                    appContext.favoriteManager.toggleFavorite(musicHash)
                    isFavorite = appContext.favoriteManager.checkIsFavorite(musicHash)
                    heartIcon.scale = 0.7
                    heartTimer.restart()
                }
            }
        }
        Timer { id: heartTimer; interval: 100; onTriggered: heartIcon.scale = 1.0 }
    }

    // 歌曲信息
    Column {
        anchors {
            left: cover.right; leftMargin: 10 * s
            right: heartBtn.left; rightMargin: 4 * s
            verticalCenter: parent.verticalCenter
        }
        spacing: 3 * s

        Text {
            width: parent.width; text: musicTitle || "未知歌曲"; color: Design.Colors.textPrimary
            font.pixelSize: 17 * s; font.weight: Font.Medium
            elide: Text.ElideRight; maximumLineCount: 1
        }

        Row {
            spacing: 6 * s; width: parent.width

            Rectangle {
                id: qualityRect
                visible: musicQuality !== ""; anchors.verticalCenter: parent.verticalCenter
                width: qualityTag.implicitWidth + 6 * s; height: 16 * s; radius: 2 * s
                color: musicQuality !== "LQ" ? (qualityColor || Design.Colors.primary) : "transparent"
                border.width: musicQuality === "LQ" ? 1 : 0; border.color: Design.Colors.textHint
                Text {
                    id: qualityTag; anchors.centerIn: parent; text: musicQuality
                    color: musicQuality === "LQ" ? Design.Colors.textDisabled : Design.Colors.textPrimary
                    font.pixelSize: 10 * s; font.weight: Font.Bold
                }
            }

            Text {
                id: durationText
                anchors.verticalCenter: parent.verticalCenter
                text: musicDuration ? " · " + musicDuration : ""
                color: Design.Colors.textSecondary; font.pixelSize: 14 * s
            }

            Text {
                id: artistText
                anchors.verticalCenter: parent.verticalCenter
                text: musicArtist || "佚名"
                color: Design.Colors.textSecondary; font.pixelSize: 14 * s
                elide: Text.ElideRight; maximumLineCount: 1
                width: Math.max(0, parent.width - qualityRect.width - durationText.width - 12 * s)
            }
        }
    }

    onMusicHashChanged: {
        if (musicHash && appContext && appContext.favoriteManager) {
            isFavorite = appContext.favoriteManager.checkIsFavorite(musicHash)
        } else {
            isFavorite = false
        }
    }

    MouseArea {
        anchors.fill: parent; anchors.rightMargin: 80 * s
        onClicked: root.clicked()
        onPressAndHold: root.longPressed()
    }
}
