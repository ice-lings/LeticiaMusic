/**
 * 正在播放信息组件（简化版）
 * 显示当前播放歌曲信息
 */

import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

Rectangle {
    id: nowPlayingInfo

    property var playerController: null
    property var currentMusicItem: null

    color: "transparent"

    // 与 currentFile 保持绑定，确保在 QML 侧可靠刷新（优于仅依赖 Connections）
    readonly property string playingPath: playerController ? playerController.currentFile : ""

    function updateMusicInfo() {
        if (playerController && playingPath.length > 0) {
            // QVariantMap 在 QML 中一般为普通对象，键与 C++ 侧一致
            currentMusicItem = appContext.getMusicItemForFile(playingPath)
        } else {
            currentMusicItem = null
        }
    }

    function _str(v) {
        if (v === undefined || v === null)
            return ""
        return String(v)
    }

    RowLayout {
        anchors.fill: parent
        spacing: Design.Spacing.spaceMd

        // 圆形封面
        Rectangle {
            Layout.preferredWidth: Design.Spacing.iconSizeXl * 1.5
            Layout.preferredHeight: Design.Spacing.iconSizeXl * 1.5
            radius: width / 2
            color: Design.Colors.withAlpha(Design.Colors.iconNormal, 0.1)
            clip: true

            Image {
                id: coverImage
                anchors.fill: parent
                asynchronous: true
                cache: true
                source: {
                    if (!currentMusicItem)
                        return ""
                    var s = currentMusicItem.coverPath !== undefined ? currentMusicItem.coverPath
                                                                      : currentMusicItem["coverPath"]
                    if (!_str(s))
                        return ""
                    return _str(s)
                }
                fillMode: Image.PreserveAspectCrop
                visible: source.toString() !== ""
                mipmap: true
                layer.enabled: true
                layer.smooth: true
            }

            Image {
                anchors.centerIn: parent
                source: "qrc:/img/music-note.svg"
                width: parent.width * 0.4
                height: parent.width * 0.4
                fillMode: Image.PreserveAspectFit
                opacity: 0.5
                visible: !coverImage.visible
            }

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                radius: parent.radius
                border.color: Design.Colors.withAlpha(Design.Colors.iconNormal, 0.2)
                border.width: 1
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Design.Spacing.spaceXs

            Text {
                Layout.fillWidth: true
                text: {
                    if (!currentMusicItem || !_str(currentMusicItem.filePath !== undefined ? currentMusicItem.filePath : currentMusicItem["filePath"]))
                        return qsTr("未播放")
                    var t = currentMusicItem.title !== undefined ? currentMusicItem.title : currentMusicItem["title"]
                    if (_str(t).length > 0)
                        return _str(t)
                    return qsTr("未知标题")
                }
                font.pixelSize: Design.Fonts.sizeMd
                font.weight: Font.Medium
                color: Design.Colors.textPrimary
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: {
                    if (!currentMusicItem || !_str(currentMusicItem.filePath !== undefined ? currentMusicItem.filePath : currentMusicItem["filePath"]))
                        return ""
                    var a = currentMusicItem.artist !== undefined ? currentMusicItem.artist : currentMusicItem["artist"]
                    if (_str(a).length > 0)
                        return _str(a)
                    return qsTr("未知艺术家")
                }
                font.pixelSize: Design.Fonts.sizeSm
                color: Design.Colors.textSecondary
                elide: Text.ElideRight
            }
        }
    }

    Component.onCompleted: updateMusicInfo()

    onPlayerControllerChanged: updateMusicInfo()

    onPlayingPathChanged: updateMusicInfo()
}
