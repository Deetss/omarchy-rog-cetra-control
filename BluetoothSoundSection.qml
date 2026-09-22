pragma ComponentBehavior: Bound
import QtQuick
import qs.Commons

Column {
  id: section
  required property var root
  spacing: Style.space(6)
  visible: root.bluetoothAudioConnected === true && !root.connected
  Text {
    textFormat: Text.PlainText
    width: section.width
    visible: root.bluetoothAudioConnected === true && !root.connected && root.bluetoothTelemetryState !== "ready" && root.bluetoothTelemetryStatusText() !== ""
    text: root.bluetoothTelemetryStatusText()
    color: root.dim
    font.family: root.fontFamily
    font.pixelSize: Style.font.caption
    wrapMode: Text.WordWrap
  }
  Text {
    textFormat: Text.PlainText
    width: section.width
    visible: root.bluetoothAudioConnected === true && !root.connected && root.usesBluetoothTelemetry
    text: root.bluetoothAncText()
    color: root.dim
    font.family: root.fontFamily
    font.pixelSize: Style.font.caption
    wrapMode: Text.WordWrap
  }
  ControlButton {
    panelRoot: root
    width: parent.width
    visible: root.bluetoothAudioConnected === true && !root.connected
    enabled: root.bluetoothTelemetryCanRefresh && !root.bluetoothTelemetryBusy
    label: root.tr("bluetooth.refresh", "Refresh telemetry")
    leftAlign: true
    horizontalPadding: 0
    fontFamily: root.fontFamily
    fontSize: Style.font.bodySmall
    foreground: root.foreground
    accent: root.accent
    onClicked: root.refreshBluetoothTelemetry()
  }
}
