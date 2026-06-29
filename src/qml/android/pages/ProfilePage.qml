import QtQuick
import QtQuick.Controls
import QtQuick.Window

import "../component"

/* 我的页面 — 扫描管理 + NAS 同步 + 设置入口 */
Item {
    id: page

    readonly property real s: Design.Responsive.guiScale


    Rectangle { anchors.fill: parent; color: Design.Colors.background }

    Column {
        anchors.fill: parent; anchors.topMargin: 14 * s; spacing: 14 * s

        Rectangle {
            width: parent.width - 28 * s; height: 90 * s; anchors.horizontalCenter: parent.horizontalCenter
            radius: 14 * s; color: Design.Colors.surface

            Row {
                anchors.fill: parent; anchors.leftMargin: 18 * s; anchors.rightMargin: 18 * s; spacing: 14 * s

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter; width: 44 * s; height: 44 * s; radius: 22 * s; color: Design.Colors.listItemHovered
                    Image { anchors.centerIn: parent; width: 22 * s; height: 22 * s; source: "qrc:/img/music-note.svg"; fillMode: Image.PreserveAspectFit }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.parent.width - 44 * s - 14 * s - scanButton.width - 52 * s; spacing: 3 * s

                    Text { text: "音乐库"; font.pixelSize: 18 * s; font.weight: Font.Medium; color: Design.Colors.textPrimary }
                    Text {
                        text: musicCount > 0 ? qsTr("共 %1 首歌曲").arg(musicCount) : "点击扫描查找本地音乐"
                        font.pixelSize: 14 * s; color: Design.Colors.textSecondary; elide: Text.ElideRight; width: parent.width
                    }
                }

                Button {
                    id: scanButton
                    anchors.verticalCenter: parent.verticalCenter
                    text: appContext.scanning ? "扫描中…" : "扫描"; font.pixelSize: 13 * s
                    enabled: !appContext.scanning; width: 76 * s; height: 36 * s
                    onClicked: scanSettingsPage.open()
                    background: Rectangle { color: parent.enabled ? Design.Colors.primary : Design.Colors.textHint; radius: 18 * s }
                    contentItem: Text { text: parent.text; color: Design.Colors.textPrimary; font.pixelSize: 13 * s; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        Rectangle {
            width: parent.width - 28 * s; height: 60 * s; anchors.horizontalCenter: parent.horizontalCenter
            radius: 14 * s; color: Design.Colors.surface; visible: appContext.scanning

            Column {
                anchors.fill: parent; anchors.leftMargin: 18 * s; anchors.rightMargin: 18 * s
                anchors.verticalCenter: parent.verticalCenter; spacing: 7 * s

                Text { text: appContext.scanPhaseText || "正在扫描..."; font.pixelSize: 14 * s; font.weight: Font.Medium; color: Design.Colors.textPrimary; elide: Text.ElideRight; width: parent.width }

                Row { spacing: 8 * s; anchors.left: parent.left; anchors.right: parent.right
                    ProgressBar {
                        width: parent.width - 80 * s; height: 5 * s; anchors.verticalCenter: parent.verticalCenter
                        value: appContext.scanTotal > 0 ? appContext.scanCurrent / appContext.scanTotal : 0
                        background: Rectangle { color: Design.Colors.divider; radius: 3 * s }
                        contentItem: Rectangle { color: Design.Colors.primary; radius: 3 * s; width: parent.visualPosition * parent.width }
                    }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: appContext.scanTotal > 0 ? Math.round(appContext.scanCurrent / appContext.scanTotal * 100) + "%" : ""; font.pixelSize: 12 * s; color: Design.Colors.textSecondary; width: 40 * s; horizontalAlignment: Text.AlignRight }
                }
            }
        }

        // NAS 同步入口
        Rectangle {
            width: parent.width - 28 * s
            height: 60 * s
            anchors.horizontalCenter: parent.horizontalCenter
            radius: 14 * s
            color: Design.Colors.surface

            Row {
                anchors.fill: parent
                anchors.leftMargin: 18 * s
                anchors.rightMargin: 18 * s
                spacing: 14 * s

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 44 * s
                    height: 44 * s
                    radius: 22 * s
                    color: Design.Colors.listItemHovered

                    Image {
                        anchors.centerIn: parent
                        width: 22 * s; height: 22 * s
                        source: "qrc:/img/cloud-sync.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3 * s
                    width: parent.width - 44 * s - 14 * s - 30 * s

                    Text {
                        text: "NAS 同步"
                        font.pixelSize: 18 * s
                        font.weight: Font.Medium
                        color: Design.Colors.textPrimary
                    }
                    Text {
                        id: syncStatusText
                        text: syncConfig.enabled ? "已启用" : "未配置"
                        font.pixelSize: 14 * s
                        color: Design.Colors.textSecondary
                        elide: Text.ElideRight
                        width: parent.width
                    }
                }

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20 * s; height: 20 * s
                    source: "qrc:/img/chevron-right.svg"
                    fillMode: Image.PreserveAspectFit
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: syncConfigSheet.open(syncConfig)
            }
        }

        // 诊断工具
        Rectangle {
            width: parent.width - 28 * s
            height: 60 * s
            anchors.horizontalCenter: parent.horizontalCenter
            radius: 14 * s
            color: Design.Colors.surface

            Row {
                anchors.fill: parent
                anchors.leftMargin: 18 * s
                anchors.rightMargin: 18 * s
                spacing: 14 * s

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 44 * s; height: 44 * s; radius: 22 * s
                    color: Design.Colors.listItemHovered
                    Image {
                        anchors.centerIn: parent
                        width: 22 * s; height: 22 * s
                        source: "qrc:/img/gear.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3 * s
                    width: parent.width - 44 * s - 14 * s - exportBtn.width - 20 * s

                    Text {
                        text: "诊断工具"
                        font.pixelSize: 18 * s; font.weight: Font.Medium; color: Design.Colors.textPrimary
                    }
                    Text {
                        text: "导出运行日志以排查问题"
                        font.pixelSize: 14 * s; color: Design.Colors.textSecondary
                        elide: Text.ElideRight; width: parent.width
                    }
                }

                Button {
                    id: exportBtn
                    anchors.verticalCenter: parent.verticalCenter
                    text: exporting ? "导出中…" : "导出日志"
                    font.pixelSize: 13 * s
                    enabled: !exporting
                    width: 76 * s; height: 36 * s
                    onClicked: {
                        exporting = true
                        appContext.exportLogs()
                    }
                    background: Rectangle { color: parent.enabled ? "#4a7a5a" : Design.Colors.textHint; radius: 18 * s }
                    contentItem: Text { text: parent.text; color: Design.Colors.textPrimary; font.pixelSize: 13 * s; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        // 查找重复歌曲
        Rectangle {
            width: parent.width - 28 * s
            height: 60 * s
            anchors.horizontalCenter: parent.horizontalCenter
            radius: 14 * s
            color: Design.Colors.surface

            Row {
                anchors.fill: parent
                anchors.leftMargin: 18 * s
                anchors.rightMargin: 18 * s
                spacing: 14 * s

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 44 * s; height: 44 * s; radius: 22 * s
                    color: Design.Colors.listItemHovered
                    Image {
                        anchors.centerIn: parent
                        width: 20 * s; height: 20 * s
                        source: "qrc:/img/duplicate.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3 * s
                    width: parent.width - 44 * s - 14 * s - dedupBtn.width - 20 * s

                    Text {
                        text: "查找重复歌曲"
                        font.pixelSize: 18 * s; font.weight: Font.Medium; color: Design.Colors.textPrimary
                    }
                    Text {
                        text: "按标题/艺术家合并不同音质版本"
                        font.pixelSize: 14 * s; color: Design.Colors.textSecondary
                        elide: Text.ElideRight; width: parent.width
                    }
                }

                Button {
                    id: dedupBtn
                    anchors.verticalCenter: parent.verticalCenter
                    text: "查找"
                    font.pixelSize: 13 * s
                    width: 76 * s; height: 36 * s
                    onClicked: duplicateSongsPage.open()
                    background: Rectangle { color: Design.Colors.primary; radius: 18 * s }
                    contentItem: Text { text: parent.text; color: Design.Colors.textPrimary; font.pixelSize: 13 * s; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    SyncConfigSheet {
        id: syncConfigSheet
    }

    property int musicCount: 0
    property var musicModel: null
    property var syncConfig: ({})
    property bool exporting: false

    Component.onCompleted: {
        musicModel = appContext.getViewModel("local_music")
        if (musicModel) { musicCount = musicModel.count; musicModel.countChanged.connect(function() { musicCount = musicModel.count }) }
        if (appContext) { syncConfig = appContext.getSyncConfig() }
    }

    Connections {
        target: page
        function onVisibleChanged() {
            if (page.visible && appContext) {
                syncConfig = appContext.getSyncConfig()
            }
        }
    }

    Connections {
        target: appContext
        function onLogExported(destPath, success) {
            exporting = false
            toastMessage = success ? ("日志已导出到\n" + destPath) : "日志导出失败"
            toastVisible = true
            toastTimer.restart()
        }
    }

    // Toast 提示
    property string toastMessage: ""
    property bool toastVisible: false

    Timer {
        id: toastTimer
        interval: 4000
        onTriggered: toastVisible = false
    }

    Rectangle {
        anchors.centerIn: parent
        width: toastLabel.contentWidth + 36 * s
        height: toastLabel.contentHeight + 24 * s
        radius: 10 * s
        color: "#cc2d2d37"
        visible: toastVisible
        z: 999

        Text {
            id: toastLabel
            anchors.centerIn: parent
            text: toastMessage
            font.pixelSize: 14 * s
            color: Design.Colors.textPrimary
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
