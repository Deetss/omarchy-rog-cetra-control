// Unit tests for CetraTelemetry.qml and CetraService.qml lifecycle
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const path = require('node:path');

let read;
let extractFunctions;
try {
  const qmlSource = require('./qml-source.js');
  if (typeof qmlSource.read === 'function') read = qmlSource.read;
  if (typeof qmlSource.extractFunctions === 'function') extractFunctions = qmlSource.extractFunctions;
} catch (_) {}
if (!read) {
  read = file => fs.readFileSync(path.resolve(__dirname, '..', file), 'utf8');
}

function extractBalancedFunctions(source) {
  const fns = {};
  const regex = /function\s+(\w+)\s*\(([^)]*)\)\s*\{/g;
  let match;
  while ((match = regex.exec(source)) !== null) {
    const name = match[1];
    const startIndex = match.index;
    let braceCount = 1;
    let i = regex.lastIndex;
    while (i < source.length && braceCount > 0) {
      const ch = source[i];
      if (ch === '{') braceCount++;
      else if (ch === '}') braceCount--;
      i++;
    }
    if (braceCount === 0) {
      fns[name] = source.slice(startIndex, i);
    }
  }
  return fns;
}

function normalizeJson(val) {
  if (val === undefined) return undefined;
  return JSON.parse(JSON.stringify(val));
}

const telemetrySource = read('CetraTelemetry.qml');
const telemetryFns = (extractFunctions ? extractFunctions(telemetrySource) : null) || extractBalancedFunctions(telemetrySource);

const requiredFns = [
  'normalizeAddress',
  'isValidBattery',
  'validateSnapshot',
  'parseTelemetryLine',
  'isEligible',
  'executeLaunch',
  'queuePendingLaunch',
  'drainPendingLaunch',
  'requestLaunch',
  'terminateProcess',
  'handleProcessStopped',
  'handleLine',
  'handleWatchdogTimeout',
  'handleKillDeadline',
  'handleDeactivation',
  'reconcileEligibility',
  'refresh',
  'checkFreshness',
  'handlePanelOpenChanged',
  'cleanup'
];
for (const fnName of requiredFns) {
  assert.ok(telemetryFns[fnName], `Function ${fnName} must be extracted`);
}

function createTelemetryContext(initial = {}) {
  let pendingCallbacks = [];
  const ctx = vm.createContext({
    active: false,
    address: '',
    deviceGeneration: 0,
    panelOpen: false,
    snapshot: null,
    state: 'idle',
    helperCommand: '/test/bin/cetra-bt-read',
    requestGeneration: 0,
    capturedGeneration: -1,
    capturedAddress: '',
    launchWallTime: 0,
    lastAttemptWallTime: 0,
    snapshotAcceptedAt: 0,
    stagedResponse: null,
    stdoutFrameCount: 0,
    stdoutInvalid: false,
    lastExitCode: -1,
    processStopHandled: true,
    requestInFlight: false,
    pendingLaunch: false,
    proc: {
      running: false,
      processId: 0,
      command: [],
      signals: [],
      signal(s) {
        this.signals.push(s);
      }
    },
    pollTimer: {
      interval: 120000,
      running: false,
      restart() { this.running = true; },
      stop() { this.running = false; }
    },
    watchdogTimer: {
      interval: 15000,
      running: false,
      restart() { this.running = true; },
      stop() { this.running = false; }
    },
    killDeadlineTimer: {
      interval: 1000,
      running: false,
      restart() { this.running = true; },
      stop() { this.running = false; }
    },
    cooldownTimer: {
      interval: 30000,
      running: false,
      restart() { this.running = true; },
      stop() { this.running = false; }
    },
    freshnessTimer: {
      interval: 1000,
      running: false,
      restart() { this.running = true; },
      stop() { this.running = false; }
    },
    Qt: {
      callLater(cb) {
        pendingCallbacks.push(cb);
      }
    },
    flushCallLater() {
      const cbs = pendingCallbacks.slice();
      pendingCallbacks = [];
      for (const cb of cbs) cb();
    },
    pendingCallCount() {
      return pendingCallbacks.length;
    },
    Date,
    JSON,
    Math,
    isFinite,
    ...initial
  });

  ctx.telemetryRoot = ctx;
  Object.defineProperty(ctx, 'normalizedAddress', { get() { return ctx.normalizeAddress(ctx.address); } });
  Object.defineProperty(ctx, 'eligible', { get() { return ctx.isEligible(); } });
  Object.defineProperty(ctx, 'busy', { get() { return ctx.requestInFlight || ctx.proc.running || ctx.pendingLaunch; } });
  Object.defineProperty(ctx, 'canRefresh', {
    get() { return ctx.isEligible() && !ctx.cooldownTimer.running && !ctx.proc.running && !ctx.requestInFlight && !ctx.pendingLaunch; }
  });

  for (const code of Object.values(telemetryFns)) {
    vm.runInContext(code, ctx);
  }
  return ctx;
}

