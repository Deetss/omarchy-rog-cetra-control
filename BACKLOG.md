# Active backlog

Updated 2026-09-16. This file owns release status; RESEARCH.md owns protocol
evidence. The detailed pre-modularization audit is preserved in
[docs/archive/BACKLOG-2026-09-14-before-modules.md](docs/archive/BACKLOG-2026-09-14-before-modules.md).

## Release status

Version 1.6.0 is published as a GitHub pre-release at commit d5a687b, not a stable
release. Do not overwrite existing tags. Automated checks passed on that snapshot;
final modular connected-device acceptance and Marketplace submission remain
pending. See RELEASE.md for the evidence and release URL.

The user accepted the documented compatibility/test limitations and requested
release preparation. Version 1.7.0 is the candidate. Accepted limitations below
remain visible; this decision is not a claim that omitted tests passed.

## P0 — release acceptance

| ID | Implementation / evidence | Remaining acceptance |
| --- | --- | --- |
| P0.1 False absolute microphone state | Removed inferred mute and manual resync. JSON stays unknown in offline and live trials | Implementation closed; unavailable native readback is an accepted product limitation |
| P0.2 Implicit lighting writes | Starts unknown; explicit session preference only. 181 mocked transaction cases and live effects/restart checks pass | Fresh colored case/USB replay retest deferred and accepted for this candidate; exact official Off/duplicate sequence remains a documented research limit |
| P0.3 Fabricated settings | Nullable settings, 30 s freshness and presence veto. Parser/owner tests and live controls/case/USB pass | Implementation closed; stopped-report expiry is verified offline, not by deliberately disrupting hardware |
| P0.4 Call restart reconciliation | Latest-owner Discord restart recovered context; user confirmed Off/On prompts; ending call stopped the meter | Passed for the recorded Discord/EasyEffects trial; requested context is not tap/mute readback |
| P0.5 Truthful publication | Version 1.7.0, release notes and accepted limitations prepared; historical preview labeled | Delivery gate: validated source archive, exact publication commit and separately authorized publication; author confirms asset submission rights |

P0 runtime fixes and accepted limits are distinct from the publication gate.
Do not claim the candidate is already published or independently verified.

## Bluetooth development stage — 2026-09-16

Bluetooth observation and read-only vendor telemetry are in the working tree, based on 1.7.0. USB
commands and native mute constraints are unchanged. Scope and installed-API
adaptations are in SDD-BLUETOOTH.md; this is not a publication/readiness claim.

- Implemented: manual Bluetooth audio identity, panel availability independent
  of USB, separate system-reported charge, active route/profile observations,
  USB-only hardware controls/call/meter, disconnect generation invalidation.
- Automated acceptance: production JS and complete Qt observer bindings with
  controlled native models; see BLUETOOTH-ACCEPTANCE.md for the exact results.
- Live delivery: new Russian panel rendered after an explicitly authorized shell
  restart; one HID owner and valid status cache observed, no plugin QML errors.
- Passed live: user selected the main audio record, then removed the USB receiver;
  panel stayed open, USB values cleared, USB controls disappeared, Bluetooth charge
  stayed separate. User confirmation agrees with runtime cache and rendered UI.
- Live HFP observation exposed a public loopback source without an address field;
  identity now follows its explicit device.id relation. New regression cases added.
- Corrected HFP profile and microphone endpoint labels passed visible verification
  after restart; actual recording remains on another device.
- Passed live return to USB: user confirmed restored charge/controls and one ANC
  prompt. Historical duplicate prompting remains unreproduced, not fixed.
- Initial clarification: the panel-persistence trial did not test Bluetooth
  listening; playback/capture routes were other devices at that time.
- Subsequent playback passed: after a user-operated earbud/case reconnection,
  the user heard audio in the earbuds; the host showed USB absent, an A2DP sink
  and active EasyEffects-to-Bluetooth links. Automatic handoff is not established;
  the reported red-to-blue case indicator has no verified interpretation.
- Open live acceptance: complete keyboard focus, marked charge change and actual
  HFP microphone capture. Historical single-prompt tests are
  evidence only; this change does not fix a reproduced duplicate-prompt issue.
- Native API limits: no verified model/UUID auto-identification, audio/LE mapping,
  monotonic battery report timestamp, or card/profile-off enumeration. Explicit
  selection and unknown fields are used. The vendor extension below uses a separate bounded reader.
- Separate RFCOMM research: confirmed `Asus_APP` getters return three battery
  fields and charging flags. Marked user gestures verified ANC enum 1/2/0 against
  all three voice prompts. Per-side docking, case freshness and unsolicited
  events remained open at that research checkpoint; the vendor extension below now integrates the verified getters.
