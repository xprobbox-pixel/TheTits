# The Tits — 0.3.1

A JUCE/C++ audio transition effect with the supplied hot-pink poster design. The editor has exactly three controls: **Main Effect**, **Preset**, and **Output**. The category selector, favourites/STAR, PANIC, advanced panel, settings, trigger, reset, bar controls, mix, bypass and file buttons have been removed from the UI. The standalone also uses this same editor without JUCE's extra Options/audio-settings bar.

## Use

- Choose one of 24 original factory presets with the glossy pink selector. The smaller selector opens a compact two-column list with 13-unit text and 20-unit rows. The menu stays readable independently of the artwork scale. The first displays as "Jiggle Nipples" to match the mockup; its underlying original factory design is Disco Lift.
- Drag or scroll the central EFFECT dial. Arrow keys adjust a focused dial in 0.1% steps; double-click returns to zero. The dial controls the host-automatable `build` parameter.
- Adjust OUTPUT from -24 to +6 dB; double-click restores unity gain.

The reference's title, bust illustration and selector texture are embedded. The working dial replaces the reference's baked example pointer/value, with matching concentric pink rims, peach face, tick marks and percentage. The needle is fully opaque, with a bright outline and a solid hot-pink indicator for visibility. The artwork moves independently on both sides, anchored around the central dial. A late-rising animation boost makes 100% effect visibly stronger than 75%, while zero settles naturally. Animation uses effective audio intensity, including DAW automation.

The editor opens at **768×512** and resizes freely in width and height, from 480×360 to 2560×1600 logical pixels. A corner resize handle is available in plugin hosts. The artwork and circular dial scale proportionally inside the window; extra space uses the yellow background. The output slider stays readable at the bottom right, and the compiled version number appears at the bottom left. The standalone also has no fixed aspect ratio.

## Standalone audition

The application uses the default system audio output and opens **no microphone input**. Drop a WAV, AIFF, FLAC, MP3 or Ogg file onto the artwork to loop it through the effect. Space pauses/resumes file playback. Read-ahead runs on a worker thread; source replacement stops callbacks before changing file ownership. There are no extra visible playback controls. Siren/noise presets can also be auditioned without a file. In a DAW, insert the AU/VST3 on an audio track or bus normally.

## Factory bank 0.3.0

The 24 presets were rebuilt around distinct effect chains: pure high-pass and low-pass sweeps, reverb washes, exposed tape/ping-pong throws, full-band crushing, held forward/reverse loops, accelerating stutters, chopping, stereo movement, chorus, phasing, positive/negative flanging, generators and six-stage combinations. [Preset guide](docs/PRESET-GUIDE.md) lists every chain and its behavior. The UI still has only three controls.

Existing DAW sessions retain their saved chain and mappings. To hear this bank in an older session, explicitly choose a factory preset after installing 0.3.0. Preset names/order and previous module IDs are preserved; Flanger is appended as module 18. The macro and output positions are preserved when selecting a preset.

## DSP

Six ordered, preallocated effect slots support HP/LP/BP filtering, actual three-band shelf/bell EQ, stereo/ping-pong delay, tape echo, algorithmic room reverb, forward/reverse stutter capture, tremolo/gate, autopan, chorus, resonant phaser, positive/negative-feedback flanging, bit/rate degradation, compensated soft drive, tuned procedural sirens, band-limited white/pink-style noise and gain/width utility.

Factory presets contain actual slot settings and macro mappings in `Resources/factory.json`; `scripts/make_presets.py` reproduces them. BUILD controls coordinated parameter curves, rather than merely wet gain. Factory recall preserves effect position and global output. The plugin remains an audio effect, supports mono and stereo matching input/output layouts, and reports zero latency. At zero effect, default output and cleared tails, dry audio is sample-exact unity. Tails stop receiving new input and decay; generators fade out on reset-to-zero or timed-host stop. Worst-case declared tail length is 420 seconds for bounded user delay settings; factory tails are much shorter.

Tempo-sensitive modules follow host BPM, PPQ and meter when supplied; missing timing uses a 126 BPM, 4/4 clock. Delay time changes pitch-glide smoothly. Stutter captures continuously maintained history into preallocated ping-pong banks and crossfades musical division changes; reverse is not time stretching. Reverb is JUCE's real room model with size/damping/spread/predelay, with no independent measured decay-time claim. Bit/rate reduction deliberately aliases; tanh drive is not oversampled. Delay feedback and drive include 20 Hz DC-removal high-pass filtering. Output protection, where retained in an older host state, is a soft clipper and does not claim true-peak limiting.

Existing fixed host parameter IDs and versioned state are retained for session compatibility, including former advanced/performance parameters. Those controls have no editor surface. New sessions default to Manual mode and full global MIX. The dry/processed crossfade is linear, with smoothed gain. The effect never depends on the UI timer. Cached parameter atomics and generation-checked configuration snapshots cross into the audio callback; no DSP objects are allocated or destroyed during a chain change. Ring buffers are owned by the processor and allocated in prepare; clearing uses generation stamps. All graph changes use a controlled wet fade and reset. The UI timer stops when its editor is closed.

## Build

Pinned **JUCE 8.0.6**, **C++17**, CMake ≥3.24, macOS deployment target 11.0. No cloud processing, login, external samples or paid runtime. JUCE is downloaded only at build time if absent; pass `-DJUCE_DIR=/path/to/JUCE-8.0.6` to use an existing checkout. Download SHA-256: `95752f8e8b37d802b4c10182bc757de7d88f0c8899e879afa8798589be306a11`.

