import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components" as Components
import "../theme" as ThemeTokens

Rectangle {
    id: root
    required property var appController
    readonly property var playback: root.appController
                                    ? (root.appController.playbackService || root.appController.playback)
                                    : null
    readonly property var lyrics: root.appController
                                  ? (root.appController.lyricsService || root.appController.lyrics)
                                  : null
    property string currentAudioFilePath: ""

    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: theme.radiusMd
    color: "transparent"

    ThemeTokens.Theme {
        id: theme
    }

    function normalizeLocalPath(urlString) {
        const raw = String(urlString || "").trim()
        if (raw.length === 0) {
            return ""
        }
        const decoded = decodeURIComponent(raw)
        if (decoded.indexOf("file:///") === 0) {
            return decoded.substring("file:///".length)
        }
        if (decoded.indexOf("file://") === 0) {
            return decoded.substring("file://".length)
        }
        return decoded
    }

    function tryLoadLyricsForCurrentAudio() {
        if (!root.lyrics || !root.playback) {
            return
        }

        const trackKey = root.playback.currentPath || ""
        if (trackKey === "") {
            root.lyrics.clearLyrics()
            return
        }

        root.currentAudioFilePath = trackKey

        // 优先：同名 .lrc → 本地缓存 → 在线 Meting
        if (root.lyrics.loadLyricsForTrack(trackKey, trackKey)) {
            return
        }

        let title = ""
        let artist = ""
        const trackName = root.playback.currentTrackName || ""
        if (trackName !== "" && trackName !== "未选择歌曲") {
            const splitIndex = trackName.indexOf(" - ")
            if (splitIndex > 0) {
                title = trackName.substring(0, splitIndex)
                artist = trackName.substring(splitIndex + 3)
            } else {
                title = trackName
            }
        }

        if (root.playback.currentTrackIsOnline) {
            root.lyrics.requestOnlineLyrics(trackKey,
                                              title,
                                              artist,
                                              root.playback.currentOnlineServer,
                                              root.playback.currentOnlineId,
                                              root.playback.currentLrcUrl)
        } else if (title !== "") {
            root.lyrics.requestOnlineLyrics(trackKey, title, artist)
        }
    }

    function retryOnlineLyrics() {
        if (!root.lyrics) {
            return
        }
        if (root.lyrics.onlineRetryRequired) {
            root.lyrics.retryOnlineLyrics()
            return
        }
        if (root.playback && root.playback.currentTrackIsOnline) {
            root.lyrics.requestOnlineLyrics("",
                                              root.playback.currentTrackName,
                                              "",
                                              root.playback.currentOnlineServer,
                                              root.playback.currentOnlineId,
                                              root.playback.currentLrcUrl)
            return
        }
        const title = root.playback ? root.playback.currentTrackName : ""
        root.lyrics.requestOnlineLyrics("", title, "")
    }

    FileDialog {
        id: openFileDialog
        title: "选择音频文件"
        fileMode: FileDialog.OpenFile
        nameFilters: ["音频文件 (*.mp3 *.wav *.flac *.aac *.m4a *.ogg)", "所有文件 (*)"]
        onAccepted: {
            if (root.playback && selectedFile.toString() !== "") {
                const localPath = root.normalizeLocalPath(selectedFile.toString())
                if (localPath !== "" && typeof root.playback.openFilePath === "function") {
                    root.playback.openFilePath(localPath)
                } else {
                    root.playback.openFile(selectedFile)
                }
                root.currentAudioFilePath = localPath
                root.tryLoadLyricsForCurrentAudio()
            }
        }
    }

    FileDialog {
        id: selectLyricDialog
        title: "选择歌词文件"
        fileMode: FileDialog.OpenFile
        nameFilters: ["歌词文件 (*.lrc)", "所有文件 (*)"]
        onAccepted: {
            if (root.lyrics && selectedFile.toString() !== "") {
                const localPath = root.normalizeLocalPath(selectedFile.toString())
                if (localPath !== "") {
                    const trackKey = root.playback ? root.playback.currentPath : ""
                    root.lyrics.loadLyricsFromFile(localPath, trackKey)
                }
            }
        }
    }

    Connections {
        target: root.playback
        ignoreUnknownSignals: true

        function onPositionChanged() {
            if (root.lyrics) {
                root.lyrics.setPlaybackPositionMs(root.playback.position)
            }
        }

        function onCurrentTrackChanged() {
            root.tryLoadLyricsForCurrentAudio()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.space3

        Label {
            text: "正在播放"
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH1
            color: theme.colorTextPrimary
        }

        Rectangle {
            Layout.fillWidth: true
            radius: theme.radiusMd
            color: theme.colorBgPanel
            border.width: 1
            border.color: root.playback && root.playback.errorMessage !== "" ? theme.colorStateError : theme.colorBorderDefault

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: theme.space3
                spacing: theme.space2

                Label {
                    text: root.playback ? root.playback.currentTrackName : "未注入 playback"
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontH2
                    font.weight: 600
                    color: theme.colorTextPrimary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    text: root.playback ? root.playback.statusText : "待机"
                    color: theme.colorTextSecondary
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontBody
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }

                Label {
                    visible: root.playback && root.playback.errorMessage !== ""
                    text: root.playback ? root.playback.errorMessage : ""
                    color: theme.colorStateError
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: theme.space2

                    Button {
                        text: "打开文件"
                        hoverEnabled: true
                        onClicked: openFileDialog.open()
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            border.width: 1
                            border.color: theme.colorBorderDefault
                        }
                    }

                    Button {
                        text: "自动加载本地歌词"
                        enabled: !!root.lyrics && root.currentAudioFilePath !== ""
                        hoverEnabled: true
                        onClicked: root.tryLoadLyricsForCurrentAudio()
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: !parent.enabled
                                   ? theme.colorBgHover
                                   : (parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent"))
                            border.width: 1
                            border.color: !parent.enabled ? theme.colorTextMuted : theme.colorBorderDefault
                        }
                    }

                    Button {
                        text: "选择歌词文件"
                        enabled: !!root.lyrics
                        hoverEnabled: true
                        onClicked: selectLyricDialog.open()
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: !parent.enabled
                                   ? theme.colorBgHover
                                   : (parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent"))
                            border.width: 1
                            border.color: !parent.enabled ? theme.colorTextMuted : theme.colorBorderDefault
                        }
                    }

                    Button {
                        text: root.lyrics && root.lyrics.onlineFetchState === "searching" ? "检索中..." : "手动重试在线检索"
                        enabled: !!root.lyrics && (!root.playback || root.playback.currentTrackName !== "") && (root.lyrics.onlineFetchState !== "searching")
                        hoverEnabled: true
                        onClicked: root.retryOnlineLyrics()
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: !parent.enabled
                                   ? Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.25)
                                   : (parent.down
                                      ? Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.95)
                                      : (parent.hovered ? Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.9) : theme.colorStateError))
                            border.width: 0
                        }
                        contentItem: Label {
                            text: parent.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#ffffff"
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            font.weight: 600
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: theme.colorTextMuted
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                        text: root.lyrics && root.lyrics.lyricFilePath !== "" ? "当前歌词: " + root.lyrics.lyricFilePath : "当前歌词: 未加载"
                    }
                }
            }
        }

        Components.LyricsPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            appController: root.appController
            onRetryRequested: root.retryOnlineLyrics()
            onSelectLocalLyricRequested: selectLyricDialog.open()
            onLineActivated: {
                if (root.playback) {
                    root.playback.setPosition(timestampMs)
                }
            }
        }
    }
}
