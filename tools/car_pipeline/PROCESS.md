# Car onboarding process

1. Choose the exact trim/year/performance baseline and record specification sources.
2. Inspect the model before rendering: dimensions/front axis, real road wheels,
   calipers, hood, paint, glass, plate surfaces and lamp/indicator geometry.
3. Save reviewed assignments in a source-specific preparation recipe. Preserve
   world transforms when detaching imported hierarchies. Never guess wheels from
   low height alone; do not include lamps, body panels or steering wheels.
4. Review a small front/rear preview and spin sample before committing to a full
   pass. Test black body paint and black hood to expose mask gaps. Check dark-grey
   windows, charcoal tyres with preserved hubs, white blank plates and lamps
   individually. Preview-first is a review practice; the wrapper currently runs
   the full recipe rather than providing a dedicated preview mode.
5. Render 4x at 512px, resolve to 128px/26px per metre. Six camera/steering poses,
   eight wheel phases plus blur, and four body pitch samples around neutral yield
   30 body views. Body and complete rim/tyre wheel units remain separate.
6. Pack categorical masks and 16-bit geometry without blurring encoded values.
   Apply material/crease-aware body cleanup; preserve lamp masks and silhouettes.
7. Verify hashes, canvas/margins, front/back depth, paint/hood exclusions and
   physically visible lamp channels. Far-side lights may legitimately self-occlude.
8. Promote runtime exports with the checked exporter, then test in-game lighting,
   shadows, body/wheel grounding, black paint, decals, shifts and save migration.
9. Run the sound checks and audition described in ENGINE_SOUND.md before approval.
10. Commit only code, recipes, documentation and reviewed runtime exports. Keep
    original models, intermediate passes, diagnostics and labs outside the repo.

## Timing and improvements

The initial Supra asset pass measured about 3m19s. One source-lamp correction
required about 2m05s more processing. Full onboarding took about 20 minutes because
inspection, research, game/audio integration and rework were also required; those
activities overlapped rendering, so subtraction does not measure manual effort.
Record first-pass processing, repairs and overall onboarding separately.

Already implemented: one command through export/cleanup/verification, explicit
external paths, failure logs, stage timing, checked runtime promotion and reusable
recipes. Next improvements are preview-only mode, dependency-aware incremental
renders, recipe/config registration and data-driven garage/audio integration.
These are planned work, not existing wrapper features. An arbitrary new model
still needs a reviewed recipe; copying the Supra recipe without inspection is
not an approved shortcut.

## Rim tint coverage gate

Playable exports include all 54 rim masks per car. Follow [RIM_MASKS.md](RIM_MASKS.md):
use explicit material IDs, categorical supersample votes, protected-material tie
rejection and coverage-weighted speed blur. Verify every pose/frame and inspect
native/enlarged recolour previews. This replaces the need to render each colour
or wheel-design combination; it does not add a wheel-selection menu.
