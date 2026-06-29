import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import "../component"

/* 本地音乐页面 */
Item {
    id: page

    readonly property real s: Design.Responsive.guiScale


    // 搜索状态
    property bool searchActive: false
    property string searchKeyword: ""
    property var searchResults: []
    property int highlightRow: -1

    Rectangle { anchors.fill: parent; color: Design.Colors.background }

    // ===== 搜索栏(独立于标题栏，始终在最顶部) =====
    Rectangle {
        id: searchBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: searchActive ? 48 * s : 0
        color: Design.Colors.surface
        clip: true
        z: 1
        visible: searchActive

        Row {
            anchors { fill: parent; leftMargin: 8 * s; rightMargin: 8 * s }
            spacing: 8 * s

            // 返回/取消按钮
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 40 * s; height: 32 * s; radius: 6 * s
                color: "transparent"

                Image {
                    anchors.centerIn: parent
                    width: 18 * s; height: 18 * s
                    source: "qrc:/img/back-arrow.svg"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4 * s
                    onClicked: closeSearch()
                }
            }

            // 搜索输入框
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 48 * s; height: 36 * s
                radius: 8 * s
                color: "#15ffffff"

                TextInput {
                    id: searchField
                    anchors { fill: parent; leftMargin: 12 * s; rightMargin: 34 * s }
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15 * s
                    color: Design.Colors.textPrimary
                    clip: true
                    focus: searchActive
                    activeFocusOnPress: true

                    Text {
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        text: "搜索音乐..."
                        font.pixelSize: 15 * s
                        color: "#666"
                        visible: !searchField.text && !searchField.activeFocus
                    }

                    onTextChanged: {
                        searchKeyword = text
                        debounceTimer.restart()
                    }

                    Keys.onEscapePressed: { closeSearch() }
                }

                Image {
                    anchors { right: parent.right; rightMargin: 10 * s; verticalCenter: parent.verticalCenter }
                    width: 14 * s; height: 14 * s
                    source: "qrc:/img/close-x.svg"
                    fillMode: Image.PreserveAspectFit
                    visible: searchField.text

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -6 * s
                        onClicked: { searchField.text = ""; searchResults = [] }
                    }
                }
            }
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 0.5; color: Design.Colors.divider
        }
    }

    // ===== 标题栏 =====
    Rectangle {
        id: topBar
        anchors { top: searchBar.bottom; left: parent.left; right: parent.right }
        height: 50 * s
        color: Design.Colors.surface

        RowLayout {
            anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s }

            Text {
                text: "本地音乐"; font.pixelSize: 20 * s; font.bold: true
                color: Design.Colors.textPrimary; Layout.fillWidth: true
            }
            Text {
                text: qsTr("共 %1 首").arg(musicModel ? musicModel.count : 0); font.pixelSize: 13 * s
                color: Design.Colors.textSecondary; visible: !appContext.scanning
            }

            // 搜索图标
            Rectangle {
                width: 32 * s; height: 32 * s; radius: 16 * s
                color: "#15ffffff"

                Image {
                    anchors.centerIn: parent
                    width: 16 * s; height: 16 * s
                    source: "qrc:/img/search.svg"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4 * s
                    onClicked: {
                        if (searchActive) { closeSearch() }
                        else { openSearch() }
                    }
                }
                visible: !appContext.scanning
            }
        }
    }

    // ===== 歌曲列表 =====
    ListView {
        id: listView
        anchors { top: topBar.bottom; left: parent.left; right: alphabetIndex.left; bottom: parent.bottom }
        clip: true
        model: musicModel
        spacing: 0
        reuseItems: true
        cacheBuffer: 64 * s * 3

        Label {
            anchors.centerIn: parent
            text: "暂无音乐文件"
            color: Design.Colors.textSecondary
            font.pixelSize: 15 * s
            visible: listView.count === 0 && !appContext.scanning
        }

        delegate: Item {
            width: ListView.view.width
            height: 64 * s

            Rectangle {
                anchors.fill: parent
                color: index === page.highlightRow ? "#30ffeb3b" : "transparent"
                visible: page.highlightRow >= 0
            }

            MusicListItem {
                anchors.fill: parent
                musicTitle: model.title || ""
                musicArtist: model.artist || ""
                musicDuration: model.duration || ""
                coverSource: model.coverArt || ""
                musicQuality: model.quality || ""
                qualityColor: model.qualityColor || ""
                musicHash: model.musicHash || ""
                musicFilePath: model.filePath || ""
                fileMissing: model.fileExists === false

                onClicked: {
                    if (musicFilePath && player) {
                        var all = musicModel.getAllItems()
                        player.setQueueSourceType(0)
                        player.setPlayQueue(0, all)
                        player.playAt(index)
                    }
                }

                onShowActions: {
                    actionSheet.open({
                        musicHash: model.musicHash || "",
                        musicFilePath: model.filePath || "",
                        musicTitle: model.title || "",
                        musicArtist: model.artist || ""
                    })
                }
            }
        }
    }

    // ===== A-Z 字母索引 =====
    AlphabetIndex {
        id: alphabetIndex
        anchors { right: parent.right; rightMargin: 2 * s; top: topBar.bottom; bottom: parent.bottom }
        width: 22 * s
        s: page.s
        listView: listView
        activeColor: Design.Colors.primary
        normalColor: Design.Colors.textSecondary
        visible: listView.count > 0 && !searchActive
        z: 10
    }

    // ===== 空白区域关闭层 =====
    Rectangle {
        anchors { top: topBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        color: Design.Colors.background
        visible: searchActive
        z: 50

        MouseArea {
            anchors.fill: parent
            onClicked: closeSearch()
        }
    }

    // ===== 搜索结果浮动层 =====
    Rectangle {
        id: resultsOverlay
        anchors { top: topBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        color: Design.Colors.background
        visible: searchActive && searchResults && searchResults.length > 0
        z: 100

        ListView {
            id: resultsListView
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: Math.min(resultsListView.count * 60 * s, parent.height)
            clip: true
            model: searchResults
            spacing: 0
            reuseItems: true

            delegate: Rectangle {
                width: ListView.view.width
                height: 60 * s
                color: "transparent"

                Row {
                    anchors { fill: parent; leftMargin: 16 * s; rightMargin: 16 * s }
                    spacing: 12 * s

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 40 * s; height: 40 * s; radius: 6 * s
                        color: "#1affffff"

                        Image {
                            anchors.fill: parent; anchors.margins: 1 * s
                            asynchronous: true
                            cache: true
                            source: modelData.coverPath || ""
                            visible: status === Image.Ready
                            fillMode: Image.PreserveAspectCrop
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 52 * s
                        spacing: 2 * s

                        Text {
                            text: modelData.titleHighlighted || modelData.title || ""
                            font.pixelSize: 14 * s
                            color: Design.Colors.textPrimary
                            elide: Text.ElideRight
                            textFormat: modelData.titleHighlighted ? Text.RichText : Text.PlainText
                            width: parent.width
                            maximumLineCount: 1
                        }

                        Text {
                            text: modelData.artistHighlighted || modelData.artist || ""
                            font.pixelSize: 12 * s
                            color: Design.Colors.textSecondary
                            elide: Text.ElideRight
                            textFormat: modelData.artistHighlighted ? Text.RichText : Text.PlainText
                            width: parent.width
                            maximumLineCount: 1
                        }
                    }
                }

                Rectangle {
                    anchors { left: parent.left; leftMargin: 68 * s; right: parent.right; bottom: parent.bottom }
                    height: 0.5
                    color: Design.Colors.divider
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: activateResult(index)
                }
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("未找到 %1 相关歌曲").arg(searchKeyword)
                color: Design.Colors.textSecondary
                font.pixelSize: 14 * s
                visible: resultsListView.count === 0 && searchKeyword
            }
        }
    }

    // 音乐操作面板
    MusicActionSheet {
        id: actionSheet
        onEditMetadata: {
            metadataSheet.open({
                musicHash: actionSheet.musicHash,
                musicTitle: actionSheet.musicTitle,
                musicArtist: actionSheet.musicArtist
            })
        }
    }

    // 元数据编辑器
    MetadataEditSheet {
        id: metadataSheet
    }

    // 防抖定时器
    Timer {
        id: debounceTimer
        interval: 150
        onTriggered: performSearch()
    }

    // 高亮清除定时器
    Timer {
        id: highlightTimer
        interval: 2500
        onTriggered: { page.highlightRow = -1 }
    }

    function performSearch() {
        var kw = searchKeyword.trim()
        if (!kw || !musicModel) {
            searchResults = []
            return
        }
        try {
            searchResults = musicModel.search(kw, 20)
        } catch (e) {
            searchResults = []
        }
    }

    function activateResult(idx) {
        if (!searchResults || idx >= searchResults.length) return
        var item = searchResults[idx]
        if (!item) return

        var modelIdx = item.modelIndex
        if (modelIdx !== undefined && modelIdx >= 0) {
            listView.positionViewAtIndex(modelIdx, ListView.Center)
            page.highlightRow = modelIdx
            highlightTimer.restart()
        }
        closeSearch()
    }

    function openSearch() {
        searchActive = true
        searchField.text = ""
        searchKeyword = ""
        searchResults = []
        searchField.forceActiveFocus()
    }

    function closeSearch() {
        searchActive = false
        searchField.text = ""
        searchKeyword = ""
        searchResults = []
    }

    property var musicModel: null
    property var player: appContext.playerController
    Component.onCompleted: { musicModel = appContext.getViewModel("local_music") }
}
