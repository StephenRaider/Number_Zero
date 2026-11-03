# 3D car to isometric pixel-art pipeline

Goal: turn a 3D car model into game-ready 128 px isometric sprites with flat colour, normal, depth, specular and emission maps, so the game can relight them in real time.

## 1. Inputs and tools
- A car model (.blend). Glass is tinted dark grey, not black, so light can show on it.
- Blender 5.x as the Python module: `pip install bpy` inside a **python3.11 venv**. Run scripts with `python -I`, otherwise Blender crashes with a "InitGoogleLogging twice" error.
- Python with numpy and Pillow for packing. No GPU needed; Cycles on CPU is fast enough because everything is 1 sample.

## 2. Camera and grid (render_v3.py)
- Orthographic camera, 2:1 pixel-art isometric: azimuth 45 degrees, elevation 30 degrees. A 1 m ground tile is 28x14 px at 128 px scale.
- Every sprite shares one anchor pixel (64,79 at 128 px), so the car stays on the isometric grid however it is cropped.
- Only two views are rendered: front (azimuth -45) and back (azimuth 135). Left and right are mirrors: flip the image and negate normal X.
- Two headlight states: down (animation frame 1) and up (frame 60).
- Wheels: 8 spin frames, 9 degrees apart (rims repeat every 72 degrees), plus one blurred frame for high speed.

## 3. Render passes (all at 512 px, 4x supersampled)
Cycles, 1 sample, filter width 0.01 (no anti-aliasing), emission-only materials, transparent film. Each pass is a separate render with the materials swapped:
- **Albedo**: flat material colours.
- **Normal**: camera-space, 16-bit (X and Y rendered; Z reconstructed later).
- **Depth**: 16-bit, white is nearest, range -1.8 to +1.8 m around the anchor.
- **Far depth**: depth rendered from a camera behind the car, flipped horizontally. It gives the car a back surface so shadows know the car has volume.
- **Specular mask**: by material name.
- **Emission**: front lamps in R, rear lights in G, indicators in B.

## 4. Packing (pack4.py)
1. Downsample 512 to 128 by 4x4 blocks. Albedo takes the **most common colour** in each block, with no blending, so the palette stays flat. Normals, depth, far depth and specular use coverage-weighted averages. A pixel is opaque when at least half its sub-pixels are covered.
2. **Albedo cleanup** (can be switched off in the viewer):
   - merge near-identical colours into the more common one;
   - replace isolated or ragged pixels with the neighbour majority;
   - darken the outermost silhouette pixels by about a third for a clean edge.
   - Exempt: light pixels, the yellow badge pixel, and the wheels (found as the pixels that change between spin frames, grown by 4 px).
3. Encode as PNGs: 16-bit normals are split across two images, depth and far depth likewise, and everything is base64-embedded in one JSON.

## 5. Relighting in the viewer (template.html, plain JS and canvas)
Per pixel: Lambert plus specular from the normal map, quantised into light bands (8 by default), hemispheric ambient, a depth-map shadow ray-march with self-shadowing only between the front and far depth, emission glow, and own-light spill on the road. Highway mode scrolls the road and passes lamps overhead.

## 6. Files
All in `art-tests/isometric-street-racing/testarossa/`: render_v3.py, pack4.py, template.html, the .blend files, sprite PNGs, and the zips of the raw 512 px maps. The viewer is published as the Testarossa Relight Test artifact.

## 7. Can Codex do this?
Yes, in principle, if its environment allows the following. This was done in a Linux sandbox with no Blender install and no GPU.
- Python 3.11 with `pip install bpy` working (needs network access to PyPI), plus numpy and Pillow.
- Running `python -I render_v3.py` then `python -I pack4.py`. Each is plain Python with no external services.
- Looking at images to judge the result. A model that cannot view PNGs can still run the scripts but cannot judge pixel quality, so the human has to review the sprites.
- Reproducing the viewer only needs a browser or Playwright; the final page is one HTML file.

For a new car, only the .blend path, the material names used for the specular and emission masks, and the anchor offset need to change.
