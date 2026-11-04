# Car asset pipeline

Versioned code and process for producing Number Zero runtime cars. This folder is
the maintained game-repo copy. Source GLBs/Blender files, prepared models, raw
renders, caches, preview sheets and timing logs stay in an **external workspace**.
Only reviewed PNG/runtime metadata exports are written to `assets/cars` or
`assets/traffic`. No HTML labs or experiment history are included here.

## Setup and commands

Use Python 3.10+ with the tested NumPy/Pillow versions in `requirements.txt`, plus
Blender 5.2. Install dependencies into your own environment:

```powershell
python -m pip install -r tools/car_pipeline/requirements.txt
python tools/car_pipeline/run.py render --car supra --workspace "D:/Raylib Projects/Isometric pixel car racer" --source-dir "D:/Cars for game" --blender "D:/Blender/blender.exe" --dry-run
```

Remove `--dry-run` to run the complete Supra recipe. Paths above are examples;
pass the actual directories on another machine. Run from any working directory.
The source file must be `toyota_supra_mk4__supra_1997__free_download.glb` in the
source directory. Source files are user-supplied and are not bundled.

```powershell
python tools/car_pipeline/run.py verify --car supra --workspace "D:/Raylib Projects/Isometric pixel car racer"
python tools/car_pipeline/run.py sound-check --workspace "D:/Raylib Projects/Isometric pixel car racer"
```

`sound-check` uses the sibling w64devkit compiler if present; otherwise pass
`--compiler <path-to-g++>` or put g++ on PATH. It compiles the current game audio
tests, runs all four profiles and writes diagnostic WAVs in the external workspace.
`--car traffic` renders/verifies the existing five traffic recipes; rendering needs
the supplied GLBs in `--source-dir`.

Each run stops on failure and writes per-stage logs and `timing.json` under the
external workspace's `output/pipeline-runs/`. A dry run executes nothing. Runtime
promotion verifies manifests and refuses to overwrite unrecorded game-side edits.

## What is implemented

- Full prepared-Supra path: source preparation, neutral/steering renders, separate
  wheel phases, suspension pitch, packing, export, surface cleanup and verification.
- Existing five traffic recipes, native packed lighting/depth/emission maps.
- Shared modular/pitch render and pack code used by S13, EK9 and Testarossa.
- Sound regression runner using the authoritative game DSP and audio tests.

The main wrapper supports Supra and the traffic batch. Other playable cars still
need their existing prepared source layouts and popup-headlight export handling;
this is not a one-click importer for arbitrary models. Those source-specific
assignments remain explicit in the preparation/render recipes. No fine-grained
cache or automatic material recognition is claimed here.

Read [PROCESS.md](PROCESS.md) for the workflow and acceptance gates, and
[ENGINE_SOUND.md](ENGINE_SOUND.md) for sound standards and current test coverage.

## Rim colour masks

All four playable cars now export rim-only tint coverage for each of the six
view/steering poses and nine wheel animation states (54 masks per car).
Generate/publish/verify them from existing material renders without Blender:

```powershell
python tools/car_pipeline/run.py rim-masks --car all --workspace "D:/Raylib Projects/Isometric pixel car racer"
```

Use `--car silvia`, `civic`, `testarossa` or `supra` for one car. Normal game-asset
export also regenerates and registers the masks automatically. See
[RIM_MASKS.md](RIM_MASKS.md) for the material assignments, blur rule, runtime
contract and verification. A wheel-selection menu is not part of this export.
