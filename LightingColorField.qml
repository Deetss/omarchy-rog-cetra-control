import QtQuick
import QtQuick.Shapes
import qs.Commons

Item {
  id: field
  required property var root
  implicitHeight: wheel.height + Style.space(14)
  property real hue: 0
  property real saturation: 0
  property real brightness: 1
  readonly property string hexValue: "#" + root.lightingDraftRgb.map(function(value) { return value.toString(16).padStart(2, "0") }).join("").toUpperCase()
  signal acceptRequested()
  signal edited()

  function resetFromDraft(preserveHue) {
    var rgb = root.lightingDraftRgb
    var color = Qt.rgba(rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, 1)
    if (color.hsvHue >= 0 || !preserveHue) hue = Math.max(0, color.hsvHue)
    saturation = color.hsvSaturation
    brightness = color.hsvValue
  }
  function publishDraft() {
    if (!root.lightingColorExpanded || !root.connected) return
    var color = Qt.hsva(hue, saturation, brightness, 1)
    root.setLightingDraftChannel(0, Math.round(color.r * 255))
    root.setLightingDraftChannel(1, Math.round(color.g * 255))
    root.setLightingDraftChannel(2, Math.round(color.b * 255))
    edited()
  }
  function choosePoint(x, y) {
    if (wheel.width <= 0) return
    var dx = x - wheel.width / 2, dy = wheel.height / 2 - y
    var distance = Math.sqrt(dx * dx + dy * dy)
    if (distance > 0.01) hue = (Math.atan2(dy, dx) / (2 * Math.PI) + 1) % 1
    saturation = Math.min(1, distance / (wheel.width / 2))
    publishDraft()
  }
  function setBrightness(value) {
    brightness = Math.max(0, Math.min(1, value))
    publishDraft()
  }
  function setChannel(index, value) {
    root.setLightingDraftChannel(index, value)
    resetFromDraft(true)
    edited()
  }
  function setHex(text) {
    if (!root.lightingColorExpanded || !root.connected || typeof text !== "string") return false
    var hex = text.trim().replace(/^#/, "")
    if (!/^[0-9a-fA-F]{6}$/.test(hex)) return false
    for (var i = 0; i < 3; i++) root.setLightingDraftChannel(i, parseInt(hex.slice(i * 2, i * 2 + 2), 16))
    resetFromDraft(true)
    return true
  }
  onVisibleChanged: if (visible) {
    resetFromDraft(false)
    Qt.callLater(function() {
      if (field && field.visible && field.root && typeof field.root.focusControl === "function") field.root.focusControl(wheel)
    })
  }
  Component.onCompleted: resetFromDraft(false)

  Item {
    id: wheel
    width: Math.max(0, Math.min(parent.width - Style.space(94), Style.space(184)))
    y: Style.space(7)
    height: width
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.horizontalCenterOffset: -Style.space(40)
    activeFocusOnTab: true
    Accessible.role: Accessible.Slider
    Accessible.name: root.tr("lighting.palette", "Color palette")
    Accessible.description: root.tr("lighting.paletteHelp", "Left and right change hue; up and down change saturation.")
    Keys.onTabPressed: root.moveFocus(1, true)
    Keys.onBacktabPressed: root.moveFocus(-1, true)
    Keys.onPressed: function(event) {
      if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
        field.hue = (field.hue + (event.key === Qt.Key_Right ? 1/360 : -1/360) + 1) % 1
      } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
        field.saturation = Math.max(0, Math.min(1, field.saturation + (event.key === Qt.Key_Up ? 0.01 : -0.01)))
      } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
        field.acceptRequested()
        event.accepted = true
        return
      } else return
      field.publishDraft()
      event.accepted = true
    }
    // Native gradients express selectable HSV data, not decorative theme colors.
    Shape {
      anchors.fill: parent
      preferredRendererType: Shape.CurveRenderer
      ShapePath {
        strokeWidth: -1
        fillGradient: ConicalGradient {
          centerX: wheel.width / 2; centerY: wheel.height / 2; angle: 0
          GradientStop { position: 0 / 6; color: Qt.hsva(0 / 6, 1, field.brightness, 1) }
          GradientStop { position: 1 / 6; color: Qt.hsva(1 / 6, 1, field.brightness, 1) }
          GradientStop { position: 2 / 6; color: Qt.hsva(2 / 6, 1, field.brightness, 1) }
          GradientStop { position: 3 / 6; color: Qt.hsva(3 / 6, 1, field.brightness, 1) }
          GradientStop { position: 4 / 6; color: Qt.hsva(4 / 6, 1, field.brightness, 1) }
          GradientStop { position: 5 / 6; color: Qt.hsva(5 / 6, 1, field.brightness, 1) }
          GradientStop { position: 6 / 6; color: Qt.hsva(6 / 6, 1, field.brightness, 1) }
        }
        PathAngleArc { centerX: wheel.width / 2; centerY: wheel.height / 2; radiusX: wheel.width / 2; radiusY: wheel.height / 2; startAngle: 0; sweepAngle: 360 }
      }
      ShapePath {
        strokeWidth: -1
        fillGradient: RadialGradient {
          centerX: wheel.width / 2; centerY: wheel.height / 2
          focalX: centerX; focalY: centerY; centerRadius: wheel.width / 2
          GradientStop { position: 0; color: Qt.rgba(field.brightness, field.brightness, field.brightness, 1) }
          GradientStop { position: 1; color: Qt.rgba(field.brightness, field.brightness, field.brightness, 0) }
        }
        PathAngleArc { centerX: wheel.width / 2; centerY: wheel.height / 2; radiusX: wheel.width / 2; radiusY: wheel.height / 2; startAngle: 0; sweepAngle: 360 }
      }
    }
    Rectangle {
      anchors.fill: parent
      radius: width / 2
      color: "transparent"
      border.width: wheel.activeFocus ? 2 : 1
      border.color: wheel.activeFocus ? root.accent : root.dim
      antialiasing: true
    }
    Rectangle {
      width: Style.space(14); height: width; radius: width / 2
      x: wheel.width / 2 + Math.cos(field.hue * 2 * Math.PI) * field.saturation * wheel.width / 2 - width / 2
      y: wheel.height / 2 - Math.sin(field.hue * 2 * Math.PI) * field.saturation * wheel.height / 2 - height / 2
      color: "transparent"
      border.width: 2; border.color: Qt.rgba(0, 0, 0, 1)
      antialiasing: true
      Rectangle {
        anchors.fill: parent; anchors.margins: 2
        radius: width / 2; color: "transparent"
        border.width: 2; border.color: Qt.rgba(1, 1, 1, 1)
        antialiasing: true
      }
    }
    MouseArea {
      anchors.fill: parent
      cursorShape: Qt.CrossCursor
      onPressed: function(mouse) {
        var dx = mouse.x - width / 2, dy = mouse.y - height / 2
        if (dx * dx + dy * dy > width * width / 4) { mouse.accepted = false; return }
        root.focusControl(wheel)
        field.choosePoint(mouse.x, mouse.y)
      }
      onPositionChanged: function(mouse) { if (pressed) field.choosePoint(mouse.x, mouse.y) }
    }
  }
  Column {
    width: Style.space(64)
    spacing: Style.space(8)
    anchors.left: wheel.right
    anchors.leftMargin: Style.space(16)
    anchors.verticalCenter: wheel.verticalCenter
    Rectangle {
      anchors.horizontalCenter: parent.horizontalCenter
      width: Style.space(40)
      height: width
      radius: width / 2
      color: Qt.rgba(root.lightingDraftRgb[0] / 255, root.lightingDraftRgb[1] / 255, root.lightingDraftRgb[2] / 255, 1)
      border.width: 1
      border.color: root.dim
      antialiasing: true
      Accessible.role: Accessible.StaticText
      Accessible.name: root.tr("lighting.selectedColor", "Selected color")
      Accessible.description: field.hexValue
    }
    Text {
      textFormat: Text.PlainText
      width: parent.width
      text: field.hexValue
      color: root.foreground
      font.family: root.fontFamily
      font.pixelSize: Style.font.caption
      horizontalAlignment: Text.AlignHCenter
    }
  }

}
