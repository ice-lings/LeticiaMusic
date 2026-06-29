import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "component"
import "pages"

/* LeticiaMusic 安卓主窗口 */
Window {
    id: window
    visible: true
    title: "LeticiaMusic"
    color: Design.Colors.background

    Component.onCompleted: appContext.markFirstFrameRendered()

    // 焦点守卫：始终持有 activeFocus，拦截 Android 返回手势
    Item {
        id: backGuard
        z: -1
        anchors.fill: parent
        focus: true

        Keys.onReleased: function(event) {
            if (event.key === Qt.Key_Back) {
                event.accepted = contentArea.handleBackPress()
            }
        }

        onActiveFocusChanged: {
            if (!activeFocus) {
                forceActiveFocus()
            }
        }
    }

    readonly property int statusBarHeight: 0

    Item {
        id: contentArea
        anchors {
            fill: parent
            topMargin: statusBarHeight
        }

        function handleBackPress() {
            if (playingPage.isOpen) {
                playingPage.close()
                return true
            } else if (queueSheet.isOpen) {
                queueSheet.close()
                return true
            } else if (scanSettingsPage.visible) {
                if (scanSettingsPage.hasSubPageOpen()) {
                    scanSettingsPage.closeSubPage()
                } else {
                    scanSettingsPage.close()
                }
                return true
            } else if (duplicateSongsPage.visible) {
                duplicateSongsPage.close()
                return true
            }
            appContext.moveToBackground()
            return true
        }

        StackLayout {
            id: pageStack
            anchors {
                fill: parent
                bottomMargin: miniPlayer.height + bottomNavBar.height
            }
            currentIndex: bottomNavBar.currentIndex

            LocalMusicPage {}
            FavoritesPage {}
            PlaylistsPage {}
            ProfilePage {}
        }

        // 重复歌曲管理页面（左滑进入，z=200 在 StackLayout 上方，MiniPlayerBar z=250 在最上）
        DuplicateSongsPage {
            id: duplicateSongsPage
        }

        MiniPlayerBar {
            id: miniPlayer
            z: 250
            anchors {
                left: parent.left
                right: parent.right
                bottom: duplicateSongsPage.visible ? parent.bottom : bottomNavBar.top
            }
            onShowQueue: queueSheet.open()
            onShowNowPlaying: playingPage.open()
        }

        BottomNavBar {
            id: bottomNavBar
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
        }
    }

    // 播放队列弹窗（覆盖层）
    PlayQueueSheet {
        id: queueSheet
    }

    // 播放详情页（覆盖层）
    NowPlayingPage {
        id: playingPage
        onShowQueue: queueSheet.open()
    }

    // 通用浮动进度球（同步/扫描）
    FloatingProgressBall {
        id: progressBall
    }

    // 扫描设置页面（左滑进入）
    ScanSettingsPage {
        id: scanSettingsPage
    }
}
