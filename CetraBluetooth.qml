import QtQuick
import Quickshell
import Quickshell.Bluetooth

// One service-owned, read-only observer. Selection is local preference, not pairing.
Item {
  id: observer
  required property bool active
  required property string selectedAddress
  required property var audioNodes
  // Installed Bluetooth qmltypes omits the ObjectModel dependency; keep the
  // singleton boundary dynamic, as host-injected service APIs are.
  readonly property var bluetoothApi: Bluetooth
  readonly property var devices: active ? bluetoothApi.devices.values : []
  readonly property string availability: active && bluetoothApi.adapters.values.length ? "ready" : "unavailable"
  readonly property var candidates: audioCandidates(devices, audioNodes)
  readonly property var device: resolveDevice(devices, selectedAddress)
  readonly property string identity: device ? "selected" : candidates.length ? "ambiguous" : "missing"
  readonly property var audioConnected: availability !== "ready" ? null : device ? device.connected : null
  // Native API exposes neither model/UUIDs nor a proven audio-to-LE relationship.
  readonly property var leConnected: null
  property int generation: 0
  property var trackedDevice: null
  property var battery: ({ percent: null, source: "bluez-audio", quality: "unavailable", snapshot: "none", observedAt: null, hardwareReportedAt: null, generation: 0 })
  onDeviceChanged: resetEvidence()
  onAudioConnectedChanged: resetEvidence()
  Component.onCompleted: resetEvidence()

  Connections {
    target: observer.device
    function onBatteryChanged() { observer.acceptBattery(observer.device, observer.generation, true) }
    function onBatteryAvailableChanged() { observer.acceptBattery(observer.device, observer.generation, false) }
    function onAddressChanged() { observer.resetEvidence() }
  }

  function normalizeAddress(value) {
    return typeof value === "string" && /^(?:[0-9a-f]{2}:){5}[0-9a-f]{2}$/i.test(value) ? value.toUpperCase() : ""
  }
  function audioCandidates(values, nodes) {
    if (values.length > 64 || nodes.length > 512) return []
    var addresses = new Set()
    for (var i = 0; i < nodes.length; i++) {
      var node = nodes[i], p = node.properties || {}
      if (node.ready && node.isStream === false && p["device.api"] === "bluez5"
          && /^(Audio\/Sink|Audio\/Source)$/.test(p["media.class"] || "")) {
        var address = normalizeAddress(p["api.bluez5.address"])
        if (address) addresses.add(address)
      }
    }
    return values.filter(function (entry) {
      return entry.connected && addresses.has(normalizeAddress(entry.address))
    }).map(function (entry) { return { address: normalizeAddress(entry.address), name: entry.name || entry.deviceName || entry.address } })
  }
  function resolveDevice(values, address) {
    var key = normalizeAddress(address)
    if (!key || values.length > 64) return null
    var matches = values.filter(function (entry) { return normalizeAddress(entry.address) === key })
    return matches.length === 1 ? matches[0] : null
  }
  function batteryPercent(available, value) {
    return available === true && typeof value === "number" && isFinite(value) && value >= 0 && value <= 1
      ? Math.round(value * 100) : null
  }
  function resetEvidence() {
    generation += 1
    trackedDevice = device
    acceptBattery(device, generation, false)
  }
  function acceptBattery(source, epoch, updated) {
    if (epoch !== generation || source !== device || source !== trackedDevice) return
    var value = source && audioConnected === true ? batteryPercent(source.batteryAvailable, source.battery) : null
    // No monotonic report timestamp in this API. A changed property is not proof
    // of hardware freshness; even after reconnect it may be BlueZ's cached value.
    battery = { percent: value, source: "bluez-audio", quality: value === null ? "unavailable" : "system-reported",
      snapshot: value === null ? "none" : updated ? "property-update" : "cached", observedAt: null,
      hardwareReportedAt: null, generation: epoch }
  }
}
