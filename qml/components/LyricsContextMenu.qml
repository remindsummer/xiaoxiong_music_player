import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import "../theme" as ThemeTokens

Menu {
    id: root

    property var lyrics: null

    signal retryRequested()
    signal selectLocalLyricRequested()

    ThemeTokens.Theme {
        id: theme
    }

    topPadding: theme.space2
    bottomPadding: theme.space2
    leftPadding: theme.space2
    rightPadding: theme.space2
    spacing: theme.space1

    background: Rectangle {
        implicitWidth: 232
        color: theme.colorBgPanel
        radius: theme.radiusMd
        border.color: theme.colorBorderDefault
        border.width: 1

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 6
            radius: 18
            samples: 21
            color: theme.shadowSoft
            transparentBorder: true
        }
    }

    component StyledMenuItem : MenuItem {
        id: menuItem
        implicitHeight: 40
        leftPadding: theme.space3
        rightPadding: theme.space3

        contentItem: Label {
            text: menuItem.text
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
            color: menuItem.enabled ? theme.colorTextPrimary : theme.colorTextMuted
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: theme.radiusSm
            color: menuItem.highlighted ? theme.colorBgHover : "transparent"
            Behavior on color {
                ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
            }
        }
    }

    StyledMenuItem {
        text: qsTr("删除当前歌词匹配")
        enabled: root.lyrics && root.lyrics.hasLyrics
        onTriggered: {
            if (root.lyrics) {
                root.lyrics.removeCurrentLyricsBinding()
            }
        }
    }

    StyledMenuItem {
        text: qsTr("重新在线匹配")
        onTriggered: root.retryRequested()
    }

    StyledMenuItem {
        text: qsTr("选择本地歌词文件")
        onTriggered: root.selectLocalLyricRequested()
    }
}
