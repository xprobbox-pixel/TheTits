param([switch]$SkipInstaller)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $projectDir 'build-windows'
$outputDir = Join-Path $projectDir 'output'

# Use the static MSVC runtime so a clean PC does not need a separate VC++ runtime install.
cmake -S $projectDir -B $buildDir -G 'Visual Studio 17 2022' -A x64 '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>'
if ($LASTEXITCODE) { throw "CMake configure failed: $LASTEXITCODE" }
cmake --build $buildDir --config Release --parallel 4
if ($LASTEXITCODE) { throw "Windows compilation failed: $LASTEXITCODE" }
ctest --test-dir $buildDir -C Release --output-on-failure
if ($LASTEXITCODE) { throw "Windows verification failed: $LASTEXITCODE" }
if ($SkipInstaller) { return }

$compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue
$iscc = if ($compiler) { $compiler.Source } else { Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe' }
if (!(Test-Path $iscc)) {
    $iscc = Join-Path $env:ProgramFiles 'Inno Setup 7\ISCC.exe'
}
if (!(Test-Path $iscc)) { throw 'Install Inno Setup 6.3 or later, or put ISCC.exe on PATH.' }
& $iscc (Join-Path $PSScriptRoot 'windows-installer.iss')
if ($LASTEXITCODE) { throw "Installer compilation failed: $LASTEXITCODE" }
$installer = Join-Path $outputDir 'TheTits-0.3.1-Windows-x64-unsigned.exe'
if (!(Test-Path $installer)) { throw "Expected installer not produced: $installer" }
$hash = (Get-FileHash $installer -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $(Split-Path $installer -Leaf)" | Set-Content "$installer.sha256" -Encoding ascii
Write-Host "Windows installer: $installer"
