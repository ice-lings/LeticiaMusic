import QtQuick

/* 安卓 A-Z 字母索引滚动条 (支持滑动 + 大号指示器) */
Item {
    id: root

    property ListView listView: null
    property real s: 1
    property color activeColor: Design.Colors.primary
    property color normalColor: Design.Colors.withAlpha(Design.Colors.iconNormal, 0.5)

    property var letters: ["A","B","C","D","E","F","G","H","I","J","K","L","M",
                           "N","O","P","Q","R","S","T","U","V","W","X","Y","Z","0","#"]
    property string currentSection: ""
    property bool touching: false
    property string touchLetter: ""
    property real touchY: 0

    function sectionPriority(key) {
        return Tools.sectionPriority(key)
    }

    function updateCurrentSection() {
        if (!listView || listView.count === 0) return
        var idx = listView.indexAt(listView.contentX + 1, listView.contentY + listView.height / 2)
        if (idx < 0 || idx >= listView.count) {
            idx = listView.contentY <= 0 ? 0 : listView.count - 1
        }
        var item = listView.model.get(idx)
        if (item && item.sectionKey) currentSection = item.sectionKey
    }

    Connections {
        target: listView
        function onContentYChanged() { if (!touching) root.updateCurrentSection() }
        function onCountChanged() { root.updateCurrentSection() }
    }

    Component.onCompleted: root.updateCurrentSection()

    // 字母列表
    Column {
        anchors.fill: parent

        Repeater {
            model: root.letters

            Item {
                width: parent.width
                height: parent.height / root.letters.length

                property string letter: modelData
                property bool isActive: touching ? (letter === root.touchLetter)
                                                 : (letter === root.currentSection)

                Text {
                    anchors.centerIn: parent
                    text: letter
                    font.pixelSize: Math.max(8 * s, Math.min(11 * s, parent.height - 2 * s))
                    font.bold: isActive
                    color: isActive ? root.activeColor : root.normalColor
                }
            }
        }
    }

    // 统一 MouseArea 实现滑动
    MouseArea {
        id: touchArea
        anchors.fill: parent
        anchors.margins: -16 * s    // 扩大触摸区域方便拇指操作

        function letterAt(y) {
            var idx = Math.floor(y / (root.height / root.letters.length))
            if (idx < 0) idx = 0
            if (idx >= root.letters.length) idx = root.letters.length - 1
            return root.letters[idx]
        }

        onPressed: {
            touching = true
            touchY = mouseY
            var lt = letterAt(mouseY)
            touchLetter = lt
            root.jumpToSection(lt)
        }

        onPositionChanged: {
            touchY = mouseY
            var lt = letterAt(mouseY)
            if (lt !== touchLetter) {
                touchLetter = lt
                root.jumpToSection(lt)
            }
        }

        onReleased: {
            touching = false
            touchLetter = ""
        }

        onExited: {
            touching = false
            touchLetter = ""
        }
    }

    // 滑动时的大号字母指示器 (手指左侧跟随 + 上下边界钳制)
    Rectangle {
        id: indicator
        x: -width - 10 * s
        y: Math.max(0, Math.min(root.height - height, touchY - height / 2))
        width: 56 * s
        height: 56 * s
        radius: 12 * s
        color: root.activeColor
        visible: touching
        opacity: 0.9
        z: 100

        Text {
            anchors.centerIn: parent
            text: root.touchLetter
            font.pixelSize: 28 * s
            font.bold: true
            color: Design.Colors.textPrimary
        }
    }

    function jumpToSection(letter) {
        if (!listView || !listView.model || listView.count === 0) return

        var targetPri = sectionPriority(letter)
        var firstIndexByKey = {}

        for (var i = 0; i < listView.count; i++) {
            var item = listView.model.get(i)
            if (!item) continue
            var key = item.sectionKey || "#"
            if (!(key in firstIndexByKey)) firstIndexByKey[key] = i
        }

        if (letter in firstIndexByKey) {
            listView.positionViewAtIndex(firstIndexByKey[letter], ListView.Beginning)
            return
        }

        var bestKey = null
        var bestPri = 99
        for (var k in firstIndexByKey) {
            var pri = sectionPriority(k)
            if (pri >= targetPri && pri < bestPri) { bestPri = pri; bestKey = k }
        }

        if (bestKey !== null) {
            listView.positionViewAtIndex(firstIndexByKey[bestKey], ListView.Beginning)
        }
    }
}
