#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
stage="${1:?Usage: package-macos-pkg.sh staged-archive-folder architecture}"
architecture="${2:-universal}"
work="$project_dir/output/pkg-work-0.3.1-$architecture"
components="$work/components"
mkdir -p "$components" "$work/au/Library/Audio/Plug-Ins/Components" "$work/vst3/Library/Audio/Plug-Ins/VST3" "$work/app/Applications" "$work/app/Library/Application Support/House Rocks/The Tits"
ditto "$stage/Plugins/Components/The Tits.component" "$work/au/Library/Audio/Plug-Ins/Components/The Tits.component"
ditto "$stage/Plugins/VST3/The Tits.vst3" "$work/vst3/Library/Audio/Plug-Ins/VST3/The Tits.vst3"
ditto "$stage/Applications/The Tits.app" "$work/app/Applications/The Tits.app"
ditto "$stage/Documentation" "$work/app/Library/Application Support/House Rocks/The Tits/Documentation"
for part in au vst3 app; do
 case "$part" in
  au) bundle_path="Library/Audio/Plug-Ins/Components/The Tits.component" ;;
  vst3) bundle_path="Library/Audio/Plug-Ins/VST3/The Tits.vst3" ;;
  app) bundle_path="Applications/The Tits.app" ;;
 esac
 cat > "$work/$part-components.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><array><dict>
<key>RootRelativeBundlePath</key><string>$bundle_path</string>
<key>BundleIsRelocatable</key><false/>
<key>BundleIsVersionChecked</key><true/>
<key>BundleHasStrictIdentifier</key><true/>
<key>BundleOverwriteAction</key><string>upgrade</string>
</dict></array></plist>
PLIST
 pkgbuild --component-plist "$work/$part-components.plist" --root "$work/$part" --identifier "audio.houserocks.thetits.$part" --version 0.3.1 --ownership recommended --install-location / "$components/$part.pkg"
done
cat > "$work/Distribution.xml" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
 <title>The Tits 0.3.1</title>
 <options customize="always" require-scripts="false" rootVolumeOnly="true"/>
 <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
 <choices-outline><line choice="au"/><line choice="vst3"/><line choice="app"/></choices-outline>
 <choice id="au" title="Audio Unit" description="AU effect for Logic and other Audio Unit hosts." start_selected="true"><pkg-ref id="audio.houserocks.thetits.au"/></choice>
 <choice id="vst3" title="VST3" description="VST3 effect for Ableton and other VST3 hosts." start_selected="true"><pkg-ref id="audio.houserocks.thetits.vst3"/></choice>
 <choice id="app" title="Standalone application" description="Three-control app with drag-and-drop audio-file audition." start_selected="true"><pkg-ref id="audio.houserocks.thetits.app"/></choice>
 <pkg-ref id="audio.houserocks.thetits.au" version="0.3.1" auth="Root">au.pkg</pkg-ref>
 <pkg-ref id="audio.houserocks.thetits.vst3" version="0.3.1" auth="Root">vst3.pkg</pkg-ref>
 <pkg-ref id="audio.houserocks.thetits.app" version="0.3.1" auth="Root">app.pkg</pkg-ref>
</installer-gui-script>
XML
installer="$project_dir/output/TheTits-0.3.1-macOS-$architecture-unsigned.pkg"
productbuild --distribution "$work/Distribution.xml" --package-path "$components" "$installer"
pkgutil --check-signature "$installer" || test "$?" -eq 1
shasum -a 256 "$installer" > "$installer.sha256"
printf '%s\n' "$installer"