From this project folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
cmake -S . -B build-universal -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build-universal --parallel 6
# Debug:
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-debug --parallel 6
```

In this workspace CMake/CTest are available as `../.tools/bin/cmake` and `../.tools/bin/ctest`. Apple command-line tools are sufficient; Xcode's GUI is not required. Binaries are in `build[-universal]/TheTits_artefacts/Release/{AU,VST3,Standalone}`.

Windows: Visual Studio 2022 C++ desktop tools, Windows SDK and CMake. Run `scripts/build-windows.ps1` to configure Visual Studio 2022 x64 with static CRT, build, run CTest and compile the installer with Inno Setup. Use `-SkipInstaller` to build/test only. Inno Setup 6.3+ must be installed or ISCC.exe placed on PATH. The GitHub Actions workflow `.github/workflows/windows-installer.yml` runs this pipeline on Windows and uploads the EXE and SHA-256 checksum. For Debug use `cmake --build build-windows --config Debug`. The native Windows x64 build and installer were successfully compiled and tested on GitHub Actions; see [Windows test report](docs/WINDOWS-TEST-REPORT.md).

## Installers and installation

```sh
./scripts/package-macos.sh build-universal universal
```

This builds an actual **unsigned macOS PKG installer** with selectable AU, VST3 and standalone components, plus a portable ZIP. The PKG installs to `/Library/Audio/Plug-Ins/Components`, `/Library/Audio/Plug-Ins/VST3`, and `/Applications`. Installation uses the standard administrator prompt. Documentation/licenses go under `/Library/Application Support/House Rocks/The Tits/Documentation`.

For manual per-user installation from the ZIP: quit the DAW, copy the component to `~/Library/Audio/Plug-Ins/Components/`, the VST3 to `~/Library/Audio/Plug-Ins/VST3/`, and the app to your Applications folder. Restart/rescan your DAW. Logic: Settings → Plug-in Manager; Ableton: Settings → Plug-ins. Windows installer targets `Common Files/VST3/The Tits.vst3` and `Program Files/House Rocks/The Tits`.

Bundles have local ad-hoc signatures, **not Developer ID signatures or notarisation**. The installer is unsigned. Distribution signing/notarisation needs the owner's certificates and credentials. Stable IDs: House Rocks / `HsRk`, The Tits / `TtBs`, bundle `audio.houserocks.thetits`. Public release requires the owner's identifier and licensing clearance. No VST2 SDK dependency exists.

## Verification

```sh
./build-universal/TheTitsTests_artefacts/Release/TheTitsTests --stress --render --screens
ctest --test-dir build-universal --output-on-failure
auval -v aufx TtBs HsRk -strict  # after installing the AU
```

The harness checks five rates (44.1/48/88.2/96/192 kHz), mono/stereo, changing 1/32/64/127/512/2048-sample blocks, dry/bypass unity, finite/bounded output, noise/sweeps/impulses/silence, automation, transport changes, state validation/recall, concurrent preset changes, callback C++ allocation auditing, mono collapse and spring stability. It also compares all 276 factory-preset pairs using RMS-normalized audio captured at 25%, 60% and 100%, and checks positive/negative millisecond comb taps from the real flanger. This detects near-duplicate processing beyond gain differences; listening remains necessary to assess musical character. The editor check checks the preset, effect and output controls, verifies keyboard/double-click behavior, tests stronger high-intensity motion, and captures the actual composed editor at 0/25/50/65/75/100%, small/large sizes and 2× resolution. A 120-frame animation capture is generated.

`--render` writes exact eight-bar 126 BPM builds and three seconds of reset/tail on an original synthetic dance loop for all 24 presets. Peak/RMS metrics and six-slot process-call timings are recorded in `output/`. Listening tests and broad DAW/Windows acceptance are separate release tasks; numerical tests do not establish musical quality. `docs/TEST-REPORT.md` records the actual final results and limitations.

## Artwork, dependencies and licensing

The supplied 1536×1024 mockup is embedded as `Resources/poster.png`. The bust is rendered through a fitted, shared-vertex textured mesh; the central dial collar stays anchored and the background is reconstructed behind moving regions. The title remains static. This uses the original brief's mesh fallback; no unrelated artwork or ellipse replacements are used for the illustration. Production-layer extraction was rejected by the image service. Contour/texture cleanup remains an artist finishing option; exact working controls require the baked sample pointer/text to be replaced.

JUCE 8 modules are dual licensed **AGPLv3 / JUCE commercial licence**; no owner commercial licence is assumed. Full pinned-source notices are in `licenses/`, including the bundled VST3 SDK and AudioUnitSDK. Closed-source distribution needs a suitable JUCE licence; open-source distribution must comply with its applicable terms. The existing Roboto face is SIL OFL 1.1. Roboto Bold is from Google's roboto-2 repository under Apache 2.0; both full notices are included. Other bundled codec/font dependencies keep their notices in the pinned JUCE source. Rights to the supplied illustration remain with its owner. Choose the final application-source distribution licence before public release.

Original workflow research: [Dada Life plugins](https://dadalife.com/plugins/?id=endless-smile) and [Baby Audio Transit](https://babyaud.io/transit). This project uses original DSP and factory designs, with the user's supplied visual identity; it does not reproduce their proprietary algorithms, presets or branding.
