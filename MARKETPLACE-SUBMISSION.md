# Marketplace update — 1.9.0

Update existing issue https://github.com/omacom/omarchy-plugin-marketplace/issues/7774.
It is already open; editing the target commit starts fresh validation. Preserve
its discussion and do not change maintainer approval labels.

Title: **[Verify]: ROG Cetra Control 1.9.0 — panel rework and Bluetooth telemetry**

After freezing the candidate, replace the Target commit placeholder with the
full `git rev-parse HEAD` SHA in the outgoing body. Keep this source template
free of a self-referential candidate SHA. Publish only after approval of the
exact candidate and issue update. The outgoing body begins at the marker below. Its six level-three headings
must remain unchanged; review notes precede them because the Marketplace parser
rejects extra fields.

<!-- issue-body:start -->

## Changes and review notes

Please review this 1.9.0 candidate in place of the previous 1.8.0 target
`a300fbe1dc627570f5c498c5cba702cd2fb4081c`. It includes that candidate's read-only
Bluetooth telemetry and the subsequent panel rework:

- Separate Sound and Device pages, with Settings and Color tabs and header
  language selection.
- Redrawn earbud/case icons and smoother 27 x 26 bar rendering, with independent
  bottom-to-top left/right charge and a stationary microphone capsule.
- A dim empty microphone outline for unavailable capture data, bright empty
  outline for measured silence and fill for amplitude. No native mute inference.
- Inline color palette with HEX/RGB and brightness controls; Apply saves and sends
  once, while Cancel sends nothing.
- Actual recording/dictation admission for the optional USB signal meter without
  forcing communication mode. Inactive or unavailable capture cannot paint signal.
- Current QML-rendered preview with illustrative values and obsolete SVG cleanup.

Manual setup remains required; the unchecked standard-installation field is
intentional. The setup script is unchanged from the previous candidate. It builds
four helpers from source and can install `base-devel`, `hidapi`, `libpulse`,
`bluez-libs` and `pkgconf`. The installer/package-manager capabilities reported by
the previous baseline still need maintainer review. USB remains the hardware
control transport; Bluetooth controls and native microphone mute are not claimed.

The repository suite (including semantic QML lint), plugin validation and diff
checks passed. The icon runtime also passed isolated Qt/OpenGL checks; prior
bounded Gemini reviews covered the UI/color work and artwork. These checks are
not a security audit or new full hardware/lifecycle acceptance. The final preview
is an isolated render, not a live device capture.

The user accepted the panel rework. Earlier marked Bluetooth tests matched all
three ANC voice prompts, changing earbud reports and automatic recovery after a
case cycle. Physical battery/case freshness, power impact, Bluetooth microphone
capture, live suspend/resume and multimonitor behavior remain unverified.
See RELEASE-NOTES-1.9.0.md, BACKLOG.md and the dated acceptance records in the
candidate. Existing releases/tags and the currently listed snapshot are unchanged
by this verification request.

### Verification action

Verify and publish a newer upstream commit

### Plugin ID

io.github.pavellizunov.rog-cetra-control

### Repository URL

https://github.com/PavelLizunov/omarchy-rog-cetra-control

### Target commit

<full SHA of the frozen 1.9.0 candidate>

### Verification acknowledgment

- [x] I understand that only the exact target commit can become a verified marketplace snapshot and that verification is not a security audit.

### Standard installation acknowledgment

- [ ] I confirm that this listed root plugin supports the standard Omarchy installation path and does not require manual setup.
