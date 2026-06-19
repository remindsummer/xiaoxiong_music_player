import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
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

    ButtonGroup {
        id: spectrumStyleGroup
    }

    readonly property bool spectrumOptionsEnabled: root.settingsReady && root.settings.spectrumEnabled

    function syncSpectrumStyleRadios() {
        if (!root.settingsReady) {
            return
        }
        const style = root.settings.spectrumStyle
        spectrumStyleSmooth.checked = style === "smoothMirror"
        spectrumStyleBars.checked = style === "mirrorBars"
        spectrumStyleLine.checked = style === "lineTrace"
        spectrumStyleUpper.checked = style === "upperGlow"
        spectrumStyleDots.checked = style === "particleDots"
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
        spectrumEnabledSwitch.checked = root.settings.spectrumEnabled
        spectrumFpsSlider.value = root.settings.spectrumFps
        spectrumFpsValueLabel.text = root.settings.spectrumFps + " FPS"
        spectrumOpacitySlider.value = root.settings.spectrumOpacity
        spectrumOpacityValueLabel.text = Math.round(root.settings.spectrumOpacity * 100) + "%"
        syncSpectrumStyleRadios()
        metingApiBasesField.text = root.settings.metingApiBases
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.space3

        Label {
            text: qsTr("设置")
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH1
            color: theme.colorTextPrimary
        }

        Label {
            text: root.settingsReady
                  ? qsTr("修改后点击保存，配置会持久化到本地。")
                  : qsTr("settings 服务尚未暴露到 appController.settingsService")
            wrapMode: Text.Wrap
            color: root.settingsReady ? theme.colorTextSecondary : theme.colorStateWarning
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
        }

        Components.GlassPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            cornerRadius: theme.radiusMd
            padding: theme.space4
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
                        text: qsTr("外观")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("主题模式（浅色/深色/跟随系统）")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ComboBox {
                        id: themeCombo
                        Layout.fillWidth: true
                        model: [
                            { label: qsTr("跟随系统"), value: "system" },
                            { label: qsTr("浅色"), value: "light" },
                            { label: qsTr("深色"), value: "dark" }
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
                        text: qsTr("通用")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("界面语言")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: theme.space1

                        ComboBox {
                            id: languageCombo
                            Layout.fillWidth: true
                            model: [
                                { label: qsTr("简体中文"), value: "zh_CN" },
                                { label: qsTr("English"), value: "en_US" }
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
                                    if (root.appController) {
                                        root.appController.applyUiLanguage()
                                    }
                                    languageApplyHint.text = qsTr("界面语言已更新。")
                                }
                            }
                        }

                        Label {
                            id: languageApplyHint
                            Layout.fillWidth: true
                            text: ""
                            visible: text !== ""
                            wrapMode: Text.Wrap
                            color: theme.colorTextMuted
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                        }
                    }

                    Label {
                        text: qsTr("通用")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("默认扫描目录")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    TextField {
                        id: scanPathField
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如: D:/Music")
                        onEditingFinished: {
                            if (root.settingsReady) {
                                root.settings.defaultScanDirectory = text
                            }
                        }
                    }

                    Label {
                        text: qsTr("通用")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("在线下载目录")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    TextField {
                        id: downloadPathField
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如: D:/Music/Downloads")
                        onEditingFinished: {
                            if (root.settingsReady) {
                                root.settings.defaultDownloadDirectory = text
                            }
                        }
                    }

                    Label {
                        text: qsTr("网络")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("歌词联网开关")
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

                    Label {
                        text: qsTr("播放")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: theme.space1

                        Label {
                            text: qsTr("歌词页频谱显示")
                            color: theme.colorTextSecondary
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                        }
                        Label {
                            text: qsTr("开启后会进行音频分析与界面绘制，可能增加 CPU 占用。")
                            wrapMode: Text.Wrap
                            color: theme.colorTextMuted
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            Layout.fillWidth: true
                        }
                    }
                    Switch {
                        id: spectrumEnabledSwitch
                        onToggled: {
                            if (root.settingsReady) {
                                root.settings.spectrumEnabled = checked
                            }
                        }
                    }

                    Label {
                        text: qsTr("播放")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("频谱刷新率（15–60 FPS）")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: theme.space1

                        Slider {
                            id: spectrumFpsSlider
                            Layout.fillWidth: true
                            from: 15
                            to: 60
                            stepSize: 1
                            onMoved: {
                                if (root.settingsReady) {
                                    const fps = Math.round(value)
                                    root.settings.spectrumFps = fps
                                    spectrumFpsValueLabel.text = fps + " FPS"
                                }
                            }
                            onValueChanged: {
                                spectrumFpsValueLabel.text = Math.round(value) + " FPS"
                            }
                        }

                        Label {
                            id: spectrumFpsValueLabel
                            text: "30 FPS"
                            color: theme.colorTextMuted
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                        }
                    }

                    Label {
                        text: qsTr("播放")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("频谱样式")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: theme.space1
                        enabled: root.spectrumOptionsEnabled

                        RadioButton {
                            id: spectrumStyleSmooth
                            text: qsTr("平滑镜像")
                            ButtonGroup.group: spectrumStyleGroup
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            onClicked: {
                                if (root.settingsReady) {
                                    root.settings.spectrumStyle = "smoothMirror"
                                }
                            }
                        }
                        RadioButton {
                            id: spectrumStyleBars
                            text: qsTr("镜像柱状")
                            ButtonGroup.group: spectrumStyleGroup
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            onClicked: {
                                if (root.settingsReady) {
                                    root.settings.spectrumStyle = "mirrorBars"
                                }
                            }
                        }
                        RadioButton {
                            id: spectrumStyleLine
                            text: qsTr("线条描边")
                            ButtonGroup.group: spectrumStyleGroup
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            onClicked: {
                                if (root.settingsReady) {
                                    root.settings.spectrumStyle = "lineTrace"
                                }
                            }
                        }
                        RadioButton {
                            id: spectrumStyleUpper
                            text: qsTr("上半光晕")
                            ButtonGroup.group: spectrumStyleGroup
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            onClicked: {
                                if (root.settingsReady) {
                                    root.settings.spectrumStyle = "upperGlow"
                                }
                            }
                        }
                        RadioButton {
                            id: spectrumStyleDots
                            text: qsTr("点阵屏")
                            ButtonGroup.group: spectrumStyleGroup
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            onClicked: {
                                if (root.settingsReady) {
                                    root.settings.spectrumStyle = "particleDots"
                                }
                            }
                        }
                    }

                    Label {
                        text: qsTr("播放")
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }
                    Label {
                        text: qsTr("频谱透明度")
                        color: theme.colorTextSecondary
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: theme.space1
                        enabled: root.spectrumOptionsEnabled

                        Slider {
                            id: spectrumOpacitySlider
                            Layout.fillWidth: true
                            from: 0.15
                            to: 1.0
                            stepSize: 0.05
                            onMoved: {
                                if (root.settingsReady) {
                                    root.settings.spectrumOpacity = value
                                    spectrumOpacityValueLabel.text = Math.round(value * 100) + "%"
                                }
                            }
                            onValueChanged: {
                                spectrumOpacityValueLabel.text = Math.round(value * 100) + "%"
                            }
                        }

                        Label {
                            id: spectrumOpacityValueLabel
                            text: "58%"
                            color: theme.colorTextMuted
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
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
                    text: qsTr("在线搜索、歌词、封面与流媒体解析使用的 Meting 接口。每行填写一个镜像根地址（需含 /api 或 /meting 路径），请求失败时会按顺序尝试下一行。")
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
                    placeholderText: "https://meting-api-omega.vercel.app/api\nhttps://api.injahow.cn/meting"
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
                          ? qsTr("配置文件：%1\n应用数据目录（歌单、曲库缓存、歌词缓存等）：%2")
                                .arg(root.settings.storageLocation)
                                .arg(root.settings.appDataLocation)
                          : ""
                    wrapMode: Text.Wrap
                    color: theme.colorTextMuted
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("快捷键：Ctrl+P 播放/暂停，Ctrl+Left 上一首，Ctrl+Right 下一首，Ctrl+F 聚焦曲库搜索。")
                    wrapMode: Text.Wrap
                    color: theme.colorTextMuted
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                }

                RowLayout {
                    spacing: theme.space2

                    Button {
                        text: qsTr("保存设置")
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
                        text: qsTr("重新加载")
                        hoverEnabled: true
                        onClicked: {
                            root.settings.load()
                            root.syncForm()
                            if (root.appController) {
                                root.appController.applyUiLanguage()
                            }
                        }
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            border.width: 1
                            border.color: theme.colorBorderDefault
                        }
                    }

                    Button {
                        text: qsTr("恢复默认")
                        hoverEnabled: true
                        onClicked: {
                            root.settings.reset()
                            root.syncForm()
                            if (root.appController) {
                                root.appController.applyUiLanguage()
                            }
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
        function onSpectrumEnabledChanged() { spectrumEnabledSwitch.checked = root.settings.spectrumEnabled }
        function onSpectrumFpsChanged() {
            spectrumFpsSlider.value = root.settings.spectrumFps
            spectrumFpsValueLabel.text = root.settings.spectrumFps + " FPS"
        }
        function onSpectrumStyleChanged() { root.syncSpectrumStyleRadios() }
        function onSpectrumOpacityChanged() {
            spectrumOpacitySlider.value = root.settings.spectrumOpacity
            spectrumOpacityValueLabel.text = Math.round(root.settings.spectrumOpacity * 100) + "%"
        }
        function onMetingApiBasesChanged() { metingApiBasesField.text = root.settings.metingApiBases }
    }
}
