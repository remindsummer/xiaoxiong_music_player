import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

Button {
    id: root

    hoverEnabled: true

    ThemeTokens.Theme {
        id: theme
    }

    background: Rectangle {
        radius: theme.radiusXs
        color: !root.enabled
               ? theme.colorBgHover
               : (root.down ? theme.colorBgPressed : (root.hovered ? theme.colorBgHover : "transparent"))
        border.width: 1
        border.color: !root.enabled ? theme.colorTextMuted : theme.colorBorderDefault
        Behavior on color {
            ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
        }
    }

    contentItem: Label {
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: root.enabled ? theme.colorTextPrimary : theme.colorTextMuted
        font.family: theme.fontFamily
        font.pixelSize: theme.fontCaption
    }
}
