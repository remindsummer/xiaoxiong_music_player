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
               ? theme.colorDangerDisabled
               : (root.down ? theme.colorDangerPressed : (root.hovered ? theme.colorDangerHover : theme.colorDanger))
        border.width: 0
        Behavior on color {
            ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
        }
    }

    contentItem: Label {
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#ffffff"
        font.family: theme.fontFamily
        font.pixelSize: theme.fontCaption
        font.weight: 600
        opacity: root.enabled ? 1 : 0.85
    }
}
