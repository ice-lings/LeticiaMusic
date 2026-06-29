import QtQuick
import QtQuick.Controls

Item {
    id: root
    implicitHeight: 34

    property var searchModel: null
    property int maxResults: 20
    signal resultClicked(int modelIndex)

    function clearSearch() {
        searchInput.text = ""
        searchInput.focus = false
    }

    Rectangle {
        id: searchField
        anchors.fill: parent
        radius: height / 2
        color: Design.Colors.buttonNormal
        border.color: searchInput.activeFocus
                      ? Design.Colors.primary
                      : Design.Colors.divider
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: 150 } }

        Text {
            text: qsTr("🔍")
            font.pixelSize: 14
            color: Design.Colors.textSecondary
            anchors {
                left: parent.left
                leftMargin: 12
                verticalCenter: parent.verticalCenter
            }
        }

        TextField {
            id: searchInput
            anchors.fill: parent
            leftPadding: 36
            rightPadding: clearBtn.visible ? 34 : 12
            color: Design.Colors.textPrimary
            font.pixelSize: 13
            verticalAlignment: TextField.AlignVCenter
            placeholderText: qsTr("搜索歌曲...")
            placeholderTextColor: Design.Colors.textSecondary
            background: null
            selectByMouse: true

            onTextChanged: {
                if (text.trim().length === 0) {
                    resultsPopup.close()
                } else {
                    searchTimer.restart()
                }
            }

            Keys.onUpPressed: {
                if (resultsPopup.visible && resultsListView.count > 0) {
                    resultsListView.decrementCurrentIndex()
                }
            }

            Keys.onDownPressed: {
                if (resultsPopup.visible && resultsListView.count > 0) {
                    resultsListView.incrementCurrentIndex()
                }
            }

            Keys.onReturnPressed: {
                if (resultsPopup.visible && resultsListView.currentIndex >= 0) {
                    activateResult(resultsListView.currentIndex)
                }
            }

            Keys.onEscapePressed: {
                clearSearch()
            }
        }

        Text {
            id: clearBtn
            text: qsTr("✕")
            font.pixelSize: 13
            color: Design.Colors.textSecondary
            visible: searchInput.text.length > 0
            anchors {
                right: parent.right
                rightMargin: 12
                verticalCenter: parent.verticalCenter
            }

            MouseArea {
                anchors.fill: parent
                anchors.margins: -6
                cursorShape: Qt.PointingHandCursor
                onClicked: root.clearSearch()
            }
        }
    }

    Timer {
        id: searchTimer
        interval: 150
        onTriggered: refreshResults()
    }

    function refreshResults() {
        var kw = searchInput.text.trim()
        if (kw.length === 0) {
            resultsPopup.close()
            return
        }

        var list = root.searchModel ? root.searchModel.search(kw, root.maxResults) : []
        resultListModel.clear()
        for (var i = 0; i < list.length; i++) {
            resultListModel.append(list[i])
        }
        resultsListView.currentIndex = -1

        if (resultListModel.count > 0) {
            if (!resultsPopup.opened) {
                resultsPopup.x = 0
                resultsPopup.y = searchField.height + 4
                resultsPopup.open()
            }
            resultsPopup.height = Math.min(
                resultListModel.count * 56 + 16,
                window.height * 0.4)
        } else if (resultsPopup.opened) {
            resultsPopup.height = 56
        }
    }

    function activateResult(idx) {
        if (idx < 0 || idx >= resultListModel.count) return
        var item = resultListModel.get(idx)
        if (item && item.modelIndex !== undefined) {
            var mi = item.modelIndex
            root.resultClicked(mi)
            root.clearSearch()
        }
    }

    ListModel {
        id: resultListModel
    }

    Popup {
        id: resultsPopup
        width: searchField.width
        height: 100
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        onClosed: {
            resultListModel.clear()
        }

        background: Rectangle {
            color: Design.Colors.surface
            border.color: Design.Colors.divider
            border.width: 1
            radius: 8
        }

        contentItem: Column {
            ListView {
                id: resultsListView
                width: parent.width
                height: resultsPopup.height - resultsPopup.topPadding - resultsPopup.bottomPadding
                clip: true
                model: resultListModel
                spacing: 2
                currentIndex: -1
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    width: resultsListView.width
                    height: 52
                    radius: 6
                    color: mouseArea.containsMouse
                           ? Design.Colors.withAlpha(Design.Colors.primary, 0.2)
                           : (resultsListView.currentIndex === index
                              ? Design.Colors.withAlpha(Design.Colors.primary, 0.15)
                              : "transparent")

                    Behavior on color { ColorAnimation { duration: 100 } }

                    Row {
                        anchors {
                            fill: parent
                            leftMargin: 8
                            rightMargin: 8
                        }
                        spacing: 10

                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 40; height: 40
                            source: model.coverPath || "qrc:/img/assets/images/default_music_cover.png"
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: false
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2

                            Text {
                                text: model.titleHighlighted || model.title || "Unknown"
                                textFormat: Text.StyledText
                                color: Design.Colors.textPrimary
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                                width: resultsListView.width - 85
                            }

                            Text {
                                text: model.artistHighlighted || model.artist || ""
                                textFormat: Text.StyledText
                                color: Design.Colors.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                width: resultsListView.width - 85
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activateResult(index)
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: searchInput.text.trim().length > 0
                          ? qsTr("未找到相关歌曲")
                          : ""
                    color: Design.Colors.textSecondary
                    font.pixelSize: 13
                    visible: resultListModel.count === 0
                }
            }
        }
    }
}