// 1. Address normalization
const ctx = createTelemetryContext();
assert.equal(ctx.normalizeAddress('AA:BB:CC:DD:EE:FF'), 'AA:BB:CC:DD:EE:FF');
assert.equal(ctx.normalizeAddress('aa:bb:cc:dd:ee:ff'), 'AA:BB:CC:DD:EE:FF');
assert.equal(ctx.normalizeAddress('  aa:bb:cc:dd:ee:ff  '), 'AA:BB:CC:DD:EE:FF');
assert.equal(ctx.normalizeAddress(''), '');
assert.equal(ctx.normalizeAddress('AA:BB:CC:DD:EE'), '');
assert.equal(ctx.normalizeAddress('GG:BB:CC:DD:EE:FF'), '');
assert.equal(ctx.normalizeAddress('AABBCCDDEEFF'), '');
assert.equal(ctx.normalizeAddress(null), '');
assert.equal(ctx.normalizeAddress(12345), '');

// 2. Battery domain validation
assert.equal(ctx.isValidBattery(0), true);
assert.equal(ctx.isValidBattery(100), true);
assert.equal(ctx.isValidBattery(45), true);
assert.equal(ctx.isValidBattery(null), true);
assert.equal(ctx.isValidBattery(-1), false);
assert.equal(ctx.isValidBattery(101), false);
assert.equal(ctx.isValidBattery(50.5), false);
assert.equal(ctx.isValidBattery('50'), false);
assert.equal(ctx.isValidBattery(NaN), false);
assert.equal(ctx.isValidBattery(Infinity), false);
assert.equal(ctx.isValidBattery(undefined), false);

// 3. Snapshot schema validation
const expectedAddr = 'AA:BB:CC:DD:EE:FF';
const validRaw = {
  status: 'ok',
  address: expectedAddr,
  left: 89,
  right: null,
  case: 100,
  mode: 'off',
  left_charging: false,
  right_charging: true,
  case_charging: true
};
assert.deepEqual(normalizeJson(ctx.validateSnapshot(validRaw, expectedAddr)), validRaw);
assert.equal(ctx.validateSnapshot({ ...validRaw, address: '11:22:33:44:55:66' }, expectedAddr), null);
assert.equal(ctx.validateSnapshot({ ...validRaw, status: 'unavailable' }, expectedAddr), null);
assert.equal(ctx.validateSnapshot({ ...validRaw, mode: 'invalid-mode' }, expectedAddr), null);
assert.equal(ctx.validateSnapshot({ ...validRaw, left: 150 }, expectedAddr), null);
assert.equal(ctx.validateSnapshot({ ...validRaw, left_charging: true, right_charging: null }, expectedAddr), null);
assert.equal(ctx.validateSnapshot({ ...validRaw, left_charging: null, right_charging: null }, expectedAddr).left_charging, null);
assert.equal(ctx.validateSnapshot({ ...validRaw, case_charging: 'yes' }, expectedAddr), null);

// 4. Bounded parser <= 1024 bytes
assert.notEqual(ctx.parseTelemetryLine(JSON.stringify(validRaw), expectedAddr), null);
assert.equal(ctx.parseTelemetryLine('', expectedAddr), null);
assert.equal(ctx.parseTelemetryLine('not-json', expectedAddr), null);
assert.equal(ctx.parseTelemetryLine('x'.repeat(1025), expectedAddr), null);

// 5. Single flight eligible coalesced startup & pending membership prevents double launch
const startCtx = createTelemetryContext({ active: true, address: expectedAddr });
startCtx.queuePendingLaunch();
assert.equal(startCtx.pendingLaunch, true);
assert.equal(startCtx.pendingCallCount(), 1);
// Repeated calls while pending must not enqueue duplicate callbacks
startCtx.queuePendingLaunch();
startCtx.reconcileEligibility();
assert.equal(startCtx.pendingCallCount(), 1, 'Only one callLater scheduled during startup coalescing');
assert.equal(startCtx.proc.running, false);
// Drain callLater queue executes exactly once
startCtx.flushCallLater();
assert.equal(startCtx.pendingLaunch, false);
assert.equal(startCtx.proc.running, true);
assert.equal(startCtx.requestInFlight, true);
assert.equal(startCtx.state, 'loading');
assert.equal(startCtx.capturedAddress, expectedAddr);

