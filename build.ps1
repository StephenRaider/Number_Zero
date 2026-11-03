param([switch]$Run,[switch]$Test)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$compiler=Join-Path $root '..\toolchain\w64devkit\bin\g++.exe'
if (-not (Test-Path -LiteralPath $compiler)) { $compiler=(Get-Command g++ -ErrorAction Stop).Source }
Push-Location -LiteralPath $root
try {
    New-Item -ItemType Directory -Path build -Force | Out-Null
    & $compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic -Ithird_party/raylib/include src/main.cpp -o build/NumberZero.exe -Lthird_party/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
    if ($Test) { & $compiler -std=c++17 -O2 -Wall -Wextra -Isrc tests/vehicle_tests.cpp -o build/vehicle_tests.exe; if ($LASTEXITCODE -ne 0) { throw 'Vehicle test build failed' }; & .\build\vehicle_tests.exe; if ($LASTEXITCODE -ne 0) { throw 'Driving model tests failed' } }
    if ($Test) { & $compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc tests/audio_tests.cpp -o build/audio_tests.exe; if ($LASTEXITCODE -ne 0) { throw 'Audio test build failed' }; & .\build\audio_tests.exe; if ($LASTEXITCODE -ne 0) { throw 'Engine audio tests failed' } }
    if ($Run) { & .\build\NumberZero.exe }
} finally { Pop-Location }
