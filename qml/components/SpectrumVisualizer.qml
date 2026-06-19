import QtQuick
import "../theme" as ThemeTokens

Item {
    id: root

    property var levels: []
    property bool active: true
    property bool showEnabled: true
    property int refreshFps: 30
    property string style: "smoothMirror"
    property real styleOpacity: 0.58
    property int pointCount: 128
    property int sourceBinCount: 32
    property real maxHeightRatio: 0.2
    property real idleLevel: 0.04

    ThemeTokens.Theme {
        id: theme
    }

    readonly property int paintIntervalMs: Math.max(16, Math.round(1000 / Math.max(15, root.refreshFps)))

    readonly property int sourceMaxIndex: {
        if (root.levels && root.levels.length > 0) {
            return root.levels.length - 1
        }
        return root.sourceBinCount - 1
    }

    readonly property real maxAmplitude: height * root.maxHeightRatio

    function edgeFade(t) {
        return Math.sin(Math.PI * t)
    }

    function mirroredSourcePosition(t) {
        const span = Math.max(1, root.pointCount - 1)
        const displayIndex = t * span
        const half = span / 2
        if (displayIndex <= half) {
            return root.sourceMaxIndex * (1 - displayIndex / half)
        }
        return root.sourceMaxIndex * ((displayIndex - half) / half)
    }

    function binLevel(index) {
        if (root.levels && root.levels.length > index) {
            return Number(root.levels[index]) || 0
        }
        return root.idleLevel + 0.03 * Math.sin(index * 0.45)
    }

    function rawLevelAtSource(sourcePos) {
        const i0 = Math.floor(sourcePos)
        const i1 = Math.min(root.sourceMaxIndex, i0 + 1)
        const frac = sourcePos - i0
        const v0 = root.binLevel(i0)
        const v1 = root.binLevel(i1)
        return v0 + (v1 - v0) * frac
    }

    function sampleLevel(t) {
        const fade = root.edgeFade(t)
        if (!root.active) {
            return root.idleLevel * fade
        }
        return root.rawLevelAtSource(root.mirroredSourcePosition(t)) * fade
    }

    function spectrumColor(alpha) {
        return Qt.rgba(theme.colorPrimary.r,
                         theme.colorPrimary.g,
                         theme.colorPrimary.b,
                         alpha * root.styleOpacity)
    }

    function buildAmplitudes(maxAmp, steps) {
        const amps = []
        for (let i = 0; i < steps; ++i) {
            const t = i / (steps - 1)
            const level = root.sampleLevel(t)
            amps.push(level * maxAmp)
        }
        return amps
    }

    function buildWavePoints(width, centerY, amplitudes, sign) {
        const points = []
        const count = amplitudes.length
        for (let i = 0; i < count; ++i) {
            points.push({
                x: (i / (count - 1)) * width,
                y: centerY - sign * amplitudes[i]
            })
        }
        return points
    }

    function traceSmoothCurve(ctx, points, moveToFirst) {
        if (points.length < 2) {
            return
        }

        if (moveToFirst) {
            ctx.moveTo(points[0].x, points[0].y)
        }

        for (let i = 0; i < points.length - 1; ++i) {
            const p0 = points[Math.max(0, i - 1)]
            const p1 = points[i]
            const p2 = points[i + 1]
            const p3 = points[Math.min(points.length - 1, i + 2)]

            const cp1x = p1.x + (p2.x - p0.x) / 6
            const cp1y = p1.y + (p2.y - p0.y) / 6
            const cp2x = p2.x - (p3.x - p1.x) / 6
            const cp2y = p2.y - (p3.y - p1.y) / 6

            ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y)
        }
    }

    function paintSmoothMirror(ctx, w, cy, maxAmp, amps) {
        const upper = root.buildWavePoints(w, cy, amps, 1)
        const lower = root.buildWavePoints(w, cy, amps, -1)

        const gradient = ctx.createLinearGradient(0, cy - maxAmp, 0, cy + maxAmp)
        gradient.addColorStop(0, root.spectrumColor(0.12))
        gradient.addColorStop(0.42, root.spectrumColor(0.06))
        gradient.addColorStop(0.5, root.spectrumColor(0.02))
        gradient.addColorStop(0.58, root.spectrumColor(0.06))
        gradient.addColorStop(1, root.spectrumColor(0.12))

        ctx.beginPath()
        ctx.moveTo(upper[0].x, upper[0].y)
        root.traceSmoothCurve(ctx, upper, false)
        ctx.lineTo(lower[lower.length - 1].x, lower[lower.length - 1].y)
        root.traceSmoothCurve(ctx, lower.slice().reverse(), false)
        ctx.closePath()
        ctx.fillStyle = gradient
        ctx.fill()
    }

    function paintMirrorBars(ctx, w, cy, maxAmp, amps) {
        const barCount = 48
        const slotWidth = w / barCount
        const barWidth = slotWidth * 0.55
        const radius = Math.min(3, barWidth / 2)

        ctx.fillStyle = root.spectrumColor(0.35)

        for (let i = 0; i < barCount; ++i) {
            const t = i / (barCount - 1)
            const ampIndex = Math.round(t * (amps.length - 1))
            const amp = amps[ampIndex]
            if (amp < 0.5) {
                continue
            }

            const x = i * slotWidth + (slotWidth - barWidth) / 2
            const halfH = amp

            ctx.beginPath()
            if (typeof ctx.roundRect === "function") {
                ctx.roundRect(x, cy - halfH, barWidth, halfH, radius)
                ctx.fill()
                ctx.beginPath()
                ctx.roundRect(x, cy, barWidth, halfH, radius)
            } else {
                ctx.rect(x, cy - halfH, barWidth, halfH)
                ctx.fill()
                ctx.beginPath()
                ctx.rect(x, cy, barWidth, halfH)
            }
            ctx.fill()
        }
    }

    function paintLineTrace(ctx, w, cy, maxAmp, amps) {
        const upper = root.buildWavePoints(w, cy, amps, 1)
        const lower = root.buildWavePoints(w, cy, amps, -1)

        ctx.lineWidth = 1.5
        ctx.strokeStyle = root.spectrumColor(0.55)
        ctx.beginPath()
        root.traceSmoothCurve(ctx, upper, true)
        ctx.stroke()

        ctx.beginPath()
        root.traceSmoothCurve(ctx, lower, true)
        ctx.stroke()
    }

    function paintUpperGlow(ctx, w, cy, maxAmp, amps) {
        const upper = root.buildWavePoints(w, cy, amps, 1)

        const gradient = ctx.createLinearGradient(0, cy - maxAmp, 0, cy)
        gradient.addColorStop(0, root.spectrumColor(0.14))
        gradient.addColorStop(0.55, root.spectrumColor(0.06))
        gradient.addColorStop(1, root.spectrumColor(0))

        ctx.beginPath()
        ctx.moveTo(upper[0].x, upper[0].y)
        root.traceSmoothCurve(ctx, upper, false)
        ctx.lineTo(w, cy)
        ctx.lineTo(0, cy)
        ctx.closePath()
        ctx.fillStyle = gradient
        ctx.fill()
    }

    function paintParticleDots(ctx, w, cy, maxAmp, amps) {
        const colCount = 56
        const slotW = w / colCount
        const cellW = Math.max(2, Math.min(5, slotW * 0.62))
        const cellH = cellW
        const rowGap = 1
        const centerGap = 1
        const rowPitch = cellH + rowGap
        const maxRows = Math.max(1, Math.floor((maxAmp - centerGap * 0.5) / rowPitch))

        ctx.fillStyle = root.spectrumColor(0.42)

        for (let i = 0; i < colCount; ++i) {
            const t = i / (colCount - 1)
            const ampIndex = Math.round(t * (amps.length - 1))
            const amp = amps[ampIndex]
            const litRows = Math.min(maxRows, Math.max(0, Math.floor(amp / rowPitch)))
            if (litRows === 0) {
                continue
            }

            const x = i * slotW + (slotW - cellW) / 2

            for (let row = 0; row < litRows; ++row) {
                const offset = centerGap * 0.5 + row * rowPitch
                ctx.fillRect(x, cy - offset - cellH, cellW, cellH)
                ctx.fillRect(x, cy + offset, cellW, cellH)
            }
        }
    }

    function paintSpectrum(ctx, w, h) {
        const cy = h / 2
        const maxAmp = root.maxAmplitude
        const steps = root.pointCount
        const amps = root.buildAmplitudes(maxAmp, steps)

        switch (root.style) {
        case "mirrorBars":
            root.paintMirrorBars(ctx, w, cy, maxAmp, amps)
            break
        case "lineTrace":
            root.paintLineTrace(ctx, w, cy, maxAmp, amps)
            break
        case "upperGlow":
            root.paintUpperGlow(ctx, w, cy, maxAmp, amps)
            break
        case "particleDots":
            root.paintParticleDots(ctx, w, cy, maxAmp, amps)
            break
        default:
            root.paintSmoothMirror(ctx, w, cy, maxAmp, amps)
            break
        }
    }

    Timer {
        id: paintTimer
        interval: root.paintIntervalMs
        running: root.showEnabled && root.visible
        repeat: true
        onTriggered: waveCanvas.requestPaint()
    }

    onRefreshFpsChanged: paintTimer.interval = root.paintIntervalMs

    Canvas {
        id: waveCanvas
        anchors.fill: parent
        visible: root.showEnabled

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const w = width
            const h = height
            if (w <= 0 || h <= 0) {
                return
            }

            root.paintSpectrum(ctx, w, h)
        }
    }

    onLevelsChanged: if (showEnabled) waveCanvas.requestPaint()
    onActiveChanged: if (showEnabled) waveCanvas.requestPaint()
    onShowEnabledChanged: if (showEnabled) waveCanvas.requestPaint()
    onStyleChanged: if (showEnabled) waveCanvas.requestPaint()
    onStyleOpacityChanged: if (showEnabled) waveCanvas.requestPaint()
    onWidthChanged: if (showEnabled) waveCanvas.requestPaint()
    onHeightChanged: if (showEnabled) waveCanvas.requestPaint()
}
