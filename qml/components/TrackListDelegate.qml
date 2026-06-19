import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Rectangle {
    id: root

    property bool isPlaying: false
    property bool isHovered: false
    property string titleText: ""
    property string subtitleText: ""
    property string detailText: ""
    property bool showActions: false
    default property alias actions: actionsLayout.data

    signal clicked()
    signal doubleClicked()

    implicitHeight: 56
    radius: theme.radiusSm
    color: root.isPlaying
           ? theme.colorPrimaryTint
           : (root.isHovered ? theme.colorBgHover : theme.colorBgHoverClear)
    border.width: root.isPlaying ? 1 : 0
    border.color: theme.colorBorderFocus

    ThemeTokens.Theme {
        id: theme
    }

    Behavior on color {
        ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: theme.space3
        anchors.rightMargin: theme.space3
        spacing: theme.space2

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.titleText
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                font.weight: root.isPlaying ? 600 : 400
                color: root.isPlaying ? theme.colorPrimary : theme.colorTextPrimary
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: root.subtitleText !== ""
                text: root.subtitleText
                font.family: theme.fontFamily
                font.pixelSize: theme.fontCaption
                color: theme.colorTextMuted
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: root.detailText !== ""
                text: root.detailText
                font.family: theme.fontFamily
                font.pixelSize: theme.fontCaption
                color: theme.colorTextMuted
                elide: Text.ElideMiddle
            }
        }

        RowLayout {
            id: actionsLayout
            spacing: theme.space1
            visible: root.showActions && root.isHovered
        }
    }

    HoverHandler {
        id: hoverHandler
        onHoveredChanged: root.isHovered = hovered
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: root.clicked()
        onDoubleClicked: root.doubleClicked()
    }
}
