import QtQuick
import QtQuick.Controls
import QtQuick.Window

/* NAS 同步配置面板 */
Item {
    id: root

    readonly property real s: Design.Responsive.guiScale


    anchors.fill: parent
    z: 1000
    visible: false

    // 配置数据
    property var configData: ({})

    // 动画位置
    property real panelTargetY: parent.height * 0.35
    property real panelHomeY: parent.height

    function open(cfg) {
        // 始终从 C++ 后端获取最新配置，避免使用过期缓存
        if (appContext) {
            configData = appContext.getSyncConfig()
        } else {
            configData = cfg || {}
        }
        loadConfig()
        visible = true
        panelTargetY = parent.height * 0.35
        panel.y = parent.height
        panel.visible = true
    }

    function close() {
        panelTargetY = parent.height
        visible = false
    }

    function updateBtnText() {
        if (!appContext) {
            syncBtnText.text = "立即同步"
            return
        }
        if (appContext.syncRunning) {
            syncBtnText.text = "同步中…"
        } else {
            syncBtnText.text = "立即同步"
        }
    }

    function loadConfig() {
        enabledSwitch.checked = (configData.enabled === true)
        hostInput.text = configData.nasHost || ""
        portInput.text = configData.nasPort ? String(configData.nasPort) : "21"
        userInput.text = configData.nasUser || "admin"
        passInput.text = configData.nasPassword || ""
        rootInput.text = configData.nasSyncRoot || "MUSIC/LeticiaMusic"
        deviceIdText.text = configData.deviceId || ""
        var ts = configData.lastSyncTime || 0
        lastSyncText.text = ts > 0 ? formatTime(ts) : "从未同步"
    }

    function saveConfig() {
        var map = {
            enabled: enabledSwitch.checked,
            nasHost: hostInput.text.trim(),
            nasPort: parseInt(portInput.text.trim()) || 21,
            nasUser: userInput.text.trim(),
            nasPassword: passInput.text,
            nasSyncRoot: rootInput.text.trim()
        }
        if (appContext) {
            appContext.setSyncConfig(map)
        }
        close()
    }

    function formatTime(ts) {
        return Tools.formatDateTime(ts)
    }

    // 遮罩层
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // 底部弹出面板
    Rectangle {
        id: panel
        width: parent.width
        height: parent.height * 0.65
        x: 0
        y: parent.height
        color: Design.Colors.background
        radius: 16 * s

        Behavior on y {
            NumberAnimation {
                target: panel
                property: "y"
                to: root.panelTargetY
                duration: 280
                easing.type: Easing.OutCubic
            }
        }

        // 底部填充
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 16 * s
            color: parent.color
        }

        // 标题栏
        Rectangle {
            id: titleBar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 50 * s
            color: "transparent"

            // 取消
            Text {
                anchors { left: parent.left; leftMargin: 20 * s; verticalCenter: parent.verticalCenter }
                text: "取消"
                font.pixelSize: 15 * s
                color: Design.Colors.textSecondary
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8 * s
                    onClicked: root.close()
                }
            }

            // 标题
            Text {
                anchors.centerIn: parent
                text: "NAS 同步配置"
                font.pixelSize: 16 * s
                font.weight: Font.Bold
                color: Design.Colors.textPrimary
            }

            // 保存
            Text {
                anchors { right: parent.right; rightMargin: 20 * s; verticalCenter: parent.verticalCenter }
                text: "保存"
                font.pixelSize: 15 * s
                font.weight: Font.Bold
                color: Design.Colors.primary
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8 * s
                    onClicked: root.saveConfig()
                }
            }

            Rectangle {
                anchors { left: parent.left; leftMargin: 20 * s; right: parent.right; rightMargin: 20 * s; bottom: parent.bottom }
                height: 0.5
                color: Design.Colors.divider
            }
        }

        // 表单内容
        Flickable {
            anchors { top: titleBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.topMargin: 10 * s
            clip: true
            contentHeight: contentColumn.implicitHeight + 30 * s

            Column {
                id: contentColumn
                anchors { left: parent.left; right: parent.right }
                anchors.leftMargin: 20 * s
                anchors.rightMargin: 20 * s
                spacing: 18 * s

                // 启用开关
                Row {
                    width: parent.width
                    height: 46 * s

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "启用同步"
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                    }

                    Switch {
                        id: enabledSwitch
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        checked: false
                    }
                }

                // 分割线
                Rectangle {
                    anchors { left: parent.left; right: parent.right }
                    height: 0.5
                    color: Design.Colors.divider
                    anchors.leftMargin: -20 * s
                    anchors.rightMargin: -20 * s
                }

                // ━━━ NAS 地址 ━━━
                Text {
                    text: "NAS 地址"
                    font.pixelSize: 13 * s
                    color: Design.Colors.textSecondary
                }
                Rectangle {
                    width: parent.width; height: 44 * s; radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: hostInput
                        anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                        inputMethodHints: Qt.ImhNoAutoUppercase
                    }
                }

                // ━━━ 端口 ━━━
                Text {
                    text: "端口"
                    font.pixelSize: 13 * s
                    color: Design.Colors.textSecondary
                }
                Rectangle {
                    width: parent.width; height: 44 * s; radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: portInput
                        anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                        inputMethodHints: Qt.ImhDigitsOnly

                        Text {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            text: "21"
                            font.pixelSize: 15 * s
                            color: Design.Colors.textHint
                            visible: !portInput.text
                        }
                    }
                }

                // ━━━ 用户名 ━━━
                Text {
                    text: "用户名"
                    font.pixelSize: 13 * s
                    color: Design.Colors.textSecondary
                }
                Rectangle {
                    width: parent.width; height: 44 * s; radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: userInput
                        anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                        inputMethodHints: Qt.ImhNoAutoUppercase
                    }
                }

                // ━━━ 密码 ━━━
                Text {
                    text: "密码"
                    font.pixelSize: 13 * s
                    color: Design.Colors.textSecondary
                }
                Rectangle {
                    width: parent.width; height: 44 * s; radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: passInput
                        anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                        echoMode: TextInput.Password
                        inputMethodHints: Qt.ImhNoAutoUppercase
                    }
                }

                // ━━━ 同步目录 ━━━
                Text {
                    text: "同步目录"
                    font.pixelSize: 13 * s
                    color: Design.Colors.textSecondary
                }
                Rectangle {
                    width: parent.width; height: 44 * s; radius: 8 * s
                    color: "#14ffffff"
                    TextInput {
                        id: rootInput
                        anchors { fill: parent; leftMargin: 12 * s; rightMargin: 12 * s }
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15 * s
                        color: Design.Colors.textPrimary
                        clip: true
                        inputMethodHints: Qt.ImhNoAutoUppercase
                    }
                }

                // 分割线
                Rectangle {
                    anchors { left: parent.left; right: parent.right }
                    height: 0.5
                    color: Design.Colors.divider
                    anchors.leftMargin: -20 * s
                    anchors.rightMargin: -20 * s
                }

                // 只读信息
                Column {
                    width: parent.width
                    spacing: 10 * s

                    Row {
                        width: parent.width
                        Text {
                            text: "设备 ID"
                            font.pixelSize: 13 * s
                            color: Design.Colors.textSecondary
                            width: 80 * s
                        }
                        Text {
                            id: deviceIdText
                            text: ""
                            font.pixelSize: 12 * s
                            color: "#666"
                            elide: Text.ElideMiddle
                            maximumLineCount: 1
                            width: parent.width - 80 * s
                        }
                    }

                    Row {
                        width: parent.width
                        Text {
                            text: "上次同步"
                            font.pixelSize: 13 * s
                            color: Design.Colors.textSecondary
                            width: 80 * s
                        }
                        Text {
                            id: lastSyncText
                            text: "从未同步"
                            font.pixelSize: 12 * s
                            color: "#666"
                        }
                    }
                }

                // 底部留白 + 同步按钮
                Item { width: 1; height: 10 * s }

                Rectangle {
                    width: parent.width
                    height: 44 * s
                    radius: 22 * s
                    color: {
                        if (!enabledSwitch.checked) return Design.Colors.divider
                        if (appContext && appContext.syncRunning) return Design.Colors.textHint
                        return Design.Colors.primary
                    }

                    Text {
                        id: syncBtnText
                        anchors.centerIn: parent
                        font.pixelSize: 15 * s
                        font.weight: Font.Bold
                        color: Design.Colors.textPrimary
                    }

                    Connections {
                        target: appContext
                        function onSyncRunningChanged() { root.updateBtnText() }
                    }

                    Component.onCompleted: root.updateBtnText()

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -4 * s
                        onClicked: {
                            if (!appContext || !enabledSwitch.checked) return
                            if (appContext.syncRunning) {
                                return
                            } else {
                                root.saveConfig()
                                appContext.startSync()
                            }
                        }
                    }
                }

                Item { width: 1; height: 20 * s }
            }
        }
    }
}
