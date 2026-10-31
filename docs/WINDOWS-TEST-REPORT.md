# Windows x64 installer — 0.3.1, 31 October 2026

The Windows installer was compiled successfully using native Visual Studio 2022 / MSVC 19.44.35229 on GitHub's `windows-2022` runner. The build uses a static MSVC runtime, builds the standalone application and VST3 plugin, runs CTest, then invokes Inno Setup. No cross-compiled or renamed macOS binary is involved.

- Source commit: `9f3b8867eeaf23d92d7ecd84298cfa4c901231c0`.
- [Successful build](https://github.com/xprobbox-pixel/TheTits/actions/runs/41221814559).
- CTest: **1/1 passed**, 9.94 seconds.
- Native DSP/state verification: **1,783,296 checks, zero failures**.
- Includes preset-pair audio comparisons, all 24 factory recalls, dry/bypass unity across five rates and mono/stereo buffer combinations, transport/state/JSON checks, and callback C++ allocation auditing.
- Inno Setup compilation succeeded. Installer contains the Windows x64 VST3 and standalone application, README, documentation and license notices.
- Installer: `TheTits-0.3.1-Windows-x64-unsigned.exe`.
- SHA-256: `4dd144dad6e383e4f4e42581b67bce8b9381c8d4b5b5d60838303b95a4d88f72`.

Both downloaded GitHub artifact archives matched their published SHA-256 digests. The extracted installer matched the checksum produced on the Windows runner, and its PE header was checked locally. Complete build logs and downloaded test reports are saved under `output/`.

The installer is unsigned. Native Windows DAW acceptance and an installed application's GUI have not been tested; the successful native module build, test executable and installer compilation do not substitute for those checks. The Windows artifact was built before this documentation-only update; it contains the preceding documentation snapshot.
