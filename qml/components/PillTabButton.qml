import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

Button {
    id: root

    property bool tabSelected: false

    hoverEnabled: true
    font.family: theme.fontFamily
    font.pixelSize: theme.fontBody

    ThemeTokens.Theme {
        id: theme
    }

    background: Item {
        implicitWidth: root.implicitWidth
        implicitHeight: root.implicitHeight

        Rectangle {
            anchors.fill: parent
            radius: theme.radiusXs
            color: "transparent"
            border.width: root.tabSelected ? 0 : 1
            border.color: theme.colorBorderDefault
            clip: true

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: root.down ? theme.colorBgPressed : theme.colorPrimaryTint
                opacity: !root.tabSelected && (root.hovered || root.down) ? 1 : 0

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
                color: root.down ? theme.colorPrimaryActive : theme.colorPrimary
                opacity: root.tabSelected ? 1 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: theme.motionFast
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }

    contentItem: Label {
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: root.tabSelected ? "#ffffff" : theme.colorTextPrimary
        font.family: theme.fontFamily
        font.pixelSize: theme.fontBody
        font.weight: root.tabSelected ? 600 : 400

        Behavior on color {
            ColorAnimation {
                duration: theme.motionFast
                easing.type: Easing.OutCubic
            }
        }
    }
}
