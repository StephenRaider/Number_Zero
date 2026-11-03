param(
    [string]$BlenderPath = 'D:\Blender\blender.exe',
    [string]$PackingPython = 'C:\Program Files\Blender Foundation\Blender 3.6\3.6\python\bin\python.exe'
)
$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath $PSScriptRoot
try {
    $pipelinePython = $PackingPython
    if (-not (Test-Path -LiteralPath $pipelinePython)) { throw "Packing Python was not found: $pipelinePython" }
    foreach ($car in @('silvia', 'civic')) {
        & $BlenderPath --background --factory-startup --python-exit-code 1 --python pipeline/render_cars.py -- $car 8
        if ($LASTEXITCODE -ne 0) { throw "Rendering failed: $car" }
        & $pipelinePython pipeline/pack_cars.py $car
        if ($LASTEXITCODE -ne 0) { throw "Packing failed: $car" }
        & $BlenderPath --background --factory-startup --python-exit-code 1 --python pipeline/render_cars.py -- $car 8 --detailed-lamps
        if ($LASTEXITCODE -ne 0) { throw "Detailed lamp rendering failed: $car" }
        & $pipelinePython pipeline/pack_cars.py "$car-detailed"
        if ($LASTEXITCODE -ne 0) { throw "Detailed lamp packing failed: $car" }
    }
    & $pipelinePython pipeline/build_viewer.py
    if ($LASTEXITCODE -ne 0) { throw 'Viewer build failed' }
    & $pipelinePython pipeline/verify_outputs.py
    if ($LASTEXITCODE -ne 0) { throw 'Output verification failed' }
    & $pipelinePython pipeline/verify_cleanup.py
    if ($LASTEXITCODE -ne 0) { throw 'Cleanup verification failed' }
    & $pipelinePython pipeline/verify_wheels.py
    if ($LASTEXITCODE -ne 0) { throw 'Wheel cleanup verification failed' }
    & $pipelinePython pipeline/verify_lamps.py
    if ($LASTEXITCODE -ne 0) { throw 'Detailed lamp verification failed' }
    & (Join-Path $PSScriptRoot 'pipeline/Run-Customization.ps1') -BlenderPath $BlenderPath -PackingPython $PackingPython
    & $pipelinePython pipeline/bundle_outputs.py
    if ($LASTEXITCODE -ne 0) { throw 'Sprite packaging failed' }
} finally { Pop-Location }
