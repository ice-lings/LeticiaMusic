import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* 重复歌曲管理页面 — 多选保留，试听对比 */
Item {
    id: root

    x: 5000
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    z: 200
    visible: false

    readonly property real s: Design.Responsive.guiScale

    readonly property color clrBg:           "#191925"
    readonly property color clrSurface:      "#1e1e2e"
    readonly property color clrRowNormal:    "#232338"
    readonly property color clrRowKeep:      "#1a3028"
    readonly property color clrDivider:      "#2a2a3d"
    readonly property color clrPrimary:      Design.Colors.primary
    readonly property color clrSuccess:      "#5cb87a"
    readonly property color clrTextPrimary:  Design.Colors.textPrimary
    readonly property color clrTextSecondary:"#a0a0b8"

    property var duplicateGroups: []
    property var _selected: ({})

    function qualityColor(quality) {
        switch (quality) {
            case "HI": return "#E5C07B"
            case "SQ": return "#C678DD"
            case "HQ": return "#61AFEF"
            case "SD": return "#98C379"
            default:   return Design.Colors.textDisabled
        }
    }

    function selectedCount(groupKey) {
        var arr = _selected[groupKey]
        return arr ? arr.length : 0
    }

    function toggleSelect(groupKey, fileHash) {
        var copy = JSON.parse(JSON.stringify(_selected))
        if (!copy[groupKey]) copy[groupKey] = []
        var idx = copy[groupKey].indexOf(fileHash)
        if (idx >= 0) copy[groupKey].splice(idx, 1)
        else copy[groupKey].push(fileHash)
        _selected = copy
    }

    function isSelected(groupKey, fileHash) {
        var arr = _selected[groupKey]
        return arr ? arr.indexOf(fileHash) >= 0 : false
    }

    function resolveGroup(groupKey, entries) {
        var keepList = _selected[groupKey] || []
        var keep = keepList.length > 0 ? keepList[0] : ""
        var del = []
        for (var i = 0; i < entries.length; i++) {
            var h = entries[i].fileHash
            if (keepList.indexOf(h) < 0) del.push(h)
        }
        if (del.length > 0) appContext.resolveDuplicate(keep, del)
        else {
            for (var j = 0; j < keepList.length; j++) {
                appContext.resolveDuplicate(keepList[j], [])
            }
        }
        delete _selected[groupKey]
        var groups = []
        for (var gi = 0; gi < duplicateGroups.length; gi++) {
            if (duplicateGroups[gi].groupKey !== groupKey) groups.push(duplicateGroups[gi])
        }
        duplicateGroups = groups
    }

    Rectangle { anchors.fill: parent; color: clrBg }

    // 标题栏
    Rectangle {
        id: titleBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 48 * s; color: clrSurface
        Row {
            anchors { fill: parent; leftMargin: 14 * s; rightMargin: 14 * s }

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 18 * s; height: 18 * s
                source: "qrc:/img/back-arrow.svg"
                fillMode: Image.PreserveAspectFit
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter; anchors.horizontalCenter: parent.horizontalCenter
                text: "重复歌曲" + (duplicateGroups && duplicateGroups.length > 0 ? " (" + duplicateGroups.length + " 组)" : "")
                font.pixelSize: 18 * s; font.weight: Font.Medium; color: clrTextPrimary
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.close()
            }
        }
    }

    Rectangle {
        id: titleDivider
        anchors { top: titleBar.bottom; left: parent.left; right: parent.right }
        height: 1; color: clrDivider
    }

    // 内容
    ListView {
        id: listView
        anchors { top: titleDivider.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        clip: true
        model: duplicateGroups || []
        spacing: 14 * s
        reuseItems: true
        topMargin: 10 * s
        bottomMargin: 14 * s

        delegate: Rectangle {
            width: parent.width - 24 * s
            height: groupCard.height + 14 * s
            anchors.horizontalCenter: parent.horizontalCenter
            radius: 12 * s
            color: clrSurface

            property string gKey: modelData.groupKey || ""

            Column {
                id: groupCard
                width: parent.width - 28 * s
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 10 * s
                spacing: 0

                // 组标题 — 仅歌名
                Text {
                    width: parent.width
                    text: modelData.displayTitle || "未知歌曲"
                    font.pixelSize: 16 * s; font.weight: Font.Medium
                    color: clrTextPrimary
                    elide: Text.ElideRight
                }

                Item { width: 1; height: 6 * s }

                // 条目列表
                Repeater {
                    model: modelData.entries || []
                    delegate: Rectangle {
                        width: parent.width; height: 60 * s
                        color: root.isSelected(gKey, modelData.fileHash) ? clrRowKeep : clrRowNormal
                        radius: 6 * s

                        Rectangle {
                            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                            anchors.leftMargin: 8 * s; anchors.rightMargin: 8 * s
                            height: 0.5; color: clrDivider
                        }

                        MouseArea {
                            anchors { fill: parent; rightMargin: 52 * s }
                            onClicked: root.toggleSelect(gKey, modelData.fileHash)
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10 * s; anchors.rightMargin: 10 * s
                            spacing: 10 * s

                            // 复选框
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 22 * s; height: 22 * s; radius: 4 * s
                                color: root.isSelected(gKey, modelData.fileHash) ? clrPrimary : "transparent"
                                border { width: 1.5; color: root.isSelected(gKey, modelData.fileHash) ? clrPrimary : Design.Colors.textHint }
                                Image {
                                    anchors.centerIn: parent
                                    width: 12 * s; height: 12 * s
                                    source: "qrc:/img/checkmark.svg"
                                    fillMode: Image.PreserveAspectFit
                                    visible: root.isSelected(gKey, modelData.fileHash)
                                }
                            }

                            // 封面
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 40 * s; height: 40 * s; radius: 6 * s
                                color: "#15ffffff"
                                clip: true
                                Image {
                                    anchors.fill: parent
                                    source: modelData.coverPath || ""
                                    fillMode: Image.PreserveAspectCrop
                                    visible: source.toString() !== ""
                                    asynchronous: true; cache: true; smooth: true
                                }
                                Image {
                                    anchors.centerIn: parent
                                    width: 18 * s; height: 18 * s
                                    source: "qrc:/img/music-note.svg"
                                    fillMode: Image.PreserveAspectFit
                                    visible: !modelData.coverPath || modelData.coverPath === ""
                                }
                            }

                            // 文件信息
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 22*s - 10*s - 40*s - 10*s - 10*s - 36*s
                                spacing: 3 * s

                                Text {
                                    width: parent.width
                                    text: modelData.title || modelData.filePath.split("/").pop() || "未知文件"
                                    font.pixelSize: 14 * s; color: clrTextPrimary
                                    elide: Text.ElideRight; maximumLineCount: 1
                                }
                                Row {
                                    spacing: 4 * s; width: parent.width
                                    Text {
                                        width: Math.min(80 * s, parent.width - durText.contentWidth - qualityText.contentWidth - 12 * s)
                                        text: modelData.artist || ""
                                        font.pixelSize: 12 * s; color: clrTextSecondary
                                        elide: Text.ElideRight; maximumLineCount: 1
                                    }
                                    Text {
                                        id: durText
                                        text: modelData.duration || ""
                                        font.pixelSize: 12 * s; color: clrTextSecondary
                                    }
                                    Text {
                                        id: qualityText
                                        text: modelData.quality || ""
                                        font.pixelSize: 12 * s; font.weight: Font.Bold
                                        color: root.qualityColor(modelData.quality)
                                        visible: modelData.quality && modelData.quality.length > 0
                                    }
                                }
                            }

                            // 播放按钮
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 36 * s; height: 36 * s; radius: 18 * s
                                color: "#18ffffff"
                                Image {
                                    anchors.centerIn: parent
                                    width: 16 * s; height: 16 * s
                                    source: "qrc:/img/play-circle.svg"
                                    fillMode: Image.PreserveAspectFit
                                }
                                MouseArea {
                                    anchors.fill: parent; anchors.margins: -4 * s
                                    onClicked: {
                                        if (appContext && appContext.playerController && modelData.filePath) {
                                            appContext.playerController.playFile(modelData.filePath)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 保留所选按钮
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 8 * s
                    width: 110 * s; height: 30 * s
                    text: "保留所选" + (root.selectedCount(gKey) > 0 ? " (" + root.selectedCount(gKey) + ")" : "")
                    font.pixelSize: 12 * s
                    enabled: root.selectedCount(gKey) > 0
                    onClicked: root.resolveGroup(gKey, modelData.entries || [])
                    background: Rectangle { color: parent.enabled ? clrSuccess : "#3a3a50"; radius: 15 * s }
                    contentItem: Text { text: parent.text; color: parent.enabled ? "white" : clrTextSecondary; font.pixelSize: 12 * s; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    // 空状态
    Text {
        anchors.centerIn: listView
        text: duplicateGroups && duplicateGroups.length > 0 ? "" : "未发现重复歌曲"
        font.pixelSize: 15 * s; color: clrTextSecondary
        visible: !duplicateGroups || duplicateGroups.length === 0
    }

    Behavior on x {
        NumberAnimation { duration: 250; easing.type: Easing.OutQuad }
    }
    Behavior on visible {
        NumberAnimation { duration: 250 }
    }

    function open() {
        visible = true
        duplicateGroups = appContext ? appContext.findDuplicateSongs() : []
        _selected = {}
        x = 0
    }

    function close() {
        x = parent ? parent.width : 420
        closeTimer.start()
    }

    Timer {
        id: closeTimer
        interval: 280
        onTriggered: visible = false
    }
}
