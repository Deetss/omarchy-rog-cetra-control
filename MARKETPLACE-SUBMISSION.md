# Marketplace update — 1.8.0

Use the existing-listing verification form, not a duplicate plugin submission.
After the final candidate is committed and pushed, insert its full 40-character
main SHA under Target commit. The request body below is the complete draft.
Keep the accepted 1.7.0 snapshot and existing tags unchanged while review is pending.

### Verification action

Verify and publish a newer upstream commit

### Plugin ID

io.github.pavellizunov.rog-cetra-control

### Repository URL

https://github.com/PavelLizunov/omarchy-rog-cetra-control

### Target commit

(To be filled from the exact validated candidate after commit.)

### Verification acknowledgment

- [x] I understand that only the exact target commit can become a verified marketplace snapshot and that verification is not a security audit.

### Standard installation acknowledgment

_Not requested. The existing manual setup requirement is unchanged._

### Maintainer notes

ROG Cetra Control 1.8.0 adds read-only Bluetooth battery/ANC telemetry, automatic
refresh/retry and a compact panel. USB remains the control transport; native
microphone mute remains unknown. No pairing, profile switching or unverified
hardware writes are added. No AGENTS.md files or prebuilt executables are included.

The source-built Bluetooth reader adds bluez-libs/libbluetooth. Existing manual
setup builds four helpers and requires an unlocked Omarchy session. The plugin
runs unsandboxed; Bluetooth connects only to the selected device. README lists
dependencies, diagnostics, installation/removal and known limitations.

The exact candidate passes the repository tests, QML lint, manifest and whitespace
checks. Marked Bluetooth-only trials matched ANC/Ambient/Off, showed changing
reported battery and recovered telemetry after a case cycle without Refresh.
Physical battery/case freshness and power impact are not established. Live
suspend/resume, multimonitor and Bluetooth microphone capture remain unverified.
Details: ACCEPTANCE-2026-09-20.md and RELEASE-NOTES-1.8.0.md in the target commit.
