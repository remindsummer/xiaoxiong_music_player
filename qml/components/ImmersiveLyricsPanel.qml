import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

GlassPanel {
    id: root

    property var appController: null
    property var audioVisualizer: null

    signal lineActivated(int timestampMs)
    signal retryRequested()
    signal selectLocalLyricRequested()

    cornerRadius: theme.radiusMd
    showShadow: false
    padding: 0
    clip: true

    readonly property var playback: root.appController
                                    ? (root.appController.playbackService || root.appController.playback)
                                    : null
    readonly property var lyrics: root.appController
                                  ? (root.appController.lyricsService || root.appController.lyrics)
                                  : null
    readonly property var visualizer: root.audioVisualizer
                                      ? root.audioVisualizer
                                      : (root.appController
                                         ? (root.appController.audioVisualizer || null)
                                         : null)
    readonly property var settings: root.appController
                                    ? (root.appController.settingsService || root.appController.settings)
                                    : null

    readonly property var lyricLines: privateState.lines
    readonly property int activeLineIndex: privateState.activeIndex
    readonly property string errorText: privateState.errorText
    readonly property string effectiveState: privateState.state
    readonly property bool lyricsSynced: root.lyrics ? root.lyrics.lyricsSynced : true
    readonly property string plainLyricsText: root.lyrics ? (root.lyrics.plainLyricsText || "") : ""

    ThemeTokens.Theme {
        id: theme
    }

    QtObject {
        id: privateState
        property var lines: []
        property int activeIndex: -1
        property string errorText: ""
        property string state: "idle"
    }

    function refreshLyricLines() {
        if (!root.lyrics) {
            privateState.lines = []
            privateState.activeIndex = -1
            privateState.state = "idle"
            return
        }

        privateState.lines = root.lyrics.lyricLinesForQml()
        privateState.activeIndex = root.lyrics.currentLineIndex
        privateState.errorText = root.lyrics.onlineLastError || ""

        if (privateState.lines.length > 0 || (root.lyrics.plainLyricsText || "") !== "") {
            privateState.state = "ready"
        } else if ((privateState.errorText || "").length > 0) {
            privateState.state = "error"
        } else if (root.lyrics.onlineFetchState === "searching") {
            privateState.state = "loading"
        } else {
            privateState.state = "empty"
        }

        if (privateState.activeIndex >= 0 && root.lyricsSynced && privateState.activeIndex < lyricListView.count) {
            lyricListView.positionViewAtIndex(privateState.activeIndex, ListView.Center)
        }
    }

    Connections {
        target: root.lyrics
        ignoreUnknownSignals: true
        function onLyricsChanged() { root.refreshLyricLines() }
        function onLyricsSyncedChanged() { root.refreshLyricLines() }
        function onOnlineFetchStateChanged() { root.refreshLyricLines() }
        function onOnlineLastErrorChanged() {
            privateState.errorText = root.lyrics ? (root.lyrics.onlineLastError || "") : ""
        }
        function onCurrentLineIndexChanged() {
            privateState.activeIndex = root.lyrics ? root.lyrics.currentLineIndex : -1
            if (privateState.activeIndex >= 0 && root.lyricsSynced && privateState.activeIndex < lyricListView.count) {
                lyricListView.positionViewAtIndex(privateState.activeIndex, ListView.Center)
            }
        }
    }

    Component.onCompleted: refreshLyricLines()

    SpectrumVisualizer {
        id: spectrum
        anchors.fill: parent
        anchors.leftMargin: theme.space8
        anchors.rightMargin: theme.space8
        levels: root.visualizer ? root.visualizer.spectrumLevels : []
        active: root.visualizer ? root.visualizer.spectrumAvailable : false
        showEnabled: root.settings ? root.settings.spectrumEnabled : true
        visible: showEnabled
        refreshFps: root.settings ? root.settings.spectrumFps : 30
        style: root.settings ? root.settings.spectrumStyle : "smoothMirror"
        styleOpacity: root.settings ? root.settings.spectrumOpacity : 0.58
    }

    ListView {
        id: lyricListView
        anchors.fill: parent
        model: privateState.lines
        clip: true
        spacing: theme.space2
        boundsBehavior: Flickable.StopAtBounds
        visible: root.effectiveState === "ready" && root.lyricsSynced
        opacity: visible ? 1 : 0
        z: 1

        delegate: Item {
            required property int index
            required property var modelData

            width: lyricListView.width
            height: lyricText.implicitHeight + theme.space2

            Label {
                id: lyricText
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - theme.space8
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: index === root.activeLineIndex ? theme.colorLyricActive : theme.colorLyricInactive
                font.bold: index === root.activeLineIndex
                font.family: theme.fontFamily
                font.pixelSize: index === root.activeLineIndex ? 20 : 15
                opacity: index === root.activeLineIndex ? 1.0 : 0.72
                text: {
                    const lineText = modelData && modelData.text ? modelData.text : ""
                    return lineText !== "" ? lineText : "..."
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!modelData) {
                        return
                    }
                    const timestamp = Number(modelData.timestampMs || modelData.timeMs || 0)
                    root.lineActivated(timestamp)
                }
            }
        }
    }

    ScrollView {
        id: plainLyricsScroll
        anchors.fill: parent
        clip: true
        visible: root.effectiveState === "ready" && !root.lyricsSynced
        opacity: visible ? 1 : 0
        z: 1

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        Column {
            width: plainLyricsScroll.availableWidth
            spacing: theme.space2

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: theme.colorTextMuted
                font.family: theme.fontFamily
                font.pixelSize: theme.fontCaption
                text: qsTr("静态歌词，不支持滚动同步与点击跳转")
            }

            Text {
                width: parent.width - theme.space8
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                color: theme.colorLyricInactive
                font.family: theme.fontFamily
                font.pixelSize: 15
                text: root.plainLyricsText
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        z: 4
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                lyricsContextMenu.popup()
            }
        }
    }

    LyricsContextMenu {
        id: lyricsContextMenu
        lyrics: root.lyrics
        onRetryRequested: root.retryRequested()
        onSelectLocalLyricRequested: root.selectLocalLyricRequested()
    }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: theme.space10
        radius: theme.radiusMd
        gradient: Gradient {
            GradientStop { position: 0.0; color: theme.surfaceGlass }
            GradientStop { position: 1.0; color: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0) }
        }
        z: 2
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: theme.space10
        radius: theme.radiusMd
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0) }
            GradientStop { position: 1.0; color: theme.surfaceGlass }
        }
        z: 2
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - theme.space8
        spacing: theme.space2
        visible: root.effectiveState !== "ready"
        z: 3

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: root.effectiveState === "error" ? theme.colorStateError : theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
            text: root.effectiveState === "loading"
                  ? qsTr("正在加载歌词，请稍候...")
                  : qsTr("暂无歌词，支持自动加载同名 .lrc、Meting 在线歌词或手动选择歌词文件。")
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: theme.colorStateError
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
            text: root.errorText
            visible: text !== ""
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: theme.space2
            visible: root.effectiveState === "error" || root.effectiveState === "empty"

            PrimaryButton {
                text: qsTr("重试")
                onClicked: root.retryRequested()
            }

            GhostButton {
                text: qsTr("选择歌词文件")
                onClicked: root.selectLocalLyricRequested()
            }
        }
    }
}
