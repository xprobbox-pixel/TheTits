#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-$project_dir/build}"
architecture="${2:-arm64}"
stage="$project_dir/output/macOS-0.3.1-$architecture-unsigned"
mkdir -p "$stage/Plugins/Components" "$stage/Plugins/VST3" "$stage/Applications" "$stage/Documentation"
ditto "$build_dir/TheTits_artefacts/Release/AU/The Tits.component" "$stage/Plugins/Components/The Tits.component"
ditto "$build_dir/TheTits_artefacts/Release/VST3/The Tits.vst3" "$stage/Plugins/VST3/The Tits.vst3"
ditto "$build_dir/TheTits_artefacts/Release/Standalone/The Tits.app" "$stage/Applications/The Tits.app"
for bundle in "$stage/Plugins/Components/The Tits.component" "$stage/Plugins/VST3/The Tits.vst3" "$stage/Applications/The Tits.app"; do
 codesign --force --deep --sign - "$bundle"
 codesign --verify --deep --strict "$bundle"
done
cp "$project_dir/README.md" "$stage/Documentation/"
cp "$project_dir/docs/TEST-REPORT.md" "$stage/Documentation/"
cp -R "$project_dir/docs" "$stage/Documentation/"
cp -R "$project_dir/licenses" "$stage/Documentation/"
cat > "$stage/INSTALL.txt" <<'TXT'
UNSIGNED LOCAL TEST BUILD. Not notarised. No developer certificate was supplied.
Quit your DAW. Copy Plugins/Components/The Tits.component to ~/Library/Audio/Plug-Ins/Components/.
Copy Plugins/VST3/The Tits.vst3 to ~/Library/Audio/Plug-Ins/VST3/.
Copy Applications/The Tits.app to your preferred Applications folder.
Restart your DAW and rescan plugins. AU identity: aufx TtBs HsRk.
Logic: Settings > Plug-in Manager. Ableton: Settings > Plug-ins; enable AU or VST3.
The standalone uses your system output. Drop an audio file onto its artwork to audition; Space pauses/resumes. No microphone input is opened.
Read Documentation/README.md for controls, licensing and known limitations.
TXT
ditto -c -k --sequesterRsrc --keepParent "$stage" "$stage.zip"
shasum -a 256 "$stage.zip" > "$stage.zip.sha256"
printf '%s\n' "$stage.zip"

"$project_dir/scripts/package-macos-pkg.sh" "$stage" "$architecture"
