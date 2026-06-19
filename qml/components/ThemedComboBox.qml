import QtQuick
import QtQuick.Controls
import "../theme" as ThemeTokens

ComboBox {
    id: root

    ThemeTokens.Theme {
        id: theme
    }

    font.family: theme.fontFamily
    font.pixelSize: theme.fontBody

    background: Rectangle {
        radius: theme.radiusSm
        color: root.enabled ? theme.surfaceGlass : theme.colorBgHover
        border.width: 1
        border.color: root.activeFocus ? theme.colorBorderFocus : theme.colorBorderDefault
    }

    contentItem: Label {
        leftPadding: theme.space2
        rightPadding: theme.space2 + root.indicator.width
        text: root.displayText
        font: root.font
        color: root.enabled ? theme.colorTextPrimary : theme.colorTextMuted
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        x: root.width - width - theme.space2
        y: (root.height - height) / 2
        width: 10
        height: 6
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = theme.colorTextSecondary
            ctx.lineWidth = 1.5
            ctx.beginPath()
            ctx.moveTo(0, 0)
            ctx.lineTo(width / 2, height)
            ctx.lineTo(width, 0)
            ctx.stroke()
        }
    }

    popup: Popup {
        y: root.height + 2
        width: root.width
        padding: theme.space1
        background: Rectangle {
            radius: theme.radiusSm
            color: theme.colorBgPanel
            border.color: theme.colorBorderDefault
            border.width: 1
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }

    delegate: ItemDelegate {
        width: root.width
        text: modelData
        font.family: theme.fontFamily
        font.pixelSize: theme.fontBody
        highlighted: root.highlightedIndex === index
        background: Rectangle {
            color: parent.highlighted ? theme.colorBgHover : "transparent"
            radius: theme.radiusXs
        }
    }
}
