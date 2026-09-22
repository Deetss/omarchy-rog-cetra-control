# Device color tab and interactive swatch

User approved: Device has a separate Color tab; clicking its round swatch opens
color selection. On 2026-09-23 the user approved the OpenDesign layout and a push
to this repository's origin/main. No Marketplace submission or release authorized.

## Intent and invariants

Reuse host buttons and native Qt color/gradient primitives and existing USB lighting commands. No native modal
window, new dependency, Bluetooth write, automatic HID send on tab/editor opening,
or implicit application of a draft. Preserve theme synchronization policy.

## Behavior

Language selection uses the MX Ergo standard: a compact 文 + language-code button
in PanelHero.trailingControl, with the picker immediately below the header on
both Sound and Device. Device keeps compact, borderless Settings / Color sub-tabs.
Settings retains mic preference, details and voice prompts. Outer Sound / Device
tabs stay larger; no other navigation moves. Color contains lighting effects, theme
preferences, a round focusable swatch and explicit Apply. Bluetooth-only Color
explains that USB is required. The swatch opens an inline visual palette initialized
from the selected color: circular hue/saturation wheel and brightness slider. An optional Exact color
section provides a six-digit HEX input and R/G/B sliders (0–255), synchronized
with the wheel. Invalid/incomplete HEX never mutates the draft and disables
Apply; fixing it or selecting with another control clears the error. Closing
Exact color abandons only invalid text. Pointer or
arrow-key changes edit a local RGB draft; black/grey preserve the draft hue. Cancel, panel close,
leaving Color or losing USB discards it. Apply saves RGB and manual-color mode
in one self-scoped settings update, then sends the exact draft using the existing
validated USB command. Save failure sends nothing. Dispatch rejection retains the
editor and reports failure; the saved preference remains saved. Retry is explicit.
One successful click sends once and closes the editor. The closed manual editor
has no second Apply; a theme-color Apply remains available for theme mode.
Hide effect/theme controls while editing. Keep the selected swatch and HEX code
beside the wheel. Keyboard can activate, edit and reach Apply/Cancel.

## Verification

Exercise atomic save/no-op/rejection/stale fallback/invalid channels;
production draft functions and cancel paths, USB removal, theme changes mid-edit,
keyboard routing and focus. Lint, catalogs, full suite, plugin validation and diff
check. Verify delivered Device/Color/editor screenshots and interactions on the
actual desktop with user participation if native controls are unavailable.

## Accepted one-step flow — 2026-09-23

The user accepted the locally served OpenDesign preview and requested application
and Git push. This supersedes historical Choose-without-HID checkpoints below.
Keep USB-only lighting and the host theme; no transport dependencies change.
Test one command per accepted Apply, no command for movement/cancel/failed save,
helper-down retry, exact RGB despite stale settings, Off/Cycle→Static and retained
Breathing/Strobing. Preview and native hardware evidence remain distinct.

### Final implementation checkpoint

Full `./tests/run.sh`, `omarchy plugin validate .` and `git diff --check` passed.
The color suite passes 19 fixtures, including one-click dispatch, rejected save,
stopped-helper retry and stale host bindings. The complete Qt wheel with the
separate 40-unit swatch and HEX text was rendered and inspected. The fixture's
host font/foreground substitutes were updated for the new text; targeted Qt
checks passed again. The bar also passed actual OpenGL pixel checks after adding
explicit USB/active-capture gating; its regression rejects retained numeric levels
while USB or capture is unavailable.

Independent Gemini review R2 found no blocking color-flow defects. R1's bar
gating finding was accepted; its speculative missing host import was rejected
(ControlButton is a local component). These are bounded source reviews, not
physical device acceptance.

Supported shell restart completed after an unlocked check. One cetra-watch,
valid connected USB status and ongoing telemetry were observed. The running
Sound panel was captured and inspected. Full live Color clicking and physical
lighting remain unverified; the next session starts there. Browser preview and
isolated Qt checks are not substituted for native hardware acceptance.

Subsequent user acceptance: after activation the user reported "Я потестил все
круто". The current UI is accepted; no per-action physical test transcript was
provided, so the statement does not extend coverage to unrelated hardware cases.

Research verdict: Extend existing ControlButton, LightingPalette, Qt HSV/gradients and host
self-scoped preferences. No installed host color picker was found in Ui/plugins.
Worker P1 drafts atomic preference save/tests; coordinator implements frontend,
integrates verified worker output and owns acceptance.

## Prior RGB-editor checkpoint (superseded by visual palette)

P1's atomic-save draft was integrated with the specified array-only RGB contract;
extra proposed object formats were rejected as unnecessary. T1 provided eight
locale translations, reviewed and placeholder-checked. Independent R1 review
accepted save/cancel/USB invariants but alleged that Bound delegates cannot access
outer ids and that child.visible ignores ancestors. Both premises were disproved
by an isolated test on the installed Qt: outer id resolves and child.visible is
false under a hidden parent. Navigation already restores the visible destination
tab through its deferred callback. No speculative aliases were added.

`./tests/run.sh`, `omarchy plugin validate .` and `git diff --check` passed.
Production-function tests cover atomic save, rejection, no-op, stale fallback,
draft cancel, theme changes, no HID on Choose, explicit Apply, keyboard channel
editing, Device tabs and USB loss. Native popup interaction/visual verification
remains separate and requires an opened Color editor. Local evidence, worker
reports and the Qt probe are in `/tmp/cetra-color-editor/`.

