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

    property var queueItems: []

    color: theme.colorBgElevated

    ThemeTokens.Theme {
        id: theme
    }

    function refreshQueue() {
        if (!root.playback) {
            root.queueItems = []
            return
        }
        root.queueItems = root.playback.queue()
    }

    Connections {
        target: root.playback
        ignoreUnknownSignals: true
        function onQueueChanged() { root.refreshQueue() }
        function onCurrentTrackChanged() { root.refreshQueue() }
    }

    Component.onCompleted: root.refreshQueue()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.space4
        spacing: theme.space3

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2

            Label {
                Layout.fillWidth: true
                text: "播放队列（" + root.queueItems.length + "）"
                font.bold: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontH2
                color: theme.colorTextPrimary
            }

            Button {
                text: "清空"
                enabled: !!root.playback && root.queueItems.length > 0
                hoverEnabled: true
                font.family: theme.fontFamily
                font.pixelSize: theme.fontCaption
                onClicked: root.playback.clearQueue()
                background: Rectangle {
                    radius: theme.radiusXs
                    color: !parent.enabled
                           ? theme.colorBgHover
                           : (parent.down || parent.hovered ? theme.colorBgPressed : theme.colorBgHover)
                    border.width: 1
                    border.color: theme.colorBorderDefault
                }
                contentItem: Label {
                    text: parent.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: theme.colorTextSecondary
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: theme.radiusMd
            color: theme.colorBgPanel
            border.color: theme.colorBorderDefault

            Label {
                anchors.centerIn: parent
                visible: root.queueItems.length === 0
                text: "队列为空\n在曲库或歌单中双击歌曲开始播放"
                horizontalAlignment: Text.AlignHCenter
                color: theme.colorTextMuted
                font.family: theme.fontFamily
                font.pixelSize: theme.fontCaption
            }

            ListView {
                id: queueListView
                anchors.fill: parent
                anchors.margins: theme.space2
                clip: true
                spacing: theme.space1
                model: root.queueItems

                delegate: Rectangle {
                    id: queueDelegate
                    required property var modelData
                    required property int index
                    readonly property bool hovered: queueHover.hovered
                    readonly property bool isCurrent: !!modelData && modelData.isCurrent === true
                    width: queueListView.width
                    implicitHeight: 52
                    radius: theme.radiusSm
                    color: queueDelegate.isCurrent
                           ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.15)
                           : (queueDelegate.hovered ? theme.colorBgHover : "transparent")
                    border.width: queueDelegate.isCurrent ? 1 : 0
                    border.color: theme.colorPrimaryActive

                    HoverHandler {
                        id: queueHover
                    }

                    MouseArea {
                        id: queueRowMouse
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: {
                            if (root.playback) {
                                root.playback.playTrackAt(queueDelegate.index)
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: theme.space2
                        anchors.rightMargin: theme.space1
                        spacing: theme.space2

                        Image {
                            visible: queueDelegate.isCurrent
                            source: "../../assets/icons/play.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            Layout.preferredWidth: 14
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: (queueDelegate.modelData.title || "未知标题")
                                color: queueDelegate.isCurrent ? theme.colorPrimaryActive : theme.colorTextPrimary
                                font.family: theme.fontFamily
                                font.pixelSize: theme.fontBody
                                font.bold: queueDelegate.isCurrent
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: text.length > 0
                                text: queueDelegate.modelData.artist || ""
                                color: theme.colorTextMuted
                                font.family: theme.fontFamily
                                font.pixelSize: theme.fontCaption
                                elide: Text.ElideRight
                            }
                        }

                        ToolButton {
                            opacity: queueDelegate.hovered ? 1 : 0
                            enabled: queueDelegate.hovered
                            icon.source: "../../assets/icons/queue.svg"
                            icon.color: theme.colorStateError
                            icon.width: 16
                            icon.height: 16
                            display: AbstractButton.IconOnly
                            ToolTip.visible: hovered
                            ToolTip.text: "从队列移除"
                            onClicked: {
                                if (root.playback) {
                                    root.playback.removeFromQueue(queueDelegate.index)
                                }
                            }
                            background: Rectangle {
                                radius: theme.radiusXs
                                color: parent.down ? theme.colorBgPressed : (parent.hovered ? theme.colorBgHover : "transparent")
                            }
                        }
                    }
                }
            }
        }
    }
}
