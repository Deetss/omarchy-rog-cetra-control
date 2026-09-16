# Bluetooth vendor telemetry

Date:2026-09-16. Status: implemented and delivered locally; verified on the installed panel. This extends SDD-BLUETOOTH.md for vendor reads; publication is separate.

## Intent and invariants

Show per-earbud/case device-reported battery and current ANC through the selected
Bluetooth audio device when USB control is unavailable. Keep USB commands and
its daemon JSON contract separate. Native microphone mute stays unknown.
No pairing, controller/profile changes, scanning, root, vendor setters or HID
access by the new helper. No inferred power-off or presence from unknown battery.

Verdict: Extend — existing verified SDP/RFCOMM protocol and installed libbluetooth;
use one bounded native C helper with existing Quickshell service ownership.
Native Bluetooth Battery1 lacks these per-device fields and stays a separate
source. No additional Python runtime or package is required.

## Interface and resource contract

- `cetra-bt-read ADDRESS`: exact selected address, SDP SPP lookup for one Asus_APP
  service/channel, three verified getters, one bounded JSON snapshot, then exit.
- JSON success: status/address, nullable left/right/case, mode, nullable reported
  left_charging/right_charging/case_charging. Unknown/out-of-domain battery is null.
- Failures invalidate the transaction; no stale-value fallback. The marked
  right-field transition is91 -> null ->93 while connection stays available.
- One per-user helper owner, no child processes, parent-death teardown,12s total
  helper budget; SDP4s, connection5s, each getter2s under the global budget.
- Service-owned `CetraTelemetry.qml` serializes requests, validates complete
  JSON and rejects late generations/addresses/USB transitions.15s watchdog,
  bounded shutdown, no overlap while an earlier child terminates.
- Automatic read on eligible connection;15s cadence with a panel open,120s
  closed after success. Failure stops automatic retries. Manual refresh is
  rate-limited, including30s cooldown after failure; reconnect permits a new try.
- Report expires after180s or clock rollback; delayed post-suspend completion
  is rejected. No claim of hardware measurement freshness or power savings.
- USB has priority when existing service.connected is true. Otherwise show a
  complete Bluetooth snapshot; never combine fields from USB, vendor and BlueZ.
- Existing three columns show the selected source. Bluetooth ANC is read-only,
  with refresh/status/source and last-reported/case-freshness explanation.
- Shared service owns request and open-view membership. View destruction removes
  its membership. No separate Quickshell instance, daemon, or process per view.

## Verification and delivery

Exercise production native parser/address/framing/timeout behavior offline;
validate null mapping, modes, charging bits and exact allowed getter packets.
Exercise production QML functions and real Qt bindings for single flight,
generation cancellation, failed start, expiry, USB source priority and91/null/93.
Run the repository aggregate tests, manifest validation and diff checks. Inspect
the installed panel and a bounded actual helper read, confirm one USB owner and
valid cache. Preserve unresolved case freshness, native mute, cause of earlier
RFCOMM failure, and unperformed suspend/multimonitor acceptance. Runtime delivery
uses the existing unlocked-shell guard. No commit, release or publication here.

## Acceptance — 2026-09-16

- Full `./tests/run.sh` passed in the isolated candidate; native helper compiled
  with `-O2 -Wall -Wextra -Werror`. Manifest validation and diff whitespace checks
  passed. The additional whole-source frontend case then passed with 25/25
  lifecycle cases; no runtime source changed after the aggregate gate.
- Native production parser tests cover exact getter packets, all ANC mappings,
  power0/100/invalid, right91/null/93, charging masks, split/coalesced frames,
  duplicate/unrelated frames, EOF/EPIPE, frame cap, invalid CLI and reader busy.
- Complete production `CetraTelemetry` loaded in a real QQmlEngine with a C++
  process fixture: coalesced launch, staged JSON, generation/address/USB
  cancellation, reconnect, failed start, TERM/KILL watchdog, expiry and clock
  rollback passed. The fixture does not replace live suspend/resume acceptance.
- Independent Gemini native/QML reviews found mapping, fixture and process
  lifecycle defects. Their corrections were integrated and verified by the
  coordinator; raw worker output was not treated as passing evidence.
- Bounded real helper read succeeded without sudo. Following guarded installation
  and one previously authorized shell restart, the actual Russian panel displayed
  Bluetooth source, left86%, right unknown, last-reported case100%, ANC Off and
  Refresh. USB was absent; one cetra-watch owner, valid USB status JSON and
  continued owner logging were verified. No Cetra QML errors were observed.
- Remaining limits: case measurement freshness/physical charging, actual battery
  cost, original RFCOMM failure cause, live suspend/resume and multimonitor
  acceptance, native mute readback and Bluetooth controls/capture. The system
  Battery1 value remains visibly separate; it is not a vendor fallback.
- This is local delivery of an unreleased development change. No commit, push,
  Marketplace update or publication was performed.

## Compact panel follow-up — 2026-09-16

User requested fewer default labels and a local commit checkpoint. Keep charge,
source, last-reported notice, read-only ANC and Refresh visible. Put route/profile,
LE, system Battery1, case freshness and Bluetooth microphone explanation behind
one Details disclosure, collapsed initially and when USB becomes available.
Device selection remains accessible without an existing selected identity.
The hover tooltip shows only device, connection status and last-reported charge.
This is presentation-only; transport cadence, nullable data and USB command gates
are unchanged. No publication is included in the commit request.
