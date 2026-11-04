# Traffic-only runtime sprites

Five supplied models are rendered as 256 x 256 sprites at 26 pixels/metre,
ground anchor (128,170), 30-degree elevation, NE driving orientation and a
12-metre packed depth range. Each has 3 steering poses and 4 whole-car wheel
phases, with no suspension combinations. Paint/finish identities and side-tagged
head/tail/indicator masks are resolved within material boundaries from 3x renders.
PNG albedo uses straight alpha. Packed geometry stays point sampled; lit colour
is area-resolved when minified. Finish is in pack alpha (128 paint / 255 glass).
The five-map layout otherwise matches runtime car normal/pack/pack2/emission.

Source GLBs, Blender inspection, raw renders and diagnostics remain in the sibling
experiment workspace. Source files were supplied in D:/Cars for game:
2005_toyota_corolla_x_assista_package.glb, 2004_suzuki_swift.glb,
2001_toyota_crown_comfort_s10.glb, 1995_-_1998_toyota_hiace_h1.glb,
mitsubishi_fuso_rosa.glb. This folder contains runtime exports only.
Per-model manifest.json records physical size, anchor, native scale and hashes.

Wheel animation uses reviewed per-model axle centres and material/geometry gates.
Only road-wheel vertices rotate; lamps, badges, steering wheels, spare wheels and
bodywork remain fixed across spin frames. Rotation compensates for source tyre
aspect ratio, keeping contact height stable. Traffic ground shadows ray-cast the
front/back sprite depths toward the actual street lamps; there are no blob ellipses.