// 6. Native ordering: stdout, onExited(0), then runningChanged(false) -> success
startCtx.handleLine(JSON.stringify(validRaw));
assert.notEqual(startCtx.stagedResponse, null);
startCtx.handleProcessStopped(0);
assert.equal(startCtx.state, 'ready');
assert.deepEqual(normalizeJson(startCtx.snapshot), validRaw);
assert.equal(startCtx.requestInFlight, false);
assert.equal(startCtx.cooldownTimer.running, true);
assert.equal(startCtx.cooldownTimer.interval, 2000);
startCtx.proc.running = false;
// runningChanged false after already handled does not alter success
assert.equal(startCtx.state, 'ready');

// 7. Native exit sequence: onExited(0) delivers completion, subsequent runningChanged(false) does not alter success
const exitSeqCtx = createTelemetryContext({ active: true, address: expectedAddr });
exitSeqCtx.executeLaunch();
exitSeqCtx.handleLine(JSON.stringify(validRaw));
// Native Quickshell emits exited first, then runningChanged
exitSeqCtx.handleProcessStopped(0);
assert.equal(exitSeqCtx.state, 'ready');
assert.deepEqual(normalizeJson(exitSeqCtx.snapshot), validRaw);
exitSeqCtx.proc.running = false;
if (!exitSeqCtx.proc.running && exitSeqCtx.requestInFlight && !exitSeqCtx.processStopHandled) {
  exitSeqCtx.handleProcessStopped(-1);
}
assert.equal(exitSeqCtx.state, 'ready');

// 8. Failed start: binary cannot be started (proc.running remains false, runningChanged triggers failure)
const failCtx = createTelemetryContext({ active: true, address: expectedAddr });
Object.defineProperty(failCtx.proc, 'running', { get() { return false; }, set(_) {} });
failCtx.executeLaunch();
assert.equal(failCtx.requestInFlight, true);
assert.equal(failCtx.state, 'loading');
// Quickshell onErrorOccurred(FailedToStart) emits runningChanged(false) without exited signal
if (!failCtx.proc.running && failCtx.requestInFlight && !failCtx.processStopHandled) {
  failCtx.handleProcessStopped(-1);
}
assert.equal(failCtx.state, 'unavailable');
assert.equal(failCtx.snapshot, null);
assert.equal(failCtx.requestInFlight, false);
assert.equal(failCtx.cooldownTimer.running, true);
assert.equal(failCtx.cooldownTimer.interval, 30000);

// 9. Failed start via asynchronous runningChanged false without onExited
const asyncFailCtx = createTelemetryContext({ active: true, address: expectedAddr });
asyncFailCtx.executeLaunch();
assert.equal(asyncFailCtx.proc.running, true);
assert.equal(asyncFailCtx.requestInFlight, true);
// Quickshell emits runningChanged(false) on FailedToStart without exited signal
asyncFailCtx.proc.running = false;
if (!asyncFailCtx.proc.running && asyncFailCtx.requestInFlight && !asyncFailCtx.processStopHandled) {
  asyncFailCtx.handleProcessStopped(-1);
}
assert.equal(asyncFailCtx.state, 'unavailable');
assert.equal(asyncFailCtx.snapshot, null);
assert.equal(asyncFailCtx.requestInFlight, false);
assert.equal(asyncFailCtx.cooldownTimer.interval, 30000);

// 10. Watchdog with no events: handles requestInFlight when proc.running is false
const noEventCtx = createTelemetryContext({ active: true, address: expectedAddr });
noEventCtx.requestInFlight = true;
noEventCtx.capturedGeneration = noEventCtx.requestGeneration;
noEventCtx.processStopHandled = false;
noEventCtx.proc.running = false;
noEventCtx.handleWatchdogTimeout();
assert.equal(noEventCtx.state, 'unavailable');
assert.equal(noEventCtx.snapshot, null);
assert.equal(noEventCtx.requestInFlight, false);
assert.equal(noEventCtx.cooldownTimer.interval, 30000);

