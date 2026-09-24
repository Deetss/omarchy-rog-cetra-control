# ROG Cetra Control 1.9.0

This release combines the panel rework with the Bluetooth telemetry developed
after the last published 1.7.0 release.

## Changes

- Show independent left/right charge inside the bar earbuds, with smoother
  rendering in the same 27 × 26 slot. Redraw the panel's pair, individual earbuds
  and charging-case icons, and remove unused microphone assets.
- Keep the microphone capsule fixed beside the earbuds. No capture data uses a
  dim empty outline; measured silence uses a bright empty outline; speech fills
  it from below. These states do not establish native microphone mute.
- Separate Sound and Device, with Settings and Color tabs. Keep interface
  language selection accessible in the header.
- Edit lighting with a circular palette, HEX/RGB fields and brightness control.
  Apply saves and sends the selected color once; Cancel sends nothing.
- Start the optional USB level meter when real recording or dictation begins,
  without requesting communication mode for ordinary recording.
- Include read-only Bluetooth earbud/case battery, ANC and reported charging
  telemetry, automatic refresh and bounded retries. USB retains priority and
  remains the only transport for hardware controls.
- Replace the historical README preview with a rendering of the actual panel
  QML and Omarchy theme. Its battery values are illustrative.

## Updating

After updating the source, run the plugin's `setup` in an unlocked Omarchy
session, then reload the shell to load the QML changes. Manual setup builds four
local helpers and may install `base-devel`, `hidapi`, `libpulse`, `bluez-libs` and
`pkgconf`. The installer and dependency set are unchanged from the 1.8.0 candidate.
See README for installation, removal and diagnostic settings.

For Bluetooth without USB, select the main Cetra audio entry under
Device → Settings → Details. The plugin does not pair devices or change profiles.

## Verification and limits

The repository suite covers native selftests, service lifecycles, telemetry,
localization, semantic QML lint, color-editor behavior and microphone admission.
The final icons were rendered and checked in isolated Qt/OpenGL, including
independent charge, invalid/missing values, silence, speech and a hidden meter.
These fixtures do not replace live desktop lifecycle or hardware acceptance.

Earlier marked Bluetooth trials matched all three ANC voice prompts, observed
changing earbud reports and automatic recovery after a case cycle. Physical
battery/case freshness, power impact, Bluetooth microphone capture, live
suspend/resume and multimonitor behavior remain outside the verified scope.
Native microphone mute remains unknown. See BACKLOG.md and the dated acceptance
records for these limits.

GitHub publication and Marketplace acceptance are separate. Updating request
#7774 starts verification of the exact new commit; approval of 1.7.0 does not
cover this release.
