import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as ThemeTokens

Item {
    id: root

    property string text: ""
    property string placeholderText: qsTr("搜索歌名 / 歌手 / 专辑")
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

    ThemeTokens.Theme {
        id: theme
    }

    Components.GlassPanel {
        anchors.fill: parent
        cornerRadius: theme.radiusMd
        showShadow: false
        padding: 0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: theme.space3
            anchors.rightMargin: theme.space2
            spacing: theme.space2

            Label {
                text: "♪"
                color: root.hasError ? theme.colorStateError : theme.colorTextMuted
                font.pixelSize: theme.fontBody
            }

            TextField {
                id: searchField
                Layout.fillWidth: true
                text: root.text
                placeholderText: root.placeholderText
                enabled: root.enabled
                color: enabled ? theme.colorTextPrimary : theme.colorTextMuted
                placeholderTextColor: theme.colorTextMuted
                font.family: theme.fontFamily
                font.pixelSize: theme.fontBody
                background: Rectangle { color: "transparent"; border.width: 0 }
                onTextChanged: {
                    root.text = text
                    root.keywordChanged(text)
                }
                onTextEdited: root.textEdited(text)
                onAccepted: root.searchCommitted(text)
            }

            Components.GhostButton {
                text: qsTr("清空")
                enabled: searchField.text.length > 0
                onClicked: {
                    searchField.clear()
                    root.cleared()
                    root.searchCommitted("")
                }
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
