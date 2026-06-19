import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Rectangle {
    id: root
    required property var appController

    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: theme.radiusMd
    color: "transparent"

    readonly property var playlistService: root.appController
                                           ? (root.appController.playlistService || root.appController.playlist)
                                           : null
    readonly property var libraryRepository: root.appController
                                             ? (root.appController.libraryService || root.appController.libraryRepository)
                                             : null
    readonly property var playback: root.appController
                                    ? (root.appController.playbackService || root.appController.playback)
                                    : null
    readonly property string currentPlayingPath: playback ? playback.currentPath : ""

    property var trackItems: []
    property var libraryTrackMap: ({})

    ThemeTokens.Theme {
        id: theme
    }

    function rebuildLibraryTrackMap() {
        const map = {}
        if (!libraryRepository) {
            root.libraryTrackMap = map
            return
        }
        const allTracks = libraryRepository.allTracks()
        for (let i = 0; i < allTracks.length; ++i) {
            const track = allTracks[i]
            if (track && track.path) {
                map[track.path] = track
            }
        }
        root.libraryTrackMap = map
    }

    function resolveTrackItem(rawItem) {
        if (!rawItem) {
            return null
        }
        const path = rawItem.path || ""
        if (path.startsWith("online:") || rawItem.isOnline) {
            return rawItem
        }
        const libraryTrack = root.libraryTrackMap[path]
        if (libraryTrack) {
            return libraryTrack
        }
        return rawItem
    }

    function refreshTrackItems() {
        if (!playlistService) {
            trackItems = []
            return
        }
        const favoritesId = playlistService.favoritesPlaylistId()
        if (!favoritesId) {
            trackItems = []
            return
        }
        trackItems = playlistService.tracks(favoritesId)
    }

    Component.onCompleted: {
        rebuildLibraryTrackMap()
        refreshTrackItems()
    }

    Connections {
        target: root.playlistService
        ignoreUnknownSignals: true
        function onChanged() { root.refreshTrackItems() }
    }

    Connections {
        target: root.libraryRepository
        ignoreUnknownSignals: true
        function onTracksUpdated() { root.rebuildLibraryTrackMap() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.space3

        Label {
            text: "我喜欢"
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH1
            color: theme.colorTextPrimary
        }

        Label {
            text: "共 " + root.trackItems.length + " 首 · 点击播放栏红心可添加/移除 · 移除仅取消收藏，不删除本地文件"
            wrapMode: Text.WordWrap
            color: theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2

            Item { Layout.fillWidth: true }

            Button {
                text: "播放全部"
                enabled: !!playback && root.trackItems.length > 0
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: {
                    const playable = []
                    for (let i = 0; i < root.trackItems.length; ++i) {
                        const resolved = root.resolveTrackItem(root.trackItems[i])
                        if (resolved) {
                            playable.push(resolved)
                        }
                    }
                    if (playable.length > 0) {
                        playback.playQueue(playable, 0)
                    }
                }
                background: Rectangle {
                    radius: theme.radiusXs
                    color: !parent.enabled
                           ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.35)
                           : (parent.down
                              ? theme.colorPrimaryActive
                              : (parent.hovered ? theme.colorPrimaryActive : theme.colorPrimary))
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
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: theme.radiusMd
            color: theme.colorBgPanel
            border.color: theme.colorBorderDefault

            ListView {
                id: trackListView
                anchors.fill: parent
                anchors.margins: theme.space2
                clip: true
                spacing: theme.space2
                model: root.trackItems

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Rectangle {
                    id: favDelegate
                    required property var modelData
                    required property int index
                    readonly property var resolvedTrack: root.resolveTrackItem(modelData)
                    readonly property bool hovered: favHover.hovered
                    readonly property bool isPlaying: !!resolvedTrack
                                                      && (resolvedTrack.path || "") !== ""
                                                      && resolvedTrack.path === root.currentPlayingPath
                    width: trackListView.width
                    implicitHeight: 52
                    radius: theme.radiusSm
                    color: favDelegate.isPlaying
                           ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.15)
                           : (favDelegate.hovered ? theme.colorBgHover : "transparent")
                    border.width: (favDelegate.isPlaying || favDelegate.hovered) ? 1 : 0
                    border.color: favDelegate.isPlaying ? theme.colorPrimaryActive : theme.colorBorderFocus

                    HoverHandler {
                        id: favHover
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: {
                            if (root.playback && favDelegate.resolvedTrack) {
                                root.playback.enqueueAndPlay(favDelegate.resolvedTrack)
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: theme.space2
                        spacing: theme.space2

                        Image {
                            visible: favDelegate.isPlaying
                            source: "../../assets/icons/play.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            Layout.preferredWidth: visible ? 14 : 0
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (favDelegate.resolvedTrack && favDelegate.resolvedTrack.title || "未知标题")
                                  + " - "
                                  + (favDelegate.resolvedTrack && favDelegate.resolvedTrack.artist || "未知歌手")
                            color: favDelegate.isPlaying ? theme.colorPrimaryActive : theme.colorTextPrimary
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontBody
                            elide: Text.ElideMiddle
                        }

                        Button {
                            text: "移除"
                            hoverEnabled: true
                            onClicked: {
                                if (playlistService && favDelegate.modelData && favDelegate.modelData.path) {
                                    playlistService.removeTrack(playlistService.favoritesPlaylistId(),
                                                                favDelegate.modelData.path)
                                    playlistService.saveToStorage()
                                    root.refreshTrackItems()
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
                                font.pixelSize: theme.fontCaption
                            }
                        }
                    }
                }
            }
        }
    }
}
