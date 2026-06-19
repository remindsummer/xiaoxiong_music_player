import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

Menu {
    id: root

    property var lyrics: null

    signal retryRequested()
    signal selectLocalLyricRequested()

    ThemeTokens.Theme {
        id: theme
    }

    topPadding: theme.space1
    bottomPadding: theme.space1

    background: Rectangle {
        implicitWidth: 220
        color: theme.colorBgPanel
        radius: theme.radiusSm
        border.color: theme.colorBorderDefault
        border.width: 1
    }

    component StyledMenuItem : MenuItem {
        implicitHeight: 36
        leftPadding: theme.space3
        rightPadding: theme.space3

        contentItem: Label {
            text: parent.text
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
            color: parent.enabled ? theme.colorTextPrimary : theme.colorTextMuted
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: theme.radiusXs
            color: parent.highlighted ? theme.colorBgHover : "transparent"
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
