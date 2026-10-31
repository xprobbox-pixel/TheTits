# The Tits 0.3.1 — verification, 30 October 2026

This revision changes the editor and standalone window layout. The DSP, factory bank and parameter/state schema are unchanged from 0.3.0. The detailed preceding DSP/host results are preserved in [history/TEST-REPORT-0.3.0.md](history/TEST-REPORT-0.3.0.md).

The editor now opens at 768×512, down from 1152×768. Both plugin and standalone remove their fixed aspect-ratio constraints and allow independent width/height changes between 480×360 and 2560×1600. The plugin exposes a corner resize handle. Artwork and dial scale proportionally inside the available window; extra space uses the yellow background. Output remains at a readable fixed size at the bottom right. The version text reads the build's `JucePlugin_VersionString` and appears at the bottom left.

The closed pink selector is reduced from 558×155 to 380×102 in the artwork coordinate system, in addition to the smaller default window. Its expanded list uses two columns, 13-unit text and 20-unit rows. Popup scaling is independent of artwork scaling and the popup is constrained to the editor, preserving readability and reducing the expanded menu height.

UI verification captures the actual composed editor at 0/25/50/65/75/100%, at 480×360, 1100×450, 600×900, 768×512 and 1536×1024, and at 2× pixel scale. Checks cover default dimensions, absence of a fixed aspect ratio, an unclipped circular dial across wide/tall/minimum layouts, and keyboard/double-click behavior. The expanded real preset menu is captured within the editor. The only effect controls remain Preset, Main Effect and Output; the resize handle is a window affordance.

The final desktop-enabled `--screens --stills --output=output/0.3.1` run passed **1,783,309 checks with zero failures** (`output/verification-0.3.1.log`). The small/wide/tall and expanded-menu captures were visually inspected. A sandboxed offscreen attempt initially had no macOS display information; JUCE's popup placement code dereferenced a missing display. The harness now skips native popup capture when no display exists; the desktop-enabled run verified the real popup. `--stills` omits the unchanged 120-frame animation export while retaining still captures and behavior checks.

The run also exercises the existing DSP/state/preset-diversity suite. No DSP or factory audio changes were made in 0.3.1. The broader five-rate stress/render results and AU validation in the archived 0.3.0 report apply to that DSP revision; a new DAW acceptance or 0.3.1 AU validation is not claimed here.

Universal arm64/x86_64 Release AU, VST3 and standalone bundles are rebuilt as 0.3.1. Packaging verifies component version, architecture slices, ad-hoc signatures, destinations and checksums. The PKG is unsigned and not notarised. Windows scripts are updated to 0.3.1, but no Windows compiler/SDK is available on this Mac.

The guarded offscreen run also passed 1,783,309 checks with zero failures (`output/verification-0.3.1-headless.log`), confirming that UI capture remains usable without desktop display access.
