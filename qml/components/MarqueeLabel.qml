import QtQuick

Item {
    id: root

    property string text: ""
    property color color: "#111827"
    property string fontFamily: "Microsoft YaHei UI"
    property int fontPixelSize: 14
    property bool fontBold: false
    property int pauseDuration: 1500
    property int scrollDuration: 4000

    implicitHeight: label.implicitHeight
    clip: true

    readonly property bool needsScroll: label.implicitWidth > root.width + 1

    Text {
        id: label
        x: 0
        y: 0
        text: root.text
        color: root.color
        font.family: root.fontFamily
        font.pixelSize: root.fontPixelSize
        font.bold: root.fontBold
    }

    SequentialAnimation {
        id: marqueeAnimation
        loops: Animation.Infinite
        running: root.needsScroll && root.width > 0

        onStopped: label.x = 0

        PauseAnimation { duration: root.pauseDuration }

        NumberAnimation {
            target: label
            property: "x"
            from: 0
            to: root.needsScroll ? -(label.implicitWidth - root.width) : 0
            duration: root.scrollDuration
            easing.type: Easing.InOutQuad
        }

        PauseAnimation { duration: root.pauseDuration }

        NumberAnimation {
            target: label
            property: "x"
            from: root.needsScroll ? -(label.implicitWidth - root.width) : 0
            to: 0
            duration: root.scrollDuration
            easing.type: Easing.InOutQuad
        }
    }

    onNeedsScrollChanged: {
        if (!needsScroll) {
            marqueeAnimation.stop()
            label.x = 0
        }
    }

    onWidthChanged: {
        if (!needsScroll) {
            marqueeAnimation.stop()
            label.x = 0
        }
    }

    onTextChanged: {
        label.x = 0
    }
}