// 11. Watchdog when process is running: sends SIGTERM, kill deadline sends SIGKILL, prevents late exit 0 accept
const hangCtx = createTelemetryContext({ active: true, address: expectedAddr });
hangCtx.executeLaunch();
hangCtx.proc.processId = 4444;
hangCtx.handleLine(JSON.stringify(validRaw));
assert.equal(hangCtx.busy, true);

hangCtx.handleWatchdogTimeout();
assert.equal(hangCtx.state, 'unavailable');
assert.equal(hangCtx.snapshot, null);
assert.equal(hangCtx.stdoutInvalid, true, 'Watchdog marks stdout invalid');
assert.equal(hangCtx.stagedResponse, null, 'Watchdog discards staged response');
assert.equal(hangCtx.proc.signals.includes(15), true, 'Watchdog sends SIGTERM');
assert.equal(hangCtx.busy, true, 'Busy until process actually exits');

hangCtx.handleKillDeadline();
assert.equal(hangCtx.proc.signals.includes(9), true, 'Kill deadline sends SIGKILL');

// Native process exits with code 0 later; must not accept snapshot
hangCtx.proc.running = false;
hangCtx.handleProcessStopped(0);
assert.equal(hangCtx.state, 'unavailable', 'Late exit 0 must be rejected after watchdog invalidation');
assert.equal(hangCtx.snapshot, null);
assert.equal(hangCtx.busy, false);

// 12. Sticky bad stdout: malformed followed by valid, duplicate frames, 3 frames
const stickyCtx = createTelemetryContext({ active: true, address: expectedAddr });
stickyCtx.executeLaunch();
stickyCtx.handleLine('bad json');
assert.equal(stickyCtx.stdoutInvalid, true);
stickyCtx.handleLine(JSON.stringify(validRaw));
assert.equal(stickyCtx.stagedResponse, null, 'Malformed followed by valid rejected');
stickyCtx.proc.running = false;
stickyCtx.handleProcessStopped(0);
assert.equal(stickyCtx.snapshot, null);
assert.equal(stickyCtx.state, 'unavailable');

// Duplicate frame
stickyCtx.executeLaunch();
stickyCtx.handleLine(JSON.stringify(validRaw));
assert.notEqual(stickyCtx.stagedResponse, null);
stickyCtx.handleLine(JSON.stringify(validRaw));
assert.equal(stickyCtx.stdoutInvalid, true);
assert.equal(stickyCtx.stagedResponse, null, 'Duplicate frame rejected');
stickyCtx.proc.running = false;
stickyCtx.handleProcessStopped(0);
assert.equal(stickyCtx.snapshot, null);

// 13. Cancellation on address change: clears snapshot immediately, old reaped, new queued via drain
const addrCtx = createTelemetryContext({ active: true, address: expectedAddr });
addrCtx.snapshot = validRaw;
addrCtx.executeLaunch();
addrCtx.proc.processId = 8888;

addrCtx.address = '11:22:33:44:55:66';
addrCtx.reconcileEligibility();
assert.equal(addrCtx.snapshot, null, 'Snapshot cleared immediately on address change');
assert.equal(addrCtx.pollTimer.running, false, 'Poll timer stopped on address change');
assert.equal(addrCtx.pendingLaunch, true);
assert.equal(addrCtx.proc.signals.includes(15), true, 'Old process terminated');

// Late stdout and exit from old generation
addrCtx.handleLine(JSON.stringify(validRaw));
assert.equal(addrCtx.stagedResponse, null, 'Late stdout from old generation ignored');
addrCtx.proc.running = false;
addrCtx.handleProcessStopped(0);
assert.equal(addrCtx.snapshot, null);

// Draining executes launch for new address
addrCtx.flushCallLater();
assert.equal(addrCtx.proc.running, true);
assert.equal(addrCtx.capturedAddress, '11:22:33:44:55:66');

// 14. Latched failure: panel open does NOT auto-retry; only explicit refresh retries
const latchCtx = createTelemetryContext({ active: true, address: expectedAddr });
latchCtx.executeLaunch();
latchCtx.proc.running = false;
latchCtx.handleProcessStopped(1);
assert.equal(latchCtx.state, 'unavailable');
assert.equal(latchCtx.snapshot, null);
assert.equal(latchCtx.cooldownTimer.running, true);
assert.equal(latchCtx.canRefresh, false);

// Opening panel while unavailable does not retry
latchCtx.panelOpen = true;
latchCtx.handlePanelOpenChanged();
assert.equal(latchCtx.proc.running, false);
assert.equal(latchCtx.state, 'unavailable');

