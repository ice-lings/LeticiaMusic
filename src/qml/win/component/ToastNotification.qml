import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    
    property string message: ""
    property int displayDuration: 1500 // 1.5秒
    property int toastType: 0 // 0: info, 1: success, 2: warning, 3: error
    
    // 类型颜色定义
    readonly property var typeColors: [
        Design.Colors.primary,      // info - 蓝色
        "#2ecc71",                   // success - 绿色  
        "#f39c12",                   // warning - 橙色
        "#e74c3c"                    // error - 红色
    ]
    
    // 计算宽度（最小200，最大400）
    width: Math.max(200, Math.min(messageText.implicitWidth + 40, 400))
    height: 50
    
    // 居中偏下显示
    x: (parent.width - width) / 2
    y: parent.height - 120
    
    // 无模态，不拦截点击
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose
    
    // 背景样式
    background: Rectangle {
        color: Design.Colors.withAlpha(Design.Colors.surface, 0.95)
        border.color: Design.Colors.divider
        border.width: 1
        radius: 8
        
        // 左侧彩色指示条
        Rectangle {
            width: 4
            height: parent.height - 8
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            color: typeColors[root.toastType]
            radius: 2
        }
    }
    
    // 内容文本
    contentItem: Text {
        id: messageText
        text: root.message
        color: Design.Colors.textPrimary
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        anchors.fill: parent
        anchors.margins: 12
        anchors.leftMargin: 20
    }
    
    // 进入动画（上滑 + 淡入）
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "y"
                from: root.y + 20
                to: root.y
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }
    
    // 退出动画（淡出）
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 200
            easing.type: Easing.InCubic
        }
    }
    
    // 自动关闭定时器
    Timer {
        id: hideTimer
        interval: root.displayDuration
        onTriggered: root.close()
    }
    
    // 公共方法
    function show(msg, type) {
        message = msg
        toastType = type
        open()
        hideTimer.start()
    }
    
    function showInfo(msg) { show(msg, 0) }
    function showSuccess(msg) { show(msg, 1) }
    function showWarning(msg) { show(msg, 2) }
    function showError(msg) { show(msg, 3) }
}
