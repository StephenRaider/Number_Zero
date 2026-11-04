# Skyline GT-R R34 runtime assets

Imported from the user's nissan_skyline_r34_gtr_assets.zip. It contains 576
runtime PNGs: 30 body states x 6 maps, 54 wheel states x 6 maps, 54 rim-only
masks and 18 separate body paint/hood masks. Camera/scale/depth conventions match
the existing roster: 128x128, 26 pixels/metre, anchor (64,79), 30-degree elevation
and 5.2 m depth range. Frame 8 is wheel-speed blur. Headlights are fixed, not
pop-ups. Window colour is dark grey and number plates are plain white.

The source asset named in the supplied recipe is
nissan_skyline_r34_gtr_2002__www.vecarz.com.glb. Original source models, supplied
recipe/patches, run logs and archive remain in the external experiment workspace
under output/imports/r34-pack. Archive contents were inspected as input data;
its scripts/patches were not executed or applied automatically. Only runtime
maps and metadata were promoted here.

All supplied manifest PNG hashes differed from the archive's actual PNG bytes.
Import decoded the images, checked dimensions and map semantics, preserved the
PNG bytes, and rebuilt the checksum index. Run python tests/r34_assets_test.py
for hash, map, light, paint/glass/plate, depth, pose and rim-coverage checks.

The rim masks are supplied exports with explicit wheel_rim identity. Their
raw material renders are not bundled, so the local rim-masks generator's existing
four raw-render recipes do not regenerate this imported car. Its 54 masks are
verified by r34_assets_test.py; regeneration needs the source recipe and external
material passes. No alternative wheel designs or tint selector are introduced.

Performance is registered in src/vehicle.h; see docs/PERFORMANCE.md for sources
and assumptions. Sound is the RB26 procedural profile in src/engine_sound.h.
