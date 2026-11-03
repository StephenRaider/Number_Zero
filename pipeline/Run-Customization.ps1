param(
    [string]$BlenderPath = 'D:\Blender\blender.exe',
    [string]$PackingPython = 'C:\Program Files\Blender Foundation\Blender 3.6\3.6\python\bin\python.exe'
)
$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath (Split-Path $PSScriptRoot -Parent)
try {
    foreach ($car in @('silvia','civic')) {
        & $BlenderPath -b --factory-startup --python-exit-code 1 --python pipeline/render_modular.py -- $car 8
        if ($LASTEXITCODE -ne 0) { throw "Modular render failed: $car" }
        & $PackingPython pipeline/pack_modular.py $car
        if ($LASTEXITCODE -ne 0) { throw "Modular packing failed: $car" }
    }
    & $PackingPython pipeline/build_customizer.py
    if ($LASTEXITCODE -ne 0) { throw 'Customizer build failed' }
    & $PackingPython pipeline/build_integrated_lab.py
    if ($LASTEXITCODE -ne 0) { throw 'Integrated Relight Lab build failed' }
    & $PackingPython pipeline/verify_modular.py
    if ($LASTEXITCODE -ne 0) { throw 'Modular verification failed' }
    & $PackingPython pipeline/verify_integrated.py
    if ($LASTEXITCODE -ne 0) { throw 'Integrated viewer verification failed' }
} finally { Pop-Location }
