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

    signal closeRequested()

    radius: theme.radiusLg
    color: theme.colorBgApp
    border.color: theme.colorBorderDefault
    border.width: 1

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

    function parseTrackTitleArtist() {
        let title = ""
        let artist = ""
        const trackName = root.playback ? (root.playback.currentTrackName || "") : ""
        if (trackName !== "" && trackName !== "未选择歌曲") {
            const splitIndex = trackName.indexOf(" - ")
            if (splitIndex > 0) {
                title = trackName.substring(0, splitIndex)
                artist = trackName.substring(splitIndex + 3)
            } else {
                title = trackName
            }
        }
        return { title: title, artist: artist }
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

        if (root.lyrics.loadLyricsForTrack(trackKey, trackKey)) {
            return
        }

        const parsed = root.parseTrackTitleArtist()

        if (root.playback.currentTrackIsOnline) {
            root.lyrics.requestOnlineLyrics(trackKey,
                                              parsed.title,
                                              parsed.artist,
                                              root.playback.currentOnlineServer,
                                              root.playback.currentOnlineId,
                                              root.playback.currentLrcUrl)
        } else if (parsed.title !== "") {
            root.lyrics.requestOnlineLyrics(trackKey, parsed.title, parsed.artist)
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
        const parsed = root.parseTrackTitleArtist()
        if (root.playback && root.playback.currentTrackIsOnline) {
            root.lyrics.requestOnlineLyrics(root.playback.currentPath || "",
                                              parsed.title,
                                              parsed.artist,
                                              root.playback.currentOnlineServer,
                                              root.playback.currentOnlineId,
                                              root.playback.currentLrcUrl)
            return
        }
        root.lyrics.requestOnlineLyrics(root.playback ? root.playback.currentPath : "",
                                          parsed.title,
                                          parsed.artist)
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
        anchors.margins: theme.space4
        spacing: theme.space3

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2

            Label {
                Layout.fillWidth: true
                text: root.playback ? root.playback.currentTrackName : "未选择歌曲"
                font.bold: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontH2
                color: theme.colorTextPrimary
                elide: Text.ElideRight
            }

            ToolButton {
                icon.source: "../../assets/icons/back.svg"
                icon.color: theme.colorTextSecondary
                icon.width: 24
                icon.height: 24
                display: AbstractButton.IconOnly
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: "返回"
                onClicked: root.closeRequested()
                background: Rectangle {
                    radius: width / 2
                    color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: theme.space4

            RoundedCover {
                Layout.preferredWidth: 300
                Layout.preferredHeight: 300
                Layout.maximumHeight: 300
                cornerRadius: theme.radiusMd
                placeholderColor: theme.colorPrimary
                imageSource: {
                    if (root.playback && root.playback.currentCoverPath !== "") {
                        return root.playback.currentCoverPath
                    }
                    return "../../assets/icons/music_note.svg"
                }
                fallbackSource: "../../assets/icons/music_note.svg"
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

    onVisibleChanged: {
        if (visible && root.lyrics && !root.lyrics.hasLyrics
                && root.lyrics.onlineFetchState !== "searching") {
            tryLoadLyricsForCurrentAudio()
        }
    }
}
