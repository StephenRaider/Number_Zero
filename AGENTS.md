# Project instructions

- This checkout is the Number Zero game repository. Keep game code, runtime
  assets/data, build configuration, necessary game support tools and relevant
  documentation here.
- Keep Blender source models, asset/render experiments, HTML labs, raw render
  passes, research, caches and diagnostics in the sibling
  `../Isometric pixel car racer/` workspace.
- Import reviewed runtime exports rather than copying the experiment workspace.
- Inspect `git status` before changes. Preserve unrelated local work.
- Keep this local checkout and its GitHub repository in sync using normal Git
  commits, fast-forward pulls and pushes for the user's authorized game work.
  Never force-push or reset away unrelated changes.
- The native Raylib highway prototype is implemented. Build with build.ps1; run
  build.ps1 -Test for driving-model checks. Do not assume the stock-wheel
  exports implement the planned reusable standard-size wheel library; see `assets/README.md` for current asset state.
- Engine sound uses the supplied procedural synthesiser in third_party/car_engine_synth. Keep its DSP single-thread owned; publish game inputs through the atomic snapshot in engine_audio.h. M mutes; pause/garage fade out. build.ps1 -Test includes offline audio checks; --audio-smoke-test exercises the real output device.

- Body pitch maps are now exported for both cars at +/-1.5 and +/-3 degrees for
  every front/back steering pose. Existing wheel textures are reused unchanged.
- Every display fits the same road visibility corridor. Keep future traffic
  visibility tied to nz::VisibleAhead, not the viewport bounds, to avoid screen
  aspect advantages. Out-of-view lights may still illuminate that corridor.
- Manual gearbox (garage selection) uses Q down/E up; Z/C indicators, X hazards,
  H headlights and T tails work in the garage and on the road.

- Body surface cleanup now covers neutral and pitch poses; use the experiment
  cleanup_body_views.py after packing and before final verification/export.
- Keep categorical normal/depth/mask maps point sampled. Shade the native car
  first, then minify its lit colour with car_resolve.fs. Lamp-only bloom is in
  lamp_glow.fs; --glow-smoke-test verifies on/off and brake intensity on the GPU.
- Runtime depth-map alpha encodes finish (0 other, 128 paint, 255 glass); coverage and shadow visibility use albedo alpha. Keep this channel point sampled. Paint/glass specular widths and diffuse responses are independent in car.fs.

- Highway assets are in assets/highway (credits included); source models/renders
  remain in the experiment workspace. Generated layout.h defines real lamp-head
  positions. Keep geometry and active light placement sourced from that metadata.
- Road/car/props are separate layers. highway_prop.fs compares prop/car depth
  per fragment and hardware-depth-tests props. Never return to drawing all lamps
  before the car or forcing all lamp pixels into the foreground.
- --scenery-smoke-test includes GPU occlusion checks and an outer-lane drive.
- Raylib batches track only four auxiliary textures. The road pass explicitly binds car maps at units 5-7; the prop pass binds car depth at unit 5 via ShaderProgram.TextureSlot. Flush draws before releasing those units, and retain the encoded-depth dependency probe.
