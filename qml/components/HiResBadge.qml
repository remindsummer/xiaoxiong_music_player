import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

Rectangle {
    id: root

    implicitWidth: 72
    implicitHeight: 28
    radius: theme.radiusXs
    gradient: Gradient {
        GradientStop { position: 0; color: theme.colorHiResBadge }
        GradientStop { position: 1; color: theme.colorHiResBadgeEnd }
    }

    ThemeTokens.Theme {
        id: theme
    }

    Label {
        anchors.centerIn: parent
        text: "HI-RES"
        color: "#ffffff"
        font.family: theme.fontFamily
        font.pixelSize: 11
        font.weight: 700
        font.letterSpacing: 0.5
    }
}
