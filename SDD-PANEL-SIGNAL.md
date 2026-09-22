# Sound / Device panel and microphone signal

Status: user approved local implementation, including Bluetooth, 2026-09-22.
No publication, desktop/audio/Bluetooth configuration changes or new recorder.

## 1. Intent and invariants

Use the MX Ergo panel's shared host controls and progressive disclosure to make
Cetra easier to scan. Keep bar silhouette D, per-earbud charge fill, actions and
all hardware/protocol ownership unchanged. Native microphone mute stays unknown.
Preserve USB and Bluetooth paths, nullable data, last-report semantics, bounded
Bluetooth telemetry refresh/retries and the existing opt-in level preference.

## 2. Behavior contract

Two ephemeral tabs, Sound (default) and Device; switching tabs never persists a
setting or sends a hardware command. Header includes transport; batteries retain
a concise last-report label. Sound contains three charge columns, USB ANC controls
and contextual strength, Bluetooth reported ANC/status/refresh, and compact mic
signal state. Device contains language, connection diagnostics and existing
lighting/voice/mic-level preference. Unsupported controls are hidden or explained;
Bluetooth ANC stays read-only and Bluetooth PCM level remains unsupported.

Expose capture observation separately from numeric level: no active recording,
unknown route, awaiting data, valid zero and positive level must not imply mute.
Bar: no-data uses an unfilled track with one central break (no tiny dashes),
available data an unbroken bright
outline, speech adds existing amplitude fill. No invented signal floor or gain.
Start peak observation immediately on a real USB capture admission; retain >=2s
retry after failure and exclusive process stop/start on source changes. Keep
/dev/null, self, DSP and keepalive exclusions. No recording files are created.

Keyboard focus remains in visible/enabled controls. Switching tab resets the
viewport; transport removal cannot leave focus in hidden controls. Both tabs
remain reachable when USB disappears but Bluetooth audio remains connected.

## 3. Verification

Production-function tests for initial start, backoff, inactive/unknown capture,
source change, stop, stale signal and keepalive exclusion. Qt render actual bar
states (no data, zero, positive, unknown/unequal batteries) in Software and OpenGL.
Exercise panel composition and focus with USB, Bluetooth-only, dual connection,
no data and disconnect fixtures. Check translations/fallback, scrolling and
keyboard dismissal. Run full tests/run.sh, plugin validation and diff check.

Activate once through supported shell restart after unlocked check and notice.
Inspect live screenshots of Sound/Device and one-owner/status/log evidence. Use
existing recording activity if available; do not launch an unsolicited recorder.
Physical Bluetooth hotplug/mic recording needs user participation if unavailable;
report fixtures separately from physical tests. Workers M1 (meter patch) and U1
(UI behavior review), then independent final review of the frozen implementation.

## Review checkpoint (2026-09-22)

M1 supplied the meter draft; integration preserves a running cooldown across
capture toggles. U1 supplied panel constraints; its suggestion to start on popup
opening was rejected because only an independently observed capture may admit PCM.
R1 (fixed Gemini helper, session `ses_f354bab3dffe746sgOFLV1G22b`)
found missing Bluetooth-only focus recovery: accepted and covered by the keyboard
fixture. Its alleged missing `leftAlign` property was rejected: the installed
`/usr/share/omarchy/shell/Ui/Button.qml:47` declares the inherited property, and
QML lint resolves the production component without semantic diagnostics.

The observed `pw-record` writing `/dev/null` is a keepalive and is intentionally
excluded. Its presence without cetra-peak was not a reproduced capture failure.
The verified fix removes the initial two-second delay while retaining failure
cooldown; it does not change capture eligibility or audio routing.

Live review also exposed uneven battery rails when status captions differ. Values
now share a fixed-height row and rails precede variable-height status captions.
Popup SVG masks use fourfold source resolution with smooth mipmapped reduction,
matching the inspected MX Ergo approach; the selected silhouette is unchanged.

## Verification and remaining limits

- Aggregate `./tests/run.sh`, QML lint, locale checks, plugin validation and
  whitespace checks pass. Actual bar rendered in software and OpenGL; the
  enlarged OpenGL artifact and live desktop crops were inspected.
- Supported shell restart completed after an explicit unlocked response. Live
  Sound panel was inspected with USB (including positive PCM level) and with
  Bluetooth telemetry unavailable. The final USB crop includes aligned charge rails.
- A native delayed-focus warning during QML reload led to teardown guards in
  queued focus callbacks and a regression fixture. No new audio or Bluetooth
  command was added. Final runtime log is checked for recurrence after the fix.
- Device-tab screenshot still needs the user's click: native pointer/keyboard
  control is unavailable on this host. Its composition, guarded controls and focus
  behavior have automated coverage; this is not a live interaction pass.
- Existing genuine capture displayed signal; initial-start latency is verified
  in the controlled Qt fixture, not measured end to end against a fresh recording.
  Bluetooth hotplug and fresh vendor telemetry were not revalidated physically.
- All work remains local. No external submission, push or release was performed.

Task evidence is retained in `/tmp/cetra-panel-work/` (including source hashes,
worker reports and cropped desktop images). Full screenshots contain unrelated
desktop content and are not repository/publication artifacts.

## Device follow-up from user screenshots

The user supplied upper/lower Device screenshots with Color palette expanded.
Scrolling reached the lower controls, but always-visible microphone explanations
made the page unnecessarily long. Move those existing texts under Details, reuse
the MX Ergo host ScrollBar pattern for overflow, and replace the selected-color
strip with the explicitly chosen 32-unit circular sample. RGB, Apply, theme sync,
USB guards, translations and command behavior remain unchanged. This is a local
layout refinement; screenshots of the final loaded palette are still required.
