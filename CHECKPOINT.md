# Bluetooth development checkpoint — 2026-09-16

## Resumed — 2026-09-20

Marketplace accepted the corrected 1.7.0 snapshot at `0b491be` on 2026-09-18:
https://github.com/omacom/omarchy-plugin-marketplace/issues/6873#issuecomment-5735399691.
The earlier reference to issue6942 was incorrect; it belongs to another plugin.
The user authorized merging Bluetooth development into main and preparing an
update. Preserve the accepted documentation rename: no AGENTS.md files may return
to the distributable tree. The saved Bluetooth branch remains a recovery point.
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
