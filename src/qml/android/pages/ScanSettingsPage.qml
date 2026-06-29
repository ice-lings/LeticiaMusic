import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 扫描设置页面 — 左滑进入 */
Item {
    id: root

    // 初始在屏幕外，open() 时滑入；使用显式绑定避免 anchors.fill 覆盖 x
    x: 5000
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    z: 200
    visible: false       // 启动时隐藏，open() 时设为 true

    readonly property real s: Design.Responsive.guiScale


    Rectangle { anchors.fill: parent; color: Design.Colors.background }

    // ═══════ 顶部导航栏 ═══════
    Rectangle {
        id: topBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 50 * s; color: Design.Colors.surface

        Text {
            anchors { left: parent.left; leftMargin: 12 * s; verticalCenter: parent.verticalCenter }
            text: "← 返回"
            font.pixelSize: 15 * s; color: Design.Colors.primary
            MouseArea {
                anchors.fill: parent; anchors.margins: -8 * s
                onClicked: root.close()
            }
        }

        Text {
            anchors.centerIn: parent; text: "扫描设置"
            font.pixelSize: 18 * s; font.weight: Font.Bold
            color: Design.Colors.textPrimary
        }
    }

    // ═══════ 内容滚动区 ═══════
    Flickable {
        anchors { top: topBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        clip: true
        contentHeight: contentColumn.implicitHeight + 30 * s

        Column {
            id: contentColumn
            anchors { left: parent.left; right: parent.right; topMargin: 16 * s }
            anchors.leftMargin: 16 * s; anchors.rightMargin: 16 * s
            spacing: 14 * s

            // ━━ 扫描过滤 ━━
            Rectangle {
                width: parent.width; height: 180 * s
                radius: 14 * s; color: Design.Colors.surface

                Column {
                    anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s; topMargin: 16 * s }
                    spacing: 14 * s

                    Text { text: "扫描过滤"; font.pixelSize: 16 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }

                    // 最小文件大小
                    Row { width: parent.width; height: 40 * s; spacing: 10 * s
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "最小文件大小"
                               font.pixelSize: 14 * s; color: Design.Colors.textSecondary; width: 100 * s }
                        Text { id: sizeLabel; anchors.verticalCenter: parent.verticalCenter
                               text: minSizeSlider.value + " KB"; font.pixelSize: 13 * s; color: Design.Colors.primary; width: 60 * s }
                        Slider { id: minSizeSlider; width: parent.width - 170 * s; height: 30 * s
                            from: 0; to: 500; stepSize: 10
                            value: appContext ? appContext.getMinFileSizeKB() : 100
                            onMoved: { if (appContext) appContext.setMinFileSizeKB(value) }
                            background: Rectangle { x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - 2
                                width: parent.availableWidth; height: 4; radius: 2; color: Design.Colors.divider
                                Rectangle { width: parent.width * minSizeSlider.visualPosition
                                    height: parent.height; radius: 2; color: Design.Colors.primary } }
                            handle: Rectangle { x: minSizeSlider.visualPosition * (parent.width - width)
                                width: 20 * s; height: 20 * s; radius: 10 * s; color: Design.Colors.primary } }
                    }

                    // 最小时长
                    Row { width: parent.width; height: 40 * s; spacing: 10 * s
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "最小时长"
                               font.pixelSize: 14 * s; color: Design.Colors.textSecondary; width: 100 * s }
                        Text { id: durationLabel; anchors.verticalCenter: parent.verticalCenter
                               text: minDurationSlider.value + " 秒"; font.pixelSize: 13 * s; color: Design.Colors.primary; width: 60 * s }
                        Slider { id: minDurationSlider; width: parent.width - 170 * s; height: 30 * s
                            from: 0; to: 60; stepSize: 5
                            value: appContext ? appContext.getMinDurationSec() : 10
                            onMoved: { if (appContext) appContext.setMinDurationSec(value) }
                            background: Rectangle { x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - 2
                                width: parent.availableWidth; height: 4; radius: 2; color: Design.Colors.divider
                                Rectangle { width: parent.width * minDurationSlider.visualPosition
                                    height: parent.height; radius: 2; color: Design.Colors.primary } }
                            handle: Rectangle { x: minDurationSlider.visualPosition * (parent.width - width)
                                width: 20 * s; height: 20 * s; radius: 10 * s; color: Design.Colors.primary } }
                    }
                }
            }

            // ━━ 全局扫描按钮 ━━
            Rectangle {
                width: parent.width; height: 48 * s; radius: 24 * s
                color: appContext && (appContext.scanning || appContext.syncRunning) ? Design.Colors.divider : Design.Colors.primary
                Text { anchors.centerIn: parent; text: "开始全局扫描"
                    font.pixelSize: 16 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }
                MouseArea { anchors.fill: parent; anchors.margins: -4 * s
                    enabled: appContext && !appContext.scanning && !appContext.syncRunning
                    onClicked: { if (appContext) { appContext.scanMusicNow() } } }
            }

            // ━━ 查看已删除的音乐 ━━
            Rectangle {
                width: parent.width; height: 48 * s; radius: 24 * s
                color: Design.Colors.surface
                border { width: 1; color: appContext && appContext.deletedMusicCount > 0 ? Design.Colors.error : Design.Colors.divider }
                Text { anchors.centerIn: parent
                    text: "查看已删除的音乐 (" + (appContext ? appContext.deletedMusicCount : 0) + ")"
                    font.pixelSize: 14 * s; font.weight: Font.Medium
                    color: appContext && appContext.deletedMusicCount > 0 ? Design.Colors.error : Design.Colors.textSecondary }
                MouseArea { anchors.fill: parent; anchors.margins: -4 * s
                    onClicked: deletedMusicPage.open() }
            }

            // ━━ 已发现的文件夹 ━━
            Rectangle {
                width: parent.width
                height: Math.max(80 * s, 46 * s + folderRepeater.count * 58 * s)
                radius: 14 * s; color: Design.Colors.surface

                Column {
                    anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s; topMargin: 16 * s }
                    spacing: 4 * s

                    Text { text: "已发现的文件夹 (" + folderRepeater.count + " 个)"
                        font.pixelSize: 14 * s; font.weight: Font.Bold; color: Design.Colors.textPrimary }

                    Text {
                        text: "暂无已发现的文件夹\n点击上方「开始全局扫描」进行扫描"
                        font.pixelSize: 13 * s; color: Design.Colors.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        anchors { left: parent.left; right: parent.right }
                        visible: folderRepeater.count === 0
                        lineHeight: 1.5
                    }

                    Repeater {
                        id: folderRepeater
                        model: scanFoldersModel

                        Rectangle {
                            width: parent.width; height: 52 * s; color: "transparent"

                            // 复选框 — 左侧固定
                            Rectangle {
                                id: checkBox
                                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                                width: 24 * s; height: 24 * s; radius: 4 * s
                                color: model.enabled ? Design.Colors.primary : Design.Colors.divider
                                Image { anchors.centerIn: parent; width: 14 * s; height: 14 * s
                                    source: model.enabled ? "qrc:/img/checkmark.svg" : ""
                                    fillMode: Image.PreserveAspectFit }
                                MouseArea { anchors.fill: parent; anchors.margins: -6 * s
                                    onClicked: { var nv = !model.enabled
                                        scanFoldersModel.setProperty(index, "enabled", nv)
                                        if (appContext) appContext.setScanFolderEnabled(model.path, nv) } }
                            }

                            // 歌曲数量 — 右侧固定
                            Text {
                                id: countText
                                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                                text: model.musicCount + " 首"
                                font.pixelSize: 13 * s; font.weight: Font.Medium
                                color: model.enabled ? Design.Colors.primary : Design.Colors.textSecondary
                                width: 60 * s; horizontalAlignment: Text.AlignRight
                            }

                            // 文件夹名 + 路径 — 填充中间
                            Column {
                                anchors {
                                    left: checkBox.right; leftMargin: 10 * s
                                    right: countText.left; rightMargin: 10 * s
                                    verticalCenter: parent.verticalCenter
                                }
                                spacing: 2 * s

                                Text {
                                    text: model.folderName || model.path.split("/").filter(function(s) { return s }).pop() || model.path
                                    font.pixelSize: 14 * s; font.weight: Font.Medium
                                    color: model.enabled ? Design.Colors.textPrimary : Design.Colors.textSecondary
                                    elide: Text.ElideRight; width: parent.width; maximumLineCount: 1
                                }
                                Text {
                                    text: model.path || ""
                                    font.pixelSize: 11 * s; color: model.enabled ? "#888" : Design.Colors.textHint
                                    elide: Text.ElideMiddle; width: parent.width; maximumLineCount: 1
                                }
                            }

                            // 分割线
                            Rectangle {
                                anchors { left: parent.left; leftMargin: 34 * s; right: parent.right; bottom: parent.bottom }
                                height: 0.5; color: Design.Colors.divider
                            }
                        }
                    }
                }
            }

            // 底部留白
            Item { width: 1; height: 20 * s }
        }
    }

    // ═══════ 文件夹数据模型 ═══════
    ListModel { id: scanFoldersModel }

    function refreshFolderList() {
        scanFoldersModel.clear()
        if (!appContext) return
        // 列表为空且未在扫描中 → 从已有音乐反推文件夹
        var list = appContext.getScanFolders()
        if (list.length === 0 && !appContext.scanning) {
            appContext.rebuildScanFoldersFromLibrary()
            list = appContext.getScanFolders()
        }
        for (var i = 0; i < list.length; i++) {
            var path = list[i].path || ""
            var parts = path.split("/").filter(function(s) { return s.length > 0 })
            var folderName = parts.length > 0 ? parts[parts.length - 1] : path
            scanFoldersModel.append({
                path: path, folderName: folderName,
                musicCount: list[i].musicCount, enabled: list[i].enabled })
        }
    }

    // ═══════ 开关动画 ═══════
    function open() {
        root.visible = true
        x = 0
        refreshFolderList()
    }
    function close() {
        x = parent.width
        closeTimer.start()
    }

    function hasSubPageOpen() {
        return deletedMusicPage.visible
    }

    function closeSubPage() {
        deletedMusicPage.close()
    }

    Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

    Timer {
        id: closeTimer
        interval: 300
        onTriggered: root.visible = false
    }

    // 扫描完成后自动刷新文件夹列表
    Connections {
        target: appContext
        function onScanCompleted() { refreshFolderList() }
        function onDeletedMusicCountChanged() { /* 数量变化由 deletedMusicCount 属性自动驱动 */ }
    }

    // ═══════ 已删除音乐三级页面 ═══════
    DeletedMusicPage {
        id: deletedMusicPage
    }
}
