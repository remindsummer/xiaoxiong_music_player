import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Item {
    id: root

    property string iconText: ""
    property string labelText: ""
    property bool selected: false

    implicitHeight: 44

    property bool hovered: false
    property bool pressed: false

    signal clicked()

    ThemeTokens.Theme {
        id: theme
    }

    Rectangle {
        anchors.fill: parent
        radius: theme.radiusSm
        color: "transparent"
        clip: true

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.pressed ? theme.colorBgPressed : theme.colorPrimaryTint
            opacity: !root.selected && (root.hovered || root.pressed) ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: theme.motionFast
                    easing.type: Easing.OutCubic
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.pressed && root.selected ? theme.colorPrimaryActive : theme.colorPrimary
            opacity: root.selected ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: theme.motionFast
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: theme.space3
        anchors.rightMargin: theme.space3
        spacing: theme.space3

        Label {
            text: root.iconText
            color: root.selected ? "#ffffff" : theme.colorTextSecondary
            font.pixelSize: theme.fontBody
            font.family: theme.fontFamily

            Behavior on color {
                ColorAnimation {
                    duration: theme.motionFast
                    easing.type: Easing.OutCubic
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.labelText
            color: root.selected ? "#ffffff" : theme.colorTextPrimary
            font.pixelSize: theme.fontBody
            font.family: theme.fontFamily
            font.weight: root.selected ? 600 : 400

            Behavior on color {
                ColorAnimation {
                    duration: theme.motionFast
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited: {
            root.hovered = false
            root.pressed = false
        }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }
}
