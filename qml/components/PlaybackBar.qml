import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Rectangle {
    id: root

    required property var appController
    readonly property var playback: root.appController
                                    ? (root.appController.playbackService || root.appController.playback)
                                    : null
    readonly property var playlistService: root.appController
                                           ? (root.appController.playlistService || root.appController.playlist)
                                           : null

    // 由 Main 监听，用于打开/关闭歌词浮层。
    signal lyricsRequested()
    signal queueRequested()
    property bool nowPlayingOverlayVisible: false

    // 静音切换时记忆此前音量。
    property int lastVolume: 80
    property bool favoriteState: false

    Layout.fillWidth: true
    radius: theme.radiusLg
    color: theme.colorBgElevated
    border.color: theme.colorBorderDefault
    border.width: 1
    implicitHeight: 118

    ThemeTokens.Theme {
        id: theme
    }

    readonly property bool isPlaying: !!root.playback && root.playback.playbackStateText === "播放中"
    readonly property bool hasCurrentTrack: !!root.playback && (root.playback.currentPath || "") !== ""
    readonly property int modeIndex: root.playback ? root.playback.playbackMode : 0
    readonly property int volumeValue: root.playback ? root.playback.volume : 80

    function refreshFavoriteState() {
        if (!root.playlistService || !root.playback) {
            root.favoriteState = false
            return
        }
        const path = root.playback.currentPath || ""
        root.favoriteState = path !== "" && root.playlistService.isFavorite(path)
    }

    function buildFavoriteTrackMap() {
        if (!root.playback) {
            return {}
        }
        const path = root.playback.currentPath || ""
        if (path === "") {
            return {}
        }
        const trackName = root.playback.currentTrackName || ""
        let title = trackName
        let artist = ""
        const splitIndex = trackName.indexOf(" - ")
        if (splitIndex > 0) {
            title = trackName.substring(0, splitIndex)
            artist = trackName.substring(splitIndex + 3)
        }
        const map = {
            path: path,
            title: title,
            artist: artist,
            isOnline: root.playback.currentTrackIsOnline
        }
        if (root.playback.currentTrackIsOnline) {
            map.onlineId = root.playback.currentOnlineId
            map.server = root.playback.currentOnlineServer
            map.picUrl = root.playback.currentPicUrl
        }
        return map
    }

    function toggleFavorite() {
        if (!root.playlistService || !root.playback) {
            return
        }
        const trackMap = root.buildFavoriteTrackMap()
        if (!trackMap.path) {
            return
        }
        root.playlistService.toggleFavoriteTrack(trackMap)
        root.playlistService.saveToStorage()
        root.refreshFavoriteState()
    }

    Component.onCompleted: refreshFavoriteState()

    Connections {
        target: root.playlistService
        ignoreUnknownSignals: true
        function onChanged() { root.refreshFavoriteState() }
    }

    Connections {
        target: root.playback
        ignoreUnknownSignals: true
        function onCurrentTrackChanged() { root.refreshFavoriteState() }
    }

    function modeIconSource() {
        // 0 顺序 / 1 列表循环 / 2 单曲循环 / 3 随机
        if (root.modeIndex === 1) {
            return "../../assets/icons/mode_repeat_all.svg"
        }
        if (root.modeIndex === 2) {
            return "../../assets/icons/mode_repeat_one.svg"
        }
        if (root.modeIndex === 3) {
            return "../../assets/icons/mode_shuffle.svg"
        }
        return "../../assets/icons/mode_sequential.svg"
    }

    function toggleMute() {
        if (!root.playback) {
            return
        }
        if (root.volumeValue > 0) {
            root.lastVolume = root.volumeValue
            root.playback.setVolume(0)
        } else {
            root.playback.setVolume(root.lastVolume > 0 ? root.lastVolume : 60)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: theme.space4
        anchors.rightMargin: theme.space4
        anchors.topMargin: theme.space2
        anchors.bottomMargin: theme.space2
        spacing: theme.space3

        // ----- 左：封面（跨进度条 + 控制行两行）-----
        RoundedCover {
            id: coverBlock
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            Layout.maximumHeight: 64
            cornerRadius: theme.radiusMd
            placeholderColor: theme.colorPrimary
            opacity: root.hasCurrentTrack ? 1 : 0.75
            imageSource: {
                if (root.playback && root.playback.currentCoverPath !== "") {
                    return root.playback.currentCoverPath
                }
                return "../../assets/icons/music_note.svg"
            }
            fallbackSource: "../../assets/icons/music_note.svg"

            MouseArea {
                anchors.fill: parent
                enabled: root.hasCurrentTrack
                hoverEnabled: root.hasCurrentTrack
                cursorShape: root.hasCurrentTrack ? Qt.PointingHandCursor : Qt.ArrowCursor
                ToolTip.visible: containsMouse && root.hasCurrentTrack
                ToolTip.text: root.nowPlayingOverlayVisible ? "关闭歌词" : "查看歌词"
                onClicked: root.lyricsRequested()
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: theme.space1

            // ----- 第 1 行：进度条（封面右侧，播放按钮上方）-----
            RowLayout {
                Layout.fillWidth: true
                spacing: theme.space2

                Label {
                    text: root.playback ? root.playback.formatTime(root.playback.position) : "0:00"
                    color: theme.colorTextMuted
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    Layout.preferredWidth: 40
                    horizontalAlignment: Text.AlignRight
                }

                ThemedSlider {
                    id: progressSlider
                    Layout.fillWidth: true
                    from: 0
                    to: root.playback ? Math.max(root.playback.duration, 1) : 1
                    value: root.playback ? root.playback.position : 0
                    enabled: root.hasCurrentTrack
                    opacity: enabled ? 1 : 0.55
                    onMoved: {
                        if (root.playback && root.hasCurrentTrack) {
                            root.playback.setPosition(value)
                        }
                    }
                }

                Label {
                    text: root.playback ? root.playback.formatTime(root.playback.duration) : "0:00"
                    color: theme.colorTextMuted
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    Layout.preferredWidth: 40
                }
            }

            // ----- 第 2 行：歌名 | 播放控制 | 右侧工具 -----
            RowLayout {
                Layout.fillWidth: true
                spacing: theme.space3

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    spacing: theme.space1

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        MarqueeLabel {
                            Layout.fillWidth: true
                            text: root.playback ? root.playback.currentTrackName : "未选择歌曲"
                            color: theme.colorTextPrimary
                            fontFamily: theme.fontFamily
                            fontPixelSize: theme.fontBody
                            fontBold: true
                        }

                        Label {
                            text: root.playback
                                  ? (root.playback.errorMessage !== "" ? root.playback.errorMessage : root.playback.playbackStateText)
                                  : "待机"
                            color: root.playback && root.playback.errorMessage !== "" ? theme.colorStateError : theme.colorTextMuted
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontCaption
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    ToolButton {
                        icon.source: root.favoriteState ? "../../assets/icons/heart_filled.svg" : "../../assets/icons/heart.svg"
                        icon.color: root.favoriteState ? theme.colorStateError : theme.colorTextMuted
                        icon.width: 20
                        icon.height: 20
                        display: AbstractButton.IconOnly
                        hoverEnabled: true
                        enabled: root.hasCurrentTrack
                        ToolTip.visible: hovered
                        ToolTip.text: root.favoriteState ? "取消收藏" : "添加到我喜欢"
                        onClicked: root.toggleFavorite()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }
                }

                RowLayout {
                    spacing: theme.space3

                    ToolButton {
                        icon.source: "../../assets/icons/prev.svg"
                        icon.color: enabled ? theme.colorTextSecondary : theme.colorTextMuted
                        icon.width: 24
                        icon.height: 24
                        display: AbstractButton.IconOnly
                        enabled: !!root.playback && root.playback.hasPrevious
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: "上一首"
                        onClicked: root.playback.previous()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }

                    RoundButton {
                        implicitWidth: 52
                        implicitHeight: 52
                        radius: width / 2
                        icon.source: root.isPlaying ? "../../assets/icons/pause.svg" : "../../assets/icons/play.svg"
                        icon.color: "#ffffff"
                        icon.width: 28
                        icon.height: 28
                        display: AbstractButton.IconOnly
                        enabled: !!root.playback
                        hoverEnabled: true
                        onClicked: root.playback.togglePlayPause()
                        background: Rectangle {
                            radius: width / 2
                            color: !parent.enabled
                                   ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.35)
                                   : (parent.down || parent.hovered ? theme.colorPrimaryActive : theme.colorPrimary)
                            Behavior on color {
                                ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
                            }
                        }
                    }

                    ToolButton {
                        icon.source: "../../assets/icons/next.svg"
                        icon.color: enabled ? theme.colorTextSecondary : theme.colorTextMuted
                        icon.width: 24
                        icon.height: 24
                        display: AbstractButton.IconOnly
                        enabled: !!root.playback && root.playback.hasNext
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: "下一首"
                        onClicked: root.playback.next()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }

                    ToolButton {
                        icon.source: root.modeIconSource()
                        icon.color: root.modeIndex === 0 ? theme.colorTextSecondary : theme.colorPrimary
                        icon.width: 22
                        icon.height: 22
                        display: AbstractButton.IconOnly
                        enabled: !!root.playback
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: root.playback ? ("播放模式：" + root.playback.playbackModeText) : "播放模式"
                        onClicked: root.playback.cyclePlaybackMode()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    spacing: theme.space1

                    Item { Layout.fillWidth: true }

                    ToolButton {
                        icon.source: root.volumeValue > 0 ? "../../assets/icons/volume.svg" : "../../assets/icons/volume_mute.svg"
                        icon.color: theme.colorTextSecondary
                        icon.width: 22
                        icon.height: 22
                        display: AbstractButton.IconOnly
                        enabled: !!root.playback
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: root.volumeValue > 0 ? "静音" : "取消静音"
                        onClicked: root.toggleMute()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }

                    ThemedSlider {
                        from: 0
                        to: 100
                        value: root.volumeValue
                        enabled: !!root.playback
                        Layout.preferredWidth: 96
                        opacity: enabled ? 1 : 0.55
                        onMoved: {
                            if (root.playback) {
                                root.playback.setVolume(Math.round(value))
                            }
                        }
                    }

                    ToolButton {
                        icon.source: "../../assets/icons/lyrics.svg"
                        icon.color: theme.colorTextSecondary
                        icon.width: 22
                        icon.height: 22
                        display: AbstractButton.IconOnly
                        hoverEnabled: true
                        enabled: root.hasCurrentTrack
                        ToolTip.visible: hovered
                        ToolTip.text: root.nowPlayingOverlayVisible ? "关闭歌词" : "歌词"
                        onClicked: root.lyricsRequested()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }

                    ToolButton {
                        icon.source: "../../assets/icons/queue.svg"
                        icon.color: theme.colorTextSecondary
                        icon.width: 22
                        icon.height: 22
                        display: AbstractButton.IconOnly
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: "播放列表"
                        onClicked: root.queueRequested()
                        background: Rectangle {
                            radius: width / 2
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                        }
                    }
                }
            }
        }
    }
}
