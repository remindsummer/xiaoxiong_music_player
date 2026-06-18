import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

Rectangle {
    id: root

    property string text: ""
    property string placeholderText: "搜索歌名 / 歌手 / 专辑"
    property bool loading: false
    property bool hasError: false
    property bool hovered: false
    signal keywordChanged(string keyword)
    signal textEdited(string value)
    signal searchCommitted(string value)
    signal cleared()

    function forceActiveFocusOnInput() {
        searchField.forceActiveFocus()
        if (typeof searchField.selectAll === "function") {
            searchField.selectAll()
        }
    }

    Layout.fillWidth: true
    implicitHeight: 44
    radius: theme.radiusSm
    color: enabled ? theme.colorBgPanel : theme.colorBgHover
    border.width: 1
    border.color: !enabled
                  ? theme.colorTextMuted
                  : (hasError
                     ? theme.colorStateError
                     : (searchField.activeFocus
                        ? theme.colorBorderFocus
                        : (hovered ? theme.colorTextSecondary : theme.colorBorderDefault)))

    ThemeTokens.Theme {
        id: theme
    }

    Behavior on border.color {
        ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: theme.space3
        anchors.rightMargin: theme.space2
        spacing: theme.space2

        Label {
            text: loading ? "检索中" : "搜索"
            color: root.hasError ? theme.colorStateError : theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            text: root.text
            placeholderText: root.placeholderText
            enabled: root.enabled
            color: enabled ? theme.colorTextPrimary : theme.colorTextMuted
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
            background: Rectangle {
                color: "transparent"
                border.width: 0
            }
            onTextChanged: {
                root.text = text
                root.keywordChanged(text)
            }
            onTextEdited: root.textEdited(text)
            onAccepted: root.searchCommitted(text)
        }

        Button {
            text: "清空"
            enabled: searchField.text.length > 0
            hoverEnabled: true
            font.family: theme.fontFamily
            font.pixelSize: theme.fontCaption
            onClicked: {
                searchField.clear()
                root.cleared()
                root.searchCommitted("")
            }
            background: Rectangle {
                radius: theme.radiusXs
                color: !parent.enabled
                       ? theme.colorBgHover
                       : (parent.down
                          ? theme.colorBgPressed
                          : (parent.hovered ? theme.colorBgHover : "transparent"))
                border.width: 1
                border.color: !parent.enabled ? theme.colorTextMuted : theme.colorBorderDefault
                Behavior on color {
                    ColorAnimation { duration: theme.motionFast; easing.type: Easing.OutCubic }
                }
            }
            contentItem: Label {
                text: parent.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: !parent.enabled ? theme.colorTextMuted : theme.colorTextSecondary
                font.family: theme.fontFamily
                font.pixelSize: theme.fontCaption
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onEntered: root.hovered = true
        onExited: root.hovered = false
    }
}
