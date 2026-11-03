# Handoff
Run order: python3.11 venv with bpy numpy pillow; `python -I render_v3.py` (writes /tmp/work/out_v3), then `python -I pack4.py` (reads out_v3 and /tmp/work/out/iso2to1_meta.json, writes assets4.json). Paths are hard-coded to /tmp/work; edit them. pack4.py needs the 'out/iso2to1_meta.json' written by the earlier render.py (see meta in render_sprites.py).
Viewer: template.html has an __ASSETS__ placeholder: replace it with the JSON (assets4.json, plus an optional "alt" key of uncleaned albedo images per view_state).
See PIPELINE.md for the method. The .blend has packed data only if the model had textures; materials are flat colours.
