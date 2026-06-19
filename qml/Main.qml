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

        root.nowPlayingVisible = !root.nowPlayingVisible

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



                            delegate: Item {

                                id: navItem

                                required property int index

                                required property var modelData



                                Layout.fillWidth: true

                                implicitHeight: 44



                                property bool hovered: false

                                property bool pressed: false

                                readonly property bool selected: root.currentPageIndex === navItem.index



                                Rectangle {

                                    anchors.fill: parent

                                    radius: theme.radiusSm

                                    color: navItem.selected

                                           ? theme.colorPrimary

                                           : (navItem.pressed

                                              ? theme.colorBgPressed

                                              : (navItem.hovered ? theme.colorBgHover : "transparent"))

                                    border.width: 0



                                    Behavior on color {

                                        ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }

                                    }

                                }



                                RowLayout {

                                    anchors.fill: parent

                                    anchors.leftMargin: theme.space3

                                    anchors.rightMargin: theme.space3

                                    spacing: theme.space3



                                    Label {

                                        text: navItem.modelData.icon

                                        color: navItem.selected ? "#ffffff" : theme.colorTextSecondary

                                        font.pixelSize: theme.fontBody

                                        font.family: theme.fontFamily

                                    }



                                    Label {

                                        Layout.fillWidth: true

                                        text: navItem.modelData.name

                                        color: navItem.selected ? "#ffffff" : theme.colorTextPrimary

                                        font.pixelSize: theme.fontBody

                                        font.family: theme.fontFamily

                                        font.weight: navItem.selected ? 600 : 400

                                    }

                                }



                                MouseArea {

                                    anchors.fill: parent

                                    hoverEnabled: true

                                    onEntered: navItem.hovered = true

                                    onExited: {

                                        navItem.hovered = false

                                        navItem.pressed = false

                                    }

                                    onPressed: navItem.pressed = true

                                    onReleased: navItem.pressed = false

                                    onClicked: root.currentPageIndex = navItem.index

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



                    StackLayout {

                        anchors.fill: parent

                        currentIndex: root.currentPageIndex



                        Pages.LibraryPage {

                            id: librarySection

                            appController: root.controller

                        }



                        Pages.PlaylistPage {

                            appController: root.controller

                        }



                        Pages.FavoritesPage {

                            appController: root.controller

                        }



                        Pages.SettingsPage {

                            appController: root.controller

                        }

                    }

                }

            }



            Components.NowPlayingOverlay {

                id: nowPlayingOverlay

                anchors.fill: parent

                visible: root.nowPlayingVisible

                z: 20

                appController: root.controller

                onCloseRequested: root.nowPlayingVisible = false

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

