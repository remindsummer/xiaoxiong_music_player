import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import "../theme" as ThemeTokens

Dialog {
    id: root

    modal: true
    anchors.centerIn: parent
    standardButtons: Dialog.NoButton
    padding: 0

    signal trayChosen(bool remember)
    signal quitChosen(bool remember)

    ThemeTokens.Theme {
        id: theme
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(15 / 255, 23 / 255, 42 / 255, 0.28)
    }

    background: Rectangle {
        implicitWidth: 380
        implicitHeight: cardContent.implicitHeight + theme.space5 * 2
        radius: theme.radiusLg
        color: theme.colorBgPanel
        border.color: theme.colorBorderDefault
        border.width: 1

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: theme.shadowOffsetY
            radius: theme.shadowRadius
            samples: 25
            color: theme.shadowSoft
            transparentBorder: true
        }
    }

    onAboutToShow: rememberChoice.checked = false

    contentItem: ColumnLayout {
        id: cardContent
        anchors.fill: parent
        anchors.margins: theme.space5
        spacing: theme.space4

        Label {
            Layout.fillWidth: true
            text: qsTr("关闭窗口")
            color: theme.colorTextPrimary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontH2
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("是否最小化到系统托盘并继续后台播放？")
            color: theme.colorTextSecondary
            font.family: theme.fontFamily
            font.pixelSize: theme.fontBody
            lineHeight: 1.45
        }

        ThemedCheckBox {
            id: rememberChoice
            Layout.fillWidth: true
            text: qsTr("记住我的选择")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.space2

            Item { Layout.fillWidth: true }

            PrimaryButton {
                implicitHeight: 36
                Layout.minimumWidth: 112
                text: qsTr("最小化到托盘")
                onClicked: {
                    root.close()
                    root.trayChosen(rememberChoice.checked)
                }
            }

            GhostButton {
                implicitHeight: 36
                Layout.minimumWidth: 88
                text: qsTr("退出")
                onClicked: {
                    root.close()
                    root.quitChosen(rememberChoice.checked)
                }
            }
        }
    }
}
