import QtQuick

QtObject {
    readonly property color colorBgApp: "#F6F7F9"
    readonly property color colorBgPanel: "#FFFFFF"
    readonly property color colorBgElevated: "#FFFFFFF2"
    readonly property color colorBgHover: "#EEF2F7"
    readonly property color colorBgPressed: "#E3E9F1"
    readonly property color colorBgHoverClear: Qt.rgba(238 / 255, 242 / 255, 247 / 255, 0)
    readonly property color colorBgPressedClear: Qt.rgba(227 / 255, 233 / 255, 241 / 255, 0)

    readonly property color colorPrimary: "#3B82F6"
    readonly property color colorPrimaryActive: "#2563EB"
    readonly property color colorPrimaryTint: Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.15)
    readonly property color colorPrimaryTintStrong: Qt.rgba(59 / 255, 130 / 255, 246 / 255, 0.22)

    readonly property color colorTextPrimary: "#111827"
    readonly property color colorTextSecondary: "#4B5563"
    readonly property color colorTextMuted: "#9CA3AF"

    readonly property color colorBorderDefault: "#E5E7EB"
    readonly property color colorBorderFocus: "#60A5FA"

    readonly property color colorStateError: "#DC2626"
    readonly property color colorStateWarning: "#D97706"
    readonly property color colorStateSuccess: "#16A34A"
    readonly property color colorDanger: "#DC2626"
    readonly property color colorDangerHover: Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.9)
    readonly property color colorDangerPressed: Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.95)
    readonly property color colorDangerDisabled: Qt.rgba(220 / 255, 38 / 255, 38 / 255, 0.25)

    readonly property color surfaceGlass: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0.68)
    readonly property color surfaceGlassBorder: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0.85)
    readonly property color shadowSoft: Qt.rgba(15 / 255, 23 / 255, 42 / 255, 0.08)
    readonly property color colorLyricActive: "#1D4ED8"
    readonly property color colorLyricInactive: "#9CA3AF"
    readonly property color colorHiResBadge: "#F59E0B"
    readonly property color colorHiResBadgeEnd: "#EA580C"

    readonly property int radiusXs: 6
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 24
    readonly property int radiusPill: 999

    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 20
    readonly property int space6: 24
    readonly property int space8: 32
    readonly property int space10: 40

    readonly property string fontFamily: "Microsoft YaHei UI"
    readonly property int fontH1: 24
    readonly property int fontH2: 18
    readonly property int fontBody: 14
    readonly property int fontCaption: 12
    readonly property int fontMono: 12

    readonly property int motionFast: 120
    readonly property int motionNormal: 180
    readonly property int motionSlow: 220

    readonly property int sliderHandleSize: 12
    readonly property int sliderGrooveHeight: 3
    readonly property int glassBlurRadius: 24
    readonly property int shadowRadius: 20
    readonly property int shadowOffsetY: 8
}
