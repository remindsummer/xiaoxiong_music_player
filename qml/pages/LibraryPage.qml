import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components" as Components
import "../theme" as ThemeTokens

Rectangle {
    id: root
    required property var appController

    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: theme.radiusMd
    color: "transparent"

    property var visibleTracks: []
    property bool scanPathError: false
    property string selectedPath: ""
    property var playlistChoices: []
    property var pendingTrack: null
    property int libraryTabIndex: 0
    property int libraryTabSlideDirection: 1
    property var onlineTracks: []
    readonly property var libraryRepository: root.resolveControllerService(["libraryService", "libraryRepository"], "LibraryRepository")
    readonly property var searchService: root.resolveControllerService(["searchService", "search"], "SearchService")
    readonly property var playback: root.resolveControllerService(["playbackService", "playback"], "PlaybackService")
    readonly property var playlistService: root.resolveControllerService(["playlistService", "playlist"], "PlaylistService")
    readonly property var downloadService: root.resolveControllerService(["downloadService"], "OnlineDownloadService")
    readonly property string currentPlayingPath: playback ? playback.currentPath : ""

    ThemeTokens.Theme {
        id: theme
    }

    function resolveControllerService(propertyNames, className) {
        if (!root.appController) {
            return null
        }

        for (let index = 0; index < propertyNames.length; index += 1) {
            const directService = root.appController[propertyNames[index]]
            if (directService) {
                return directService
            }
        }

        const controllerChildren = root.appController.children || []
        for (let i = 0; i < controllerChildren.length; ++i) {
            const child = controllerChildren[i]
            if (!child || !child.toString) {
                continue
            }
            if (child.toString().indexOf(className) !== -1) {
                return child
            }
        }

        return null
    }

    function formatDuration(durationMs) {
        const safeMs = Number(durationMs || 0)
        if (safeMs <= 0) {
            return "--:--"
        }
        const totalSeconds = Math.floor(safeMs / 1000)
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function playTrackAt(index) {
        if (!playback || index < 0 || index >= root.visibleTracks.length) {
            return
        }
        playback.enqueueAndPlay(root.visibleTracks[index])
    }

    function playOnlineTrackAt(index) {
        if (!playback || index < 0 || index >= root.onlineTracks.length) {
            return
        }
        playback.enqueueAndPlay(root.onlineTracks[index])
    }

    function playAllVisibleTracks() {
        if (!playback || root.visibleTracks.length === 0) {
            return
        }
        playback.playQueue(root.visibleTracks, 0)
    }

    function playAllOnlineTracks() {
        if (!playback || root.onlineTracks.length === 0) {
            return
        }
        playback.playQueue(root.onlineTracks, 0)
    }

    function downloadOnlineTrack(track) {
        if (!downloadService || !track) {
            return
        }
        downloadService.downloadTrack(track)
    }

    function triggerOnlineSearch() {
        if (!searchService) {
            return
        }
        searchService.searchOnline(onlineSearchBar.text, "netease")
    }

    function syncOnlineTracks() {
        root.onlineTracks = searchService ? searchService.onlineResults : []
    }

    function openAddToPlaylistMenu(track, anchorItem) {
        if (!playlistService || !track) {
            return
        }
        root.pendingTrack = track
        root.playlistChoices = playlistService.playlists()
        addToPlaylistMenu.parent = anchorItem
        addToPlaylistMenu.x = 0
        addToPlaylistMenu.y = anchorItem ? anchorItem.height : 0
        addToPlaylistMenu.open()
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

    function refreshVisibleTracks() {
        if (!libraryRepository) {
            visibleTracks = []
            return
        }

        const allTracks = libraryRepository.allTracks()
        if (!searchService) {
            visibleTracks = libraryRepository.filterTracks(searchBar.text)
            return
        }
        visibleTracks = searchService.searchTracks(allTracks, searchBar.text)
    }

    function focusSearchField() {
        searchBar.forceActiveFocusOnInput()
    }

    Timer {
        id: searchDebounceTimer
        interval: 150
        repeat: false
        onTriggered: root.refreshVisibleTracks()
    }

    Connections {
        target: root.libraryRepository
        ignoreUnknownSignals: true
        function onTracksUpdated() {
            root.refreshVisibleTracks()
        }
    }

    Connections {
        target: root.searchService
        ignoreUnknownSignals: true
        function onOnlineResultsChanged() { root.syncOnlineTracks() }
        function onOnlineSearchStateChanged() { root.syncOnlineTracks() }
    }

    FolderDialog {
        id: scanFolderDialog
        title: qsTr("选择要扫描的目录")
        onAccepted: {
            const selectedPath = root.normalizeLocalPath(selectedFolder.toString())
            directoryField.text = selectedPath
            if (selectedPath.length > 0 && libraryRepository) {
                root.scanPathError = false
                libraryRepository.scanAndStore(selectedPath)
                root.refreshVisibleTracks()
            }
        }
    }

    FileDialog {
        id: openSingleTrackDialog
        title: qsTr("选择音频文件")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("音频文件 (*.mp3 *.wav *.flac *.aac *.m4a *.ogg)"), qsTr("所有文件 (*)")]
        onAccepted: {
            if (!playback || selectedFile.toString() === "") {
                return
            }
            const localPath = root.normalizeLocalPath(selectedFile.toString())
            if (localPath.length === 0) {
                return
            }
            if (typeof playback.openFilePath === "function") {
                playback.openFilePath(localPath)
            } else {
                playback.openFile(selectedFile)
            }
        }
    }

    Menu {
        id: addToPlaylistMenu

        MenuItem {
            text: root.playlistChoices.length === 0 ? qsTr("暂无歌单（请到播放列表页新建）") : qsTr("添加到歌单：")
            enabled: false
        }

        Instantiator {
            model: root.playlistChoices
            delegate: MenuItem {
                required property var modelData
                text: modelData.name
                onTriggered: {
                    if (root.playlistService && root.pendingTrack) {
                        root.playlistService.addTrack(modelData.id, root.pendingTrack.path, root.pendingTrack.title || "")
                        root.playlistService.saveToStorage()
                    }
                    root.pendingTrack = null
                }
            }
            onObjectAdded: (index, object) => addToPlaylistMenu.insertItem(index + 1, object)
            onObjectRemoved: (index, object) => addToPlaylistMenu.removeItem(object)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.space3

        Label {
            text: qsTr("曲库")
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH1
            color: theme.colorTextPrimary
        }

        Label {
            text: qsTr("控制器: %1").arg(root.appController.appName)
            color: theme.colorTextMuted
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2

            Components.PillTabButton {
                text: qsTr("本地曲库")
                tabSelected: root.libraryTabIndex === 0
                onClicked: {
                    if (root.libraryTabIndex === 0) {
                        return
                    }
                    root.libraryTabSlideDirection = 0 > root.libraryTabIndex ? 1 : -1
                    root.libraryTabIndex = 0
                }
            }

            Components.PillTabButton {
                text: qsTr("在线搜歌")
                tabSelected: root.libraryTabIndex === 1
                onClicked: {
                    if (root.libraryTabIndex === 1) {
                        return
                    }
                    root.libraryTabSlideDirection = 1 > root.libraryTabIndex ? 1 : -1
                    root.libraryTabIndex = 1
                }
            }

            Item { Layout.fillWidth: true }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2
            visible: root.libraryTabIndex === 0

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 40
                radius: theme.radiusSm
                color: directoryField.enabled ? theme.colorBgPanel : theme.colorBgHover
                border.width: 1
                border.color: !directoryField.enabled
                              ? theme.colorTextMuted
                              : (root.scanPathError
                                 ? theme.colorStateError
                                 : (directoryField.activeFocus ? theme.colorBorderFocus : theme.colorBorderDefault))

                Behavior on border.color {
                    ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
                }

                TextField {
                    id: directoryField
                    anchors.fill: parent
                    anchors.leftMargin: theme.space3
                    anchors.rightMargin: theme.space3
                    placeholderText: qsTr("输入要扫描的目录，例如 D:/Music")
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontBody
                    color: enabled ? theme.colorTextPrimary : theme.colorTextMuted
                    background: Rectangle { color: "transparent"; border.width: 0 }
                    onTextChanged: {
                        if (root.scanPathError && text.trim().length > 0) {
                            root.scanPathError = false
                        }
                    }
                }
            }

            Button {
                text: qsTr("选择目录")
                enabled: !!libraryRepository
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: scanFolderDialog.open()
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
                text: qsTr("扫描")
                enabled: !!libraryRepository
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: {
                    const targetPath = directoryField.text.trim()
                    if (targetPath.length === 0 && !!libraryRepository) {
                        scanFolderDialog.open()
                        return
                    }
                    if (targetPath.length === 0 || !libraryRepository) {
                        root.scanPathError = true
                        return
                    }
                    root.scanPathError = false
                    libraryRepository.scanAndStore(targetPath)
                    root.refreshVisibleTracks()
                }
                background: Rectangle {
                    radius: theme.radiusXs
                    color: !parent.enabled
                           ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.35)
                           : (parent.down
                              ? theme.colorPrimaryActive
                              : (parent.hovered ? theme.colorPrimaryActive : theme.colorPrimary))
                    border.width: 0
                    Behavior on color {
                        ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
                    }
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
                text: qsTr("打开单曲")
                enabled: !!playback
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: openSingleTrackDialog.open()
                background: Rectangle {
                    radius: theme.radiusXs
                    color: !parent.enabled
                           ? theme.colorBgHover
                           : (parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent"))
                    border.width: 1
                    border.color: !parent.enabled ? theme.colorTextMuted : theme.colorBorderDefault
                }
            }
        }

        Components.SearchBar {
            id: searchBar
            Layout.fillWidth: true
            visible: root.libraryTabIndex === 0
            placeholderText: qsTr("搜索歌名 / 歌手 / 专辑")
            onTextEdited: searchDebounceTimer.restart()
            onSearchCommitted: root.refreshVisibleTracks()
            onCleared: root.refreshVisibleTracks()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2
            visible: root.libraryTabIndex === 0

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("播放全部")
                enabled: !!playback && root.visibleTracks.length > 0
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: root.playAllVisibleTracks()
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

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2
            visible: root.libraryTabIndex === 1

            Components.SearchBar {
                id: onlineSearchBar
                Layout.fillWidth: true
                placeholderText: qsTr("搜索在线歌曲（网易云）")
                onSearchCommitted: root.triggerOnlineSearch()
            }

            Button {
                text: searchService && searchService.onlineSearchState === "searching" ? qsTr("搜索中...") : qsTr("搜索")
                enabled: !!searchService && (!searchService || searchService.onlineSearchState !== "searching")
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: root.triggerOnlineSearch()
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

            Button {
                text: qsTr("播放全部")
                enabled: !!playback && root.onlineTracks.length > 0
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: root.playAllOnlineTracks()
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

        Label {
            visible: root.libraryTabIndex === 0
            text: qsTr("共 %1 首，当前显示 %2 首（双击播放，单击选中）")
                    .arg(libraryRepository ? libraryRepository.trackCount() : 0)
                    .arg(root.visibleTracks.length)
            color: theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
        }

        Label {
            visible: root.libraryTabIndex === 1
            text: {
                if (searchService && searchService.onlineSearchState === "searching") {
                    return qsTr("正在搜索在线歌曲...")
                }
                if (searchService && searchService.onlineLastError !== "") {
                    return searchService.onlineLastError
                }
                return qsTr("找到 %1 首在线歌曲（双击插入下一首播放，需联网）").arg(root.onlineTracks.length)
            }
            color: (searchService && searchService.onlineLastError !== "") ? theme.colorStateError : theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Components.GlassPanel {
            id: trackListPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            cornerRadius: theme.radiusMd
            padding: theme.space2
            opacity: trackListPanelOpacity

            property real trackListPanelOpacity: 1

            SequentialAnimation {
                id: tabContentFade
                running: false

                NumberAnimation {
                    target: trackListPanel
                    property: "trackListPanelOpacity"
                    to: 0.72
                    duration: theme.motionFast
                    easing.type: Easing.OutCubic
                }

                NumberAnimation {
                    target: trackListPanel
                    property: "trackListPanelOpacity"
                    to: 1
                    duration: theme.motionNormal
                    easing.type: Easing.OutCubic
                }
            }

            Connections {
                target: root
                function onLibraryTabIndexChanged() {
                    tabContentFade.restart()
                }
            }

            ListView {
                id: trackList
                anchors.fill: parent
                anchors.margins: theme.space2
                clip: true
                spacing: theme.space2
                model: root.libraryTabIndex === 0 ? root.visibleTracks : root.onlineTracks

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Rectangle {
                    id: trackDelegate
                    required property var modelData
                    required property int index
                    readonly property bool hovered: trackHover.hovered
                    readonly property bool isOnlineItem: !!(modelData && modelData.isOnline)
                    readonly property bool disabledItem: !isOnlineItem && !!modelData && modelData.path === ""
                    readonly property bool isPlaying: !!modelData && modelData.path !== "" && modelData.path === root.currentPlayingPath
                    readonly property bool isSelected: !!modelData && modelData.path !== "" && modelData.path === root.selectedPath
                    width: trackList.width
                    implicitHeight: 74
                    radius: theme.radiusSm
                    color: trackDelegate.isPlaying
                           ? theme.colorPrimaryTint
                           : (trackDelegate.isSelected
                              ? theme.colorBgPressed
                              : (trackDelegate.hovered ? theme.colorBgHover : "transparent"))
                    border.width: (trackDelegate.isPlaying || trackDelegate.hovered) ? 1 : 0
                    border.color: trackDelegate.isPlaying ? theme.colorPrimaryActive : theme.colorBorderFocus
                    opacity: trackDelegate.disabledItem ? 0.45 : 1

                    HoverHandler {
                        id: trackHover
                        enabled: !trackDelegate.disabledItem
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !trackDelegate.disabledItem
                        acceptedButtons: Qt.LeftButton
                        onClicked: root.selectedPath = trackDelegate.modelData.path || ""
                        onDoubleClicked: {
                            if (trackDelegate.isOnlineItem) {
                                root.playOnlineTrackAt(trackDelegate.index)
                            } else {
                                root.playTrackAt(trackDelegate.index)
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: theme.space2
                        spacing: theme.space2

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: theme.space1

                                Image {
                                    visible: trackDelegate.isPlaying
                                    source: "../../assets/icons/play.svg"
                                    sourceSize.width: 14
                                    sourceSize.height: 14
                                    Layout.preferredWidth: visible ? 14 : 0
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: (trackDelegate.modelData.title || qsTr("未知标题")) + " - " + (trackDelegate.modelData.artist || qsTr("未知歌手"))
                                    font.bold: true
                                    font.family: theme.fontFamily
                                    font.pixelSize: theme.fontBody
                                    color: trackDelegate.isPlaying ? theme.colorPrimaryActive : theme.colorTextPrimary
                                    elide: Text.ElideRight
                                }

                                Label {
                                    visible: trackDelegate.isOnlineItem
                                    text: qsTr("在线")
                                    color: theme.colorPrimary
                                    font.family: theme.fontFamily
                                    font.pixelSize: theme.fontCaption
                                    font.bold: true
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: !trackDelegate.isOnlineItem
                                text: qsTr("专辑：%1").arg(trackDelegate.modelData.album || qsTr("未知专辑"))
                                color: theme.colorTextSecondary
                                font.family: theme.fontFamily
                                font.pixelSize: theme.fontCaption
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: trackDelegate.isOnlineItem
                                      ? qsTr("来源：网易云音乐 · 双击插入下一首播放")
                                      : qsTr("时长：%1  路径：%2")
                                            .arg(root.formatDuration(trackDelegate.modelData.durationMs))
                                            .arg(trackDelegate.modelData.path || "")
                                color: theme.colorTextMuted
                                font.family: theme.fontFamily
                                font.pixelSize: theme.fontCaption
                                elide: Text.ElideMiddle
                            }
                        }

                        ToolButton {
                            opacity: trackDelegate.hovered ? 1 : 0
                            enabled: trackDelegate.hovered && !trackDelegate.disabledItem
                            icon.source: "../../assets/icons/play.svg"
                            icon.color: theme.colorPrimary
                            icon.width: 20
                            icon.height: 20
                            display: AbstractButton.IconOnly
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("播放")
                            onClicked: {
                                if (trackDelegate.isOnlineItem) {
                                    root.playOnlineTrackAt(trackDelegate.index)
                                } else {
                                    root.playTrackAt(trackDelegate.index)
                                }
                            }
                            background: Rectangle {
                                radius: width / 2
                                color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            }
                        }

                        ToolButton {
                            opacity: trackDelegate.hovered ? 1 : 0
                            visible: trackDelegate.isOnlineItem
                            enabled: trackDelegate.hovered && !trackDelegate.disabledItem
                                    && !!root.downloadService
                                    && (!root.downloadService || root.downloadService.downloadState !== "downloading")
                            icon.source: "../../assets/icons/download.svg"
                            icon.color: theme.colorTextSecondary
                            icon.width: 20
                            icon.height: 20
                            display: AbstractButton.IconOnly
                            ToolTip.visible: hovered
                            ToolTip.text: root.downloadService && root.downloadService.downloadState === "downloading"
                                          ? qsTr("下载中...")
                                          : qsTr("下载到本地并加入曲库")
                            onClicked: root.downloadOnlineTrack(trackDelegate.modelData)
                            background: Rectangle {
                                radius: width / 2
                                color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            }
                        }

                        ToolButton {
                            opacity: trackDelegate.hovered ? 1 : 0
                            enabled: trackDelegate.hovered && !trackDelegate.disabledItem && !trackDelegate.isOnlineItem && !!root.playlistService
                            icon.source: "../../assets/icons/playlist_add.svg"
                            icon.color: theme.colorTextSecondary
                            icon.width: 20
                            icon.height: 20
                            display: AbstractButton.IconOnly
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("添加到歌单")
                            onClicked: root.openAddToPlaylistMenu(trackDelegate.modelData, this)
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

    Component.onCompleted: root.refreshVisibleTracks()
}
