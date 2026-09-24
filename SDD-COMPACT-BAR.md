# Compact Cetra bar indicator

Status: current earbud, case and microphone artwork accepted by the user on
2026-09-24, including the reduced bar scale and complete microphone capsule.
This contract supersedes the rejected icon studies and earlier silhouette D.

## Intent and invariants

Show two recognizable Cetra earbuds with independent bottom-to-top battery fill
and a fixed microphone capsule immediately to their right. Keep the 27 x 26
logical bar slot, tooltip percentages, panel actions, theme colors and low-charge
warning. Case charge never drives an earbud fill. No changes to USB, Bluetooth,
native mute, capture admission, helpers or saved preferences.

The panel uses the same family: a filled pair in the header, outlined left/right
buds beside their percentages and a rounded case with a lid seam and status mark.
The working sources are `CetraBarIndicator.qml` and the four SVGs in `assets/`.
Rejected design studies are not distributable assets.

## Geometry and behavior

- Scale the slot with `Style.bar.iconFont / 13`. Within it, place the 27 x 26
  artwork at (2.5, 2) and apply 0.85 of the slot scale. Hiding the microphone
  does not resize the slot or move the earbuds.
- Render the artwork into a 4x texture with smooth mipmapped reduction, matching
  the accepted softer appearance. Geometry and data bindings remain unchanged.
- The mirrored earbud paths use a 64-unit canvas, displayed at 19 units inside
  the artwork at (2.5, 3.75), with a 4-unit round outer stroke. Sound chambers
  are rounded, tips face inward and stems slope outward.
- Accept only finite numeric charge values in 0..100. Each side fills
  independently from y=55 toward y=8; this is linear height, not filled area.
  A subdued shell sits above nonzero charge. Measured zero stays hollow;
  missing/invalid values stay hollow with a small internal unknown dot.
- The upright microphone capsule occupies x=22.5..25.5 and y=6.5..19.5 before
  the artwork scale. Its outline and position never change with amplitude.
  No capture data: complete dim outline, transparent interior. Valid silence:
  bright empty outline. Positive signal: bottom-to-top fill with the existing
  square-root display curve and 90 ms easing. Neither silence nor missing data
  establishes native microphone mute. No central dash, gap or invented floor.
- Keep `showMicLevel`, accessible signal text and the existing action wiring.
  `CetraIcon.qml` recolors the white SVG masks with the current theme.

## Verification

Use the existing isolated Qt fixture to exercise independent and unequal charge,
zero versus unavailable/invalid data, quiet/loud/zero/unavailable signal, hidden
meter, fixed geometry and theme scaling. Inspect actual-size pixels; explicitly
request OpenGL to verify CurveRenderer antialiasing without accepting software
fallback. Offscreen fixtures are not live desktop interaction evidence.

Before committing run `./tests/run.sh`, `omarchy plugin validate .` and
`git diff --check`. Review the frozen change against `omarchy-plugin-patterns`
and Marketplace packaging rules; retain limits for unchanged native behavior.
The user authorized isolated Qt capture on 2026-09-24. The public preview uses
the real panel components and theme with illustrative values, identified in its
caption. It does not establish live hardware or desktop integration behavior.
Do not interrupt the active desktop to collect additional UI evidence.

## Delivery boundary

The 2026-09-24 request authorizes updating this repository on GitHub with the
accepted artwork and removal of obsolete assets. It does not authorize a new
Marketplace submission, release, tag or changes to desktop/audio configuration.
