import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7
import QtQuick.Dialogs

Item {
    id: root
    
    ColumnLayout {
        anchors.fill: parent
        spacing: Design.Spacing.spaceLg
        
        // Section: Music Paths Header
        RowLayout {
            spacing: Design.Spacing.spaceSm
            Layout.fillWidth: true
            
            Text {
                text: qsTr("音乐扫描路径")
                font.pixelSize: 20
                font.bold: true
                color: Design.Colors.textPrimary
            }
            
            Item { Layout.fillWidth: true }
            
            // Scan button
            Button {
                id: scanBtn
                height: 36
                padding: 12
                hoverEnabled: true
                z: 10

                background: Rectangle {
                    anchors.fill: parent
                    radius: 18
                    color: scanBtn.hovered ? Qt.rgba(0.29, 0.56, 0.89, 0.3) : "transparent"
                    border.width: 1.5
                    border.color: scanBtn.hovered ? Design.Colors.primary : Qt.rgba(255, 255, 255, 0.3)
                }

                contentItem: Text {
                    text: appContext.scanning ? qsTr("扫描中...") : qsTr("开始扫描")
                    font.pixelSize: 14
                    color: scanBtn.hovered ? Design.Colors.primary : Design.Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (!appContext.scanning) {
                        appContext.scanMusicNow()
                    }
                }
            }
            
            Button {
                id: addPathBtn
                width: 40
                height: 40
                hoverEnabled: true
                z: 10
                
                background: Rectangle {
                    anchors.fill: parent
                    radius: 20
                    color: addPathBtn.hovered ? Qt.rgba(0.29, 0.56, 0.89, 0.3) : "transparent"
                    border.width: 1.5
                    border.color: addPathBtn.hovered ? Design.Colors.primary : Qt.rgba(255, 255, 255, 0.3)
                }
                
                contentItem: Text {
                    text: "+"
                    font.pixelSize: 24
                    font.bold: true
                    color: addPathBtn.hovered ? Design.Colors.primary : Design.Colors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: folderDialog.open()
            }
        }
        
        // Path list
        ListView {
            id: pathListView
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            model: musicPathsModel
            spacing: Design.Spacing.spaceSm
            clip: true
            
            delegate: Rectangle {
                id: pathItem
                width: ListView.view.width
                height: 48
                radius: 8
                color: pathMouseArea.containsMouse ? Qt.rgba(0.25, 0.25, 0.28, 0.6) : Qt.rgba(0.2, 0.2, 0.25, 0.3)
                border.width: 1
                border.color: pathMouseArea.containsMouse ? Design.Colors.primary : Qt.rgba(255, 255, 255, 0.05)
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Design.Spacing.spaceMd
                    anchors.rightMargin: Design.Spacing.spaceMd
                    spacing: 12
                    
                    Text {
                        id: pathText
                        text: model.path
                        font.pixelSize: 15
                        color: Design.Colors.textPrimary
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }
                    
                    Rectangle {
                        id: removeBtn
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        implicitWidth: 60
                        implicitHeight: 32
                        radius: 6
                        color: removeMouseArea.containsMouse ? Qt.rgba(0.93, 0.11, 0.14, 0.3) : "transparent"
                        border.width: 1
                        border.color: removeMouseArea.containsMouse ? Design.Colors.error : "transparent"
                        
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("移除")
                            font.pixelSize: 14
                            color: removeMouseArea.containsMouse ? Design.Colors.error : Design.Colors.textSecondary
                        }
                        
                        MouseArea {
                            id: removeMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            
                            onClicked: {
                                console.log("Remove clicked, index:", index)
                                removePath(index)
                            }
                        }
                    }
                }
                
                MouseArea {
                    id: pathMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    hoverEnabled: true
                }
            }
        }
        
        // Empty state
        Text {
            visible: musicPathsModel.count === 0
            text: qsTr("暂无音乐路径，点击右上角 + 添加")
            font.pixelSize: 16
            color: Design.Colors.textSecondary
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Design.Spacing.spaceXl
        }
        
        // Section: Scan Options
        Text {
            text: qsTr("扫描选项")
            font.pixelSize: 20
            font.bold: true
            color: Design.Colors.textPrimary
            Layout.topMargin: Design.Spacing.spaceLg
        }
        
        // Auto scan checkbox
        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignLeft
            
            CheckBox {
                id: autoScanCheckbox
                checked: appContext.getAutoScanMusicOnStartup()
                onCheckedChanged: appContext.setAutoScanMusicOnStartup(checked)
                spacing: 0
                
                indicator: Rectangle {
                    width: 20
                    height: 20
                    radius: 4
                    border.width: 2
                    border.color: autoScanCheckbox.checked ? Design.Colors.primary : Design.Colors.textSecondary
                    color: autoScanCheckbox.checked ? Design.Colors.primary : "transparent"
                    
                    Image {
                        source: "qrc:/img/checkmark.svg"
                        width: 12
                        height: 12
                        anchors.centerIn: parent
                        visible: autoScanCheckbox.checked
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }
            
            Column {
                spacing: 2
                Layout.alignment: Qt.AlignLeft
                
                Text {
                    text: qsTr("启动时自动扫描")
                    font.pixelSize: 16
                    color: Design.Colors.textPrimary
                }
                
                Text {
                    text: qsTr("程序启动时自动扫描音乐目录")
                    font.pixelSize: 12
                    color: Design.Colors.textSecondary
                }
            }
        }
        
        // Rescan button
        Button {
            id: rescanBtn
            width: 160
            height: 44
            Layout.alignment: Qt.AlignLeft

            background: Rectangle {
                anchors.fill: parent
                radius: 8
                color: rescanBtn.hovered ? Qt.rgba(0.29, 0.56, 0.89, 0.3) : "transparent"
                border.width: 1.5
                border.color: rescanBtn.hovered ? Design.Colors.primary : Qt.rgba(255, 255, 255, 0.2)
            }

            contentItem: Text {
                text: appContext.scanning ? qsTr("扫描中...") : qsTr("立即扫描")
                font.pixelSize: 16
                color: rescanBtn.hovered ? Design.Colors.primary : Design.Colors.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (!appContext.scanning) {
                    rescanMusic()
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
    
    ListModel {
        id: musicPathsModel
    }
    
    // Folder selection dialog
    FolderDialog {
        id: folderDialog
        title: qsTr("选择音乐文件夹")
        modality: Qt.ApplicationModal
        
        onAccepted: {
            console.log("FolderDialog accepted!")
            var folder = folderDialog.selectedFolder.toString()
            console.log("Selected folder URL:", folder)
            
            if (folder.startsWith("file:///")) {
                folder = folder.substring(8)
            } else if (folder.startsWith("file://")) {
                folder = folder.substring(7)
            }
            
            if (folder.indexOf("/") !== -1) {
                folder = folder.replace(/\//g, "\\")
            }
            
            if (folder.endsWith("\\")) {
                folder = folder.substring(0, folder.length - 1)
            }
            
            console.log("Processed folder path:", folder)
            
            appContext.addMusicScanDirectory(folder)
            
            var currentPaths = appContext.getMusicScanDirectories()
            console.log("Paths after adding, count:", currentPaths.length)
            
            loadPaths()
        }
        
        onRejected: {
            console.log("FolderDialog rejected/cancelled")
        }
    }
    
    Component.onCompleted: {
        loadPaths()
    }
    
    function loadPaths() {
        musicPathsModel.clear()
        var paths = appContext.getMusicScanDirectories()
        // console.log("Loading paths, count:", paths.length)
        for (var i = 0; i < paths.length; i++) {
            // console.log("Path", i, ":", paths[i])
            musicPathsModel.append({path: paths[i]})
        }
    }
    
    function removePath(index) {
        var paths = appContext.getMusicScanDirectories()
        if (index >= 0 && index < paths.length) {
            console.log("Removing path at index", index, ":", paths[index])
            appContext.removeMusicScanDirectory(paths[index])
            loadPaths()
        }
    }
    
    function rescanMusic() {
        console.log("Rescan music clicked")
        appContext.scanMusicNow()
    }
}
