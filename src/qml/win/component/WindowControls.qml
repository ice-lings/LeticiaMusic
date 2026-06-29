import QtQuick
import QtQuick.Layouts
import "../Icons"

Row {
    spacing: 0

    signal settingsClicked()

    SettingsIcon {
        anchors.verticalCenter: parent.verticalCenter
        onClicked: settingsClicked()
    }

    MinimizeIcon {
        anchors.verticalCenter: parent.verticalCenter
    }

    Loader {
        width: 36
        height: 36
        sourceComponent: window.visibility === Window.Maximized ? restoreIconComp : maximizeIconComp

        Component {
            id: maximizeIconComp
            MaximizeIcon {
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Component {
            id: restoreIconComp
            RestoreIcon {
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    CloseIcon {
        anchors.verticalCenter: parent.verticalCenter
    }
}