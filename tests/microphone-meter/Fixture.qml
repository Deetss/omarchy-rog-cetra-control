import QtQuick
import Quickshell.Io
Item {
 id: fixture
 property bool done: false
 property int phase: 0
 QtObject { id: sourceNode; property string name: "alsa_input.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_fixture.mono" }
 QtObject {
   id: graph
   property var source: sourceNode
   property var observation: ({capture:"inactive"})
 }
 MicrophoneMeter { id: meter; topology: graph }
 function check(ok, why) { if (!ok) throw new Error(why) }
 function run() {
   check(!Probe.process.running, "inactive source must not capture")
   graph.observation = {capture:"active"}
   check(Probe.process.running, "first admission must start in the same event turn")
   Probe.process.running = false
   graph.observation = {capture:"inactive"}
   graph.observation = {capture:"active"}
   check(!Probe.process.running, "capture toggles cannot bypass exit backoff")
   step.start()
 }
 Timer {
   id: step
   interval: 2100
   onTriggered: {
     if (fixture.phase === 0) {
       fixture.check(Probe.process.running, "retry starts after backoff")
       var old = Probe.process.command[1]
       sourceNode.name = "alsa_input.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_replacement.mono"
       fixture.check(!Probe.process.running, "stop old source first")
       fixture.check(Probe.process.command[1] === old, "no immediate replacement while stopping")
       fixture.phase = 1
       step.start()
     } else if (fixture.phase === 1) {
       fixture.check(Probe.process.running, "replacement starts after backoff")
       fixture.check(Probe.process.command[1] === sourceNode.name, "replacement uses current source")
       Probe.process.stdout.read('{"level":0}')
       fixture.check(meter.level === 0, "zero is valid signal data")
       fixture.phase = 2
       step.interval = 1600
       step.start()
     } else {
       fixture.check(meter.level === null, "stale data becomes unavailable")
       graph.observation = {capture:"unknown"}
       fixture.check(!Probe.process.running, "unknown route stops observation")
       fixture.done = true
     }
   }
 }
}
