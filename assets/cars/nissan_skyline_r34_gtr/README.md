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

Current runtime maps were replaced from the user's corrected
nissan_skyline_r34_gtr_assets (2).zip. All 576 supplied PNG checksums verified;
the PNG bytes are preserved. The correction changes the decoded albedo in all
30 body poses, restoring dark bumper/window/mirror trim. Other decoded maps
(normals, depths, emission, paint/hood and rim masks) match the earlier pack.
Run python tests/r34_assets_test.py for hashes, dimensions, light, paint/glass/
plate, depth, pose and rim-coverage checks.

The rim masks are supplied exports with explicit wheel_rim identity. Their
raw material renders are not bundled, so the local rim-masks generator's existing
four raw-render recipes do not regenerate this imported car. Its 54 masks are
verified by r34_assets_test.py; regeneration needs the source recipe and external
material passes. No alternative wheel designs or tint selector are introduced.

Performance is registered in src/vehicle.h; see docs/PERFORMANCE.md for sources
and assumptions. Sound is the RB26 procedural profile in src/engine_sound.h.

The earlier assisted-launch calibration measured 4.94 s to 60 mph with an ideal
traction clamp. With the independent wheel-slip/clutch model it is about 5.08 s.
Stock power/torque, mass, gearing and governor remain. See docs/TRACTION.md and
docs/PERFORMANCE.md for the current model and technique-dependent comparisons.
