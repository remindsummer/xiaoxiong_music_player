import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Rectangle {
    id: root
    required property var appController
    readonly property var settings: root.appController
                                    ? (root.appController.settingsService || root.appController.settings)
                                    : null
    readonly property bool settingsReady: root.settings !== null && root.settings !== undefined

    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: theme.radiusMd
    color: "transparent"

    ThemeTokens.Theme {
        id: theme
    }

    function syncForm() {
        if (!root.settingsReady) {
            return
        }
        themeCombo.syncFromSettings()
        languageCombo.syncFromSettings()
        scanPathField.text = root.settings.defaultScanDirectory
        downloadPathField.text = root.settings.defaultDownloadDirectory
        lyricSwitch.checked = root.settings.lyricOnlineEnabled
        metingApiBasesField.text = root.settings.metingApiBases
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.space3

        Label {
            text: "设置"
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH1
            color: theme.colorTextPrimary
        }

        Label {
            text: root.settingsReady
                  ? "修改后点击保存，配置会持久化到本地。"
                  : "settings 服务尚未暴露到 appController.settingsService"
            wrapMode: Text.Wrap
            color: root.settingsReady ? theme.colorTextSecondary : theme.colorStateWarning
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: theme.radiusMd
            color: theme.colorBgPanel
            border.width: 1
            border.color: theme.colorBorderDefault
            clip: true

            ScrollView {
                id: settingsScrollView
                anchors.fill: parent
                anchors.margins: theme.space4
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: settingsScrollView.availableWidth
                    spacing: theme.space3
                    enabled: root.settingsReady

                GridLayout {
                    columns: 3
                    columnSpacing: theme.space3
                    rowSpacing: theme.space3
                    Layout.fillWidth: true

                    Label {
                        text: "外观"
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: "主题模式（浅色/深色/跟随系统）"
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ComboBox {
                        id: themeCombo
                        Layout.fillWidth: true
                        model: [
                            { label: "跟随系统", value: "system" },
                            { label: "浅色", value: "light" },
                            { label: "深色", value: "dark" }
                        ]
                        textRole: "label"

                        function syncFromSettings() {
                            if (!root.settingsReady) {
                                return
                            }
                            const currentTheme = root.settings.theme
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].value === currentTheme) {
                                    currentIndex = i
                                    return
                                }
                            }
                            currentIndex = 0
                        }

                        Component.onCompleted: syncFromSettings()
                        onActivated: {
                            if (root.settingsReady) {
                                root.settings.theme = model[currentIndex].value
                            }
                        }
                    }

                    Label {
                        text: "通用"
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: "界面语言"
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ComboBox {
                        id: languageCombo
                        Layout.fillWidth: true
                        model: [
                            { label: "简体中文", value: "zh_CN" },
                            { label: "English", value: "en_US" }
                        ]
                        textRole: "label"

                        function syncFromSettings() {
                            if (!root.settingsReady) {
                                return
                            }
                            const currentLanguage = root.settings.language
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].value === currentLanguage) {
                                    currentIndex = i
                                    return
                                }
                            }
                            currentIndex = 0
                        }

                        Component.onCompleted: syncFromSettings()
                        onActivated: {
                            if (root.settingsReady) {
                                root.settings.language = model[currentIndex].value
                            }
                        }
                    }

                    Label {
                        text: "通用"
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: "默认扫描目录"
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    TextField {
                        id: scanPathField
                        Layout.fillWidth: true
                        placeholderText: "例如: D:/Music"
                        onEditingFinished: {
                            if (root.settingsReady) {
                                root.settings.defaultScanDirectory = text
                            }
                        }
                    }

                    Label {
                        text: "通用"
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: "在线下载目录"
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    TextField {
                        id: downloadPathField
                        Layout.fillWidth: true
                        placeholderText: "例如: D:/Music/Downloads"
                        onEditingFinished: {
                            if (root.settingsReady) {
                                root.settings.defaultDownloadDirectory = text
                            }
                        }
                    }

                    Label {
                        text: "网络"
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: "歌词联网开关"
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    Switch {
                        id: lyricSwitch
                        onToggled: {
                            if (root.settingsReady) {
                                root.settings.lyricOnlineEnabled = checked
                            }
                        }
                    }
                }

                Label {
                    text: "Meting API"
                    font.bold: true
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontBody
                    color: theme.colorTextPrimary
                }

                Label {
                    text: "在线搜索、歌词、封面与流媒体解析使用的 Meting 接口。每行填写一个镜像根地址（需含 /api 或 /meting 路径），请求失败时会按顺序尝试下一行。"
                    wrapMode: Text.Wrap
                    color: theme.colorTextSecondary
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    Layout.fillWidth: true
                }

                TextArea {
                    id: metingApiBasesField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 88
                    wrapMode: TextArea.Wrap
                    placeholderText: "https://meting.elysium-stack.cn/api\nhttps://api.injahow.cn/meting"
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    onEditingFinished: {
                        if (root.settingsReady) {
                            root.settings.metingApiBases = text
                        }
                    }
                }

                Label {
                    text: root.settingsReady
                          ? ("配置文件：" + root.settings.storageLocation
                             + "\n应用数据目录（歌单、曲库缓存、歌词缓存等）：" + root.settings.appDataLocation)
                          : ""
                    wrapMode: Text.Wrap
                    color: theme.colorTextMuted
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    Layout.fillWidth: true
                }

                Label {
                    text: "快捷键：Ctrl+P 播放/暂停，Ctrl+Left 上一首，Ctrl+Right 下一首，Ctrl+F 聚焦曲库搜索。"
                    wrapMode: Text.Wrap
                    color: theme.colorTextMuted
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                }

                RowLayout {
                    spacing: theme.space2

                    Button {
                        text: "保存设置"
                        hoverEnabled: true
                        onClicked: {
                            if (root.settingsReady) {
                                root.settings.metingApiBases = metingApiBasesField.text
                            }
                            root.settings.save()
                        }
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down
                                   ? theme.colorPrimaryActive
                                   : (parent.hovered ? theme.colorPrimaryActive : theme.colorPrimary)
                            border.width: 0
                        }
                        contentItem: Label {
                            text: parent.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#ffffff"
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontBody
                            font.weight: 600
                        }
                    }

                    Button {
                        text: "重新加载"
                        hoverEnabled: true
                        onClicked: {
                            root.settings.load()
                            root.syncForm()
                        }
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            border.width: 1
                            border.color: theme.colorBorderDefault
                        }
                    }

                    Button {
                        text: "恢复默认"
                        hoverEnabled: true
                        onClicked: {
                            root.settings.reset()
                            root.syncForm()
                        }
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down
                                   ? Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.95)
                                   : (parent.hovered ? Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.9) : theme.colorStateError)
                            border.width: 0
                        }
                        contentItem: Label {
                            text: parent.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#ffffff"
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontBody
                        }
                    }
                }
            }
        }
        }
    }

    Component.onCompleted: syncForm()

    Connections {
        target: root.settingsReady ? root.settings : null

        function onThemeChanged() { themeCombo.syncFromSettings() }
        function onLanguageChanged() { languageCombo.syncFromSettings() }
        function onDefaultScanDirectoryChanged() { scanPathField.text = root.settings.defaultScanDirectory }
        function onDefaultDownloadDirectoryChanged() { downloadPathField.text = root.settings.defaultDownloadDirectory }
        function onLyricOnlineEnabledChanged() { lyricSwitch.checked = root.settings.lyricOnlineEnabled }
        function onMetingApiBasesChanged() { metingApiBasesField.text = root.settings.metingApiBases }
    }
}
