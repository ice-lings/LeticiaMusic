import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 底部导航栏 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale

    property int currentIndex: 0
    signal tabClicked(int index)

    height: 56 * s


    Rectangle { anchors.fill: parent; color: Design.Colors.surface }
    Rectangle {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 1; color: Design.Colors.divider
    }

    Row {
        anchors.fill: parent; spacing: 0

        Repeater {
            model: ListModel {
                ListElement { label: "本地"; icon: ""; svg: "qrc:/img/music-note.svg" }
                ListElement { label: "喜欢"; icon: ""; svg: "qrc:/img/heart-fill.svg" }
                ListElement { label: "歌单";   icon: ""; svg: "qrc:/img/playlist.svg" }
                ListElement { label: "我的";   icon: ""; svg: "qrc:/img/profile.svg" }
            }

            Item {
                width: root.width / 4; height: parent.height
                property bool isActive: index === root.currentIndex

                MouseArea {
                    anchors.fill: parent
                    onClicked: { root.currentIndex = index; root.tabClicked(index) }
                }

                Column {
                    anchors.centerIn: parent; spacing: 3 * s

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: icon; font.pixelSize: 22 * s
                        color: isActive ? Design.Colors.primary : Design.Colors.textSecondary
                        visible: svg === ""
                    }
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 22 * s; height: 22 * s
                        source: svg
                        fillMode: Image.PreserveAspectFit
                        visible: svg !== ""
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: label; font.pixelSize: 11 * s
                        font.weight: isActive ? Font.Medium : Font.Normal
                        color: isActive ? Design.Colors.primary : Design.Colors.textSecondary
                    }
                }

                Rectangle {
                    anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
                    width: 20 * s; height: 3 * s; radius: 1.5 * s
                    color: Design.Colors.primary
                    opacity: isActive ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                }
            }
        }
    }
}
