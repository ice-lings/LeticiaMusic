import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7
import LeticiaMusic 1.0

Column {
    id: playbackControls
    spacing: Design.Spacing.spaceXs

    property var playerController: appContext ? appContext.playerController : null

    signal prevClicked()
    signal playClicked()
    signal pauseClicked()
    signal nextClicked()

    ProgressSlider {
        id: progressSlider
        width: Math.max(playbackControls.parent ? playbackControls.parent.width / 3 : 240, 240)
        playerController: playbackControls.playerController
    }

    PlaybackControlBar {
        id: controlBar
        anchors.horizontalCenter: parent.horizontalCenter
        playerController: playbackControls.playerController

        onPrevClicked: playbackControls.prevClicked()
        onPlayClicked: playbackControls.playClicked()
        onPauseClicked: playbackControls.pauseClicked()
        onNextClicked: playbackControls.nextClicked()
    }
}
