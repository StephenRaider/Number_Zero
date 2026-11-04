# Rim tint mask pipeline

The driving sprite keeps the car's stock wheel geometry/animation. A selected
wheel design can later supply a rim colour while a larger garage image shows
the actual design. This step supplies the coverage only; the garage selector,
wheel catalogue and runtime tint controls are not implemented here.

## Inputs and material assignments

Use existing external output/CAR-modular renders: material_ids.json, all 48
supersampled wheel albedo/material pairs and the 54 packed wheel albedos. No
model rerender is needed if these passes are present. Source folders and previews
remain outside the repo. Missing assignments/passes fail explicitly.

| Car | Tintable material names |
| --- | --- |
| S13 | wheel_rims_FL |
| EK9 | hM_Rim_Main_Max1, hM_Rim_NoTint_Max1 |
| Testarossa | rims |
| Supra | wheel_rim |

These are reviewed per-model identities, not a brightness threshold or a
circle painted over the wheel. Tyre, disc, caliper and separate bolt/badge
materials are excluded. For future models, inspect material IDs and add an
explicit assignment to rim_masks.py; do not classify every metallic material
as rim. A model using one textured material for tyres and rims needs a separate
material/UV classification pass first.

## Generation and export

At each of 128x128 pixels, vote on the visible 4x4 material samples as in the
existing packer. Mark a rim only if a rim material wins the vote strictly over
protected materials. Reject ambiguous ties, transparent source samples and
pixels outside the final wheel alpha. Do not dilate, flood-fill spoke gaps or
infer coverage from silver/white colours.

Frames 0-7 have binary 0/255 masks. Frame 8 is a coverage-weighted average of
those eight masks, clipped to the existing blur sprite alpha. This preserves
partial rim coverage in motion blur without tinting a permanently dark tyre
region. Geometry, all existing six sprite maps and body/pitch/popup assets stay
unchanged. Body pitch/ride height reuse the same wheel masks.

Output per car: 54 grayscale PNGs named
sprites/VIEW_POSE_wheels_fFRAME_rim_mask.png. The manifest's existing frame
entries gain a rim_mask path, files gains its SHA-256 and rim_tint documents
the contract. Existing map paths/schema remain valid. The normal exporter calls
the same generator before copying runtime assets, so subsequent exports keep
the rim masks and their checksums. Standalone promotion refuses unrecorded edits.

```powershell
python tools/car_pipeline/run.py rim-masks --car all --workspace "D:/Raylib Projects/Isometric pixel car racer"
```

This performs generation, promotion and verification with timing/logs in the
external workspace. Add --dry-run to inspect commands. For verification without
writing game assets, run the helper directly with --verify and omit --publish:

```powershell
python tools/car_pipeline/pipeline/rim_masks.py --workspace "D:/Raylib Projects/Isometric pixel car racer" --game . --car all --verify
```

## Runtime contract and review

Read the mask as linear single-channel data with point sampling, aligned exactly
to its wheel frame. 0 preserves the original pixel, 255 selects rim tint and
intermediate values blend tint coverage. Preserve the wheel's shade/detail
when applying colour, then use the same normal/depth/specular lighting path;
never tint the whole wheel or overwrite alpha. No extra body/steering/pitch
combinations are required for different colours.

Verify all views, all eight spin phases and speed blur. The helper checks exact
material-derived coverage, native dimensions, no transparent leakage, manifest
registration and checksums. Review gold/red diagnostic recolours enlarged and
at native scale: tyre edge, holes, brakes and separate centre badges should
remain stock. Re-run after any wheel geometry/material/render change.

Run the focused material/tie/transparent-gap/blur regression fixtures with:

```powershell
python tests/rim_mask_tests.py
```
