pragma ComponentBehavior: Bound

import QtQuick

import QtQuick.Controls

import QtQuick.Layouts

import "pages" as Pages

import "components" as Components

import "theme" as ThemeTokens



ApplicationWindow {

    id: root

    visible: true

    width: 1240

    height: 760

    minimumWidth: 1024

    minimumHeight: 680

    title: qsTr("小熊音乐播放器")

    readonly property int uiLanguageRevision: appController ? appController.uiLanguageRevision : 0
    readonly property var navItems: {
        const _rev = root.uiLanguageRevision
        return [
            { name: qsTr("曲库"), icon: "♪" },
            { name: qsTr("播放列表"), icon: "≡" },
            { name: qsTr("我喜欢"), icon: "♥" },
            { name: qsTr("设置"), icon: "⚙" }
        ]
    }

    readonly property var controller: appController

    readonly property var playback: appController && (appController.playbackService || appController.playback)

                                     ? (appController.playbackService || appController.playback)

                                     : null

    property int currentPageIndex: 0

    property int pageSlideDirection: 1

    property bool nowPlayingVisible: false

    readonly property bool trayAvailable: appController
                                          && appController.trayService
                                          && appController.trayService.available

    function showMainFromTray() {
        if (appController && appController.trayService) {
            appController.trayService.showMainWindow()
        } else {
            root.show()
            root.raise()
            root.requestActivate()
        }
    }

    function hideMainToTray() {
        if (appController && appController.trayService) {
            appController.trayService.hideMainWindow()
        } else {
            root.hide()
        }
    }

    function handleCloseChoice(toTray, remember) {
        if (remember && appController && appController.settings) {
            appController.settings.closeBehavior = toTray ? "tray" : "quit"
            appController.settings.save()
        }
        if (toTray) {
            root.hideMainToTray()
        } else {
            Qt.quit()
        }
    }

    Component.onCompleted: {
        if (appController && appController.trayService) {
            appController.trayService.setWindow(root)
        }
        if (appController && appController.mediaSessionIntegration) {
            appController.mediaSessionIntegration.setWindow(root)
        }
    }

    onClosing: function(close) {
        if (!root.trayAvailable) {
            close.accepted = true
            return
        }

        close.accepted = false
        const settings = appController ? appController.settings : null
        const behavior = settings ? settings.closeBehavior : "ask"
        if (behavior === "tray") {
            root.hideMainToTray()
        } else if (behavior === "quit") {
            close.accepted = true
        } else {
            closeChoiceDialog.open()
        }
    }



    ThemeTokens.Theme {

        id: theme

    }



    function invokePlayback(actionName) {

        if (!root.playback || typeof root.playback[actionName] !== "function") {

            return

        }

        root.playback[actionName]()

    }



    function focusLibrarySearchField() {

        if (!librarySection || typeof librarySection.focusSearchField !== "function") {

            return

        }

        librarySection.focusSearchField()

    }



    function toggleNowPlaying() {

        if (nowPlayingLayer.overlayAnimating) {

            return

        }

        if (root.nowPlayingVisible) {

            nowPlayingLayer.closeOverlay()

        } else {

            root.nowPlayingVisible = true

            nowPlayingLayer.openOverlay()

        }

    }



    Shortcut {

        sequence: "Ctrl+P"

        context: Qt.ApplicationShortcut

        onActivated: root.invokePlayback("togglePlayPause")

    }



    Shortcut {

        sequence: "Ctrl+Left"

        context: Qt.ApplicationShortcut

        onActivated: root.invokePlayback("previous")

    }



    Shortcut {

        sequence: "Ctrl+Right"

        context: Qt.ApplicationShortcut

        onActivated: root.invokePlayback("next")

    }



    Shortcut {

        sequence: "Ctrl+F"

        context: Qt.ApplicationShortcut

        onActivated: root.focusLibrarySearchField()

    }



    background: Rectangle {

        color: theme.colorBgApp

    }



    ColumnLayout {

        anchors.fill: parent

        anchors.margins: theme.space6

        spacing: theme.space4



        Item {

            id: mainWorkspace

            Layout.fillWidth: true

            Layout.fillHeight: true



            RowLayout {

                anchors.fill: parent

                spacing: theme.space4



                Components.GlassPanel {

                    Layout.preferredWidth: 240

                    Layout.fillHeight: true

                    cornerRadius: theme.radiusLg



                    ColumnLayout {

                        anchors.fill: parent

                        anchors.margins: theme.space4

                        spacing: theme.space4



                        Label {

                            text: qsTr("小熊音乐")

                            font.family: theme.fontFamily

                            font.pixelSize: theme.fontH2

                            font.weight: 600

                            color: theme.colorTextPrimary

                        }



                        Label {

                            text: appController ? appController.appName : qsTr("控制器未注入")

                            font.family: theme.fontFamily

                            font.pixelSize: theme.fontCaption

                            color: theme.colorTextMuted

                        }



                        Repeater {

                            model: root.navItems

                            delegate: Components.SidebarNavItem {

                                required property int index

                                required property var modelData

                                Layout.fillWidth: true

                                iconText: modelData.icon

                                labelText: modelData.name

                                selected: root.currentPageIndex === index

                                onClicked: {
                                    if (index === root.currentPageIndex) {
                                        return
                                    }
                                    root.pageSlideDirection = index > root.currentPageIndex ? 1 : -1
                                    root.currentPageIndex = index
                                }

                            }

                        }



                        Item { Layout.fillHeight: true }

                    }

                }



                Components.GlassPanel {

                    Layout.fillWidth: true

                    Layout.fillHeight: true

                    cornerRadius: theme.radiusLg

                    padding: theme.space4



                    Item {

                        anchors.fill: parent

                        clip: true

                        Components.AnimatedPageSlot {

                            anchors.fill: parent

                            active: root.currentPageIndex === 0

                            slideDirection: root.pageSlideDirection

                            Pages.LibraryPage {

                                id: librarySection

                                appController: root.controller

                            }

                        }

                        Components.AnimatedPageSlot {

                            anchors.fill: parent

                            active: root.currentPageIndex === 1

                            slideDirection: root.pageSlideDirection

                            Pages.PlaylistPage {

                                appController: root.controller

                            }

                        }

                        Components.AnimatedPageSlot {

                            anchors.fill: parent

                            active: root.currentPageIndex === 2

                            slideDirection: root.pageSlideDirection

                            Pages.FavoritesPage {

                                appController: root.controller

                            }

                        }

                        Components.AnimatedPageSlot {

                            anchors.fill: parent

                            active: root.currentPageIndex === 3

                            slideDirection: root.pageSlideDirection

                            Pages.SettingsPage {

                                appController: root.controller

                            }

                        }

                    }

                }

            }



            Item {

                id: nowPlayingLayer

                anchors.fill: parent

                z: 20

                clip: true

                property bool overlayAnimating: false

                property real panelSlideY: 0

                visible: root.nowPlayingVisible || overlayAnimating || panelSlideY < height - 0.5

                Rectangle {

                    id: lyricsDimOverlay

                    anchors.fill: parent

                    color: theme.colorBgApp

                    opacity: 0

                    z: 0

                    MouseArea {

                        anchors.fill: parent

                        onClicked: root.toggleNowPlaying()

                    }

                }

                Components.NowPlayingOverlay {

                    id: nowPlayingOverlay

                    z: 1

                    width: parent.width

                    height: parent.height

                    x: 0

                    y: panelSlideY

                    appController: root.controller

                    presentationActive: root.nowPlayingVisible && panelSlideY < 1

                    onCloseRequested: root.toggleNowPlaying()

                }

                NumberAnimation {

                    id: lyricsPanelEnterAnim

                    target: nowPlayingLayer

                    property: "panelSlideY"

                    duration: theme.motionSlow

                    easing.type: Easing.OutCubic

                    onStarted: nowPlayingLayer.overlayAnimating = true

                    onStopped: nowPlayingLayer.overlayAnimating = false

                }

                NumberAnimation {

                    id: lyricsPanelExitAnim

                    target: nowPlayingLayer

                    property: "panelSlideY"

                    duration: theme.motionSlow

                    easing.type: Easing.InCubic

                    onStarted: nowPlayingLayer.overlayAnimating = true

                    onFinished: {
                        nowPlayingLayer.overlayAnimating = false
                        root.nowPlayingVisible = false
                    }

                }

                NumberAnimation {

                    id: lyricsDimEnterAnim

                    target: lyricsDimOverlay

                    property: "opacity"

                    duration: theme.motionNormal

                    easing.type: Easing.OutCubic

                }

                NumberAnimation {

                    id: lyricsDimExitAnim

                    target: lyricsDimOverlay

                    property: "opacity"

                    duration: theme.motionNormal

                    easing.type: Easing.InCubic

                }

                function openOverlay() {
                    lyricsPanelEnterAnim.stop()
                    lyricsPanelExitAnim.stop()
                    lyricsDimEnterAnim.stop()
                    lyricsDimExitAnim.stop()

                    panelSlideY = height
                    lyricsDimOverlay.opacity = 0

                    lyricsPanelEnterAnim.from = height
                    lyricsPanelEnterAnim.to = 0
                    lyricsPanelEnterAnim.start()

                    lyricsDimEnterAnim.from = 0
                    lyricsDimEnterAnim.to = 0.55
                    lyricsDimEnterAnim.start()
                }

                function closeOverlay() {
                    lyricsPanelEnterAnim.stop()
                    lyricsPanelExitAnim.stop()
                    lyricsDimEnterAnim.stop()
                    lyricsDimExitAnim.stop()

                    lyricsPanelExitAnim.from = panelSlideY
                    lyricsPanelExitAnim.to = height
                    lyricsPanelExitAnim.start()

                    lyricsDimExitAnim.from = lyricsDimOverlay.opacity
                    lyricsDimExitAnim.to = 0
                    lyricsDimExitAnim.start()
                }

                Component.onCompleted: {
                    panelSlideY = height
                }

            }

        }



        Components.PlaybackBar {

            Layout.fillWidth: true

            Layout.preferredHeight: 118

            appController: root.controller

            nowPlayingOverlayVisible: root.nowPlayingVisible

            onLyricsRequested: root.toggleNowPlaying()

            onQueueRequested: queueDrawer.open()

        }

    }



    Components.CloseChoiceDialog {
        id: closeChoiceDialog
        anchors.centerIn: parent

        onTrayChosen: function(remember) {
            root.handleCloseChoice(true, remember)
        }

        onQuitChosen: function(remember) {
            root.handleCloseChoice(false, remember)
        }
    }

    Drawer {

        id: queueDrawer

        edge: Qt.RightEdge

        width: Math.min(420, root.width * 0.42)

        height: root.height

        dim: true



        background: Rectangle {

            color: theme.colorBgElevated

            border.color: theme.colorBorderDefault

        }



        Components.QueuePanel {

            anchors.fill: parent

            appController: root.controller

        }

    }

}

