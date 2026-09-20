# Bluetooth update acceptance — 2026-09-20

Runtime source: `e494e055077ed853690dd10797c878f4f14ac235` on main. Candidate 1.8.0
changes only manifest metadata and release/evidence documents from that runtime.
The user authorized Bluetooth integration and an update after 1.7.0 acceptance.
This document records tests on one ROG Cetra True Wireless SpeedNova pair.

## Marked device trial

USB was absent in the owner status cache for each observation. The user reported
headset voice prompts after gestures. The agent opened the installed panel for
screenshots; neither used Refresh in the requested test sequence. Opening a ready
panel can initiate a read, so these observations do not time uninterrupted polling.

| Moscow time | User action/prompt | Observed installed panel | L / R / case |
| --- | --- | --- | --- |
| 14:52 | ANC after gesture | ANC |89 /88 /89 |
| 14:57 | Ambient Sound | Ambient |87 /88 /89 |
| 15:00 | Noise Cancelling Off | Off |87 /87 /89 |
| 15:05 | Both earbuds in closed case | No data; automatic retry |unknown /unknown /unknown |
| 15:07:53 | Returned to ears; connection reported | Still unavailable |unknown /unknown /unknown |
| 15:08:37 | No refresh action | Loading telemetry |unknown /unknown /unknown |
| 15:09:07 | No refresh action | Off; valid Bluetooth report |87 /89 /95 |

The case-cycle trial removed the audio node and disconnected the separate BLE
record, but the main BlueZ Connected property stayed true in the sampled states.
It is therefore a telemetry-loss/recovery trial, not proof of a full Bluetooth
link disconnect/reconnect. The panel cleared previous numbers and recovered
without manual Refresh. The case charging flag was reported true after recovery;
physical charging and measurement freshness were not independently confirmed.

The system Bluetooth menu showed separate same-name records: audio Battery1=0,
BLE Battery1=89. The plugin displayed its separate vendor report 89/88/89. System
Battery1 did not replace a missing vendor field. The BLE percentage's physical
owner and freshness are unverified.

## Software evidence

- The runtime aggregate suite passed after the automatic retry change, including
  production native parser/getter packets, real Qt component bindings, failure
  backoff, cancellation, watchdog, stale reports, USB priority and single-flight.
- In an isolated original 3c8cc8d component, the new Qt scheduling assertion failed
  because a failed request left its retry timer stopped. The changed component
  passed. No live runtime files were reverted for this comparison.
- Independent Gemini R3 reviewed the retry diff and tests and found no concrete
  defect. The coordinator checked source and test results. This scoped review
  is not a security audit or proof about every hardware state.
- Guarded setup and one authorized shell restart succeeded. The USB panel showed
  94/95/89 and Ambient; one cetra-watch owner, valid JSON cache and continuing logs
  were observed. No Cetra QML error was found in the inspected shell logs.
- Final 1.8.0 candidate gates passed: `./tests/run.sh` (including QML lint),
  `omarchy plugin validate .`, `git diff --check` and tracked-file/symlink checks.
  Independent Gemini R4 found no release-document blocker. The coordinator also
  clarified Internet versus Bluetooth sockets and USB-specific expiry in README.
  Only evidence/release-document updates followed the aggregate run.

## Remaining limits

Reported battery values changed; physical measurement accuracy, case freshness,
energy use and the earlier vendor-channel failure's root cause remain unproven.
The update preserves nullable last-reported data and bounded retries; it does not
claim to fix firmware, every connection failure or the historical double prompt.
Bluetooth ANC is read-only. Native microphone mute remains unknown. Bluetooth
capture/profile switching, live suspend/resume, a second monitor, comprehensive
accessibility and fluent-human review of all locales remain unverified/deferred.
The user previously deferred suspend and multimonitor acceptance. No changed
read-only telemetry behavior is advertised as providing these capabilities.
