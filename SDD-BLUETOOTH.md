# Bluetooth support: first implementation stage

Date: 2026-09-16. Status: implemented; automated checks passed, panel persistence and USB control recovery verified; audible A2DP playback verified after user-operated reconnection; automatic audio handoff untested; remaining checks recorded in BLUETOOTH-ACCEPTANCE.md.
Baseline: `7afe8ba0a8e562bded586a1c8a81a16c077a261d` (1.7.0).
Scope authorized in this task: implement the first observation-only stage.
Publication is separate work. BACKLOG.md remains the release-status owner.

## 1. Intent and invariants

### Outcome

Keep the Cetra panel useful when the headset is connected through Bluetooth and
the USB receiver is absent. Show connection types, observed audio routes and
battery provenance without implying that Bluetooth provides USB controls.

The first stage is observation-only for Bluetooth. It must work inside the
existing `bar-widget` and shared `service` manifest contracts.

### Evidence from the 2026-09-15/16 session

| Observation | Interpretation and limit |
| --- | --- |
| `LE-ROG CTWSN` exposes GATT services and BlueZ Battery1 reports 71% across discharge/recharge observations | This value is not established as current earbud or case charge. The physical owner of the LE interface remains unresolved. |
| A distinct BlueZ record named `ROG CETRA TWS SN` exposes audio profiles; A2DP/AAC playback worked | Bluetooth audio support is confirmed on this host. Connection alone does not prove an active audio route. |
| Main Bluetooth Battery1 changed to 62% while USB reported left 62%, right 66%, case 18% | Agreement with the left side is one observation, not a confirmed left-side mapping. |
| Direct standard GATT battery read timed out after eight seconds on the LE record; the main record returned one byte, zero | Direct GATT reads are not an accepted battery source for this stage. Zero is still a valid battery-domain value for a verified source. |
| With A2DP active, Cetra exposed a playback sink and no microphone source; HFP was offered by PipeWire | Explain the current profile. Availability of HFP does not establish tested microphone capture. |
| Removing USB hid the panel while `cetra-watch` remained running | Existing visibility behavior, not evidence of a plugin crash. |
| User heard one ANC prompt with Bluetooth alone and with USB plus Bluetooth | The earlier duplicate native prompt did not reproduce. Do not mark it fixed. |
| USB mode reports briefly alternated despite a single audible prompt | Report transitions are not a count of audible prompts or physical mode changes. |
| User perceived faster discharge after adding Bluetooth | No controlled energy comparison was performed. Cause and magnitude remain unknown. |

