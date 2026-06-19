import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property url imageSource
    property url fallbackSource: ""
    property int cornerRadius: 12
    property color placeholderColor: "#3B82F6"
    property bool useCrop: true

    implicitWidth: 56
    implicitHeight: 56

    readonly property bool hasBitmapCover: {
        const src = imageSource.toString()
        return src !== "" && !src.endsWith(".svg")
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.placeholderColor
    }

    Image {
        id: coverImage
        anchors.fill: parent
        source: root.imageSource
        fillMode: root.hasBitmapCover && root.useCrop ? Image.PreserveAspectCrop : Image.Pad
        sourceSize: Qt.size(Math.round(root.width), Math.round(root.height))
        visible: false
        asynchronous: true

        onStatusChanged: {
            if (status === Image.Error && root.hasBitmapCover) {
                source = root.fallbackSource.toString() !== "" ? root.fallbackSource : ""
            }
        }
    }

    Rectangle {
        id: roundMask
        anchors.fill: parent
        radius: root.cornerRadius
        color: "white"
        visible: false
    }

    OpacityMask {
        anchors.fill: parent
        source: coverImage
        maskSource: roundMask
    }
}
