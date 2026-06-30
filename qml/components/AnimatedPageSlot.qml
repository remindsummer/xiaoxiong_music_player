import QtQuick
import "../theme" as ThemeTokens

Item {
    id: root

    property bool active: false
    property int slideDirection: 1

    opacity: active ? 1 : 0
    visible: active || opacity > 0.01
    enabled: active

    property real offsetX: active ? 0 : (root.slideDirection * 18)

    default property alias content: inner.data

    ThemeTokens.Theme {
        id: theme
    }

    transform: Translate {
        x: root.offsetX
    }

    Item {
        id: inner
        anchors.fill: parent

        onChildrenChanged: {
            for (let i = 0; i < inner.children.length; ++i) {
                const child = inner.children[i]
                child.anchors.fill = inner
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: theme.motionSlow
            easing.type: Easing.OutCubic
        }
    }

    Behavior on offsetX {
        NumberAnimation {
            duration: theme.motionSlow
            easing.type: Easing.OutCubic
        }
    }
}
