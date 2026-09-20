pragma ComponentBehavior: Bound
import QtQuick
import qs.Commons

Column {
  id: section
  required property var root
  spacing: Style.space(6)
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
  ControlButton {
    panelRoot: root
    width: parent.width
    Component.onCompleted: root.connectionDetailsButton = this
    Component.onDestruction: if (root.connectionDetailsButton === this) root.connectionDetailsButton = null
    label: root.bluetoothExpanded ? root.tr("connection.detailsCollapse", "Details  −")
      : root.tr("connection.detailsExpand", "Details  +")
    leftAlign: true
    horizontalPadding: 0
    fontFamily: root.fontFamily
    fontSize: Style.font.bodySmall
    foreground: root.foreground
    accent: root.accent
    onClicked: root.bluetoothExpanded = !root.bluetoothExpanded
  }
  Column {
    width: parent.width
    spacing: Style.space(6)
    visible: root.bluetoothExpanded
    Repeater {
      model: [
        root.bluetoothAvailability !== "ready" ? root.tr("connection.serviceUnavailable", "Bluetooth service or adapter unavailable.") : "",
        root.tr("connection.usb", "USB receiver: {state}", { state: root.connectionText(root.receiver) }),
        root.tr("connection.bluetooth", "Bluetooth audio: {state}", { state: root.connectionText(root.bluetoothAudioConnected) }),
        root.tr("connection.routes", "Playback: {output}; capture: {input}.", { output: root.routeText(root.audioStatus.output), input: root.routeText(root.audioStatus.capture) }),
        root.bluetoothAudioConnected === true ? root.bluetoothProfileText() : "",
        root.tr("connection.le", "Service BLE: association unknown; charge is not used."),
        root.connected ? "" : root.tr("connection.usbControls", "ANC, lighting and voice settings require an available earbud through USB.")
      ]
      delegate: Text {
        textFormat: Text.PlainText
        required property string modelData
        width: section.width
        visible: text !== ""
        text: modelData
        color: root.dim
        font.family: root.fontFamily
        font.pixelSize: Style.font.caption
        wrapMode: Text.WordWrap
      }
    }
    Text {
      textFormat: Text.PlainText
      width: parent.width
      visible: root.bluetoothAudioConnected === true
      text: root.bluetoothBatteryDetail()
      color: root.dim
      font.family: root.fontFamily
      font.pixelSize: Style.font.caption
      wrapMode: Text.WordWrap
    }
    Text {
      textFormat: Text.PlainText
      width: parent.width
      text: root.caseFreshnessText()
      color: root.dim
      font.family: root.fontFamily
      font.pixelSize: Style.font.caption
      wrapMode: Text.WordWrap
    }
  }
  Column {
    width: parent.width
    spacing: Style.space(4)
    visible: (root.bluetoothExpanded && (root.bluetoothCandidates.length > 0 || root.audioIdentity === "selected"))
      || (!root.connected && root.audioIdentity !== "selected" && root.bluetoothCandidates.length > 0)
    Text {
      textFormat: Text.PlainText
      width: parent.width
      text: root.tr("connection.selectionHelp", "Choose the earbuds, not LE-ROG. This only remembers which device to observe.")
      color: root.dim
      font.family: root.fontFamily
      font.pixelSize: Style.font.caption
      wrapMode: Text.WordWrap
    }
    Repeater {
      model: root.bluetoothCandidates
      delegate: ControlButton {
        panelRoot: section.root
        required property var modelData
        width: section.width
        label: root.bluetoothDeviceText(modelData)
        leftAlign: true
        fontFamily: root.fontFamily
        fontSize: Style.font.caption
        foreground: root.foreground
        accent: root.accent
        onClicked: root.selectBluetoothAudio(modelData.address)
      }
    }
    ControlButton {
      panelRoot: root
      width: parent.width
      visible: root.audioIdentity === "selected"
      label: root.tr("connection.clear", "Clear plugin device selection")
      fontFamily: root.fontFamily
      fontSize: Style.font.caption
      foreground: root.foreground
      accent: root.accent
      onClicked: root.selectBluetoothAudio("")
    }
  }
}
