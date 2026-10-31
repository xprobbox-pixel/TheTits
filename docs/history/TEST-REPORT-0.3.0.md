# The Tits 0.3.0 — verification, 29 October 2026

## Preset redesign

All 24 factory presets have distinct ordered chains. Only two use a high-pass filter. The bank includes pure high-pass and low-pass sweeps, large reverb washes, exposed tape/ping-pong delays, full-band bit/rate reduction, held forward/reverse loops, accelerating stutters, gating, stereo movement, chorus, resonant phasing, positive/negative flanging, generators and six-stage combinations. See [PRESET-GUIDE.md](PRESET-GUIDE.md).

Flanger is a real interpolated millisecond delay/comb effect with filtered feedback and selectable polarity, appended as module 18. Phaser now has a frequency sweep, adjustable depth/feedback and stereo offset. Stutter reaches division 5 (1/128-note slices); held-loop presets retain a fixed division. Wet-return calibration for delay (mapping 6, dB) and reverb (mapping 4, dB) preserves the calibration during reset tails. Existing zero-valued mappings retain their previous unity trim. Factory room returns use -6 dB and Dub Elevator's tape return uses -9 dB. The visible interface still contains only Preset, Main Effect and Output.

Preset names/order and earlier module numbers are preserved. Saved DAW sessions retain their saved chain/mappings: select a factory preset explicitly to hear the new bank. The underlying Phaser DSP has changed; parameter/state compatibility does not promise identical phaser audio to 0.2.0.

## Final tests

The final arm64 `--stress --render --screens --output=output/0.3.0` run passed **2,223,365 checks with zero failures** (`output/verification-0.3.0.log`). It covers 44.1/48/88.2/96/192 kHz, mono/stereo, changing 1/32/64/127/512/2048-sample blocks, sample-exact cleared-tail dry/bypass unity, state/JSON validation and recall, transport/timing changes, concurrent preset publication, multiple instances, output bounds, automation and callback C++ `operator new` auditing. The allocation audit does not interpose every C-library allocation or OS interaction.

The new regression checks compare **all 276 preset pairs** using actual processor output at fixed **25%, 60% and 100%** effect. Every capture uses the same synthetic source, tempo and warm-up. Each capture is RMS-normalized before comparison so a gain change alone cannot make two presets pass as different. The required aggregate normalized waveform distance is greater than 0.15; the closest final pair measured **0.474140** (Disco Lift / Digital Meltdown). This is a numerical duplicate-regression check on one source, not a psychoacoustic or genre-wide musical assessment. Recalled processor module identities are also checked against factory configurations. Dedicated impulse tests verify short flanger comb taps, polarity reversal and feedback repeats.

All 24 presets were rendered as eight bars at 126 BPM plus three seconds of reset/tail. Metrics span **120 preset/rate combinations**, all below the 0.95 sample-peak threshold at default output on the reference loop. The maximum was **0.641900** (White Heat, 192 kHz). This provides headroom on the test source; arbitrary input/output gains can still clip. WAVs, metrics, 276 pair comparisons and editor/animation captures are in `output/0.3.0/`.

Final universal-build CTest passed **1/1** (`output/ctest-0.3.0.log`). The x86_64 executable passed **1,783,296 checks with zero failures** under Rosetta (`output/verification-0.3.0-intel.log`). Native Apple `auval` verified **component version 0.3.0**, passed strict AU validation and ten simulated seconds of render stress (`output/auval-0.3.0-arm64.log`). The temporary per-user validation component was removed from that location afterward, preserving the existing system installation.

Six-slot process-call timings on this M1 Pro / macOS 15.7.9 / Apple Clang 17 Release build averaged **1.45–1.51% of one core at 48 kHz** and **2.90–3.02% at 96 kHz**, across 32/64/127/512-sample blocks. The largest measured block call was **0.256 ms**. Timing excludes preparation/file I/O and is not a hard real-time guarantee.

## Build, artifacts and limits

Final AU, VST3 and standalone Release binaries are universal arm64/x86_64, with macOS 11 deployment target and JUCE 8.0.6. The VST3 manifest helper loads the native module. Packaging verifies ad-hoc bundle signatures, both architecture slices, component version, non-relocatable standard destinations, selectable installer choices and checksums. The PKG is unsigned and not notarised; no Developer ID credentials are available. System-wide installer execution is left to the user.

`output/0.3.0/variation-audition.wav` contains the original synthetic dry source and 12 rendered preset excerpts. `AUDITION-GUIDE.md` lists timestamps and the comparison-only loudness adjustments. These adjustments are not part of the plugin. No listening test by the agent or DAW acceptance is claimed for this revision; the user's prior DAW feedback motivated the redesign. Intel AU-host, pluginval, older-macOS and Windows acceptance remain unperformed. Windows build/Inno Setup scripts are supplied; this Mac has no Windows compiler/SDK. Licensing/signing requirements remain documented in README.
