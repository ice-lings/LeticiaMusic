import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

Item {
    id: root
    clip: true

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: Design.Spacing.spaceLg

            // NAS 连接配置
            GroupBox {
                title: qsTr("NAS 连接")
                Layout.fillWidth: true

                background: Rectangle {
                    y: parent.topPadding - parent.bottomPadding
                    width: parent.width
                    height: parent.height - parent.topPadding + parent.bottomPadding
                    color: "transparent"
                    border.color: Design.Colors.divider
                    radius: 8
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Design.Spacing.spaceMd

                    RowLayout {
                        Text { text: qsTr("启用同步"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        Switch { id: syncEnabledSwitch; checked: false; onCheckedChanged: saveConfig() }
                    }
                    RowLayout {
                        Text { text: qsTr("主机地址"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        TextField {
                            id: nasHostField; Layout.fillWidth: true; placeholderText: "192.168.x.x"
                            color: Design.Colors.textPrimary; leftPadding: 8
                            background: Rectangle {
                                color: Design.Colors.surface; radius: 4
                                border.color: Design.Colors.divider; border.width: 1
                            }
                            onEditingFinished: saveConfig()
                        }
                    }
                    RowLayout {
                        Text { text: qsTr("端口"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        SpinBox { id: nasPortSpin; from: 1; to: 65535; value: 21; onValueChanged: saveConfig() }
                    }
                    RowLayout {
                        Text { text: qsTr("用户名"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        TextField {
                            id: nasUserField; Layout.fillWidth: true; placeholderText: "admin"
                            color: Design.Colors.textPrimary; leftPadding: 8
                            background: Rectangle {
                                color: Design.Colors.surface; radius: 4
                                border.color: Design.Colors.divider; border.width: 1
                            }
                            onEditingFinished: saveConfig()
                        }
                    }
                    RowLayout {
                        Text { text: qsTr("密码"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        TextField {
                            id: nasPasswordField; Layout.fillWidth: true; echoMode: TextInput.Password
                            placeholderText: "••••••••"
                            color: Design.Colors.textPrimary; leftPadding: 8
                            background: Rectangle {
                                color: Design.Colors.surface; radius: 4
                                border.color: Design.Colors.divider; border.width: 1
                            }
                            onEditingFinished: saveConfig()
                        }
                    }
                    RowLayout {
                        Text { text: qsTr("同步根目录"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        TextField {
                            id: nasSyncRootField; Layout.fillWidth: true; placeholderText: "MUSIC/LeticiaMusic"
                            color: Design.Colors.textPrimary; leftPadding: 8
                            background: Rectangle {
                                color: Design.Colors.surface; radius: 4
                                border.color: Design.Colors.divider; border.width: 1
                            }
                            onEditingFinished: saveConfig()
                        }
                    }
                }
            }

            // 下载路径配置
            GroupBox {
                title: qsTr("本地下载")
                Layout.fillWidth: true

                background: Rectangle {
                    y: parent.topPadding - parent.bottomPadding
                    width: parent.width
                    height: parent.height - parent.topPadding + parent.bottomPadding
                    color: "transparent"
                    border.color: Design.Colors.divider
                    radius: 8
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Design.Spacing.spaceSm
                    RowLayout {
                        Text { text: qsTr("下载路径"); Layout.preferredWidth: 120; color: Design.Colors.textPrimary }
                        TextField {
                            id: downloadPathField; Layout.fillWidth: true
                            color: Design.Colors.textPrimary; leftPadding: 8
                            background: Rectangle {
                                color: Design.Colors.surface; radius: 4
                                border.color: Design.Colors.divider; border.width: 1
                            }
                            onEditingFinished: saveConfig()
                        }
                    }
                    Text {
                        text: qsTr("默认: ") + defaultDownloadPath
                        color: Design.Colors.textSecondary
                        font.pixelSize: 11
                        Layout.leftMargin: 124
                    }
                }
            }

            // 同步按钮
            Button {
                id: syncButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                enabled: syncEnabledSwitch.checked && !syncRunning
                hoverEnabled: true

                background: Rectangle {
                    radius: 8
                    color: syncButton.enabled
                           ? (syncButton.hovered ? Design.Colors.primaryLight : Design.Colors.primary)
                           : Design.Colors.buttonDisabled
                }

                contentItem: Text {
                    text: syncRunning ? qsTr("同步中...") : qsTr("立即同步")
                    font.pixelSize: 16
                    font.bold: true
                    color: syncButton.enabled ? "white" : Design.Colors.textDisabled
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (appContext) appContext.startSync()
                }
            }

            // 状态信息
            GroupBox {
                title: qsTr("状态")
                Layout.fillWidth: true

                background: Rectangle {
                    y: parent.topPadding - parent.bottomPadding
                    width: parent.width
                    height: parent.height - parent.topPadding + parent.bottomPadding
                    color: "transparent"
                    border.color: Design.Colors.divider
                    radius: 8
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Design.Spacing.spaceSm

                    Text {
                        text: qsTr("设备 ID: ") + (syncConfig ? (syncConfig.deviceId || qsTr("未生成")) : qsTr("未生成"))
                        color: Design.Colors.textSecondary
                    }
                    Text {
                        text: qsTr("上次同步: ") + (lastSyncText || qsTr("从未同步"))
                        color: Design.Colors.textSecondary
                    }
                }
            }

            // 底部留白
            Item { Layout.fillHeight: true; Layout.preferredHeight: 20 }
        }
    }

    // 属性
    property var syncConfig: ({})
    property bool syncRunning: false
    property string lastSyncText: ""
    property string defaultDownloadPath: ""
    property bool initializing: true

    function loadConfig() {
        if (!appContext) return
        initializing = true
        var cfg = appContext.getSyncConfig()
        if (!cfg) { initializing = false; return }
        syncConfig = cfg
        syncEnabledSwitch.checked = cfg.enabled || false
        nasHostField.text = cfg.nasHost || ""
        nasPortSpin.value = cfg.nasPort || 21
        nasUserField.text = cfg.nasUser || ""
        nasPasswordField.text = cfg.nasPassword || ""
        nasSyncRootField.text = cfg.nasSyncRoot || ""
        downloadPathField.text = appContext.getDownloadPath() || ""
        defaultDownloadPath = appContext.getDefaultDownloadPath() || ""
        if (cfg.lastSyncTime > 0) {
            lastSyncText = new Date(cfg.lastSyncTime * 1000).toLocaleString(Qt.locale(), "yyyy-MM-dd hh:mm:ss")
        }
        initializing = false
    }

    function saveConfig() {
        if (!appContext || initializing) return
        var cfg = {
            enabled: syncEnabledSwitch.checked,
            nasHost: nasHostField.text,
            nasPort: nasPortSpin.value,
            nasUser: nasUserField.text,
            nasPassword: nasPasswordField.text,
            nasSyncRoot: nasSyncRootField.text
        }
        appContext.setSyncConfig(cfg)
        appContext.setDownloadPath(downloadPathField.text)
    }

    Connections {
        target: appContext
        function onSyncRunningChanged() {
            syncRunning = appContext.syncRunning
        }
        function onSyncPausedChanged() {}
    }

    Component.onCompleted: {
        if (appContext) {
            syncRunning = appContext.syncRunning
        }
        timer.start()
    }

    Timer {
        id: timer
        interval: 100
        repeat: false
        onTriggered: loadConfig()
    }
}
