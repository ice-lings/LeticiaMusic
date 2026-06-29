import QtQuick
import QtQuick.Controls

Popup {
    id: root
    modal: true
    closePolicy: Popup.CloseOnEscape
    width: 440; height: 510

    property string sourcePath: ""
    property string resultPath: ""
    signal cropConfirmed(string path)
    signal cropCancelled()

    onClosed: {
        if (resultPath) {
            cropConfirmed(resultPath)
        } else {
            cropCancelled()
        }
    }

    background: Rectangle {
        color: Design.Colors.surface
        border.color: Design.Colors.divider
        border.width: 1
        radius: 8
    }

    Overlay.modal: Rectangle { color: "#80000000" }

    property real cropSize: Math.min(imageContainer.width, imageContainer.height)
    property real cropX: (imageContainer.width - cropSize) / 2
    property real cropY: (imageContainer.height - cropSize) / 2

    function updateImageSize() {
        var cw = imageContainer.width
        var ch = imageContainer.height
        var sw = sourceImage.sourceSize.width
        var sh = sourceImage.sourceSize.height
        if (cw <= 0 || ch <= 0 || sw <= 0 || sh <= 0) return
        var scale = Math.max(cw / sw, ch / sh)
        sourceImage.width = sw * scale
        sourceImage.height = sh * scale
        sourceImage.x = (cw - sourceImage.width) / 2
        sourceImage.y = (ch - sourceImage.height) / 2
    }

    function getCropPixelRect() {
        var cw = imageContainer.width
        var ch = imageContainer.height
        var sw = sourceImage.sourceSize.width
        var sh = sourceImage.sourceSize.height
        if (sw <= 0 || sh <= 0 || cw <= 0 || ch <= 0) return null

        var scale = sourceImage.sourceSize.width / sourceImage.width
        var imgLX = cropX - sourceImage.x
        var imgTY = cropY - sourceImage.y

        return {
            x: Math.round(imgLX * scale),
            y: Math.round(imgTY * scale),
            size: Math.round(cropSize * scale)
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Item {
            width: parent.width
            height: 24

            Text {
                text: qsTr("裁剪封面  — 拖动图片调整选区")
                color: Design.Colors.textPrimary
                font.pixelSize: 14
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                text: qsTr("✕")
                color: Design.Colors.textSecondary
                font.pixelSize: 18
                MouseArea {
                    anchors.fill: parent; anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        Item {
            id: imageContainer
            width: parent.width
            height: parent.height - 70

            Image {
                id: sourceImage
                source: root.sourcePath ? ("file:///" + root.sourcePath) : ""
                asynchronous: true
                cache: false
                z: 0
            }

            Rectangle { z: 2; color: "#80000000"
                x: 0; y: 0; width: parent.width; height: cropY }
            Rectangle { z: 2; color: "#80000000"
                x: 0; y: cropY + cropSize; width: parent.width; height: parent.height - cropY - cropSize }
            Rectangle { z: 2; color: "#80000000"
                x: 0; y: cropY; width: cropX; height: cropSize }
            Rectangle { z: 2; color: "#80000000"
                x: cropX + cropSize; y: cropY; width: parent.width - cropX - cropSize; height: cropSize }

            Rectangle {
                id: cropBorder
                x: cropX; y: cropY
                width: cropSize; height: cropSize
                color: "transparent"
                border.color: "white"
                border.width: 2
                z: 3
            }

            Rectangle { z: 3
                x: cropX; y: cropY + cropSize / 2
                width: cropSize; height: 1
                color: "#30ffffff"
            }
            Rectangle { z: 3
                x: cropX + cropSize / 2; y: cropY
                width: 1; height: cropSize
                color: "#30ffffff"
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: false

                property real startMX: 0
                property real startMY: 0
                property real startIX: 0
                property real startIY: 0

                onPressed: function(mouse) {
                    startMX = mouse.x; startMY = mouse.y
                    startIX = sourceImage.x; startIY = sourceImage.y
                }

                onPositionChanged: function(mouse) {
                    sourceImage.x = startIX + mouse.x - startMX
                    sourceImage.y = startIY + mouse.y - startMY
                }

                onWheel: function(wheel) {
                    var zoom = 1 + wheel.angleDelta.y / 1200
                    var newW = sourceImage.width * zoom
                    var newH = sourceImage.height * zoom
                    var minW = imageContainer.width
                    var minH = imageContainer.height
                    if (newW < minW) newW = minW
                    if (newH < minH) newH = minH

                    var ratio = newW / sourceImage.width
                    sourceImage.x = wheel.x - (wheel.x - sourceImage.x) * ratio
                    sourceImage.y = wheel.y - (wheel.y - sourceImage.y) * ratio
                    sourceImage.width = newW
                    sourceImage.height = newH
                }
            }
        }

        Connections {
            target: sourceImage
            function onStatusChanged() {
                if (sourceImage.status === Image.Ready)
                    updateImageSize()
            }
        }

        Row {
            width: parent.width; height: 34
            layoutDirection: Qt.RightToLeft; spacing: 10

            Rectangle {
                width: 80; height: 34; radius: 5
                color: confirmCropBtn.containsMouse
                       ? Design.Colors.withAlpha(Design.Colors.primary, 0.85)
                       : Design.Colors.primary
                Text {
                    anchors.centerIn: parent
                    text: qsTr("确认")
                    color: Design.Colors.textPrimary; font.pixelSize: 13; font.bold: true
                }
                MouseArea {
                    id: confirmCropBtn
                    anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var rect = getCropPixelRect()
                        if (!rect) { root.close(); return }
                        var p = appContext.cropCoverToSquare(
                            root.sourcePath, rect.x, rect.y, rect.size)
                        if (p) root.resultPath = p
                        root.close()
                    }
                }
            }

            Rectangle {
                width: 80; height: 34; radius: 5
                color: cancelCropBtn.containsMouse
                       ? Design.Colors.buttonHovered : Design.Colors.buttonNormal
                border.color: Design.Colors.divider; border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: qsTr("取消")
                    color: Design.Colors.textPrimary; font.pixelSize: 13
                }
                MouseArea {
                    id: cancelCropBtn
                    anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }
    }
}