These are session observations, not automated acceptance of the future code.
Relevant primary interfaces: [Quickshell BluetoothDevice](https://quickshell.org/docs/v0.2.1/types/Quickshell.Bluetooth/BluetoothDevice/),
[BlueZ Battery API](https://bluez.readthedocs.io/en/latest/battery-api/),
[BlueZ GATT API](https://bluez.readthedocs.io/en/latest/gatt-api/).
Verify APIs against the installed host before implementation; the linked
Quickshell reference is versioned and does not establish the installed contract.

### In scope

- Observe an identified Cetra audio device and any separately identified LE record.
- Preserve the panel across USB loss while the identified Bluetooth audio device
  remains connected.
- Explain USB control availability, Bluetooth profile and actual audio routes.
- Keep USB battery columns and a separately labelled Bluetooth battery observation.
- Handle unavailable BlueZ, ambiguous identity, stale data and reconnects.
- Add targeted diagnostics and tests for these behaviors.

### Invariants

- `cetra-watch` remains the sole HID owner. Its commands, socket and JSON contract
  retain their USB meanings; Bluetooth must not set daemon `receiver` or `connected`.
- Existing USB presence, freshness, settings confirmation and lighting rules remain.
- Native microphone mute stays `unknown`. Neither taps, software mute, profile,
  signal level nor missing capture establishes native mute.
- No Bluetooth command writes, pairing, discovery scans, automatic connections,
  profile switches, audio-route changes or direct GATT reads in this stage.
- No changes to Bluetooth controller intervals, power management, mouse settings,
  other plugins, EasyEffects or WirePlumber policy.
- No new capture stream for Bluetooth. Existing USB peak capture keeps its opt-in,
  source identity, endpoint admission and teardown rules.
- No second Quickshell process, new persistent daemon or periodic CLI polling.
- Preserve theme tokens, locale catalogs, keyboard behavior and lockscreen guards.

### Deferred stages

1. Explicit A2DP/HFP selection and Bluetooth microphone metering, after route,
   quality, source identity and teardown behavior are specified and tested.
2. Verified Bluetooth ANC, lighting and voice controls, based on official-client
   captures and existing ASUS HAL research. Never copy USB bytes to an unknown
   Bluetooth characteristic or fuzz commands.
3. Controlled USB versus USB-plus-Bluetooth discharge measurements with matching
   starting charge, workload, volume, ANC, lighting and microphone use. Keep
   unrelated BLE and controller settings fixed; log connection interruptions.
4. Duplicate-prompt investigation if a marked reproduction becomes available.

Develop subsequent code separately from the submitted 1.7.0 snapshot. Do not move
tags or change the Marketplace submission as part of this specification.

## 2. Interface and data contract

### Ownership and integration

| Owner | Planned responsibility |
| --- | --- |
| New `CetraBluetooth.qml`, owned once by `CetraService.qml` | Observe BlueZ device identity, connection and Battery1 via the installed native Quickshell Bluetooth API. No device operations. |
| `AudioTopology.qml` | Add a separate read-only projection of Cetra playback/capture routes and Bluetooth profile. Preserve existing USB call/meter admission. |
| `CetraService.qml` | Combine observations and capabilities; invalidate detached generations. USB failures clear USB state without deleting valid Bluetooth state. |
| `CetraViewModel.qml` | Project labels, visibility, battery provenance and capability gates. No processes. |
| `Cetra.qml`, battery/microphone sections; optional small connection section | Render the projection and retain focus when connections change. |
| `CetraPreferences.qml`, manifest schema | Persist an optional selected Bluetooth audio identity through the existing settings path. |

Use installed native Bluetooth and PipeWire events first. If they cannot expose
an essential field, return `unknown` and document the gap. Adding a new helper or
dependency requires a revision of this specification, not a silent fallback.

### Identity and connections

The following are proposed internal QML contracts, not new daemon JSON fields.

- `bluetoothAvailability`: `ready | unavailable | error`.
- `audioIdentity`: `selected | unique | ambiguous | missing`.
- `usbReceiver`: existing daemon receiver observation.
- `bluetoothAudioConnected`, `bluetoothLeConnected`: nullable booleans;
  null means unavailable evidence, false means observed disconnection.
- `canControlUsb`: the existing USB readiness/presence predicate. Never derive it
  from Bluetooth connection or battery.
- `panelAvailable`: USB receiver present OR identified Bluetooth audio connected.
  A service-only LE connection does not prove the earbuds are available.

Installed-API adaptation: Quickshell 0.3.1 does not expose model IDs, advertised
UUIDs or PipeWire device/card objects. Require an observed native Bluetooth audio
node and manual address selection instead of automatic model matching. Names
are display hints, not sufficient identity. Do not
hardcode this user's MAC addresses. Persist a user-selected audio identity only
when needed; never automatically choose the first of several matching devices.
LE association needs an explicit identity relationship; matching names or service
UUIDs alone must not merge two physical headsets. Unknown association stays unknown.

Preserve `hideWhenReceiverMissing` as a legacy fallback. Add optional
`hideWhenDisconnected`; its effective value is the new key when set, otherwise
the legacy value (default true). It applies to `panelAvailable`. Existing error
visibility remains; show ambiguous-device selection when connected candidates
exist. Do not overwrite saved preferences during migration.

When USB disappears but Bluetooth audio remains, keep the open panel and useful
controls. Move focus to a surviving control when the focused USB action becomes
unavailable. Gate mouse, wheel, keyboard and direct service calls with USB
capabilities, not only a disabled visual button.

### Battery observations

Each source owns its value; selection never overwrites another source's history:

```text
percent: integer 0..100 | null
source: usb-left | usb-right | usb-case | bluez-audio | bluez-le
quality: usb-fresh | system-reported | unverified | stale | unavailable
observedAt: local monotonic time | null
hardwareReportedAt: local monotonic time | null
generation: current source connection generation
```

`observedAt` would mean this process received a property snapshot/event. The
installed native API has no verified monotonic event clock: this implementation
leaves it null and records `cached` versus `property-update` evidence instead. It is not
hardware freshness. `hardwareReportedAt` remains null when the API cannot prove
a new device report. Re-reading a cache, opening a panel, or a repeating timer
must not refresh hardware evidence. An unchanged percentage alone proves neither
staleness nor freshness. Do not apply the USB 30-second policy to sparse Bluetooth
Battery1 updates without a verified reporting cadence.

- Fresh USB columns retain priority and existing expiry rules.
- The selected audio record's valid Battery1 percentage may appear separately as
  “Bluetooth reported charge”, with freshness unknown and side unspecified.
  A cached initial snapshot must be distinguishable from a later observed update.
- Bluetooth-only mode leaves left/right/case unknown; it never copies the single
  percentage into them. The bar percentage and low-battery warning continue to use
  fresh USB earbud values in this stage.
- LE Battery1 and direct GATT values are unverified diagnostic evidence. Do not
  use them in battery columns, bar percentage or warnings. Do not special-case 71
  or globally reject zero; acceptance is source-based, not value-based.
- Disconnect/removal invalidates display eligibility immediately. Reconnect starts
  a new generation; late events from the old object cannot repopulate current data.
- Invalid types, out-of-range values and absent properties yield unavailable;
  never coerce missing values into zero. Quickshell's 0..1 battery representation
  is converted once at its boundary; verify the installed API's units.

### Audio and microphone

Expose `bluetoothProfile = a2dp | hfp | off | unknown` from actual profile evidence.
Expose output and capture routes separately as `usb | bluetooth | other | mixed |
inactive | unknown`. Follow active PipeWire links, including processing nodes;
the default device or a connected card alone does not establish the route.
If exact profile metadata is unavailable, describe the observed sink/source
availability instead of guessing the profile.

A2DP explains that the current Bluetooth profile exposes playback without a
microphone. HFP says a microphone endpoint is available only when it exists;
an available endpoint is distinct from an active recording path.
Mixed/unresolved graphs remain explicit. Public HFP loopback sources may inherit identity through an unambiguous
`device.id` relation to a native BlueZ node; internal loopback streams are not
application captures. Retain graph size limits and reject
both ALSA and Bluetooth alternative physical inputs when attributing a capture.
Do not let a Bluetooth-only call produce a USB call-context request.

In Bluetooth-only mode, show native mute unknown and USB-only metering unavailable.
Do not display another microphone's level as Cetra. USB plus Bluetooth must not
create a second meter or duplicate hardware commands. ANC, voice and lighting
actions remain USB-only, with a concise availability explanation.

### Resources, diagnostics and errors

One service-owned observer, no per-view observers. Track selected identities and
bounded candidate data; detach listeners and invalidate generations on removal,
BlueZ restart and service teardown. Native host-owned device models remain host
owned. Reuse the current PipeWire graph limits of 512 nodes / 2048 links.

Diagnostics retain the last 16 distinct connection/profile/route snapshots,
evidence source and LE rejection reason in service memory. Honor the existing
diagnostic opt-out; no new persistent file or rotation owner is added. No PCM, periodic duplicate logs or persistent battery sampling by default.
Existing USB diagnostic files are not opened by a competing writer. Retain a
clear distinction between an observed disconnect and its unknown physical cause.

BlueZ failure leaves working USB controls intact. USB failure leaves Bluetooth
observations visible, but cannot authorize USB actions. Missing both transports
uses the disconnected visibility preference. Recovery must not apply hardware
settings or alter the system's selected audio route.

## 3. Verification and acceptance

### Targeted automated checks

Test production projection and lifecycle functions, not copies of their logic.

| Case | Required result |
| --- | --- |
| USB only, Bluetooth only, both, neither, LE only | Correct visibility and connection labels; only existing USB readiness enables device commands. |
| Remove USB with Bluetooth connected and panel open | Panel survives; USB data/pending actions clear; keyboard focus remains usable. |
| Stuck LE 71%, changing USB/audio values, GATT zero | LE/GATT do not override accepted sources; verified source zero remains valid. |
| Cached initial Battery1, identical snapshots, sparse updates | No invented hardware timestamp or freshness from re-reading/cache access. |
| Malformed battery, missing property, wrong units | Bounded validation and unavailable state; no fabricated zero or duplicated sides. |
| BlueZ restart, renamed device, address/object replacement, late events | Identity/generation rules prevent stale or wrong-device data. |
| Multiple matching headsets and unrelated battery devices | Ambiguity is explicit; no first-match binding or automatic LE association. |
| A2DP, HFP, missing metadata, defaults differing from active routes | Correct availability/unknown labels; no synthetic profile or microphone claim. |
| USB and Bluetooth inputs through EasyEffects, mixed sources | Route attribution fails closed where ambiguous; Bluetooth calls do not activate USB context. |
| Service/view teardown and reconnect | No duplicate observers, children, meters or commands; no Bluetooth writes. |
| Legacy preference migration | Existing saved settings survive; visibility uses documented fallback precedence. |

Exercise native Qt bindings and service ownership as well as offline functions.
Update test source enumeration when adding QML files. New UI strings use existing
catalogs; check key/placeholder parity and render Russian/English at constrained
widths. Other untested locales/accessibility scenarios remain explicitly open.

### Connected-device acceptance

Using the existing shell, verify USB-only, Bluetooth-only and simultaneous
connections, panel persistence, active playback/capture route labels and a marked
charge/discharge observation. Check A2DP's lack of a microphone endpoint and HFP
labels if the user/system selects HFP; the plugin must not switch it automatically.
Confirm physical settings controls still work through USB and cannot send commands
through Bluetooth-only mouse/keyboard actions. Preserve settings on reconnect.

Live connection/profile changes are coordinated with the user because they can
interrupt current audio. Shell restart and disable/re-enable require existing or
explicit authorization. Follow the lock guard for runtime delivery. Suspend/resume
and multimonitor cases remain open until exercised; prior deferrals are not passes.

Before completing implementation, run:

```sh
./tests/run.sh
omarchy plugin validate .
git diff --check
pgrep -a cetra-watch
cat "${XDG_RUNTIME_DIR}/rog-cetra-control.status"
tail -n 10 ~/.local/state/omarchy/rog-cetra-control.log
```

Record the exact source revision, host versions, automated results, actual visible
behavior and untested cases. A passing first stage means useful Bluetooth status
with honest provenance and unchanged USB behavior. It does not mean Bluetooth
hardware control, native mute readback, reduced battery use or a fixed duplicate
prompt. Update README, MODULES, HANDBOOK and BACKLOG with implemented behavior when
that implementation is delivered; do not advertise it as available from this spec.
