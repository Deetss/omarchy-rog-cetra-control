# ROG Cetra Control 1.8.0

## Changes

- Read Bluetooth battery values for each earbud and the case, current ANC mode
  and reported charging flags through the verified Asus_APP channel.
- Keep the panel available without USB for the selected Bluetooth audio device.
  USB retains priority and all hardware control commands remain USB-only.
- Refresh automatically: 15 seconds with the panel open, two minutes closed.
  Failed reads clear old data and retry after 30/60/120/240/300 seconds. Manual
  refresh remains available after a cooldown; requests never overlap.
- Keep route/profile/system-battery explanations under collapsed Details and
  use a short hover tooltip on both transports.

## Installation

Run the plugin's `setup` while Omarchy is running and unlocked after updating.
It builds four local helpers; Bluetooth adds `bluez-libs`/libbluetooth. Existing
`hidapi`, `libpulse`, compiler and shell requirements remain documented in README.
Select the main Cetra audio entry in Details after connecting through the system
Bluetooth panel. The plugin does not pair devices or change audio profiles.

## Verified scope and limits

Marked Bluetooth-only tests matched all three ANC voice prompts, showed changing
earbud percentages and restored telemetry after a case cycle without Refresh.
See ACCEPTANCE-2026-09-20.md for timings, source identity and boundaries.

Bluetooth ANC is read-only. Case freshness, physical battery accuracy and power
impact are not established. System Bluetooth may show two same-name entries and
an unrelated/stale percentage; vendor values are kept separate. A retained BlueZ
connection flag can leave the header connected while telemetry is unavailable.
Native microphone mute stays unknown. Bluetooth microphone capture, profile
switching, live suspend/resume and multimonitor acceptance remain unverified.
The existing preview is historical. Marketplace approval applies to an exact
snapshot and is separate from preparing or publishing this version.
