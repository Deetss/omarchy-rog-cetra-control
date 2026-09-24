# Asset development notes

## Purpose

Contains static graphic assets for `io.github.pavellizunov.rog-cetra-control`.

## Asset Rules & Design Standards

1. **Symbolic SVG Only:**
   - Icons must follow the FreeDesktop / GNOME Symbolic icon specification.
   - Standard viewBox: `0 0 64 64`.
   - Stroke / Fill colors must be white (`#fff` or `#ffffff`).
   - Per-side earbud symbols use 4-unit outer strokes with round caps and joins; the bar uses the same contour.
   - Hardcoded UI hex colors (e.g. `#1a1a1a`, `#ff0000`) are strictly forbidden in SVG assets (enforced by `./tests/run.sh`).
   - Dynamic coloring is handled by `CetraIcon.qml` using
     `MultiEffect.colorizationColor`. Section components reuse CetraIcon;
     CSS is not part of this QML rendering path.

2. **Hardware Fidelity:**
   - The symbolic headset icon (`cetra-symbolic.svg`) must reflect the physical industrial design of ASUS ROG Cetra True Wireless SpeedNova:
     - In-ear silicone tips and sound chambers facing inward toward each other.
     - Rounded sound chambers and angled stems pointing downward and slightly outward.
     - The paired symbol is a clean white silhouette with restrained negative-space seams; per-side battery icons stay outlined so their percentages do not imply a full charge.
   - `cetra-case-symbolic.svg` uses a rounded charging case with a curved lid seam and a small status mark.
   - The bar microphone is drawn by `CetraBarIndicator.qml`; there are no separate microphone SVG assets.

3. **No Symlinks:**
   - Symlinks are strictly forbidden by Omarchy Plugin Marketplace rules.
   - All files must be regular files.
