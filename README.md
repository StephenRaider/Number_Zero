# Number Zero

Game prototype workspace for isometric highway racing and car discovery.

This is the canonical local checkout of:
https://github.com/StephenRaider/Number_Zero

## Contents

- `src/`: game code, to be added when prototype implementation starts.
- `assets/cars/`: runtime-ready 128 x 128 car layers, paint/hood masks, wheel
  animation frames, placement metadata and asset manifests.
- `assets/README.md`: runtime texture format and current asset limitations.

The initial asset set contains the Nissan Silvia S13 and Honda Civic Type R EK9.
The playable game and its build configuration have not been implemented yet.

## Workspace boundary

Keep only game code, runtime assets/data, build configuration, necessary game
support tools and relevant documentation in this repository.

Blender source models, rendering experiments, HTML relighting/customization labs,
raw renders, diagnostics and research remain in the sibling
`../Isometric pixel car racer/` workspace. They are not game dependencies.
Only reviewed runtime exports are copied from that workspace into this one.

The experiment workspace provides `pipeline/export_game_assets.py` to export
runtime car assets. Game code does not need Python, Blender or the experiment
workspace to load the committed assets.

## Git workflow

Work in this Number_Zero folder for game implementation. Check the working tree
and pull with `git pull --ff-only` before starting work when the tree is clean.
Review and commit completed changes, then push to `origin/main`. Do not force-push
or overwrite unrelated changes. Sync is performed through normal Git commits,
pulls and pushes; there is no background auto-sync service.
