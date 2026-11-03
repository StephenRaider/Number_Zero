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
  exports implement the planned reusable standard-size wheel library or body
  squat/dive poses; see `assets/README.md` for current asset state.
- Engine sound uses the supplied procedural synthesiser in third_party/car_engine_synth. Keep its DSP single-thread owned; publish game inputs through the atomic snapshot in engine_audio.h. M mutes; pause/garage fade out. build.ps1 -Test includes offline audio checks; --audio-smoke-test exercises the real output device.
