import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

CheckBox {
    id: root

    spacing: theme.space2

    ThemeTokens.Theme {
        id: theme
    }

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: root.leftPadding
        y: root.height / 2 - height / 2
        radius: theme.radiusXs
        color: root.checked ? theme.colorPrimary : theme.colorBgPanel
        border.color: root.checked ? theme.colorPrimary : theme.colorBorderDefault
        border.width: root.checked ? 0 : 1

        Behavior on color {
            ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
        }

        Text {
            anchors.centerIn: parent
            text: "\u2713"
            font.pixelSize: 11
            font.weight: Font.Bold
            color: "#ffffff"
            visible: root.checked
        }
    }

    contentItem: Label {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        font.family: theme.fontFamily
        font.pixelSize: theme.fontBody
        color: theme.colorTextSecondary
        verticalAlignment: Text.AlignVCenter
    }
}