// 30s cooldown finishes
latchCtx.cooldownTimer.running = false;
assert.equal(latchCtx.canRefresh, true);
latchCtx.handlePanelOpenChanged();
assert.equal(latchCtx.proc.running, false, 'Panel open does not auto-retry unavailable state');

// Explicit refresh succeeds
assert.equal(latchCtx.refresh(), true);
assert.equal(latchCtx.proc.running, true);
assert.equal(latchCtx.state, 'loading');

// 15. Freshness timer: 180s expiry and clock rollback
const freshCtx = createTelemetryContext();
const baseTime = 1700000000000;
freshCtx.snapshotAcceptedAt = baseTime;
freshCtx.snapshot = validRaw;
freshCtx.state = 'ready';

freshCtx.Date = { now: () => baseTime + 179000 };
freshCtx.checkFreshness();
assert.notEqual(freshCtx.snapshot, null);
assert.equal(freshCtx.state, 'ready');

freshCtx.Date = { now: () => baseTime + 180000 };
freshCtx.checkFreshness();
assert.equal(freshCtx.snapshot, null);
assert.equal(freshCtx.state, 'stale');

freshCtx.snapshotAcceptedAt = baseTime;
freshCtx.snapshot = validRaw;
freshCtx.state = 'ready';
freshCtx.Date = { now: () => baseTime - 1000 };
freshCtx.checkFreshness();
assert.equal(freshCtx.snapshot, null);
assert.equal(freshCtx.state, 'stale');

// 16. Wall clock > 15s or backwards clock rejects exit 0
const clockCtx = createTelemetryContext({ active: true, address: expectedAddr });
clockCtx.executeLaunch();
clockCtx.handleLine(JSON.stringify(validRaw));
clockCtx.proc.running = false;
clockCtx.Date = { now: () => clockCtx.launchWallTime + 15001 };
clockCtx.handleProcessStopped(0);
assert.equal(clockCtx.state, 'unavailable');
assert.equal(clockCtx.snapshot, null);

// 17. CetraService panel open token integration and max 16 bounds
const serviceSource = read('CetraService.qml');
const serviceFns = (extractFunctions ? extractFunctions(serviceSource) : null) || extractBalancedFunctions(serviceSource);
assert.ok(serviceFns.setBluetoothPanelOpen, 'setBluetoothPanelOpen must be extracted from CetraService.qml');

const svcCtx = vm.createContext({
  bluetoothPanelTokens: [],
  setBluetoothPanelOpen: null
});
Object.defineProperty(svcCtx, 'bluetoothPanelOpen', {
  get() { return svcCtx.bluetoothPanelTokens.length > 0; }
});
vm.runInContext(serviceFns.setBluetoothPanelOpen, svcCtx);

const t1 = { id: 1 }, t2 = { id: 2 };
svcCtx.setBluetoothPanelOpen(t1, true);
assert.equal(svcCtx.bluetoothPanelOpen, true);
assert.equal(svcCtx.bluetoothPanelTokens.length, 1);

svcCtx.setBluetoothPanelOpen(t2, true);
assert.equal(svcCtx.bluetoothPanelOpen, true);
assert.equal(svcCtx.bluetoothPanelTokens.length, 2);

svcCtx.setBluetoothPanelOpen(t1, false);
assert.equal(svcCtx.bluetoothPanelOpen, true);
assert.equal(svcCtx.bluetoothPanelTokens.length, 1);

svcCtx.setBluetoothPanelOpen(t2, false);
assert.equal(svcCtx.bluetoothPanelOpen, false);
assert.equal(svcCtx.bluetoothPanelTokens.length, 0);

for (let i = 0; i < 20; i++) {
  svcCtx.setBluetoothPanelOpen({ id: i }, true);
}
assert.equal(svcCtx.bluetoothPanelTokens.length, 16);

// Deduplication of identical view token
const dupToken = { id: 'dup' };
svcCtx.bluetoothPanelTokens = [];
svcCtx.setBluetoothPanelOpen(dupToken, true);
svcCtx.setBluetoothPanelOpen(dupToken, true);
assert.equal(svcCtx.bluetoothPanelTokens.length, 1, 'Duplicate view token must not append redundant entries');

// View destruction unregisters view token cleanly
svcCtx.setBluetoothPanelOpen(dupToken, false);
assert.equal(svcCtx.bluetoothPanelOpen, false);
assert.equal(svcCtx.bluetoothPanelTokens.length, 0);

console.log('PASS CetraTelemetry lifecycle, explicit error/exit handling, latching, single-timer rate limiting, and bounds');