- RFCOMM live failure: left-only docking read succeeded (side charging flag 01);
  right-only docking was followed by connect timeout/EBUSY. Returning both earbuds
  and a user-operated case/reconnect cycle did not restore access within bounded
  attempts, despite SDP and BlueZ connection remaining available. Cause unknown;
  vendor-channel recovery and stale-state handling are gates for integration.
- Recovery follow-up: a 30-second connect-only attempt and an explicit host-side
  Cetra disconnect/reconnect followed by another 30-second attempt also failed
  before any getter. Structured standalone diagnostics pass ten offline tests;
  diagnostics record failures before any application command. Authorized HCI capture showed PN
  request/response, followed by host SABM with no observed UA/DM; in-flight DLCI48
  was BT_CONNECT. A controlled physical case cycle with confirmed disconnection
  restored access: two getter sessions returned 91/92/100, ANC Off, charging00/01.
  Successful capture used a headset-initiated mux (DLCI49 versus failed48, same
  server channel24). This is a correlation, not a proven cause. Repeated marked
  docking/recovery, stale-state handling and battery interpretation remain open.
  See RESEARCH.md.
- Marked right docking/return after recovery passed: all getter sessions remained
  available and side flags followed `00 -> 10 -> 00`. Battery fields were 89/90/100,
  then 89/91/100 docked, then 89/90/100 returned. This supports the right charging
  bit but does not establish physical charge change or case freshness. The
  earlier failure did not recur, so repeated failure/recovery remains untested.
- Lid-only trial passed with right docked/left worn: open/closed/reopened replies
  changed the second battery field `91 -> ff -> 93`, while all getter sessions
  succeeded and charging remained `10 01`. This supports right-field mapping.
  The vendor reader now invalidates only that percentage, preserves valid
  siblings, and never falls back to the preceding 91%. Its production-parser
  regression covers `91 -> null -> 93`. Case freshness and
  physical charging remain unverified.
- Bluetooth controls, profile switching, metering and power comparisons remain
  deferred. Suspend/resume, multimonitor and fluent review of other locales stay open.

### Vendor telemetry implementation — current checkpoint

- New `cetra-bt-read` performs SDP discovery and three fixed RFCOMM getters without
  root or HID access. Battery domains, ANC, reported charging bits, fragment
  reassembly, EOF, exact request bytes and reader exclusion have offline coverage.
- One shared `CetraTelemetry` child handles serialization, late generations,
  15-second foreground/120-second background cadence, a failure latch with
  manual retry, and 180-second report expiry. USB retains control priority.
- Columns/bar use a complete USB or vendor Bluetooth source. Standard Battery1
  and unassociated LE charge never fill a missing vendor percentage. Bluetooth
  ANC is read-only; native mute remains unknown.
- Bounded live reader returned left86/right unknown/case100 and ANC Off on
  2026-09-16. This validates actual transport reading, not case measurement freshness.
- Aggregate checks and installed-panel acceptance passed for this local snapshot;
  see SDD-BLUETOOTH-TELEMETRY.md for evidence and remaining limits.
- User reported unchanged 100% earbud readings and a non-reactive ANC label.
  Fresh standalone reads on 2026-09-16 evening returned 100/100/85 while BlueZ
  Battery1 returned 0. Vendor ANC reads changed from anc to off between reads;
  the user's corresponding gestures were not marked. Verify physical battery
  progression and a marked gesture-to-panel update before accepting freshness.

## P1 — runtime and host integration

- **Media tap / continuous capture:** marked A/B/A trial reproduced lost native
  Play/Pause with Voxtype keepalive capturing via EasyEffects, recovery with no
  capture, and recurrence after restoring keepalive. Plugin context was false
  and meter absent. See RESEARCH.md. Keepalive was restored. User explicitly
  excluded changes to other projects/services. Gesture logs now report the
  observation without promising media-key delivery; README documents the limit.

- **General freshness:** presence/charging/settings expire; new battery_fresh and
  mode_fresh flags expire after 30 seconds and veto stale UI. Exact hardware expiry
  acceptance remains pending; legacy raw fields retain diagnostic history.
- **Teardown:** one live disable/re-enable test ended the owner normally and
  started one replacement. This is not proof for a hung detector or all reload races.
- **Host preference loss:** installed Omarchy removed inline preferences during
  disable/re-enable. The test restored known preferences. Document backup before
  disabling; a plugin-local disk reader cannot preserve a host-deleted entry.
