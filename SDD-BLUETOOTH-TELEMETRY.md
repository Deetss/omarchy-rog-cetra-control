# Bluetooth vendor telemetry

Updated: 2026-09-20. Status: Bluetooth development merged into local main after
Marketplace acceptance of the USB release. Automatic retry follow-up is under
verification at the initial checkpoint; the marked follow-up below now confirms
changing reported battery, all three modes and telemetry-loss recovery.
This extends SDD-BLUETOOTH.md for vendor reads; publication is separate.

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
  closed after success. Failed completed transactions retry after 30/60/120/240/300s,
  capped at300s. No retry starts while the previous child terminates. Success
  resets the delay; eligibility/address/generation changes cancel and reset it.
  Manual refresh is rate-limited, including30s cooldown after failure; an accepted
  refresh replaces the scheduled retry. Panel open/close does not change a failed
  retry deadline. Reconnect permits a new initial attempt.
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
uses the existing unlocked-shell guard. The September 20 request authorizes main
integration and update preparation; the earlier acceptance below is historical.

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

## Automatic retry follow-up — 2026-09-20

- Merged Bluetooth development into main at `3c8cc8d`, preserving the accepted
  DEVELOPMENT.md renames and the saved Bluetooth checkpoint branch.
- JS tests execute production functions and the production timer handler; real
  Qt fixtures cover repeated failures, backoff cap, manual refresh, cancellation,
  success reset and overlap prevention. The aggregate suite and manifest/diff
  checks passed. Independent Gemini source review found no concrete defect;
  its verdict does not establish live timing or hardware correctness.
- USB supplied97/98/89 and Ambient at the unmarked baseline; a standalone vendor
  read failed in discovery while BlueZ reported the audio device connected.
  Neither this read nor source inspection establishes the cause of the earlier
  frozen100%/Battery1 zero/ANC complaints. Marked live acceptance remains open.
- Status copy now describes automatic retry. Other locales retain key parity;
  fluent review of translations remains an existing limitation.
- Guarded setup succeeded and the existing shell was restarted once. The
  installed panel showed USB94/95/89 and Ambient; one cetra-watch process owned
  USB, its status cache was valid JSON and logging continued. No Cetra QML errors
  were found. Bluetooth live recovery and marked ANC acceptance are still pending.

## Marked follow-up — September 20, 15:09

All three user-reported ANC prompts matched the Bluetooth-only installed panel.
Earbud reports changed 89/88 to 87/87. A closed-case trial cleared data and a later
automatic attempt recovered 87/89/95 without Refresh. The main BlueZ connection
flag stayed true in the samples; this proves telemetry recovery for that trial,
not a full link reconnect or every failure mode. See ACCEPTANCE-2026-09-20.md.
