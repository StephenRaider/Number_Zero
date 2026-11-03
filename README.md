# Isometric car relighting

Open `output/relight-lab.html` directly in a browser. It is self-contained and works offline.

## Relight Lab and customization viewer

`output/relight-lab.html` is the original full Relight Lab, with its original
shading, depth shadows, light spill, map inspection and highway/wheel animation,
now integrated with the corrected paint/hood masks and all customization controls.
`output/customization-lab.html` is the separate customization experiment. Both
are self-contained files. The builders now use different output filenames and
different inline preview paths, so rebuilding one cannot replace the other.

Both cars now have separate body and complete-wheel layers, primary paint and
bonnet masks, three ride heights (0/1/2 projected pixels), and geometry-rendered
left/straight/right steering poses from front and rear. The Silvia also has
independent lower-body paint. The viewer can show the assembled car, body only,
or wheels only. Headlights, tail lights, brake lights and hazards are independent
and start off. White plates and dark-grey glass are protected from recolouring.
The Before/After controls have been removed from the integrated lab.

Use `pipeline/Run-Customization.ps1` to rebuild just this version, or the root
`Run-Pipeline.ps1` to rebuild both sets and both viewers.
See [pipeline/MODULAR.md](pipeline/MODULAR.md) for layer formats, replacement-wheel
GLB conventions and commands. `output/<car>-modular` contains the current maps,
prepared scenes, separate greyscale paint/hood masks and wheel placement metadata.
The replacement importer was exercised using an exported stock wheel GLB and an
isolated `silvia-modular-smoke` variant. No new wheel designs have been supplied.

## Earlier rendering and cleanup experiments

The following describes the retained earlier datasets and comparison viewer
builder. `build_integrated_lab.py` builds the current full Relight Lab;
`build_viewer.py` alone builds its legacy comparison version;
`build_customizer.py` builds the separate customization showcase.

**Detailed headlights** is enabled by default. It bakes the existing reflector, bulb, and housing appearance through camera-transparent clear lamp lenses, while retaining lens geometry for normals, depth and specular highlights. The Civic's light-bucket texture supplies its previously hidden amber front indicators. Headlight emission is a low-intensity, texture-varying mask instead of turning the full lens white; amber is a separate channel driven by hazards. The Silvia exposes its existing lamp geometry with a mild cavity bake. Glass windows remain opaque dark grey. Disable this checkbox to compare against the original opaque lens treatment. Detailed maps and prepared scenes are in `output/civic-detailed/` and `output/silvia-detailed/`.

Asset check on 2026-11-03: `D:\Cars for game\2000_honda_civic_type_r_ek9.glb` is byte-for-byte identical to the existing source extracted from the EK9 ZIP (SHA256 `880dd436379c6b02915e11c9e61ab2a37e41efbc7aa850efcee0b27a298adc1b`). No distinct replacement EK9 was found in the collection or matching Downloads. The `1999_honda_civic_si.glb` alternative was visually inspected and is a coupe; it was not substituted for the EK9 hatchback. The current EK9 source is retained pending a genuinely different asset.

The **Before / After** cleanup switch preserves the car, view, light settings and animation state. Before is frozen from the viewer preceding this experiment (`assets.before.json` and `sprites-before/`). After uses `assets.improved.json` and `sprites-improved/`. After chooses colours and map samples from the dominant material in each supersampled block, repairs small ambiguous material-boundary notches, filters normals only across similar normals/depths within one material, and lightens the silhouette outline. Plate, wheel, badge and lamp pixels are protected. Lighting bands keep their plateaus with short smooth transitions; the specular cutoff is also softened. It remains a crisp 128 px sprite: this cannot eliminate every staircase, alter the model's topology, or repair large geometry defects.

The improved set retains exactly the Before silhouette and emission masks for every frame. Checks in `pipeline/verify_cleanup.py` validate these properties, white plate and grey glass colours, finite valid normals, and actual differences between both sets. The source `.glb` files remain unchanged. Material-ID guide passes are rendered with Blender 5.2; no full geometry rerender is needed to try new packing algorithms.

**Smooth wheel edges** adds a separate, optional pass on top of After. It uses the supersampled wheel material coverage to introduce restrained, quantized rim-edge tones (35% correction) and a light normal filter (30%) within tyre/rim regions. It protects the hub centres, opaque spoke cores and dark spoke gaps; it never changes the alpha silhouette or body pixels. The speed-blur frame is rebuilt from the refined wheel frames. Disable the wheel checkbox in After to compare against the previous body-only cleanup. The optional maps are in `assets.wheels.json` and `sprites-wheels/`; `verify_wheels.py` checks isolation and detail preservation over every animation frame. At 128 px the geometric staircase remains: this pass makes the rim boundary and shading read more smoothly without drawing a generic replacement wheel.

Cars: Nissan Silvia S13 and Honda Civic Type R EK9, using the supplied GLBs. No Testarossa model was extracted or used.

Both cars have opaque dark grey windows (linear RGB 0.055, 0.060, 0.070; approximately sRGB #42454B) and untextured white license plates. The Civic's rear plate recess was changed to white and a front plate was added. Source downloads remain unchanged. Prepared scenes are in `output/<car>/<car>_prepared.blend`.

Sprites use 128 × 128 canvases, 4× supersampling, 30° camera elevation, a shared (64, 79) ground anchor, and 26 px per metre. This fits the complete 4.50 m Silvia and 4.18 m Civic without cropping. The 1 m isometric grid axes are 18.3848 × 9.1924 px; the handoff's 28 × 14 px grid would require a larger canvas for these cars. Depth spans 5.2 m around the anchor.

Each car has front/rear sets, eight 45° wheel frames over one full rotation, and an averaged blur frame. Mirroring reverses normal X in the viewer. Both models have fixed headlights; no popup animation was invented. The handoff viewer's depth-volume shadows, emission, light spill, adjustable lighting, maps inspector and highway mode are retained. Single-layer front/far depth is a volume approximation, so thin details and cavities can show shadow artifacts. Lamp masks are assigned from model materials and position.

`output/<car>/sprites/` contains packed PNGs: albedo; normal (X high, X low, Y high); pack (depth high, depth low, specular); pack2 (Y low, far-depth high, far-depth low); emit (front lamps, rear lamps, indicators). Alpha is the sprite mask. `assets.json` embeds the same PNGs and metadata. Raw 512 px maps are also retained.

## Rebuild on this machine

Use Blender 5.2 at `D:\Blender\blender.exe` with `--background --factory-startup --python-exit-code 1 --python pipeline/render_cars.py -- silvia 8` and repeat for `civic 8`. `Run-Pipeline.ps1` uses this version by default.

Use the existing Blender 3.6 bundled Python at `C:\Program Files\Blender Foundation\Blender 3.6\3.6\python\bin\python.exe` to run `pipeline/pack_cars.py silvia`, `pipeline/pack_cars.py civic`, `pipeline/build_viewer.py`, and `pipeline/verify_outputs.py`. Pillow is installed locally in `.runtime`; numpy comes with Blender. `build_viewer.py` also writes the chat preview into this chat's visualization folder.

Packing continues to use the existing Blender 3.6 Python environment where Pillow is installed; rendering uses Blender 5.2. The Silvia rear light texture is classified into separate tail-light (green) and amber indicator (blue) emission channels, gated to the rear clusters. Hazards blink only the amber sections; brake intensity affects only the red sections.

The original handoff scripts are retained in `pipeline/` for reference. The adaptations are `render_cars.py`, `pack_cars.py`, and `build_viewer.py`.
