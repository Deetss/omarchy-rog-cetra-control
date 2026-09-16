# Bluetooth observation acceptance — 2026-09-16

## Scope and source

Uncommitted first-stage implementation based on `7afe8ba0a8e562bded586a1c8a81a16c077a261d`.
Prepared in `/tmp/opencode/cetra-bluetooth` on `codex/bluetooth-status`, delivered
to `/home/slovn/.config/omarchy/plugins/slovn.cetra`. No commit, tag, binary
replacement, Marketplace change or publication was performed.

Host: Quickshell 0.3.1 (Arch), Qt 6.11.2, PipeWire 1.6.8. Native Bluetooth model
and PipeWire node properties were checked against installed qmltypes and read-only
host observations. These APIs lack model/UUID association and a verified monotonic
battery report timestamp; SDD records the explicit-selection/unknown adaptations.

## Automated evidence

- `./tests/run.sh`: exit 0 on the complete implementation. All native selftests,
  source ownership, Qt lint, settings/lifecycle, microphone, topology, translation
  and lighting suites passed.
- `node tests/bluetooth.js`: exit 0, including subsequent diagnostic ring and
  candidate-selection cases. Tests production functions, not duplicated logic.
- `python3 -B tests/bluetooth/run.py`: included in the aggregate; complete production
  observers instantiated with reactive native-model fixtures. Selection, cached
  versus changed battery, disconnect/reconnect, rename, removal, old-object events,
  adapter loss, recovery, service teardown and USB observation gate passed.
  This exercises real Qt bindings, not real Bluetooth hardware.
- New graph cases cover actual processed playback/capture, mixed USB/Bluetooth,
  foreign sinks, unknown cycles/dangling edges and profile metadata conflicts.
- Isolated regression comparison: baseline reports USB communication `active` for
  USB plus foreign Bluetooth mixed into one recorder; current code reports
  `unknown`, so it does not initiate a new USB call from that ambiguous evidence.
- `omarchy plugin validate .`: exit 0 in prepared and installed trees.
- `git diff --check`: exit 0 in prepared and installed trees.
- All 10 locale catalogs have complete new messages and matching placeholders.
  No fluent-human review of the non-Russian/English translations is claimed.

Logs and snapshot manifests are local under `/tmp/opencode/cetra-bluetooth-tests.log`
and `/tmp/opencode/cetra-bt-delivered-hashes.json`; final hashes are recorded in
`/tmp/opencode/cetra-bt-final-hashes.json` after delivery checks.

## Independent review

A bounded read-only review used the fixed OpenCode route
`ninitux/gemini-3.8-flash-high`. The initial review incorrectly assumed missing
Quickshell APIs and omitted the battery section from its UI assessment. Those
claims were rejected against installed qmltypes and BatterySection. A focused
follow-up with the installed metadata reported no concrete findings in identity,
battery generation, topology and command isolation. This is code review, not
hardware acceptance. The later HFP loopback fix was self-reviewed against the
live node properties and covered by new JS/Qt regression cases. Local responses: `/tmp/opencode/cetra-bt-review.json` and
`/tmp/opencode/cetra-bt-review-followup.json`.

## Live evidence and remaining checks

The screen-lock guard was explicitly clear before source delivery and restart.
Hot reload retained the old visible QML despite file-change logs. The user then
explicitly authorized `omarchy-restart-shell`; it returned exit 0. The new Russian
panel rendered with transport rows, actual USB playback/other capture observations
and the main Bluetooth audio candidate. Text wraps within the 380-unit panel;
USB battery and ANC controls remained visible. No Cetra QML load errors were
observed in the post-restart journal. The shell stayed responsive.

`pgrep -a cetra-watch` showed one owner; the status cache was valid JSON with
`microphone_state: "unknown"`. Telemetry continued after reload and restart.
The panel did not select the audio device automatically. LE charge was absent
from the rendered values. Native helper binaries were unchanged.

The user selected the main audio record and removed USB at 01:04:42 local time.
The panel remained open (user confirmation plus screenshot and host geometry),
USB columns cleared, ANC/settings controls disappeared, native mute stayed
unknown and a separate cached Bluetooth report showed 59%. The legacy LE 71%
was not used. The selected address persisted across the next authorized restart.

Live HFP exposed PipeWire's public loopback source without an address property.
The first implementation incorrectly showed an unknown profile when its HFP
microphone endpoint was not recognized. This was corrected using an unambiguous
node `device.id` relation, adding production JS and complete Qt binding cases.
The full test suite then passed again. After delivery/restart, the visible panel
correctly said a headset profile and available microphone endpoint. The actual
playback/capture routes remained on other devices, matching the host's current
configuration; the plugin did not reroute them. Bluetooth recording quality and
mute behavior were not tested by this observation.

The user then reconnected USB and confirmed that charge and controls returned,
and switching ANC produced exactly one native prompt. This verifies panel persistence after USB loss while BlueZ reports the selected
Bluetooth connection, and restoration of USB controls. The user clarified that
the earbuds were not physically switched/reconnected and no Bluetooth listening
test was performed at that point. Actual audio handoff was not verified in that
trial: the observed playback and capture routes were other devices. Nor does this prove the
historical intermittent duplicate prompt has been fixed.

### Subsequent audible Bluetooth playback trial

The user held the left earbud to initiate discovery, reported no immediate
connection, placed the earbuds in the case and closed it, then observed the case
indicator change from red to blue. After taking the earbuds out, the user reported
a Bluetooth connection and audible playback through the earbuds. This sequence
is a user observation, not a verified pairing procedure or interpretation of the
case indicator; causality and reproducibility have not been established.

The concurrent read-only host snapshot showed the USB receiver absent, a native
Bluetooth Audio/Sink with `api.bluez5.profile = a2dp-sink`, and two active channel
links from `ee_soe_output_level` to that sink. Together with the user's listening
confirmation, this establishes audible Bluetooth playback for this trial. It does
not establish automatic USB-to-Bluetooth switching or Bluetooth microphone capture.
No Bluetooth microphone source appeared in that A2DP snapshot. No connection,
profile or audio-route operation was issued by the plugin or assistant.

Remaining device acceptance: full live keyboard focus; marked charge/discharge;
actual HFP microphone capture.
The earlier single-voice tests do not establish a duplicate-prompt fix or a power
improvement. English full-panel rendering, full accessibility, suspend/resume and
multimonitor acceptance remain open. No unsupported native mute or case/LE
association is claimed.
