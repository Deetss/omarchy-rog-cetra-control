import QtQuick
import Quickshell
import Quickshell.Io

Item {
  id: telemetryRoot

  property bool active: false
  property string address: ""
  property int deviceGeneration: 0
  property bool panelOpen: false

  property var snapshot: null
  state: "idle"
  property bool requestInFlight: false
  property bool pendingLaunch: false
  readonly property bool busy: requestInFlight || proc.running || pendingLaunch
  readonly property bool canRefresh: eligible && !cooldownTimer.running && !proc.running && !requestInFlight && !pendingLaunch

  readonly property string helperCommand: decodeURIComponent(Qt.resolvedUrl("bin/cetra-bt-read").toString().replace("file://", ""))
  readonly property string normalizedAddress: normalizeAddress(address)
  readonly property bool eligible: active && normalizedAddress !== ""

  property int consecutiveFailures: 0
  property int requestGeneration: 0
  property int capturedGeneration: -1
  property string capturedAddress: ""
  property double launchWallTime: 0
  property double lastAttemptWallTime: 0
  property double snapshotAcceptedAt: 0
  property var stagedResponse: null
  property int stdoutFrameCount: 0
  property bool stdoutInvalid: false
  property int lastExitCode: -1
  property bool processStopHandled: true

  function normalizeAddress(addr) {
    if (typeof addr !== "string") return ""
    var trimmed = addr.trim().toUpperCase()
    return /^([0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(trimmed) ? trimmed : ""
  }

  function isEligible() {
    return active && normalizeAddress(address) !== ""
  }

  function isValidBattery(val) {
    if (val === null) return true
    return typeof val === "number" && isFinite(val) && Math.floor(val) === val && val >= 0 && val <= 100
  }

  function validateSnapshot(data, expectedAddress) {
    if (!data || typeof data !== "object" || Array.isArray(data)) return null
    if (data.status !== "ok") return null
    if (typeof data.address !== "string" || normalizeAddress(data.address) !== expectedAddress) return null
    if (!isValidBattery(data.left) || !isValidBattery(data.right) || !isValidBattery(data.case)) return null
    if (["off", "anc", "ambient", "unknown"].indexOf(data.mode) < 0) return null
    var leftChargingValid = typeof data.left_charging === "boolean" || data.left_charging === null
    var rightChargingValid = typeof data.right_charging === "boolean" || data.right_charging === null
    if (!leftChargingValid || !rightChargingValid) return null
    if ((data.left_charging === null) !== (data.right_charging === null)) return null
    if (data.case_charging !== null && typeof data.case_charging !== "boolean") return null

    return {
      status: "ok",
      address: expectedAddress,
      left: data.left,
      right: data.right,
      case: data.case,
      mode: data.mode,
      left_charging: data.left_charging,
      right_charging: data.right_charging,
      case_charging: data.case_charging
    }
  }

  function parseTelemetryLine(line, expectedAddress) {
    if (typeof line !== "string" || line.length === 0 || line.length > 1024) return null
    var data
    try {
      data = JSON.parse(line.trim())
    } catch (_) {
      return null
    }
    return validateSnapshot(data, expectedAddress)
  }

  function retryDelay(failures) {
    if (failures <= 1) return 30000
    if (failures === 2) return 60000
    if (failures === 3) return 120000
    if (failures === 4) return 240000
    return 300000
  }

  function executeLaunch() {
    var norm = normalizeAddress(address)
    if (!active || norm === "" || proc.running || requestInFlight) return

    pollTimer.stop()
    capturedGeneration = requestGeneration
    capturedAddress = norm
    stagedResponse = null
    stdoutFrameCount = 0
    stdoutInvalid = false
    launchWallTime = Date.now()
    lastAttemptWallTime = launchWallTime
    lastExitCode = -1
    processStopHandled = false
    requestInFlight = true
    state = "loading"

    watchdogTimer.interval = 15000
    watchdogTimer.restart()
    proc.command = [helperCommand, norm]
    proc.running = true
  }

  function queuePendingLaunch() {
    if (!isEligible()) {
      pendingLaunch = false
      return
    }
    if (!pendingLaunch) {
      pendingLaunch = true
      Qt.callLater(drainPendingLaunch)
    }
  }

  function drainPendingLaunch() {
    if (!pendingLaunch) return
    if (requestInFlight || proc.running) return
    pendingLaunch = false
    if (isEligible()) {
      executeLaunch()
    }
  }

  function requestLaunch() {
    if (!isEligible()) {
      pendingLaunch = false
      return
    }
    if (proc.running || requestInFlight) {
      pendingLaunch = true
      terminateProcess()
      return
    }
    queuePendingLaunch()
  }

  function terminateProcess() {
    if (proc.running) {
      if (proc.processId > 0) {
        try { proc.signal(15) } catch (_) {}
      } else {
        proc.running = false
      }
      killDeadlineTimer.interval = 1000
      killDeadlineTimer.restart()
    }
  }

  function handleProcessStopped(exitCode) {
    if (processStopHandled) return
    processStopHandled = true
    requestInFlight = false
    lastExitCode = exitCode
    watchdogTimer.stop()
    killDeadlineTimer.stop()

    if (capturedGeneration !== requestGeneration) {
      stagedResponse = null
      if (pendingLaunch && isEligible()) {
        Qt.callLater(drainPendingLaunch)
      } else {
        pendingLaunch = false
      }
      return
    }

    var now = Date.now()
    var wallElapsed = now - launchWallTime
    var clockFailed = now < launchWallTime || wallElapsed > 15000

    if (exitCode === 0 && stagedResponse !== null && !stdoutInvalid && stdoutFrameCount === 1 && !clockFailed) {
      consecutiveFailures = 0
      snapshot = stagedResponse
      stagedResponse = null
      snapshotAcceptedAt = now
      state = "ready"
      cooldownTimer.interval = 2000
      cooldownTimer.restart()
      pollTimer.interval = panelOpen ? 15000 : 120000
      pollTimer.restart()
    } else {
      consecutiveFailures = Math.min(consecutiveFailures + 1, 5)
      snapshot = null
      stagedResponse = null
      state = "unavailable"
      cooldownTimer.interval = 30000
      cooldownTimer.restart()
      if (isEligible()) {
        pollTimer.interval = retryDelay(consecutiveFailures)
        pollTimer.restart()
      } else {
        pollTimer.stop()
      }
    }

    if (pendingLaunch && isEligible()) {
      Qt.callLater(drainPendingLaunch)
    } else {
      pendingLaunch = false
    }
  }

  function handleLine(line) {
    if (!requestInFlight || processStopHandled || capturedGeneration !== requestGeneration) return
    stdoutFrameCount++
    if (stdoutInvalid || stdoutFrameCount > 1) {
      stdoutInvalid = true
      stagedResponse = null
      return
    }
    var parsed = parseTelemetryLine(line, capturedAddress)
    if (parsed) {
      stagedResponse = parsed
    } else {
      stdoutInvalid = true
      stagedResponse = null
    }
  }

  function handleWatchdogTimeout() {
    if (!requestInFlight && !proc.running) return
    stdoutInvalid = true
    stagedResponse = null
    if (!proc.running) {
      handleProcessStopped(-1)
    } else {
      if (capturedGeneration === requestGeneration) {
        snapshot = null
        state = "unavailable"
        pollTimer.stop()
        cooldownTimer.interval = 30000
        cooldownTimer.restart()
      }
      terminateProcess()
    }
  }

  function handleKillDeadline() {
    if (proc.running) {
      if (proc.processId > 0) {
        try { proc.signal(9) } catch (_) {}
      } else {
        proc.running = false
      }
    }
  }

  function handleDeactivation() {
    requestGeneration++
    consecutiveFailures = 0
    pendingLaunch = false
    snapshot = null
    stagedResponse = null
    state = "idle"
    pollTimer.stop()
    cooldownTimer.stop()
    if (!proc.running && !requestInFlight) {
      watchdogTimer.stop()
      killDeadlineTimer.stop()
    } else {
      terminateProcess()
    }
  }

  function reconcileEligibility() {
    requestGeneration++
    consecutiveFailures = 0
    snapshot = null
    stagedResponse = null
    pollTimer.stop()
    if (!isEligible()) {
      handleDeactivation()
      return
    }
    cooldownTimer.stop()
    if (state === "ready" || state === "stale") {
      state = "idle"
    }
    if (proc.running || requestInFlight) {
      pendingLaunch = true
      terminateProcess()
    } else {
      queuePendingLaunch()
    }
  }

  function refresh() {
    if (!canRefresh) return false
    executeLaunch()
    return true
  }

  function checkFreshness() {
    if (snapshot === null) return
    var now = Date.now()
    if (snapshotAcceptedAt <= 0 || now < snapshotAcceptedAt || (now - snapshotAcceptedAt) >= 180000) {
      snapshot = null
      if (state === "ready") {
        state = "stale"
      }
    }
  }

  function handlePanelOpenChanged() {
    if (!eligible) return
    if (panelOpen) {
      if (state === "ready" || state === "stale") {
        var now = Date.now()
        var elapsed = (now >= lastAttemptWallTime) ? (now - lastAttemptWallTime) : 0
        if (!busy && !cooldownTimer.running && elapsed >= 2000) {
          executeLaunch()
        }
        if (pollTimer.running) {
          pollTimer.interval = 15000
          pollTimer.restart()
        }
      }
    } else {
      if (pollTimer.running && (state === "ready" || state === "stale")) {
        pollTimer.interval = 120000
        pollTimer.restart()
      }
    }
  }

  function cleanup() {
    consecutiveFailures = 0
    requestGeneration++
    requestInFlight = false
    pendingLaunch = false
    watchdogTimer.stop()
    killDeadlineTimer.stop()
    pollTimer.stop()
    freshnessTimer.stop()
    cooldownTimer.stop()
    if (proc.running) {
      if (proc.processId > 0) {
        try { proc.signal(15) } catch (_) {}
      }
      proc.running = false
    }
  }

  onActiveChanged: reconcileEligibility()
  onAddressChanged: reconcileEligibility()
  onDeviceGenerationChanged: reconcileEligibility()
  onPanelOpenChanged: handlePanelOpenChanged()

  Process {
    id: proc
    objectName: "btProcess"
    command: []
    running: false
    stdout: SplitParser {
      onRead: function (line) {
        telemetryRoot.handleLine(line)
      }
    }
    onExited: function (exitCode) {
      telemetryRoot.handleProcessStopped(exitCode)
    }
    onRunningChanged: {
      if (!proc.running && telemetryRoot.requestInFlight && !telemetryRoot.processStopHandled) {
        telemetryRoot.handleProcessStopped(-1)
      }
    }
  }

  Timer {
    id: pollTimer
    objectName: "btPoll"
    interval: 120000
    repeat: false
    onTriggered: {
      if (telemetryRoot.eligible && !telemetryRoot.busy) {
        telemetryRoot.executeLaunch()
      }
    }
  }

  Timer {
    id: watchdogTimer
    objectName: "btWatchdog"
    interval: 15000
    repeat: false
    onTriggered: telemetryRoot.handleWatchdogTimeout()
  }

  Timer {
    id: killDeadlineTimer
    objectName: "btKill"
    interval: 1000
    repeat: false
    onTriggered: telemetryRoot.handleKillDeadline()
  }

  Timer {
    id: cooldownTimer
    objectName: "btCooldown"
    interval: 30000
    repeat: false
  }

  Timer {
    id: freshnessTimer
    objectName: "btFreshness"
    interval: 5000
    repeat: true
    running: telemetryRoot.snapshot !== null
    onTriggered: telemetryRoot.checkFreshness()
  }

  Component.onCompleted: {
    if (eligible) queuePendingLaunch()
  }

  Component.onDestruction: cleanup()
}
