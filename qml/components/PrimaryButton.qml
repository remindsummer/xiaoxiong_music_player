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
               ? Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.35)
               : (root.down ? theme.colorPrimaryActive : (root.hovered ? theme.colorPrimaryActive : theme.colorPrimary))
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
