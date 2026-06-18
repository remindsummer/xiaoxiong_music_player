import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Rectangle {
    id: root

    property var appController: null
    property var lyricsModel: null
    property int currentPositionMs: 0
    property string state: "idle"
    property string errorText: ""
    property var lyricLines: []
    property int activeLineIndex: -1

    signal lineActivated(int timestampMs)
    signal retryRequested()
    signal selectLocalLyricRequested()

    readonly property var lyrics: root.lyricsModel
                                  ? root.lyricsModel
                                  : (root.appController ? (root.appController.lyricsService || root.appController.lyrics) : null)
    readonly property string effectiveState: {
        if (!root.lyrics) {
            return "idle"
        }
        if (root.lyricLines.length > 0) {
            return "ready"
        }
        if ((root.errorText || "").length > 0) {
            return "error"
        }
        if (root.lyrics.onlineFetchState === "searching") {
            return "loading"
        }
        return "empty"
    }

    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: theme.radiusMd
    color: theme.colorBgPanel
    border.width: 1
    border.color: root.effectiveState === "error" ? theme.colorStateError : theme.colorBorderDefault

    ThemeTokens.Theme {
        id: theme
    }

    function refreshLyricLines() {
        if (!root.lyrics) {
            root.lyricLines = []
            root.activeLineIndex = -1
            return
        }

        root.lyricLines = root.lyrics.lyricLinesForQml()
        root.activeLineIndex = root.lyrics.currentLineIndex
        root.errorText = root.lyrics.onlineLastError || ""

        if (root.activeLineIndex >= 0 && root.activeLineIndex < lyricListView.count) {
            lyricListView.positionViewAtIndex(root.activeLineIndex, ListView.Center)
        }
    }

    Connections {
        target: root.lyrics
        ignoreUnknownSignals: true
        function onLyricsChanged() { root.refreshLyricLines() }
        function onOnlineLastErrorChanged() { root.errorText = root.lyrics ? (root.lyrics.onlineLastError || "") : "" }
        function onCurrentLineIndexChanged() {
            root.activeLineIndex = root.lyrics ? root.lyrics.currentLineIndex : -1
            if (root.activeLineIndex >= 0 && root.activeLineIndex < lyricListView.count) {
                lyricListView.positionViewAtIndex(root.activeLineIndex, ListView.Center)
            }
        }
    }

    Component.onCompleted: refreshLyricLines()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.space4
        spacing: theme.space3

        Label {
            text: "歌词"
            font.bold: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH2
            color: theme.colorTextPrimary
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: lyricListView
                anchors.fill: parent
                model: root.lyricLines
                clip: true
                spacing: theme.space1
                boundsBehavior: Flickable.StopAtBounds
                visible: root.effectiveState === "ready"
                opacity: visible ? 1 : 0

                Behavior on opacity {
                    NumberAnimation { duration: theme.motionNormal; easing.type: Easing.OutCubic }
                }

                delegate: Item {
                    required property int index
                    required property var modelData

                    width: lyricListView.width
                    height: lyricText.implicitHeight + theme.space2
                    property bool hovered: false

                    Rectangle {
                        anchors.fill: parent
                        radius: theme.radiusSm
                        color: index === root.activeLineIndex
                               ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.12)
                               : (hovered ? theme.colorBgHover : "transparent")
                        border.width: index === root.activeLineIndex ? 1 : 0
                        border.color: theme.colorBorderFocus
                        Behavior on color {
                            ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
                        }
                    }

                    Label {
                        id: lyricText
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - theme.space5
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        color: index === root.activeLineIndex ? theme.colorTextPrimary : theme.colorTextSecondary
                        font.bold: index === root.activeLineIndex
                        font.family: theme.fontFamily
                        font.pixelSize: index === root.activeLineIndex ? 17 : 15
                        opacity: index === root.activeLineIndex ? 1.0 : 0.8
                        text: {
                            const lineText = modelData && modelData.text ? modelData.text : ""
                            return lineText !== "" ? lineText : "..."
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: parent.hovered = true
                        onExited: parent.hovered = false
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

            Column {
                anchors.centerIn: parent
                width: parent.width - theme.space6
                spacing: theme.space2
                visible: root.effectiveState !== "ready"

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: root.effectiveState === "error" ? theme.colorStateError : theme.colorTextSecondary
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontBody
                    text: root.effectiveState === "loading"
                          ? "正在加载歌词，请稍候..."
                          : "暂无歌词，支持自动加载同名 .lrc、Meting 在线歌词或手动选择歌词文件。"
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

                    Button {
                        text: "重试"
                        hoverEnabled: true
                        onClicked: root.retryRequested()
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down
                                   ? theme.colorStateError
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
                            font.weight: 600
                        }
                    }

                    Button {
                        text: "选择歌词文件"
                        hoverEnabled: true
                        onClicked: root.selectLocalLyricRequested()
                        background: Rectangle {
                            radius: theme.radiusXs
                            color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            border.width: 1
                            border.color: theme.colorBorderDefault
                        }
                    }
                }
            }
        }
    }
}
