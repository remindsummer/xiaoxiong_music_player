import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

Slider {
    id: root

    ThemeTokens.Theme {
        id: theme
    }

    readonly property int handleSize: theme.sliderHandleSize
    readonly property int grooveHeight: theme.sliderGrooveHeight

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + (root.availableHeight - height) / 2
        implicitWidth: root.handleSize
        implicitHeight: root.handleSize
        radius: width / 2
        color: root.pressed ? theme.colorPrimaryActive
                             : (root.hovered ? theme.colorPrimary : "#ffffff")
        border.color: root.enabled ? theme.colorPrimary : theme.colorBorderDefault
        border.width: 1
    }

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + (root.availableHeight - height) / 2
        implicitWidth: 200
        implicitHeight: root.grooveHeight
        width: root.availableWidth
        height: implicitHeight
        radius: height / 2
        color: theme.colorBgPressed

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: root.enabled ? theme.colorPrimary : theme.colorTextMuted
            opacity: root.enabled ? 0.85 : 0.4
        }
    }
}
