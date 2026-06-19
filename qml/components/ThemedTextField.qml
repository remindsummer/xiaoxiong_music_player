import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

TextField {
    id: root

    property bool showError: false

    ThemeTokens.Theme {
        id: theme
    }

    font.family: theme.fontFamily
    font.pixelSize: theme.fontBody
    color: theme.colorTextPrimary
    selectionColor: theme.colorPrimaryTintStrong
    selectedTextColor: theme.colorTextPrimary
    placeholderTextColor: theme.colorTextMuted
  padding: theme.space2

    background: Rectangle {
        radius: theme.radiusSm
        color: root.enabled ? theme.surfaceGlass : theme.colorBgHover
        border.width: 1
        border.color: root.showError
                      ? theme.colorStateError
                      : (root.activeFocus ? theme.colorBorderFocus : (root.hovered ? theme.colorBorderDefault : theme.surfaceGlassBorder))
        Behavior on border.color {
            ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
        }
    }
}
