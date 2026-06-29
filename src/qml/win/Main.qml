import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts  6.7
import QtQuick.Dialogs 6.7

import "component"
import LeticiaMusic 1.0

Window {
    id: window
    width: Design.Responsive.getDpiAdjustedSize(1280)
    height: Design.Responsive.getDpiAdjustedSize(800)
    visible: true
    title: "LeticiaMusic"
    opacity: windowAnimationsEnabled ? windowOpacityValue : 1.0
    flags: Qt.FramelessWindowHint | Qt.Window | Qt.WindowSystemMenuHint |
           Qt.WindowMaximizeButtonHint | Qt.WindowMinMaxButtonsHint
    // focus: true
    
    // 动画属性
    property real windowOpacityValue: 1.0
    property bool windowAnimationsEnabled: false
    property int viewTransitionDuration: Design.Spacing.durationNormal

    property var localMusicModel: appContext.getViewModel("local_music")
    property int currentView: 0  // 0: 本地音乐, 1: 歌单详情, 2: 设置页面
    
    // 响应式布局属性
    property real sidebarWidth: {
        // 基于设计系统基础值，根据DPI和窗口大小调整
        var baseWidth = Design.Spacing.sidebarWidthMd
        var dpiAdjusted = Design.Responsive.getDpiAdjustedSize(baseWidth)
        
        // 限制最小和最大宽度
        var minWidth = Design.Spacing.sidebarWidthSm
        var maxWidth = Design.Spacing.sidebarWidthLg
        
        // 确保在合理范围内
        var finalWidth = Math.max(minWidth, Math.min(dpiAdjusted, maxWidth))
        
        // 如果窗口很小，使用百分比
        if (window.width < 1000) {
            finalWidth = window.width * 0.15
        }
        
        return finalWidth
    }
    
    // ===== 全局调试开关系统 =====
    property bool debugVisualEnabled: true  // 默认开启，方便诊断
    
    // 切换方法
    function toggleDebugVisual() {
        debugVisualEnabled = !debugVisualEnabled
        // console.log("[DEBUG-SYSTEM] Debug visual mode:", debugVisualEnabled ? "ON" : "OFF")
    }
    
    // 快捷键：Ctrl+Shift+D 切换调试模式
    Shortcut {
        sequence: "Ctrl+Shift+D"
        onActivated: window.toggleDebugVisual()
    }

    // 窗口拖动支持 - 已移动到顶部工具栏内部

    // 键盘快捷键
    Shortcut {
        sequence: "Space"
        onActivated: if (appContext && appContext.playerController) appContext.playerController.playOrPause()
    }
    Shortcut {
        sequence: "Left"
        onActivated: if (appContext && appContext.playerController) appContext.playerController.playPrev()
    }
    Shortcut {
        sequence: "Right"
        onActivated: if (appContext && appContext.playerController) appContext.playerController.playNext()
    }
    Shortcut {
        sequence: "Up"
        onActivated: if (appContext && appContext.playerController && appContext.playerController.volume !== undefined) appContext.playerController.volume = Math.min(1.0, appContext.playerController.volume + 0.1)
    }
    Shortcut {
        sequence: "Down"
        onActivated: if (appContext && appContext.playerController && appContext.playerController.volume !== undefined) appContext.playerController.volume = Math.max(0.0, appContext.playerController.volume - 0.1)
    }
    Shortcut {
        sequence: "M"
        onActivated: if (appContext && appContext.playerController && appContext.playerController.muted !== undefined) appContext.playerController.muted = !appContext.playerController.muted
    }
    Shortcut {
        sequence: "F"
        onActivated: {}
    }
    Shortcut {
        sequence: "L"
        onActivated: {}
    }

    // 使用布局管理器替代锚点
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 顶部工具栏 (1)
        Rectangle {
            id: topToolBar
            Layout.fillWidth: true
            Layout.preferredHeight: Design.Spacing.getResponsiveControlHeight("xl", window.height)
            color: Design.Colors.background

            // 窗口拖动区域（仅在顶部工具栏有效）
            MouseArea {
                anchors.fill: parent
                property point dragStartPos: Qt.point(0, 0)
                
                onPressed: function(mouse) {
                    dragStartPos = Qt.point(mouse.x, mouse.y)
                }
                
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        var deltaX = mouse.x - dragStartPos.x
                        var deltaY = mouse.y - dragStartPos.y
                        window.x += deltaX
                        window.y += deltaY
                    }
                }
            }

            WindowControls {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                id: windowsControls
                onSettingsClicked: {
                    currentView = 2
                }
            }

            SearchBar {
                id: searchBar
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: Math.min(360, parent.width * 0.42)
                searchModel: localMusicModel

                onResultClicked: function(modelIndex) {
                    currentView = 0
                    localMusicListView.highlightAndScrollTo(modelIndex)
                }
            }
        }

        // 主体区域
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // 侧边导航 (2)
            SideNavigation  {
                id: sideNavigationRect
                Layout.preferredWidth: sidebarWidth
                Layout.fillHeight: true
                color: Design.Colors.surface

                onPlaylistSelected: function(playlistId) {
                    if (currentView === 2) currentView = 1
                    else currentView = 1
                }

                onFavoriteSelected: {
                    if (currentView === 2) currentView = 1
                    else currentView = 1
                }

                onLocalMusicSelected: {
                    if (currentView === 2) currentView = 0
                    else currentView = 0
                }
            }

            // 右侧区域
            Rectangle {
                id: rightRect
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Design.Colors.card

                // 本地音乐列表
                LocalMusicListView {
                    id: localMusicListView
                    anchors.fill: parent
                    anchors.topMargin: Design.Spacing.spaceLg
                    anchors.bottomMargin: Design.Spacing.spaceXl
                    opacity: currentView === 0 ? 1.0 : 0.0
                    visible: opacity > 0 || currentView === 0
                    z: currentView === 0 ? 1 : 0
                    
                    // 视图切换动画
                    Behavior on opacity {
                        enabled: windowAnimationsEnabled
                        NumberAnimation {
                            duration: viewTransitionDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                    
                    // 可选：滑动动画
                    property real slideOffset: currentView === 0 ? 0 : 20
                    x: slideOffset
                    Behavior on x {
                        enabled: windowAnimationsEnabled
                        NumberAnimation {
                            duration: viewTransitionDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                // 歌单详情
                PlaylistDetailView {
                    id: playlistDetailView
                    anchors.fill: parent
                    anchors.topMargin: Design.Spacing.spaceLg
                    anchors.bottomMargin: Design.Spacing.spaceXl
                    opacity: currentView === 1 ? 1.0 : 0.0
                    visible: opacity > 0 || currentView === 1
                    z: currentView === 1 ? 1 : 0
                    
                    // 视图切换动画
                    Behavior on opacity {
                        enabled: windowAnimationsEnabled
                        NumberAnimation {
                            duration: viewTransitionDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                    
                    // 可选：滑动动画
                    property real slideOffset: currentView === 1 ? 0 : -20
                    x: slideOffset
                    Behavior on x {
                        enabled: windowAnimationsEnabled
                        NumberAnimation {
                            duration: viewTransitionDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                }
                
                // 设置页面
                SettingsPage {
                    id: settingsPageView
                    anchors.fill: parent
                    anchors.topMargin: Design.Spacing.spaceLg
                    anchors.bottomMargin: Design.Spacing.spaceXl
                    opacity: currentView === 2 ? 1.0 : 0.0
                    visible: opacity > 0 || currentView === 2
                    z: currentView === 2 ? 1 : 0
                    
                    Behavior on opacity {
                        enabled: windowAnimationsEnabled
                        NumberAnimation {
                            duration: viewTransitionDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
        }

        Rectangle {
            id: bottomRect
            Layout.fillWidth: true
            Layout.preferredHeight: Design.Spacing.getResponsiveControlHeight("xl", window.height) * 2.1
            color: Design.Colors.background

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: Design.Spacing.spaceMd
                anchors.rightMargin: Design.Spacing.spaceMd
                anchors.bottomMargin: Design.Spacing.spaceXs
                anchors.topMargin: Design.Spacing.spaceSm
                spacing: 8

                // 第一行：NowPlayingInfo + PlaybackControls
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: false

                    NowPlayingInfo {
                        id: nowPlayingInfo
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(parent.width * 0.32, 400)
                        height: parent.height
                        playerController: appContext.playerController
                    }

                    PlaybackControls {
                        id: playbackControls
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                
                // 第二行：播放队列和模式按钮
                Row {
                    id: queueModeButtonRow
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 24
                    
                    // 播放队列按钮
                    ToolButton {
                        id: playQueueButton
                        width: 36
                        height: 36
                        
                        contentItem: Text {
                            text: "☰"
                            color: playQueueButton.hovered ? Design.Colors.primary : Design.Colors.textSecondary
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        background: Rectangle {
                            color: "transparent"
                        }
                        
                        onClicked: {
                            playQueueView.open()
                        }
                    }
                    
                    // 播放模式按钮
                    PlayModeSelector {
                        playerController: appContext.playerController
                    }
                }
            }
        }
    } // ColumnLayout
    
    // 窗口显示动画
    Component.onCompleted: {
        if (windowAnimationsEnabled) {
            // 淡入动画
            windowOpacityValue = 0.0
            var fadeInAnimation = Qt.createQmlObject(`
                import QtQuick 6.7
                NumberAnimation {
                    target: window
                    property: "windowOpacityValue"
                    from: 0.0
                    to: 1.0
                    duration: Design.Spacing.durationNormal
                    easing.type: Easing.OutCubic
                }
            `, window)
            fadeInAnimation.start()
        } else {
            windowOpacityValue = 1.0
        }
        
        // 打印动画状态信息
        // console.log("Window animations:", windowAnimationsEnabled ? "enabled" : "disabled")
        // console.log("View transition duration:", viewTransitionDuration, "ms")
        // console.log("DPI scale:", Design.Responsive.getDpiScale().toFixed(2))
        // console.log("Window size:", width, "x", height)
        // console.log("Design.Responsive.getDpiAdjustedSize(1280):", Design.Responsive.getDpiAdjustedSize(1280))
        // console.log("Design.Responsive.getDpiAdjustedSize(800):", Design.Responsive.getDpiAdjustedSize(800))
        // 确保窗口在前台
        window.raise()
        window.requestActivate()
    }
    
    // 窗口大小变化动画
    Behavior on width {
        enabled: windowAnimationsEnabled
        NumberAnimation {
            duration: Design.Spacing.durationSlow
            easing.type: Easing.OutCubic
        }
    }
    
    Behavior on height {
        enabled: windowAnimationsEnabled
        NumberAnimation {
            duration: Design.Spacing.durationSlow
            easing.type: Easing.OutCubic
        }
    }

    // 全局 Toast 通知（置于最上层）
    ToastNotification {
        id: globalToast
        z: 9999
    }
    
    // 扫描进度条（右下角浮动，不阻止用户操作；位于底部播放栏上方）
    ScanProgressBar {
        id: scanProgressBar
        x: parent.width - width - 20
        y: bottomRect.y - height - 20
        z: 500
    }
    
    // 播放队列视图
    PlayQueueView {
        id: playQueueView
    }
    
    // ===== 调试按钮（调试用） =====
    Button {
        id: debugToggleBtn
        text: window.debugVisualEnabled ? "🔴 Debug: ON" : "⚪ Debug: OFF"
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        z: 99999
        visible: false  // 默认隐藏
        opacity: 0.8
        
        onClicked: window.toggleDebugVisual()
    }
    
    // 快捷键：Ctrl+Shift+D 显示/隐藏调试按钮
    Shortcut {
        sequence: "Ctrl+Shift+D"
        onActivated: debugToggleBtn.visible = !debugToggleBtn.visible
    }

    // 同步进度条（右下角浮动，不阻止用户操作；位于扫描进度条上方）
    SyncProgressCard {
        id: syncProgressCard
        x: parent.width - width - 20
        y: scanProgressBar.visible ? scanProgressBar.y - height - 8 : bottomRect.y - height - 20
        z: 501
    }
} // windows

