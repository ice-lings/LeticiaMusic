import QtQuick

Item {
    id: root

    property ListView listView: null
    property var letters: ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "#"]

    property string currentSection: ""

    // 分组优先级: A-Z = 0..25, '0' = 26, '#' = 27
    function sectionPriority(key) {
        return Tools.sectionPriority(key)
    }

    function updateCurrentSection() {
        if (!listView || listView.count === 0) return
        var idx = listView.indexAt(listView.contentX + 1, listView.contentY + listView.height / 2)
        if (idx < 0 || idx >= listView.count) {
            if (listView.contentY <= 0) {
                idx = 0
            } else {
                idx = listView.count - 1
            }
        }
        var item = listView.model.get(idx)
        if (item && item.sectionKey) {
            currentSection = item.sectionKey
        }
    }

    Connections {
        target: listView
        function onContentYChanged() { root.updateCurrentSection() }
        function onCountChanged() { root.updateCurrentSection() }
    }

    Component.onCompleted: root.updateCurrentSection()

    Rectangle {
        anchors.fill: parent
        color: Design.Colors.surface
        radius: 14
        opacity: 0.6
    }

    ListView {
        id: letterList
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        interactive: false
        model: root.letters

        delegate: Item {
            id: letterDelegate
            width: root.width
            height: letterList.height / root.letters.length

            property string letter: modelData
            property bool isActive: letter === root.currentSection
            property bool isHovered: letterMouseArea.containsMouse

            Text {
                anchors.centerIn: parent
                text: letterDelegate.letter
                font.pixelSize: Math.max(8, Math.min(11, letterDelegate.height - 4))
                font.bold: letterDelegate.isActive
                color: letterDelegate.isActive ? Design.Colors.primary
                       : letterDelegate.isHovered ? Design.Colors.textPrimary
                       : Design.Colors.textDisabled
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: letterMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    root.jumpToSection(letterDelegate.letter)
                }
            }
        }
    }

    // 跳转到指定字母的第一项；若该字母没有歌曲，则跳到优先级更大的下一个已有字母的第一项。
    function jumpToSection(letter) {
        if (!listView || !listView.model || listView.count === 0) return

        var targetPri = sectionPriority(letter)

        // 找到所有出现过的 sectionKey 对应的首个索引
        var firstIndexByKey = {}
        for (var i = 0; i < listView.count; i++) {
            var item = listView.model.get(i)
            if (!item) continue
            var key = item.sectionKey || "#"
            if (!(key in firstIndexByKey)) {
                firstIndexByKey[key] = i
            }
        }

        // 先尝试精确匹配
        if (letter in firstIndexByKey) {
            listView.positionViewAtIndex(firstIndexByKey[letter], ListView.Beginning)
            listView.currentIndex = firstIndexByKey[letter]
            return
        }

        // 没有精确匹配，找优先级 >= 目标的第一个已存在分组
        var bestKey = null
        var bestPri = 99
        for (var k in firstIndexByKey) {
            var pri = sectionPriority(k)
            if (pri >= targetPri && pri < bestPri) {
                bestPri = pri
                bestKey = k
            }
        }

        if (bestKey !== null) {
            listView.positionViewAtIndex(firstIndexByKey[bestKey], ListView.Beginning)
            listView.currentIndex = firstIndexByKey[bestKey]
            return
        }

        // 如果目标字母之后没有任何分组，就跳到列表最后
        listView.positionViewAtEnd()
    }
}