## Visual palette correction

Replaced RGB sliders with LightingColorField (native Qt HSV plane/hue strip),
restored language above Device sub-tabs and made those caption-sized, borderless
and intrinsic-width. R2 reviewed the new fields and supplied eight translations.
Accepted its zero-size coordinate guard. Rejected the proposed offline editing
change (USB loss explicitly cancels), redundant array guards (the model always
owns three validated bytes), and per-edit hue reset allegation (reset runs only
on opening; continuous HSV changes never reset it). Native tests cover these
contracts, including reopening via ancestor visibility. The test needs a visible
offscreen window to exercise effective visibility; a detached C++ Item has none.
Qt's specific key handlers accept their event by default; no duplicate routing
was established by the review's contrary claim.

Actual desktop capture inspected: `/tmp/cetra-color-palette/device-color-live.png`.
It shows the compact inner tabs, shared Device language, circular swatch, open
palette, hue strip and Choose/Cancel. The panel is scrolled to reveal the editor;
its header is above the viewport. Consumer click/apply outcome is not established
by this image alone. No hardware command was issued by the test harness.

Final local verification: full `./tests/run.sh` passed, including the complete
native Qt palette/reopen checks and 18/18 lighting fixtures. `omarchy plugin
validate .` and `git diff --check` passed. Shell restarted once for this correction
after an unlocked check; one cetra-watch, valid connected status JSON and ongoing
telemetry observed. No new palette/Cetra QML errors found in the checked journal.

## Header language correction — 2026-09-23

The user designated MX Ergo's header as the standard. The local ErgoPanel.qml
and supplied screenshot agree: host PanelHero.trailingControl, native Button,
文 icon and compact locale code, language grid below the hero. Cetra adopts that
composition, retaining its existing locale persistence and keyboard focus return.
Switching a main page closes the picker as MX Ergo does. No additional navigation
or device behavior changes. Verify both page access, translations, focused header
control, native rendering and the mandatory repository checks.

Header correction verification: full tests passed, focused keyboard checks passed,
plugin validation and diff checks passed. Supported shell restart loaded the
candidate; the live Bluetooth-state panel shows 文 EN at the header's right edge.
Crop inspected: `/tmp/cetra-language-header/header.png`. One owner and valid status
JSON observed. Language menu clicking was not exercised by automation in this turn.
The user's subsequent circular color-palette request remains a separate pending
interaction choice; it is not implemented by the header change.

## Circular palette and precise input — approved 2026-09-23

Replace the swatch in place with the expanded circular palette (not a separate
window or rectangular field). Native Qt Shapes conical/radial gradients encode
HSV; no new package or timer. The host PanelSlider and TextField supply precise
controls. Appearance follows the current Omarchy/MX Ergo tokens, ENERGY/RHYTHM/
MOTION 1; gradients are the selectable color data. Keep manual Apply and all USB,
cancel and atomic-save invariants. A/F/digits typed in HEX must not invoke ANC.

Verification: circle cardinal points, center, edge clamping and keyboard; RGB/HEX
roundtrips, lowercase/hashless values, invalid/partial input and no HID; black and
grey hue preservation; render the actual production wheel, inspect native panel,
and run repository checks. Physical lighting only after explicit user Apply.
Research verdict: Extend existing editor and installed Qt/Omarchy primitives.
Qt reference: https://doc.qt.io/qt-6/qml-qtquick-shapes-conicalgradient.html

### Circular palette verification checkpoint

The complete production wheel rendered in Qt (software backend), inspected at
`/tmp/cetra-color-wheel/wheel.png`. Cardinal-point geometry, bounds, achromatic
hue, HEX/RGB roundtrips and rejected input passed. The production HEX TextField
handlers were exercised with actual Qt key events: A/F/digits, length overflow
and Escape; no events reached the parent shortcut catcher. The harness uses
-fPIC for the installed Qt shared-library ABI.

R4 independent Gemini review: accepted thumb padding and added an AfterItem key
fallback. Rejected inverted-gradient claim (official Qt Shapes docs and actual
render are counter-clockwise), alias claim (LightingSection bridges toggleControl),
settingsExpanded claim (means Device, not Settings sub-tab), and float-slider claim
(host PanelSlider integer=true rounds before moved). Eight locale translations
received source/placeholder review. No unsupported reviewer assertions were adopted.

Anti-slop: functional color gradients, host typography/spacing, no decorative
motion or new package. Native wheel render and actual HEX keyboard checks PASS;
all editor interactions and color controls in the live panel remain NOT VERIFIED
until an opened panel can be captured. The first post-restart capture had no popup.
No new physical lighting acceptance is claimed.

User refinement: retain a separate round selected-color preview beside the open
wheel. It reflects the same RGB draft for wheel, brightness, HEX and RGB edits;
it is a static preview, not another Apply control. Wheel sizing reserves its
width and marker padding; the pair is centered in the available field.

Separate-preview checkpoint: the full suite passed again with the preview circle,
including Qt HEX key isolation; plugin validation/diff checks passed. Production
wheel + preview render inspected (`/tmp/cetra-color-wheel/with-preview.png`).
Supported restart completed after unlocked check, one owner and valid connected
status observed. The user supplied an open-wheel screenshot; full precise-input
and preview layout in the current live popup remains unverified (subsequent
capture found the popup closed). Anti-slop/OpenDesign review is advisory only;
no further navigation or layout changes are included without the user's next step.
