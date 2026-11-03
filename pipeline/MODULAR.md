# Car customization and replacement wheels

Run `pipeline/Run-Customization.ps1` from the workspace. It uses Blender 5.2 for
geometry and the existing packing Python with NumPy/Pillow. The finished viewer
is `output/relight-lab.html`: customization now feeds its corrected maps into
the original shading, shadow, highlight and light-spill engine. The full browser
viewer includes eight wheel phases plus blur; the inline version uses a single
phase to stay under 1 MB. `output/customization-lab.html` remains a compact
secondary viewer, rather than replacing the full lab.
The earlier Before/After outputs remain available on disk.

Each car's `output/<car>-modular` folder contains:

- A prepared Blender scene with detached complete wheel units, named LF/LR/RF/RR.
  A wheel may contain several meshes/materials; rim and tyre form one visual unit.
- Six poses: front/rear, each left/straight/right. Body yaw is 7 degrees,
  body roll 1.4 degrees, and front wheel steering 18 degrees. These are actual
  geometry renders, not whole-sprite rotations. Rear wheels keep their axle direction.
- Separate body and wheel albedo, normals, depth/far depth, specular and emission
  maps, rendered at 512 square and packed to 128 square. Normals are filtered only
  across similar surfaces of the same material; wheel-edge colour averaging is
  weak and protects dark spoke gaps.
- Paint mask RGB: primary paint, Silvia lower paint, bonnet. Independent greyscale
  files are also exported in `masks`. Glass, lamps, plates and trim are excluded.
  EK9 bonnet comes from its named hood geometry; Silvia follows its connected
  bonnet panel, including its curved nose. Material guide passes retain alpha.
  Pixel colour and mask selection share material samples; paint-edge corrections
  require real subpixel coverage, preventing cream fringes on dark recolours.
- `wheels.json` with stock axle centers, pose angles and ride-height offsets.

The viewer depth-composites the body and wheels. Lowering translates only the body
down in world Z (0, -0.04441, -0.08882 metres), with the corresponding projected
pixel and depth offsets. Orthographic projection allows this without rerendering
normals. Pixel offsets are 0/1/2; wheel contact points remain fixed.
The front wheel steer and body lean are demonstration poses, not handling physics.
Black hood is independent from paint; the optional carbon pattern is a stylized
pixel treatment, not a physically simulated carbon-fibre material.
Headlamps, tails, brakes and hazards have separate controls and start off.

## Replacement wheel input

Supply a GLB containing ONE complete rim/tyre unit, axle along X, tyre circle in
YZ, outer visible face toward +X. Geometry may have multiple objects/materials.
The renderer bakes the hierarchy, centers the unit, scales its outside diameter to
the original tyre diameter, mirrors its facing for the opposite side, and places
four copies at the car's recorded axle centers. Original wheel width proportions
are retained from the supplied model. Verify unusually wide wheels for clipping.
Convert other source formats to that GLB convention in Blender first.

Example from the workspace (replace the model path):

```powershell
& 'D:\Blender\blender.exe' -b --factory-startup --python-exit-code 1 --python pipeline/render_modular.py -- silvia 1 --wheel-model 'D:\path\wheel.glb' --variant new-wheels
& 'C:\Program Files\Blender Foundation\Blender 3.6\3.6\python\bin\python.exe' pipeline/pack_modular.py silvia-modular-new-wheels
```

This produces an isolated variant folder, preserving the stock output. The body
keeps its original materials; replacement wheels use the same camera/map passes
and steering transforms. `1` is the spin-frame count: `8` renders eight 45-degree
spin phases as raw maps for game animation. The packer exports those phases and
a blur frame, and the full Relight Lab animates them in highway mode. Calipers
do not spin. The inline preview holds a static wheel phase.

The viewer currently offers the supplied stock wheels. Once new wheel models
arrive, render and inspect a variant before adding it to a wheel picker. The
rendering contract and variant outputs are ready; no unprovided wheel designs
are invented in the showcase.
