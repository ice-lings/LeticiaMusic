import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 已删除的音乐 — 三级页面，从 ScanSettingsPage 右滑进入 */
Item {
    id: root

    x: parent ? parent.width : 0
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    z: 250
    visible: false

    readonly property real s: Design.Responsive.guiScale

    readonly property color clrDanger: Design.Colors.error

    Rectangle { anchors.fill: parent; color: Design.Colors.background }

    // ═══════ 搜索栏 ═══════
    Rectangle {
        id: searchBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: searchActive ? 48 * s : 0
        clip: true
        z: 2

        Rectangle {
            anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s; topMargin: 6 * s; bottomMargin: 6 * s }
            radius: 8 * s; color: Design.Colors.surface

            Row {
                anchors { fill: parent; leftMargin: 12 * s; rightMargin: 8 * s }
                spacing: 8 * s

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18 * s; height: 18 * s
                    source: "qrc:/img/back-arrow.svg"
                    fillMode: Image.PreserveAspectFit
                    MouseArea { anchors.fill: parent; anchors.margins: -8 * s
                        onClicked: closeSearch() }
                }

                TextInput {
                    id: searchInput
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 50 * s
                    font.pixelSize: 14 * s; color: Design.Colors.textPrimary

                    Text {
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        text: "搜索已删除的音乐..."
                        font.pixelSize: 14 * s; color: "#666"
                        visible: !searchInput.text && !searchInput.activeFocus
                    }

                    onTextChanged: {
                        searchKeyword = text
                        debounceTimer.restart()
                    }
                }

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16 * s; height: 16 * s
                    source: "qrc:/img/close-x.svg"
                    fillMode: Image.PreserveAspectFit
                    visible: searchInput.text.length > 0
                    MouseArea { anchors.fill: parent; anchors.margins: -6 * s
                        onClicked: { searchInput.text = ""; searchKeyword = ""; performSearch() } }
                }
            }
        }
    }

    signal musicRestored()
    property bool searchActive: false
    property string searchKeyword: ""
    property var fullList: []
    property var _sel: []  // 选中哈希列表 — 始终通过赋值修改以触发绑定

    function openSearch() {
        searchActive = true
        searchInput.text = ""
        searchKeyword = ""
        searchInput.focus = true
    }
    function closeSearch() {
        searchActive = false
        searchInput.text = ""
        searchKeyword = ""
        performSearch()
    }

    function performSearch() {
        var kw = searchKeyword.trim().toLowerCase()
        deletedMusicModel.clear()
        for (var i = 0; i < fullList.length; i++) {
            var item = fullList[i]
            if (kw.length === 0 || item.title.toLowerCase().indexOf(kw) >= 0) {
                deletedMusicModel.append(item)
            }
        }
        _sel = []
    }

    Timer {
        id: debounceTimer
        interval: 150
        onTriggered: performSearch()
    }

    // ═══════ 顶部导航栏 ═══════
    Rectangle {
        id: topBar
        anchors { top: searchBar.bottom; left: parent.left; right: parent.right }
        height: 50 * s; color: Design.Colors.surface
        z: 1

        Row {
            anchors { left: parent.left; leftMargin: 12 * s; verticalCenter: parent.verticalCenter }
            spacing: 2 * s
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 15 * s; height: 15 * s
                source: "qrc:/img/back-arrow.svg"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                anchors.fill: parent; anchors.margins: -8 * s
                onClicked: root.close()
            }
        }

        Text {
            anchors.centerIn: parent
            text: "已删除的音乐 (" + deletedMusicModel.count + " 首)"
            font.pixelSize: 17 * s; font.weight: Font.Bold
            color: Design.Colors.textPrimary
        }

        Row {
            anchors { right: parent.right; rightMargin: 8 * s; verticalCenter: parent.verticalCenter }

            // 搜索图标
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 18 * s; height: 18 * s
                source: "qrc:/img/search.svg"
                fillMode: Image.PreserveAspectFit
                visible: !searchActive
                MouseArea { anchors.fill: parent; anchors.margins: -8 * s
                    onClicked: openSearch() }
            }
        }
    }

    // ═══════ 全选/全不选栏 ═══════
    Rectangle {
        id: selectBar
        anchors { top: topBar.bottom; left: parent.left; right: parent.right }
        height: deletedMusicModel.count > 0 ? 36 * s : 0
        clip: true
        color: Design.Colors.surface

        property bool _allSelected: false

        Row {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 16 * s }
            spacing: 10 * s

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 16 * s; height: 16 * s
                source: selectBar._allSelected ? "qrc:/img/checkbox-checked.svg" : ""
                fillMode: Image.PreserveAspectFit
                MouseArea { anchors.fill: parent; anchors.margins: -6 * s
                    onClicked: {
                        if (selectBar._allSelected) {
                            root._sel = []
                        } else {
                            var arr = []
                            for (var i = 0; i < deletedMusicModel.count; i++) {
                                var m = deletedMusicModel.get(i)
                                if (m.fileExists) arr.push(m.fileHash)
                            }
                            root._sel = arr
                        }
                        selectBar._allSelected = !selectBar._allSelected
                    }
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: selectBar._allSelected ? "取消全选" : "全选"
                font.pixelSize: 13 * s; color: Design.Colors.textSecondary
            }
        }
    }

    // ═══════ 内容列表 ═══════
    ListView {
        id: listView
        anchors { top: selectBar.bottom; left: parent.left; right: parent.right; bottom: bottomBar.top }
        clip: true
        model: ListModel { id: deletedMusicModel }
        reuseItems: true

        Text {
            anchors.centerIn: parent
            text: searchKeyword.length > 0 ? "没有匹配结果" : "暂无已删除的音乐"
            font.pixelSize: 14 * s; color: Design.Colors.textSecondary
            horizontalAlignment: Text.AlignHCenter
            visible: deletedMusicModel.count === 0
        }

        delegate: Rectangle {
            width: listView.width; height: 56 * s
            color: isSelected ? "#2a3f5f" : "transparent"

            property bool isSelected: root._sel.indexOf(model.fileHash) >= 0

            // 复选框 — 带可见边框
            Rectangle {
                anchors { left: parent.left; leftMargin: 12 * s; verticalCenter: parent.verticalCenter }
                width: 22 * s; height: 22 * s; radius: 4 * s
                color: isSelected ? Design.Colors.primary : "transparent"
                border { width: 1.5; color: isSelected ? Design.Colors.primary
                         : (model.fileExists ? Design.Colors.textHint : "#333") }
                opacity: model.fileExists ? 1 : 0.4

                Image {
                    anchors.centerIn: parent
                    width: 14 * s; height: 14 * s
                    source: isSelected ? "qrc:/img/checkmark.svg" : ""
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea { anchors.fill: parent; anchors.margins: -8 * s
                    enabled: model.fileExists
                    onClicked: {
                        var arr = root._sel.slice()
                        var idx = arr.indexOf(model.fileHash)
                        if (idx >= 0) arr.splice(idx, 1)
                        else arr.push(model.fileHash)
                        root._sel = arr
                        selectBar._allSelected = deletedMusicModel.count > 0 && root._sel.length === deletedMusicModel.count
                    }
                }
            }

            // 标题 + 时间
            Column {
                anchors {
                    left: parent.left; leftMargin: 46 * s
                    right: actionBtn.visible ? actionBtn.left : parent.right
                    rightMargin: actionBtn.visible ? 8 * s : 12 * s
                    verticalCenter: parent.verticalCenter
                }
                spacing: 2 * s

                Text {
                    text: model.title || "未知歌曲"
                    font.pixelSize: 14 * s; font.weight: Font.Medium
                    color: model.fileExists ? Design.Colors.textPrimary : Design.Colors.textSecondary
                    elide: Text.ElideRight; width: parent.width; maximumLineCount: 1
                }
                Text {
                    text: formatTime(model.deletedAt) + (model.fileExists ? "" : "  · 文件已丢失")
                    font.pixelSize: 11 * s
                    color: model.fileExists ? "#888" : clrDanger
                    elide: Text.ElideRight; width: parent.width; maximumLineCount: 1
                }
            }

            // "移除记录"按钮（文件已丢失时显示）
            Text {
                id: actionBtn
                anchors { right: parent.right; rightMargin: 12 * s; verticalCenter: parent.verticalCenter }
                text: "移除"
                font.pixelSize: 12 * s; color: clrDanger
                visible: !model.fileExists
                MouseArea { anchors.fill: parent; anchors.margins: -6 * s
                    onClicked: {
                        if (appContext) appContext.removeDeletedEntryRecord(model.fileHash)
                        loadDeletedList()
                    }
                }
            }

            // 分割线
            Rectangle {
                anchors { left: parent.left; leftMargin: 46 * s; right: parent.right; bottom: parent.bottom }
                height: 0.5; color: Design.Colors.divider
            }
        }
    }

    // ═══════ 底部操作栏 ═══════
    Rectangle {
        id: bottomBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: root._sel.length > 0 ? 56 * s : 0
        clip: true
        color: Design.Colors.surface

        Rectangle {
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            width: parent.width - 32 * s; height: 44 * s; radius: 22 * s
            color: Design.Colors.primary

            Text {
                anchors.centerIn: parent
                text: "恢复选中 (" + root._sel.length + ")"
                font.pixelSize: 15 * s; font.weight: Font.Bold
                color: "#fff"
            }
            MouseArea { anchors.fill: parent; anchors.margins: -4 * s
                onClicked: {
                    if (root._sel.length === 0) return
                    if (appContext) {
                        appContext.restoreDeletedMusicBatch(root._sel)
                        root._sel = []
                        loadDeletedList()
                        musicRestored()
                    }
                }
            }
        }
    }

    // ═══════ 点击空白区域关闭搜索 ═══════
    MouseArea {
        anchors { top: topBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        enabled: searchActive
        z: 5
        onClicked: closeSearch()
    }

    // ═══════ 数据加载 ═══════
    function loadDeletedList() {
        fullList = []
        _sel = []
        if (!appContext) return
        fullList = appContext.getDeletedMusicList()
        performSearch()
    }

    // ═══════ 时间格式化 ═══════
    function formatTime(timestamp) {
        return Tools.formatRelative(timestamp)
    }

    // ═══════ 开关动画 ═══════
    function open() {
        root.visible = true
        x = 0
        loadDeletedList()
    }
    function close() {
        x = parent.width
        closeTimer.start()
    }

    Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

    Timer {
        id: closeTimer
        interval: 300
        onTriggered: {
            root.visible = false
            searchActive = false
        }
    }
}
