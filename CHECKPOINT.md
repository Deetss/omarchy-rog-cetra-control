# Current checkpoint — 2026-09-24 release preparation

User requested updating the earlier Marketplace request with a new release for
the current rework. Prepare 1.9.0 locally; the exact push, release/tag and issue
update will be presented for confirmation. Issue #7774 is still open at `a300fbe`,
so editing it starts a new exact-commit check without close/reopen churn.

Current published source is `cd62d30`. The full repository suite, semantic QML
lint, plugin validation and OpenGL icon checks passed. Preview is an isolated Qt
render with illustrative values. The shell was not restarted for final smoothing.
Release metadata and notes now describe 1.9.0; see RELEASE.md for the next actions
and BACKLOG.md for current limits. The unrelated REVIEW-2026-09-22.md stays local.

---

# Earlier checkpoint — 2026-09-23 UI development

User approved the OpenDesign Color flow and requested implementation, commit and
push to this repository's origin/main. No Marketplace update, tag or release is
authorized by that request. Manifest remains 1.8.0; see Unreleased in CHANGELOG.

The accumulated bar/panel/microphone changes and Color editor are integrated.
One Apply saves the RGB draft and sends it through the USB owner once; failed
save sends nothing, rejected dispatch stays open for retry. Movement/Cancel send
nothing. The wheel has a separate swatch and HEX label. Effect/theme controls
hide during editing. Bluetooth lighting stays unavailable; telemetry is read-only.

The full validation result and live activation evidence are appended to
SDD-COLOR-EDITOR.md. Independent Gemini reviewed the prior UI/meter changes and
the one-step palette. The numeric bar level now additionally requires active
USB capture; the reported missing ControlButton import was rejected because it
is a same-directory local QML type, not a missing host export.

After activation the user reported testing the result successfully ("Я потестил
все круто"). Record this as user acceptance of the current UI; individual hardware
steps were not itemized. Broader battery freshness, Bluetooth microphone and
suspend/resume limits remain unchanged. No more changes are planned today.

Unrelated Marketplace draft edits and historical design explorations remain local.

---

# Earlier checkpoint — 1.8.0 update preparation

September 20 marked Bluetooth trial completed on runtime `e494e05`: all three
voice-prompt modes matched the panel; earbud reports changed; closed-case data
cleared and returned automatically after the user removed the earbuds. Full
BlueZ link disconnection was not observed. See ACCEPTANCE-2026-09-20.md.

The 1.8.0 metadata/docs candidate passed aggregate tests, manifest/diff checks
and release-document review; next is the existing-listing update workflow. No new Marketplace approval is claimed. Keep
physical freshness, power cost, native mute and deferred host tests explicit.

---

# Bluetooth development checkpoint — 2026-09-16

## Resumed — 2026-09-20

Marketplace accepted the corrected 1.7.0 snapshot at `0b491be` on 2026-09-18:
https://github.com/omacom/omarchy-plugin-marketplace/issues/6873#issuecomment-5735399691.
The earlier reference to issue6942 was incorrect; it belongs to another plugin.
The user authorized merging Bluetooth development into main and preparing an
update. Preserve the accepted documentation rename: no AGENTS.md files may return
to the distributable tree. The saved Bluetooth branch remains a recovery point.
Local main merge: `3c8cc8d`. The subsequent automatic retry follow-up replaces
manual-only recovery with30/60/120/240/300-second backoff and preserves the shared
single-reader lifecycle. Full aggregate tests, manifest validation, diff checks
and an independent Gemini review passed for the retry implementation. The real
Qt fixture covers production QML with controlled child exits; it is not live
Bluetooth acceptance.

At this session's unmarked baseline, USB reported97/98/89 and Ambient; the
standalone Bluetooth reader failed during service discovery. BlueZ still showed
the selected audio device connected. No marked voice-prompt response was received
at this checkpoint. Battery progression, gesture-to-panel ANC and channel recovery
remain open. The update is not submitted; version remains1.7.0 pending acceptance.

The sections below record the September16 checkpoint; they do not establish
acceptance of the new update.

Implementation checkpoint: `fa2b39f`. See BACKLOG.md for open acceptance,
SDD-BLUETOOTH.md and SDD-BLUETOOTH-TELEMETRY.md for scope, and RESEARCH.md for
protocol evidence. Historical acceptance documents describe their dated stages.

## Saved work

- Explicit Bluetooth audio-device selection and USB/Bluetooth route observation.
- One bounded read-only vendor reader: ANC, individual battery values and
  reported charging flags. USB remains the sole command/control transport.
- Shared polling: 15 seconds with panel open, 120 seconds closed. Read failures
  clear telemetry and stop automatic attempts; manual retry has a cooldown.
- Compact Bluetooth panel with collapsed Details and a three-line hover tooltip.
- Unknown native microphone mute is preserved; no synthetic mute or unverified
  HID/RFCOMM writes were introduced.

## Next work after acceptance

1. Verify battery progression. Fresh vendor reads returned 100/100/85 while
   BlueZ Battery1 returned 0. A subsequent user screenshot after switching to
   USB showed 94/100/85 and ANC enabled. These were not simultaneous readings;
   neither physical freshness nor the cause of the discrepancy is established.
2. Verify ANC gesture-to-panel updates against the exact spoken prompt and time.
   The user reported a non-reactive label. Standalone reads changed anc to off;
   no marked comparison established whether polling delay explains the symptom.
3. Consider bounded automatic retries after read failure, so the user does not
   need to keep pressing Refresh. This is proposed, not implemented.
4. Keep the primary layout consistent across transports. USB currently exposes
   ANC controls, levels, adaptive ANC and device settings; Bluetooth is read-only.
   Consider folding advanced USB controls while preserving their functionality.
5. Complete the remaining BACKLOG acceptance, including channel recovery and
   real Bluetooth microphone capture. Do not call duplicate prompting fixed.

## Verification at the implementation checkpoint

`./tests/run.sh`, `omarchy plugin validate .` and `git diff --check` passed.
Live compact panel, short hover tooltip and keyboard opening of Details were
observed. One cetra-watch owner, valid status JSON and continuing logs were checked.
Complete transport/keyboard combinations and physical battery accuracy remain
unverified. Gemini reviews were checked by the coordinator; unsupported claims
of physically full batteries were rejected.

Private captures and review logs remain outside Git in the local Cetra research
directory. Generated helper binaries are excluded; setup rebuilds them.
