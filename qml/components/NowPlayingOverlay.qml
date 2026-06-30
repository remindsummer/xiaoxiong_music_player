import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components" as Components
import "../theme" as ThemeTokens

Item {
    id: root
    required property var appController

    property bool presentationActive: visible

    readonly property var playback: root.appController
                                    ? (root.appController.playbackService || root.appController.playback)
                                    : null
    readonly property var lyrics: root.appController
                                  ? (root.appController.lyricsService || root.appController.lyrics)
                                  : null

    signal closeRequested()

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

        if (root.lyrics.loadLyricsForTrack(trackKey, trackKey,
                                           root.playback.currentOnlineServer,
                                           root.playback.currentOnlineId,
                                           root.playback.currentTrackTitle,
                                           root.playback.currentTrackArtist)) {
            return
        }

        const title = root.playback.currentTrackTitle || ""
        const artist = root.playback.currentTrackArtist || ""

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
        const title = root.playback ? root.playback.currentTrackTitle : ""
        const artist = root.playback ? root.playback.currentTrackArtist : ""
        if (root.playback && root.playback.currentTrackIsOnline) {
            root.lyrics.requestOnlineLyrics(root.playback.currentPath || "",
                                              title,
                                              artist,
                                              root.playback.currentOnlineServer,
                                              root.playback.currentOnlineId,
                                              root.playback.currentLrcUrl)
            return
        }
        root.lyrics.requestOnlineLyrics(root.playback ? root.playback.currentPath : "",
                                          title,
                                          artist)
    }

    readonly property string headerSubtitle: {
        if (!root.playback) {
            return ""
        }
        const parts = []
        const artist = root.playback.currentTrackArtist || ""
        const format = root.playback.currentFileFormat || ""
        if (artist !== "") {
            parts.push(artist)
        }
        if (format !== "") {
            parts.push(format)
        }
        return parts.join(" · ")
    }

    FileDialog {
        id: selectLyricDialog
        title: qsTr("选择歌词文件")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("歌词文件 (*.lrc *.txt)"), qsTr("所有文件 (*)")]
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

    Rectangle {
        id: panelBackground
        anchors.fill: parent
        radius: theme.radiusLg
        color: theme.colorBgPanel
        border.width: 1
        border.color: theme.surfaceGlassBorder
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: theme.space6
            spacing: theme.space4

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space3

            ColumnLayout {
                Layout.fillWidth: true
                spacing: theme.space1

                Label {
                    text: root.playback ? (root.playback.currentTrackTitle || root.playback.currentTrackName) : qsTr("未选择歌曲")
                    font.bold: true
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontH1
                    color: theme.colorTextPrimary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    text: root.headerSubtitle
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    color: theme.colorTextMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    visible: text !== ""
                }
            }

            Components.HiResBadge {
                visible: root.playback && root.playback.currentIsHiRes
            }

            ToolButton {
                icon.source: "../../assets/icons/back.svg"
                icon.color: theme.colorTextSecondary
                icon.width: 24
                icon.height: 24
                display: AbstractButton.IconOnly
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("返回")
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
            spacing: theme.space6

            ColumnLayout {
                Layout.preferredWidth: 360
                Layout.maximumWidth: 360
                Layout.fillHeight: true
                spacing: theme.space4

                Item {
                    Layout.preferredWidth: 360
                    Layout.preferredHeight: 360
                    Layout.maximumHeight: 360

                    readonly property bool hasCoverArt: root.playback
                                                         && root.playback.currentCoverPath !== ""
                    readonly property bool coverFetchEnabled: hasCoverArt === false
                                                              && root.playback
                                                              && root.playback.coverFetchState !== "fetching"

                    Components.RoundedCover {
                        id: nowPlayingCover
                        anchors.fill: parent
                        cornerRadius: theme.radiusXl
                        showShadow: true
                        placeholderColor: theme.colorPrimary
                        imageSource: {
                            if (root.playback && root.playback.currentCoverPath !== "") {
                                return root.playback.currentCoverPath
                            }
                            return "../../assets/icons/music_note.svg"
                        }
                        fallbackSource: "../../assets/icons/music_note.svg"
                    }

                    MouseArea {
                        id: coverFetchMouseArea
                        anchors.fill: parent
                        enabled: parent.coverFetchEnabled
                        hoverEnabled: true
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            if (root.playback) {
                                root.playback.retryFetchCurrentCover()
                            }
                        }
                    }

                    ToolTip {
                        visible: coverFetchMouseArea.containsMouse && coverFetchMouseArea.enabled
                        delay: 300
                        text: {
                            if (root.playback && root.playback.coverFetchState === "failed") {
                                const err = root.playback.coverFetchLastError || ""
                                return err !== ""
                                        ? qsTr("%1，点击重试").arg(err)
                                        : qsTr("获取封面失败，点击重试")
                            }
                            return qsTr("暂无封面，点击尝试在线获取")
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: theme.radiusXl
                        color: Qt.rgba(15 / 255, 23 / 255, 42 / 255, 0.35)
                        visible: root.playback && root.playback.coverFetchState === "fetching"

                        Column {
                            anchors.centerIn: parent
                            spacing: theme.space2

                            BusyIndicator {
                                anchors.horizontalCenter: parent.horizontalCenter
                                running: root.playback && root.playback.coverFetchState === "fetching"
                            }

                            Text {
                                text: qsTr("正在获取封面…")
                                color: "white"
                                font.pixelSize: theme.fontCaption
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }

                Components.TrackMetadataCard {
                    Layout.fillWidth: true
                    playback: root.playback
                }
            }

            Components.ImmersiveLyricsPanel {
                id: lyricsPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                appController: root.appController
                onRetryRequested: root.retryOnlineLyrics()
                onSelectLocalLyricRequested: selectLyricDialog.open()
                onLineActivated: (timestampMs) => {
                    if (root.lyrics && !root.lyrics.lyricsSynced) {
                        return
                    }
                    if (root.playback) {
                        root.playback.setPosition(timestampMs)
                    }
                }
            }
        }
    }
    }

    onPresentationActiveChanged: {
        if (!root.presentationActive) {
            return
        }

        if (root.lyrics && root.playback) {
            root.lyrics.setPlaybackPositionMs(root.playback.position)
        }

        tryLoadLyricsForCurrentAudio()
        lyricsPanel.refreshLyricLines()
    }
}
