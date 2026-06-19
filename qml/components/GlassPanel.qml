import QtQuick
import Qt5Compat.GraphicalEffects
import "../theme" as ThemeTokens

Item {
    id: root

    property int cornerRadius: theme.radiusLg
    property bool showShadow: true
    property bool showBorder: true
    property int padding: 0

    default property alias children: inner.data

    ThemeTokens.Theme {
        id: theme
    }

    Rectangle {
        id: panel
        anchors.fill: parent
        radius: root.cornerRadius
        color: theme.surfaceGlass
        border.width: root.showBorder ? 1 : 0
        border.color: theme.surfaceGlassBorder
        layer.enabled: root.showShadow
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: theme.shadowOffsetY
            radius: theme.shadowRadius
            samples: 25
            color: theme.shadowSoft
            transparentBorder: true
        }
    }

    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
