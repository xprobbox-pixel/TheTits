; Inno Setup 6. Compile after a Release x64 CMake build.
#define AppName "The Tits"
#define AppVersion "0.3.1"
[Setup]
AppId={{A048DBB8-F9AD-4C91-99AD-7FE25D4713B1}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=House Rocks
DefaultDirName={autopf}\House Rocks\The Tits
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=TheTits-0.3.1-Windows-x64-unsigned
OutputDir=..\output
UninstallDisplayIcon={app}\The Tits.exe
[Files]
Source: "..\build-windows\TheTits_artefacts\Release\Standalone\The Tits.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build-windows\TheTits_artefacts\Release\VST3\The Tits.vst3\*"; DestDir: "{commoncf64}\VST3\The Tits.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"
Source: "..\docs\*"; DestDir: "{app}\docs"; Flags: recursesubdirs createallsubdirs
Source: "..\licenses\*"; DestDir: "{app}\licenses"; Flags: recursesubdirs
[Icons]
Name: "{group}\The Tits"; Filename: "{app}\The Tits.exe"