- **Hot reload:** stale QML was observed after logged reloads. Verify actual
  visible changes; obtain/retain explicit authorization for a shell restart.
- **Call semantics:** generic untagged browsers no longer match the name fallback.
  Known communication apps still use a heuristic when role metadata is absent.
  Manual request UI/M/Ь were removed; legacy alwaysCallContext is ignored.
- **Multimonitor:** shared-state offline coverage exists; monitor removal and
  concurrent live controls on multiple displays are not fully verified.
  User has no second monitor and explicitly requested leaving this open.
- **Suspend/resume:** user deferred the live trial as inconvenient; keep OPEN.

## P2 — transport, filesystem and resource limits

- Shared `/tmp` fallback removed; private runtime root and lock validation added.
  Descriptor-relative protection against concurrent same-user path changes remains open.
- Owner stdout and mirror buffers are bounded; owner blocked/partial output passes
  sanitizer tests. Real private mirror backpressure/ordered EOF drain passes;
  stalled EOF drain has a two-second deadline.
- Log/cache file hardening includes ancestor checks. `CETRA_DIAGNOSTICS=0` disables
  new log writes. Logging/cache remains synchronous; pathological filesystem stalls
  are an explicit remaining constraint, not a claim of bounded I/O latency.
- HID open failure still collapses absence, permissions and busy failures.
- Missing-battery counters saturate at two; UI rejects invalid battery domains.
  Hardware mask interpretation remains a research constraint.
- Host config is read by the existing cetra-status helper with a 1 MiB pre-output
  limit and three-second deadline. FileView only watches. Reads are serialized
  and stale generations rejected. User confirmed language/option persistence
  after the new reader was installed and the shell restarted.

## P3 — presentation and tests

- Unreleased opt-in microphone meter uses a libpulse peak helper. Exact
  source/link gates and self-exclusion pass offline tests. Live Discord capture
  produced one meter and a nonzero level; opting out removed its stream without
  stopping Discord. Marked call-exit and restart trials passed. This does not close
  native mute research.
- EasyEffects rerouting is prevented on the tested PipeWire 1.6.8/WirePlumber
  host by node.dont-move plus the Pulse flags. The helper checks the actual
  source and exits on mismatch. Live peaks, unchanged Discord routes and EOF /
  opt-out teardown passed; no manual EasyEffects exclusion was added. Physical
  USB recovery passed; ending the last admitted endpoint removed the meter even
  though the existing keepalive remained. See ACCEPTANCE-2026-09-14.md.
- Endpoint admission now excludes processing-only/keepalive routes. Live owned
  Production capture started one meter without call context; stopping it removed
  the meter despite the pre-existing keepalive. Real Discord call and restart
  acceptance passed with user-confirmed native Off/On prompts and level movement.
- Battery observation 2026-09-14: left battery became ff while fresh presence
  remained 11. UI separates availability from unknown percentage; reporting loss
  remains unresolved and no stale value is fabricated.

- Latest-owner USB/case transitions, all ANC levels/modes, voice settings and
  physical lighting effects were exercised. Evidence is in ACCEPTANCE-2026-09-14.md.
- Basic live keyboard traversal/activation/Escape and RGB editing passed by user
  confirmation. Full locale/RTL, screen-reader and rendered light/dark contrast
  remain incomplete. Warning contrast now has a theme-token fallback; never treat
  placeholder equality as fluent-human review.
- Test artifacts require `/tmp/opencode`; portable temporary roots are deferred.
- Offline QML tests inspect production functions, but do not prove Qt signal
  ordering or component ownership. Keep live checks after extraction/refactoring.

## Research, not release features

- No confirmed absolute native mute readback or host-native mute toggle.
- Proximity setting/readback is known; USB auto-pause delivery is unconfirmed.
  The UI/P/З shortcut remain absent; removal did not write the hardware setting.
- Runtime Off uses Static with black RGB and two commits; exact official sequence
  and need for duplicate commits require an annotated official capture.
- Ten EQ values have only eight documented frequencies. Complete the map before UI.
- Absence of a HAL/UI capability does not prove absence in every firmware path.

## Definition of done

1. Close P0 acceptance with evidence for the exact candidate.
2. Keep unknown, desired/requested and confirmed values distinct.
3. Preserve single HID ownership, verified opcodes and the screen-lock guard.
4. Run `./tests/run.sh`, `omarchy plugin validate .`, `git diff --check`.
5. Verify actual panel delivery and authorized lifecycle on the installed host.
6. Commit/push/tag/submit only with explicit publication authorization.
