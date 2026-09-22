import QtQuick
import qs.Commons

Column {
  id: section
  required property var root
  visible: root.panelAvailable
  activeFocusOnTab: true
  Keys.forwardTo: [root.keyTarget]
  Keys.onPressed: function (event) { event.accepted = true }
  spacing: Style.spacing.labelGap
  Accessible.role: Accessible.StaticText
  Accessible.name: root.tr("microphone.signalDescription", "Microphone: {state}", { state: root.microphoneLevelText() })
  Accessible.description: root.tr("microphone.tooltip", "Mic state: unknown / follow headset voice prompt")
  Text {
    textFormat: Text.PlainText
    width: parent.width
    text: root.tr("microphone.title", "Microphone")
    color: section.activeFocus ? root.accent : root.foreground
    font.family: root.fontFamily
    font.pixelSize: Style.font.bodySmall
    font.bold: true
  }
  Text {
    textFormat: Text.PlainText
    width: parent.width
    text: root.microphoneLevelText()
    color: root.dim
    font.family: root.fontFamily
    font.pixelSize: Style.font.caption
    wrapMode: Text.WordWrap
  }
  Rectangle {
    width: parent.width
    height: Style.space(4)
    radius: height / 2
    color: root.rule
    visible: root.microphoneSignalState() === "silent" || root.microphoneSignalState() === "signal"
    Rectangle {
      height: parent.height
      radius: height / 2
      width: parent.width * Math.max(0, Math.min(1, Number(root.microphoneLevel) || 0))
      color: root.foreground
    }
  }
}
