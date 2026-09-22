# Compact Cetra bar indicator

Status: user selected variant D, based on the original Cetra silhouette with
softened corners, inward ear tips and a rounded leaning microphone meter.
Local implementation and shell activation are authorized; publication is not.
Approved references: `design/earbud-references/comparison.svg` (A) and
`design/earbud-references/microphone-shapes.svg` (3). The former external
battery strip and all other proposed shapes are superseded.

## 1. Intent and invariants

Keep a compact Omarchy slot, without persistent numeric percentages. Render the
familiar mirrored Cetra outline, filling each earbud with its own charge from
bottom to top. Render the optional microphone indicator on the right with the
approved lean and oblique end cuts. Earbud geometry and spacing must stay fixed
when speech amplitude or meter visibility changes. Remove the external charge
strip. Preserve original popup art, exact tooltip/popup percentages, theme
colors, low-charge warning, left-click panel, right-click/wheel ANC and all
transport, capture admission and native mute behavior. No new dependencies,
processes, timers, audio capture, network access or publication actions.

## 2. Behavior and component contract

Use one display-only bar component receiving the existing view model. Keep
existing `CetraIcon.qml` for popup icons and `MicrophoneLevel.qml` compatibility.
Use installed Qt Quick primitives/Shapes for theme-colored vector geometry.
Baseline design coordinates: 27 x 26 slot, 16 x 16 earbud art at (5.5, 5).
Scale the whole design uniformly with `Style.bar.iconFont / 13`; host slot is
at least `Style.bar.iconSlot`, expanding only for oversized theme metrics.
Keep the full slot even if the optional microphone indicator is hidden.

- Earbud path: approved D silhouette in a 64-unit canvas,
  stroke 3.5, round joins, mirrored at x=32. Fill each side independently using
  finite numeric 0..100 data from the existing left/right view-model projection.
  Invalid, null, missing or expired charge renders an outline and small internal
  unknown dot, with no charge fill. Measured 0 renders an empty outline without
  that dot. Case charge never drives the earbud fill.
- Linear charge waterline spans y=9..53, as in the approved reference; it encodes
  height fraction, not fraction of silhouette area. Outlines remain constant.
- Mic shape: rounded leaning capsule from approved variant D, dim theme-foreground
  track and theme-foreground fill clipped to the polygon. Valid signal 0..1
  fills from y=19.4 toward 6.95. No geometry or spacing changes with amplitude.
  Unknown/unavailable signal uses a dim short mark, not a zero or mute claim.
  Keep existing `showMicLevel` preference and accessible signal description.
- No percentage Text in the bar. Keep removal of obsolete `showPercentage`
  defaults/schema; saved legacy keys are ignored, without config edits.

## 3. Verification and delivery

Exercise the real bar component in a temporary isolated Qt fixture, not another
Quickshell. Check 0/25/50/100 and unequal side values, unavailable/invalid charge,
0/quiet/loud/unknown microphone, hidden microphone, fixed geometry, theme color
changes and scaled rendering. Inspect actual Qt-rendered images and compare to
approved SVG geometry. This is offscreen evidence, not a desktop screenshot.
Keep focused regression checks for changed logic and unchanged action wiring.
Run `./tests/run.sh`, `omarchy plugin validate .`, `git diff --check`; inspect the
frozen source with an independent bounded Gemini review and resolve findings.
Update the owning README and source map. Preserve unrelated dirty changes.

Load through supported `omarchy restart shell` under the standing user exception,
with advance notice and an unlocked-session check. Verify one HID owner, valid
status JSON and continuing logs; inspect the fresh shell log for QML failures.
Use supported `omarchy capture screenshot fullscreen save` for actual desktop
visual evidence; this CLI works even when native computer-use APIs are absent.
Do not claim physical gesture checks from an offscreen render. No Marketplace or remote push.

## Research decision

Verdict: Extend — reuse the existing model, host slot metrics and approved local
vector paths, using installed Qt Quick Shapes with CurveRenderer antialiasing. No new library or protocol work.
The prior research compared the installed Omarchy power/MX Ergo widgets and the
AirPods plugin. Source provenance and reference limits are recorded in
`design/earbud-references/README.md`.

## Implementation evidence

- Implemented `CetraBarIndicator.qml` and connected it to the existing bar view
  model. Popup SVGs and native/audio/transport code are unchanged by this task.
- `CETRA_BAR_EVIDENCE_DIR=/tmp/cetra-selected python3 -B tests/bar-indicator/run.py`:
  passed; actual software Qt rendering inspected.
- The earlier `CETRA_BAR_BACKEND=default` trial also selected the software
  renderer. It did not establish OpenGL/native rendering coverage. That claim
  was incorrect and is superseded by the explicit OpenGL checks below. These
  are fixtures, not screenshots of the active desktop. Geometry/scale and rendered pixel
  checks cover independent sides, unavailable/invalid charge, microphone
  silence/speech/unavailability, hidden meter and theme colors.
