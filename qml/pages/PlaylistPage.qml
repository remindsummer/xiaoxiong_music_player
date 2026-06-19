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
    readonly property var playback: root.appController
                                    ? (root.appController.playbackService || root.appController.playback)
                                    : null
    readonly property string currentPlayingPath: playback ? playback.currentPath : ""
    property var playlistItems: []
    property var trackItems: []
    property string selectedPlaylistId: ""
    property string pendingDeletePlaylistId: ""
    property string editingPlaylistId: ""

    ThemeTokens.Theme {
        id: theme
    }

    function currentPlaylistName() {
        for (let i = 0; i < playlistItems.length; ++i) {
            if (playlistItems[i].id === selectedPlaylistId) {
                return playlistItems[i].name
            }
        }
        return "未选择"
    }

    function refreshPlaylistItems() {
        if (!playlistService) {
            playlistItems = []
            trackItems = []
            selectedPlaylistId = ""
            return
        }
        const allPlaylists = playlistService.playlists()
        const userPlaylists = []
        for (let i = 0; i < allPlaylists.length; ++i) {
            if (!allPlaylists[i].isSystem) {
                userPlaylists.push(allPlaylists[i])
            }
        }
        playlistItems = userPlaylists
        let selectedExists = false
        for (let i = 0; i < playlistItems.length; ++i) {
            if (playlistItems[i].id === selectedPlaylistId) {
                selectedExists = true
                break
            }
        }
        if (!selectedExists) {
            selectedPlaylistId = playlistItems.length > 0 ? playlistItems[0].id : ""
        }
        refreshTrackItems()
    }

    function refreshTrackItems() {
        if (!playlistService || selectedPlaylistId.length === 0) {
            trackItems = []
            return
        }
        trackItems = playlistService.tracks(selectedPlaylistId)
    }

    function commitRename(playlistId, newName) {
        const trimmed = (newName || "").trim()
        root.editingPlaylistId = ""
        if (playlistService && trimmed.length > 0) {
            if (playlistService.renamePlaylist(playlistId, trimmed)) {
                playlistService.saveToStorage()
            }
        }
    }

    Dialog {
        id: deletePlaylistDialog
        title: "确认删除"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        visible: false
        width: 360
        onAccepted: {
            if (playlistService && pendingDeletePlaylistId !== "") {
                if (playlistService.deletePlaylist(pendingDeletePlaylistId)) {
                    playlistService.saveToStorage()
                    refreshPlaylistItems()
                }
            }
            pendingDeletePlaylistId = ""
        }
        onRejected: pendingDeletePlaylistId = ""

        contentItem: Label {
            text: "删除后不可恢复，确认继续吗？"
            color: theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
        }
    }

    Connections {
        target: root.playlistService
        ignoreUnknownSignals: true
        function onChanged() {
            root.refreshPlaylistItems()
        }
    }

    Component.onCompleted: {
        if (playlistService) {
            playlistService.loadFromStorage()
        }
        refreshPlaylistItems()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.space3

        Label {
            text: "播放列表"
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH1
            color: theme.colorTextPrimary
        }

        Label {
            text: "状态: " + root.appController.healthCheck()
            color: theme.colorTextMuted
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 40
                radius: theme.radiusSm
                color: theme.colorBgPanel
                border.width: 1
                border.color: createPlaylistField.activeFocus ? theme.colorBorderFocus : theme.colorBorderDefault

                TextField {
                    id: createPlaylistField
                    anchors.fill: parent
                    anchors.leftMargin: theme.space3
                    anchors.rightMargin: theme.space3
                    placeholderText: "输入歌单名"
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontBody
                    color: theme.colorTextPrimary
                    background: Rectangle { color: "transparent"; border.width: 0 }
                }
            }

            Button {
                text: "新建"
                enabled: !!playlistService
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                onClicked: {
                    const name = createPlaylistField.text.trim()
                    if (name.length === 0) {
                        return
                    }
                    if (playlistService.createPlaylist(name)) {
                        createPlaylistField.clear()
                        playlistService.saveToStorage()
                        refreshPlaylistItems()
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

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: theme.space3

            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                radius: theme.radiusMd
                color: theme.colorBgPanel
                border.color: theme.colorBorderDefault

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: theme.space3
                    spacing: theme.space2

                    Label {
                        text: "歌单列表（" + root.playlistItems.length + "）"
                        font.bold: true
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontBody
                        color: theme.colorTextPrimary
                    }

                    ListView {
                        id: playlistListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: theme.space2
                        model: root.playlistItems

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Rectangle {
                            id: plDelegate
                            required property var modelData
                            readonly property bool hovered: plNameHover.hovered
                            readonly property bool selected: modelData.id === root.selectedPlaylistId
                            readonly property bool editing: modelData.id === root.editingPlaylistId
                            width: playlistListView.width
                            height: 48
                            radius: theme.radiusSm
                            border.color: plDelegate.selected ? theme.colorPrimaryActive : theme.colorBorderDefault
                            color: plDelegate.selected
                                   ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.15)
                                   : (plDelegate.hovered ? theme.colorBgHover : "transparent")

                            function beginRename() {
                                root.editingPlaylistId = plDelegate.modelData.id
                                renameField.text = plDelegate.modelData.name
                                renameField.committed = false
                                renameField.forceActiveFocus()
                                renameField.selectAll()
                            }

                            HoverHandler {
                                id: plNameHover
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !plDelegate.editing
                                acceptedButtons: Qt.LeftButton
                                onClicked: {
                                    root.selectedPlaylistId = plDelegate.modelData.id
                                    root.refreshTrackItems()
                                }
                                onDoubleClicked: plDelegate.beginRename()
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: theme.space2

                                Label {
                                    visible: !plDelegate.editing
                                    Layout.fillWidth: true
                                    text: plDelegate.modelData.name + " (" + (plDelegate.modelData.tracks ? plDelegate.modelData.tracks.length : 0) + ")"
                                    color: plDelegate.selected ? theme.colorPrimaryActive : theme.colorTextPrimary
                                    font.family: theme.fontFamily
                                    font.pixelSize: theme.fontBody
                                    elide: Text.ElideRight
                                }

                                TextField {
                                    id: renameField
                                    property bool committed: false
                                    visible: plDelegate.editing
                                    Layout.fillWidth: true
                                    font.family: theme.fontFamily
                                    font.pixelSize: theme.fontBody
                                    color: theme.colorTextPrimary
                                    selectByMouse: true
                                    background: Rectangle {
                                        radius: theme.radiusXs
                                        color: theme.colorBgPanel
                                        border.width: 1
                                        border.color: theme.colorBorderFocus
                                    }
                                    onEditingFinished: {
                                        if (committed) {
                                            return
                                        }
                                        committed = true
                                        root.commitRename(plDelegate.modelData.id, text)
                                    }
                                    Keys.onEscapePressed: {
                                        committed = true
                                        root.editingPlaylistId = ""
                                    }
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "删除当前歌单"
                        enabled: !!playlistService && root.selectedPlaylistId.length > 0
                        hoverEnabled: true
                        onClicked: {
                            root.pendingDeletePlaylistId = root.selectedPlaylistId
                            deletePlaylistDialog.open()
                        }
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
                            font.pixelSize: theme.fontBody
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: theme.radiusMd
                color: theme.colorBgPanel
                border.color: theme.colorBorderDefault

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: theme.space3
                    spacing: theme.space2

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: theme.space2

                        Label {
                            Layout.fillWidth: true
                            text: "当前歌单: " + root.currentPlaylistName()
                            font.bold: true
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontBody
                            color: theme.colorTextPrimary
                            elide: Text.ElideRight
                        }

                        Button {
                            text: "播放全部"
                            enabled: !!playback && root.trackItems.length > 0
                            hoverEnabled: true
                            font.family: theme.fontFamily
                            font.pixelSize: theme.fontBody
                            onClicked: {
                                if (playback) {
                                    playback.playQueue(root.trackItems, 0)
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

                    Label {
                        text: "提示：在「曲库」中悬停歌曲，点击「添加到歌单」按钮加入；双击下方曲目插入下一首播放。"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: theme.colorTextMuted
                        font.family: theme.fontFamily
                        font.pixelSize: theme.fontCaption
                    }

                    ListView {
                        id: trackListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: theme.space2
                        model: root.trackItems

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Rectangle {
                            id: plTrackDelegate
                            required property var modelData
                            required property int index
                            readonly property bool hovered: plTrackHover.hovered
                            readonly property bool isPlaying: !!modelData && (modelData.path || "") !== "" && modelData.path === root.currentPlayingPath
                            width: trackListView.width
                            implicitHeight: 44
                            radius: theme.radiusSm
                            color: plTrackDelegate.isPlaying
                                   ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.15)
                                   : (plTrackDelegate.hovered ? theme.colorBgHover : "transparent")
                            border.width: (plTrackDelegate.isPlaying || plTrackDelegate.hovered) ? 1 : 0
                            border.color: plTrackDelegate.isPlaying ? theme.colorPrimaryActive : theme.colorBorderFocus

                            HoverHandler {
                                id: plTrackHover
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                onDoubleClicked: {
                                    if (root.playback && plTrackDelegate.modelData) {
                                        root.playback.enqueueAndPlay(plTrackDelegate.modelData)
                                    }
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: theme.space2
                                spacing: theme.space2

                                Image {
                                    visible: plTrackDelegate.isPlaying
                                    source: "../../assets/icons/play.svg"
                                    sourceSize.width: 14
                                    sourceSize.height: 14
                                    Layout.preferredWidth: visible ? 14 : 0
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: (plTrackDelegate.modelData.title || "未知标题") + "  |  " + (plTrackDelegate.modelData.path || "")
                                    color: plTrackDelegate.isPlaying ? theme.colorPrimaryActive : theme.colorTextPrimary
                                    font.family: theme.fontFamily
                                    font.pixelSize: theme.fontBody
                                    elide: Text.ElideMiddle
                                }

                                Button {
                                    text: "移除"
                                    hoverEnabled: true
                                    onClicked: {
                                        if (playlistService.removeTrack(root.selectedPlaylistId, plTrackDelegate.modelData.path)) {
                                            playlistService.saveToStorage()
                                            refreshTrackItems()
                                            refreshPlaylistItems()
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
    }
}
