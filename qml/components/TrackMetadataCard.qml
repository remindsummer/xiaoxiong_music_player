import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as ThemeTokens

GlassPanel {
    id: root

    property var playback: null

    implicitWidth: 360
    implicitHeight: metadataGrid.implicitHeight + theme.space4 * 2
    padding: theme.space4
    cornerRadius: theme.radiusMd

    ThemeTokens.Theme {
        id: theme
    }

    readonly property string formatText: {
        if (!root.playback) {
            return "—"
        }
        const fmt = root.playback.currentFileFormat || ""
        return fmt !== "" ? fmt : "—"
    }

    readonly property string bitDepthText: {
        if (!root.playback) {
            return "—"
        }
        const depth = root.playback.currentBitDepth || 0
        return depth > 0 ? depth + " bit" : "—"
    }

    readonly property string sampleRateText: {
        if (!root.playback) {
            return "—"
        }
        const rate = root.playback.currentSampleRate || 0
        return rate > 0 ? (rate / 1000).toFixed(1) + " kHz" : "—"
    }

    readonly property string bitrateText: {
        if (!root.playback) {
            return "—"
        }
        const br = root.playback.currentAudioBitrate || 0
        const kbps = br >= 10000 ? Math.round(br / 1000) : br
        return kbps > 0 ? kbps + " kbps" : "—"
    }

    GridLayout {
        id: metadataGrid
        anchors.fill: parent
        columns: 2
        columnSpacing: theme.space3
        rowSpacing: theme.space2

        Repeater {
            model: [
                { label: qsTr("格式"), value: root.formatText },
                { label: qsTr("位深"), value: root.bitDepthText },
                { label: qsTr("采样率"), value: root.sampleRateText },
                { label: qsTr("码率"), value: root.bitrateText }
            ]

            delegate: RowLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: theme.space2

                Label {
                    text: modelData.label
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontCaption
                    color: theme.colorTextMuted
                    Layout.preferredWidth: 44
                }

                Label {
                    Layout.fillWidth: true
                    text: modelData.value
                    font.family: theme.fontFamily
                    font.pixelSize: theme.fontBody
                    font.weight: 600
                    color: theme.colorTextPrimary
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }
}
