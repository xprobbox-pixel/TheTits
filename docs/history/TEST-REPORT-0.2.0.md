# The Tits 0.2.0 — verification, 29 October 2026

## Build and interface

Release builds of AU, VST3 and standalone were compiled with Apple Clang 17 and JUCE 8.0.6 on macOS 15.7.9 / M1 Pro. All three final binaries contain arm64 and x86_64 slices (verified with `lipo`). Deployment target is macOS 11; execution on macOS 11 was not tested.

The final interface exposes only Preset, Main Effect and Output. The standalone uses the same editor without the JUCE Options bar. The supplied mockup supplies the artwork, title and control textures; baked sample pointer/text are replaced with functional rendering. The expanded preset menu uses an 18-unit font, 25-unit rows, a 280-unit minimum width and one column. The needle is fully opaque, with an ivory outline and solid pink core. The central collar is anchored while the surrounding illustration moves. The animation has a stronger late-rising response at 100%.

Actual composed-editor PNGs cover 0/25/50/65/75/100%, 768×512 and 1536×1024 sizes, and a 2× capture. The final UI harness completed **1,782,463 checks with zero failures** (`output/ui-final-0.2.0.log`). This includes exactly three canvas controls, keyboard increment, double-click reset, bounded animation and stronger motion at 100%. A live standalone accessibility inspection also exposed only the three functional controls. Native popup interaction was not independently confirmed by the computer-use tool; popup sizing is specified in the compiled look-and-feel implementation.

## DSP and state

The full `--stress --render --screens` run completed **2,222,431 checks with zero failures** (`output/verification-0.2.0.log`, preserved summary `output/test-results-full-0.2.0.txt`). The final changes after that run affect only presentation; the final CTest DSP/state run also passed (1/1).

Coverage includes 44.1/48/88.2/96/192 kHz; mono/stereo; 1/32/64/127/512/2048-sample and changing blocks; sample-exact cleared-tail dry/bypass unity; all 24 actual factory recalls; invalid JSON rejection and host-state roundtrip; noise, sweeps, impulses and silence; rapid automation; transport stops, seeks, tempo and 3/4 timing; mono collapse; concurrent preset loading and processing; multiple instances; and bounded spring integration. The callback audit detects C++ `operator new` allocations, not every possible C-library allocation or OS interaction.

All 24 presets were rendered using an original synthetic dance loop, eight bars at 126 BPM followed by three seconds of reset/tail. Metrics cover 120 preset/rate combinations (`output/preset-metrics-all-rates.csv`). Maximum measured peak was 0.881677 (Dub Elevator, 88.2 kHz). Numerical bounds are not a listening assessment. The WAVs are in `output/`.

Measured six-slot process-call CPU averages were 1.42–1.59% of one core at 48 kHz and 2.78–2.87% at 96 kHz across 32/64/127/512-sample blocks. Largest measured block call was 0.374 ms. These local measurements exclude preparation and file I/O and do not establish hard real-time guarantees. Raw results: `output/cpu-six-slot-final.csv`.

## Host validation and packaging

Native arm64 Apple `auval` strict validation and ten simulated seconds of stress passed (`output/auval-0.2.0-arm64.log`). This validated the same 0.2.0 DSP before the last presentation changes. The VST3 manifest helper loaded the final native module and generated its module information. The x86_64 `auval` invocation could not locate the component in its registry; Intel AU-host validation is therefore unconfirmed.

The final x86_64 test executable ran under Rosetta and passed 1,782,458 checks with zero failures (`output/intel-check/intel-final.log`). This verifies the Intel DSP/state executable on this Mac, not an Intel DAW session.

The macOS package workflow builds selectable AU, VST3 and standalone components at the standard system locations. Bundles use ad-hoc signatures. The PKG is unsigned and is not notarised; Developer ID credentials were not available. Package contents and bundle signatures are checked without installing the system-wide PKG.

Windows build and Inno Setup scripts are supplied. No Windows compiler or SDK is available on this Mac, so no Windows EXE installer or Windows-host test is claimed. Listening, extended DAW sessions in Logic/Ableton, pluginval, Intel AU-host acceptance and older-macOS acceptance remain unperformed. Public distribution also requires final artwork/application licensing and appropriate JUCE terms as documented in README.
