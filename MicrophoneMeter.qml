import QtQuick
import Quickshell.Io

// Service-owned, opt-in peak client. No file output, mute inference or HID.
Item {
  id: meter
  required property var topology
  readonly property var source: topology.source
  readonly property bool inUse: topology.observation.capture === "active"
  property var level: null
  readonly property string sourceName: source ? source.name : ""
  readonly property string peakCommand: decodeURIComponent(Qt.resolvedUrl("bin/cetra-peak").toString().replace("file://", ""))
  onInUseChanged: reconcile()
  onSourceNameChanged: reconcile()

  function boundedPeak(value) {
    return typeof value === "number" && isFinite(value) ? Math.max(0, Math.min(1, value)) : null
  }

  function parseLevel(line) {
    if (line.length > 128) return null
    try { return boundedPeak(JSON.parse(line).level) } catch (_) { return null }
  }

  function startPeaks() {
    if (!peaks.running && meter.inUse && meter.sourceName && !retry.running) {
      peaks.capturedSource = meter.sourceName
      peaks.command = [meter.peakCommand, meter.sourceName]
      peaks.running = true
    }
  }

  function reconcile() {
    var wasRunning = peaks.running
    if (!inUse || peaks.capturedSource !== sourceName) {
      level = null
      if (peaks.running) { peaks.running = false; stopDeadline.restart() }
    }
    // The first admission is immediate. Never cancel an existing exit backoff.
    if (inUse && !wasRunning) startPeaks()
  }

  function meterStopped() {
    stopDeadline.stop()
    meter.level = null
    // Even if capture changes during recovery, keep the minimum restart delay.
    retry.restart()
  }

  Process {
    id: peaks
    property string capturedSource: ""
    stdinEnabled: true
    stdout: SplitParser {
      onRead: function (line) {
        meter.level = meter.inUse && peaks.capturedSource === meter.sourceName ? meter.parseLevel(line) : null
        freshness.restart()
      }
    }
    onRunningChanged: {
      if (!running) meter.meterStopped()
    }
  }
  Timer {
    id: retry
    interval: 2000
    onTriggered: meter.startPeaks()
  }
  Component.onCompleted: reconcile()
  Timer {
    id: stopDeadline
    interval: 2000
    onTriggered: {
      if (peaks.running && peaks.processId > 0) peaks.signal(9)
    }
  }
  Timer {
    id: freshness
    interval: 1500
    onTriggered: meter.level = null
  }
  Component.onDestruction: {
    if (peaks.running && peaks.processId > 0) peaks.signal(15)
  }
}
