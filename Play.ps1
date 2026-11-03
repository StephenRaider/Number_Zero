param()
$ErrorActionPreference='Stop'
$gameExe=Join-Path $PSScriptRoot 'build\NumberZero.exe'
if (-not (Test-Path -LiteralPath $gameExe)) { & (Join-Path $PSScriptRoot 'build.ps1') }
Start-Process -FilePath $gameExe -WorkingDirectory $PSScriptRoot -WindowStyle Hidden
