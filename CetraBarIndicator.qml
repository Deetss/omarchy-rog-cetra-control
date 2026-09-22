pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import qs.Commons

Item {
  id: indicator
  required property var root

  readonly property real scaleFactor: Style.bar.iconFont / 13
  implicitWidth: 27 * scaleFactor
  implicitHeight: 26 * scaleFactor
  width: implicitWidth
  height: implicitHeight

  readonly property string leftEarbudPath: indicator.leftOutline + " M12 29.4 L18.5 30.8 Q19.5 31 19.2 32 Q19 32.9 18 32.6 L11.5 31.2 Q10.5 31 10.8 30 Q11 29.1 12 29.4 Z"
  readonly property string rightEarbudPath: indicator.rightOutline + " M52 29.4 L45.5 30.8 Q44.5 31 44.8 32 Q45 32.9 46 32.6 L52.5 31.2 Q53.5 31 53.2 30 Q53 29.1 52 29.4 Z"
  readonly property string leftOutline: "M11.848 10.383 Q13.000 9.000 14.800 9.000 L18.200 9.000 Q20.000 9.000 21.124 10.406 L23.563 13.453 Q24.000 14.000 24.000 14.700 L24.000 15.300 Q24.000 16.000 24.626 16.313 L26.600 17.300 Q28.000 18.000 28.000 19.565 L28.000 23.200 Q28.000 25.000 26.329 25.669 L24.671 26.331 Q23.000 27.000 22.074 28.543 L20.926 30.457 Q20.000 32.000 19.745 33.782 L17.255 51.218 Q17.000 53.000 15.355 52.269 L9.645 49.731 Q8.000 49.000 8.281 47.222 L10.719 31.778 Q11.000 30.000 9.848 28.617 L7.152 25.383 Q6.000 24.000 6.390 22.243 L7.610 16.757 Q8.000 15.000 9.152 13.617 Z"
  readonly property string rightOutline: "M52.152 10.383 Q51.000 9.000 49.200 9.000 L45.800 9.000 Q44.000 9.000 42.876 10.406 L40.437 13.453 Q40.000 14.000 40.000 14.700 L40.000 15.300 Q40.000 16.000 39.374 16.313 L37.400 17.300 Q36.000 18.000 36.000 19.565 L36.000 23.200 Q36.000 25.000 37.671 25.669 L39.329 26.331 Q41.000 27.000 41.926 28.543 L43.074 30.457 Q44.000 32.000 44.255 33.782 L46.745 51.218 Q47.000 53.000 48.645 52.269 L54.355 49.731 Q56.000 49.000 55.719 47.222 L53.281 31.778 Q53.000 30.000 54.152 28.617 L56.848 25.383 Q58.000 24.000 57.610 22.243 L56.390 16.757 Q56.000 15.000 54.848 13.617 Z"
  readonly property string micPath: "M23 7 C23.85 6.85 24.65 7.4 24.8 8.25 L26.35 17.5 C26.5 18.35 25.95 19.15 25.1 19.3 C24.25 19.45 23.45 18.9 23.3 18.05 L21.75 8.8 C21.6 7.95 22.15 7.15 23 7 Z"
  readonly property string micUnavailablePath: "M22.286 12 L21.75 8.8 C21.6 7.95 22.15 7.15 23 7 C23.85 6.85 24.65 7.4 24.8 8.25 L25.428 12 M25.763 14 L26.35 17.5 C26.5 18.35 25.95 19.15 25.1 19.3 C24.25 19.45 23.45 18.9 23.3 18.05 L22.621 14"

  function isValidCharge(val) {
    return typeof val === "number" && Number.isFinite(val) && val >= 0 && val <= 100
  }

  readonly property bool hasLeftLevel: indicator.isValidCharge(root.leftLevel)
  readonly property real leftPct: indicator.hasLeftLevel ? root.leftLevel : 0
  readonly property real leftFillHeight: 44 * (indicator.leftPct / 100)
  readonly property real leftFillY: 53 - indicator.leftFillHeight

  readonly property bool hasRightLevel: indicator.isValidCharge(root.rightLevel)
  readonly property real rightPct: indicator.hasRightLevel ? root.rightLevel : 0
  readonly property real rightFillHeight: 44 * (indicator.rightPct / 100)
  readonly property real rightFillY: 53 - indicator.rightFillHeight

  readonly property bool isMicSignalValid: root.connected && root.microphoneCaptureState === "active"
    && typeof root.microphoneLevel === "number" && Number.isFinite(root.microphoneLevel) && root.microphoneLevel >= 0
  readonly property real micFraction: indicator.isMicSignalValid ? Math.max(0, Math.min(1, root.microphoneLevel)) : 0
  readonly property real micFillHeight: 12.45 * indicator.micFraction
  readonly property real micFillY: 19.4 - indicator.micFillHeight
  readonly property real micFillOpacity: 1.0
  readonly property bool micFillVisible: indicator.isMicSignalValid && indicator.micFraction > 0

  Item {
    id: canvas
    width: 27
    height: 26
    scale: indicator.scaleFactor
    transformOrigin: Item.TopLeft

    // Mirrored Cetra earbuds scaled into 16x16 at (5.5, 5)
    Item {
      id: earbudArt
      x: 5.5
      y: 5
      width: 64
      height: 64
      scale: 16 / 64
      transformOrigin: Item.TopLeft

      // Left earbud dynamic charge fill (bottom-to-top waterline y=53..9)
      Item {
        id: leftFillClip
        x: 0
        y: indicator.leftFillY
        width: 32
        height: indicator.leftFillHeight
        clip: true
        visible: indicator.hasLeftLevel && indicator.leftPct > 0

        Shape {
          x: 0
          y: -indicator.leftFillY
          preferredRendererType: Shape.CurveRenderer
          ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillRule: ShapePath.OddEvenFill
            fillColor: root.barColor
            PathSvg { path: indicator.leftEarbudPath }
          }
        }
      }

      // Right earbud dynamic charge fill (bottom-to-top waterline y=53..9)
      Item {
        id: rightFillClip
        x: 32
        y: indicator.rightFillY
        width: 32
        height: indicator.rightFillHeight
        clip: true
        visible: indicator.hasRightLevel && indicator.rightPct > 0

        Shape {
          x: -32
          y: -indicator.rightFillY
          preferredRendererType: Shape.CurveRenderer
          ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillRule: ShapePath.OddEvenFill
            fillColor: root.barColor
            PathSvg { path: indicator.rightEarbudPath }
          }
        }
      }

      // Softened original Cetra silhouette; the charge fill preserves the stem joint.
      Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
          strokeWidth: 3.5
          strokeColor: root.barColor
          fillColor: "transparent"
          joinStyle: ShapePath.RoundJoin
          capStyle: ShapePath.RoundCap
          PathSvg { path: indicator.leftOutline }
        }
        ShapePath {
          strokeWidth: 3.5
          strokeColor: root.barColor
          fillColor: "transparent"
          joinStyle: ShapePath.RoundJoin
          capStyle: ShapePath.RoundCap
          PathSvg { path: indicator.rightOutline }
        }
      }

      // Small internal unknown dots when charge level is missing/invalid
      Rectangle {
        x: 13.3
        y: 19.3
        width: 5.4
        height: 5.4
        radius: 2.7
        color: root.barColor
        visible: !indicator.hasLeftLevel
      }

      Rectangle {
        x: 45.3
        y: 19.3
        width: 5.4
        height: 5.4
        radius: 2.7
        color: root.barColor
        visible: !indicator.hasRightLevel
      }
    }

    // Rounded, leaning signal meter; its geometry is independent of amplitude.
    Item {
      id: micArt
      anchors.fill: parent
      visible: root.showMicLevel
      Accessible.role: Accessible.StaticText
      Accessible.name: root.showMicLevel ? root.microphoneLevelText() : ""

      // Broken outline means no data; a continuous outline means valid capture data.
      Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
          strokeWidth: 1
          strokeColor: Qt.rgba(root.barForeground.r, root.barForeground.g, root.barForeground.b, indicator.isMicSignalValid ? 1.0 : 0.55)
          capStyle: ShapePath.RoundCap
          fillColor: indicator.isMicSignalValid ? Qt.rgba(root.barForeground.r, root.barForeground.g, root.barForeground.b, 0.16) : "transparent"
          PathSvg { path: indicator.isMicSignalValid ? indicator.micPath : indicator.micUnavailablePath }
        }
      }

      // Bottom-to-top measured signal only; never fabricate a fill for missing data.
      Item {
        id: micFillClip
        x: 21.5
        y: indicator.micFillY
        width: 5.2
        height: indicator.micFillHeight
        clip: true
        visible: indicator.micFillVisible
        opacity: indicator.micFillOpacity

        Shape {
          x: -21.5
          y: -indicator.micFillY
          preferredRendererType: Shape.CurveRenderer
          ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: root.barForeground
            PathSvg { path: indicator.micPath }
          }
        }
      }
    }
  }
}