- `./tests/run.sh`, `omarchy plugin validate .`, `git diff --check`: passed.
- Gemini I2 supplied the bounded component draft; coordinator integrated it and
  corrected dot positions and track opacity to match the approved reference.
  Independent Gemini V2 source review found no blocking defects.
- Shell log shows automatic local-plugin reloads without Cetra QML errors; shell
  ping responds, one `cetra-watch` process remains, status JSON is valid and
  telemetry continues. No explicit restart was necessary.
- Live desktop appearance, physical gestures and real speech amplitude remain
  user-observation checks. The isolated Qt rendering does not establish those.
- Runtime/test hashes and detailed tool results are under `/tmp/cetra-selected/`.
  No publication or system/audio/Bluetooth configuration change was performed.

## Correction: antialiasing at actual bar size

The user reported severe stair-stepping and poor legibility on the live bar.
The earlier 4x software rendering and source review missed this defect.
Instrumentation confirmed that both earlier runs used graphics API Software;
requesting a default renderer did not establish accelerated coverage.

Reproduced in an isolated QQuickView explicitly selecting the RHI scene graph
and OpenGL: GeometryRenderer produced zero partially covered outline pixels at
13 px. CurveRenderer produced 82 on the same fixture, without changing the
approved paths, dimensions, charge or microphone contracts. Installed Omarchy
BorderOverlay also uses CurveRenderer.

The test now includes actual-size 13 px examples, saves an unfiltered pixel zoom
for inspection, asserts partial edge coverage and rejects backend fallback when
OpenGL was requested. `CETRA_BAR_BACKEND=opengl python3 -B tests/bar-indicator/run.py`
is the explicit accelerated check. The aggregate's software run remains useful
for portable behavior checks, but must not be reported as accelerated coverage.
Native desktop capture remains unavailable; user observation of the live bar is
still necessary before claiming visual acceptance.

## Live desktop capture correction

After the user reported no visible change, performed one full supported
`omarchy restart shell` following an unlocked-session check. The previous
automatic reload had not established user-visible delivery. New shell PID
1610955 and single owner PID 1611197 were observed; shell ping returned ok.

The supported command `OMARCHY_SCREENSHOT_DIR=/tmp/cetra-aa/live omarchy capture screenshot fullscreen save`
succeeded. Inspected the actual desktop screenshot and a pixel-enlarged crop
of the bar: Cetra has antialiased outlines after the full restart. Its thin
contour is visually lighter than neighboring solid icons. Do not treat that
as user acceptance of final legibility or as evidence of clicks/voice capture.

Prior statements that desktop capture was unavailable were incorrect. Only
the native computer-use APIs were unavailable; the supported screenshot CLI
was usable and is the appropriate capture path for this task. The screenshot
is `/tmp/cetra-aa/live/screenshot-2026-09-22_21-23-15.png`; the exact bar crop is
`/tmp/cetra-aa/live/bar.png`. Keep the full desktop image outside the repository.

## Approved thickness revision

The user selected wider option 2 from `/tmp/cetra-weight/comparison.png`:
16-unit displayed canvas at (5.5, 5), stroke 5, mirrored wider paths.
The right microphone polygon is about 20% wider; slot dimensions, actions,
charge semantics and signal semantics are unchanged. Full-charge silhouette
is retained as selected, pending any further design request.

Explicit OpenGL fixture passed with 113 partial-coverage outline pixels.
Inspected `/tmp/cetra-wide-final/qt-bar-preview.png` across charge and signal
states. The complete test suite, plugin validation and whitespace check passed.
This supersedes the 13 px dimensions in historical evidence above.

## Variant D trial

User approved trying D from `/tmp/cetra-familiar/comparison.png` locally.
Use the original inward-facing tips and angled stems, soften corners with
quadratic curves and preserve a transparent stem joint in each charge fill.
The slot remains 27 x 26 and art canvas 16 x 16. Rounded microphone geometry
fits x=21.5..26.7, y=6.95..19.4. Keep all charge, unknown, theme, click and
transport contracts. The pixel fixture now covers the entire taller meter.
This supersedes the earlier wider option 2 and microphone shape 3 selection.

D validation: the first OpenGL equality check found three translated edge pixels
with a one-step (1/255) color difference for invalid/unknown states, and one for
signal states. The fixture permits at most four one-step RGB differences; any
larger difference still fails. Geometry and amplitude contracts are unchanged.

## Sound / Device follow-up

The approved D earbud silhouette is retained. `SDD-PANEL-SIGNAL.md` supersedes
the microphone no-data treatment: one central gap in a dim solid stroke replaces
small dashes; valid capture has a continuous bright outline, and only real
amplitude fills it. The stroke is one logical pixel at the base bar scale.
