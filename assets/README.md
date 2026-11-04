# Runtime car assets

`cars/catalog.json` indexes the committed cars. Each car has a `manifest.json`
listing all runtime PNG paths, six body/view poses and nine stock-wheel frames
per pose. File SHA-256 checksums allow exported textures to be verified.

## Texture contract

All textures use a 128 x 128 canvas, a shared 26 pixels/metre orthographic scale,
30-degree camera elevation and ground anchor (64, 79). Use each car's metadata.

- `albedo`: sRGB colour and binary alpha.
- `normal`: R = normal X high byte, G = X low byte, B = normal Y high byte.
- `pack`: R/G = front depth high/low bytes, B = specular strength.
- `pack2`: R = normal Y low byte, G/B = far depth high/low bytes.
- `emit`: R = headlamps, G = rear lamps, B = indicators; channels are masks,
  not display colours. Light controls must independently select their intensity.
- `mask`: R = primary paint, G = Silvia lower-body paint, B = bonnet.
  Separate greyscale paint and bonnet masks are included as well.

Alpha is the sprite coverage. Packed normal/depth bytes must be treated as data,
not sRGB colour. Decode unsigned 16-bit values as (high * 256 + low) / 65535.
Normals X/Y map from [0,1] to [-1,1]; reconstruct positive camera-space Z.
Depth maps decode as (value - 0.5) * 5.2 metres.

Body and complete rim/tyre units are separate layers. Assemble them with depth
comparison, rather than always drawing one whole layer above the other.

## Current prototype state

- Both cars have front/rear views with left, straight and right steering poses.
- Paint and bonnet masks support body recolouring and black/carbon-look hoods.
- Stock, 1-pixel and 2-pixel lowering move only the body. Use corresponding
  depth translation; tyre contact positions remain fixed.
- Wheel phases 0-7 represent 45-degree spin steps; frame 8 is a blur frame.
- These stock-wheel exports are still car-specific. The planned reusable wheel
  library with fixed standard sizes has not been implemented yet.
- Acceleration squat and braking dive now use the body pitch maps described below.

Original 3D models, Blender scenes, render passes and HTML viewers are kept in
the separate experiment workspace. They are not required at game runtime.

Body pitch is now available: each car manifest has `body_pitch` entries for
all six view/steering poses at -3, -1.5, +1.5 and +3 degrees; the existing body
maps supply zero pitch. Blender 5.2 rotates body geometry and split normals
around the suspension pivot at (0,0,0.45), keeping wheels in their original
positions. All exported body maps retain the existing panel-aware cleanup.
Runtime selects these samples from a damped acceleration/braking state. The
stock wheel files and their hashes are unchanged; this does not introduce a
wheel-and-suspension render cross product. Source renders and export scripts
remain in the experiment workspace (`render_pitch.py`, `pack_pitch.py`,
`export_pitch.py`). Manifests include SHA-256 hashes for the new maps.

The surface cleanup has been completed consistently for all 30 body samples per
car (neutral plus four pitch angles across six view/steering poses), using
`pipeline/cleanup_body_views.py` in the experiment workspace. It selects the
dominant material for paint depth/normals, filters only compatible surfaces and
repairs supported low-contrast colour speckles. Non-paint details, paint masks,
emission masks and silhouettes are preserved byte-for-byte. Wheel maps are
unchanged. Runtime shading now happens at native resolution before area-filtered
minification, so smaller cars do not inherit irregular dropped pixel rows.


## Side decals

`decals/catalog.txt` lists 24 runtime vector-authored decal sets (the rejected
dragon designs are excluded). Three independently transformed layers per car
use shared physical forward/height/width/angle coordinates. CPU composition
samples transparent ink in premultiplied form and blends it into body albedo;
normal/depth/finish/emission/coverage bytes remain unchanged. See the game README
for editor controls and the rear-body symmetry assumption.
