import QtQuick
import QtQuick.Controls

Item {
    id: root
    required property var appController

    PlaylistPage {
        anchors.fill: parent
        appController: root.appController
    }
}
