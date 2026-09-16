import QtQuick
import Quickshell.Bluetooth
import Quickshell.Services.Pipewire

Item {
  id: fixture
  property string selected: ""
  property bool observing: true
  QtObject {
    id: main
    property string address: "AA:BB:CC:DD:EE:01"
    property string name: "Cetra fixture"
    property string deviceName: name
    property bool connected: true
    property bool batteryAvailable: true
    property real battery: 0.55
  }
  QtObject {
    id: le
    property string address: "AA:BB:CC:DD:EE:02"
    property string name: "LE-ROG CTWSN"
    property string deviceName: name
    property bool connected: true
    property bool batteryAvailable: true
    property real battery: 0.71
  }
  QtObject {
    id: sink
    property string name: "bluez_output.AA_BB_CC_DD_EE_01.1"
    property bool ready: true
    property bool isStream: false
    property bool isSink: true
    property int type: 8
    property var properties: ({"device.api":"bluez5", "api.bluez5.address": main.address, "api.bluez5.profile":"a2dp-sink", "media.class":"Audio/Sink"})
  }
  QtObject {
    id: hfpSource
    property string name: "bluez_input.AA:BB:CC:DD:EE:01"
    property bool ready: true
    property bool isStream: false
    property bool isSink: false
    property int type: 9
    property var properties: ({"bluez5.loopback":true, "device.id":128, "media.class":"Audio/Source"})
  }
  AudioTopology {
    id: topology
    active: fixture.observing
    usbAvailable: false
    bluetoothAddress: observer.device && observer.audioConnected ? observer.device.address : ""
  }
  CetraBluetooth {
    id: observer
    active: fixture.observing
    selectedAddress: fixture.selected
    audioNodes: topology.nodes
  }
  function check(condition, message) {
    if (!condition) throw new Error(message)
  }
  function run() {
    Bluetooth.adapters.values = [{}]
    Bluetooth.devices.values = [le, main]
    Pipewire.nodes.values = [sink]
    check(observer.candidates.length === 1, "native model projection")
    check(observer.device === null && observer.identity === "ambiguous", "explicit selection required")
    selected = main.address
    check(observer.device === main && observer.audioConnected === true, "selection binding")
    check(observer.battery.percent === 55 && observer.battery.snapshot === "cached", "initial cached battery")
    check(topology.audio.profile === "a2dp" && topology.audio.microphone === false, "profile binding")
    check(topology.observation.communication === "unknown", "Bluetooth cannot drive USB call")
    main.battery = .62
    check(observer.battery.percent === 62 && observer.battery.snapshot === "property-update", "native battery property event")
    const before = observer.generation
    main.connected = false
    check(observer.battery.percent === null && observer.generation > before, "disconnect clears value")
    main.connected = true
    check(observer.battery.percent === 62 && observer.battery.snapshot === "cached", "reconnect uses unverified snapshot")
    main.name = "Renamed headset"
    check(observer.device === main, "rename keeps selected identity")
    Bluetooth.devices.values = [le]
    check(observer.device === null && observer.battery.percent === null, "removal invalidates")
    main.battery = .9
    check(observer.battery.percent === null, "old object signal detached")
    Bluetooth.devices.values = [le, main]
    check(observer.battery.percent === 90 && observer.battery.snapshot === "cached", "replacement snapshot")
    Bluetooth.adapters.values = []
    check(observer.availability === "unavailable" && observer.battery.percent === null, "adapter loss")
    Bluetooth.devices.values = []
    Bluetooth.adapters.values = [{}]
    check(observer.audioConnected === null, "restart without devices stays unknown")
    Bluetooth.devices.values = [le, main]
    check(observer.audioConnected === true, "native recovery")
    sink.properties = {"device.api":"bluez5", "api.bluez5.address":main.address,
      "api.bluez5.profile":"headset-head-unit", "media.class":"Audio/Sink", "device.id":128}
    Pipewire.nodes.values = [sink, hfpSource]
    check(topology.audio.profile === "hfp" && topology.audio.microphone === true, "public HFP source relation")
    observing = false
    check(observer.battery.percent === null && observer.candidates.length === 0, "service teardown")
    return true
  }
}
