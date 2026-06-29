import QtQuick
import QtQuick.Controls

Item {
    id: root
    
    width: 32
    height: 32
    
    property string musicHash: ""
    property bool isFavorite: false
    property int favoritePlaylistId: -1
    property bool showFavoriteButton: true
    
    visible: showFavoriteButton
    
    Image {
        id: heartIcon
        anchors.centerIn: parent
        width: 20
        height: 20
        source: root.isFavorite ? "qrc:/img/heart-fill.svg" : "qrc:/img/heart-line.svg"
        fillMode: Image.PreserveAspectFit
        smooth: true
        
        Behavior on scale {
            NumberAnimation { duration: 100 }
        }
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        z: 100
        
        onPressed: {
            heartIcon.scale = 0.85
        }
        
        onReleased: {
            heartIcon.scale = 1.0
        }
        
        onClicked: {
            toggleFavorite()
        }
        
        onEntered: {
            heartIcon.scale = 1.15
        }
        
        onExited: {
            heartIcon.scale = 1.0
        }
    }
    
    ToolTip {
        visible: mouseArea.containsMouse
        text: root.isFavorite ? qsTr("取消喜欢") : qsTr("添加到我喜欢")
        delay: 300
        timeout: 2000
    }
    
    Connections {
        target: appContext.playlistManager
        
        function onFavoriteStatusChanged(musicHash, isFav) {
            if (musicHash === root.musicHash) {
                console.log("[Heart] signal:", musicHash.slice(0,12), "→", isFav)
                root.isFavorite = isFav
            }
        }
    }
    
    Component.onCompleted: {
        if (!appContext || !appContext.playlistManager) {
            return
        }
        
        root.favoritePlaylistId = appContext.playlistManager.getFavoritePlaylistId()
        
        if (root.favoritePlaylistId >= 0 && musicHash !== "") {
            root.isFavorite = appContext.playlistManager.isMusicFavorite(musicHash)
        }
    }
    
    function toggleFavorite() {
        if (!appContext || !appContext.favoriteManager) {
            return
        }
        
        if (root.favoritePlaylistId < 0 || root.musicHash === "") {
            return
        }
        
        console.log("[Heart] toggle, cur:", root.isFavorite, "hash:", root.musicHash.slice(0,12))
        appContext.favoriteManager.toggleFavorite(root.musicHash)
        
        if (appContext.playlistViewModel) {
            appContext.playlistViewModel.refresh()
        }
    }
}
