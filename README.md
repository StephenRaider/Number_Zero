# Number Zero

A native Raylib highway-driving prototype with isometric pixel cars and dynamic
lighting. Choose a Silvia S13, Civic EK9, Testarossa, Supra SZ or Skyline GT-R R34 and its paint,
then drive or race along a straight four-lane highway at night.

## Play on this machine

Run `build/NumberZero.exe` after building. Convenience scripts such as
`build.ps1` and `Play.ps1` may be present on this machine, but are local-only
and intentionally excluded from Git alongside `AGENTS.md`.

The executable finds the committed assets relative to its location. It needs no
Blender, Python, browser, network connection or audio device.

In the garage, select a car and paint swatch, or use the colour picker. The S13
also offers two-tone paint, and each car can use a black bonnet. Choose DRIVE
or RACE / 10 KM, then click Start or press Enter.

| Control | Action |
| --- | --- |
| W / Up | Accelerate |
| S / Down / Space | Brake |
| A / D or Left / Right | Continuous lateral steering |
| H | Toggle headlights and beams |
| T | Toggle tail lights; braking still activates brake lights |
| Escape | Pause/resume; exit from garage |
| Tab | Return to garage |
| R | Reset the drive / restart the race |

Steering is continuous rather than lane snapping. Its response depends on speed,
car setup and available tyre grip. The car settles after releasing steering.
Choose AUTO or MANUAL in the garage. Manual uses Q down / E up with an assisted
clutch, shift delay and over-rev downshift protection; AUTO shifts itself.
The game pauses on focus loss. The window can be resized; rendering uses a
dynamic pixel canvas with nearest-neighbour integer scaling. The road camera fits a shared world-space visibility corridor into every
aspect ratio, with the same forward fog limit for all players. The garage stays centred.
Press **F11** to toggle borderless fullscreen.

## Implemented

- Garage with five cars, paint selection, layered decals and optional two-tone/black hood.
- Scrolling straight highway, lane markings, barriers and street lamps.
- GPU normal-map lighting, highlights, depth-aware self/ground shadows,
  headlight beams, tail-light spill and brake-light emission.
- Separate depth-composited body/wheel layers, steering poses and wheel spin.
- Fixed-120-Hz drivetrain, drag, rolling resistance, braking and lateral motion.
- Speed, gear, RPM, pedal inputs and distance HUD.

Drive mode is free driving with ordinary traffic. Race mode adds one white
Testarossa opponent. Parking areas and damage are future work.
The stock wheels remain the car-specific prototype exports; the planned shared
standard-size wheel library has not been implemented.

## Race mode

Select **RACE / 10 KM** in the garage. Both cars roll side by side at 50 km/h
through a three-second countdown; that distance is excluded from the race.
After GO, the first car to cover 10 km wins. The HUD shows both distances,
the lead in metres and elapsed time. A lit checkered stripe marks the finish.
Each race gets a fresh traffic seed, including R resets and Rematch. The seed
is recorded in the local race telemetry for troubleshooting.
The result screen offers Rematch and Garage; R and Tab work too. Pause stops
both racers and traffic.

The white Testarossa uses the same stock vehicle physics, automatic gearbox,
brakes and collision rules as the player. Its driver reacts about every 115 ms
and sees only traffic within the shared forward visibility range (42.5 m) and
25 m behind. It predicts 2.6 seconds of nearby motion using the selected car's
actual steering, grip, braking and drivetrain. It checks adjacent-lane passes,
including a second lane change or return, with continuous collision checks.
Only the first action is applied before observing again. It cannot read traffic
generation seeds, lane-density patterns or future spawns.

It uses full throttle on clear road and drops back to open a gap when boxed in.
There is no catch-up boost or altered vehicle physics. It can still misjudge
traffic and make contact. Traffic remains simulated around both racers even
when they separate. See `docs/RIVAL_TUNING.md` for measured comparisons and logs.

`--race` opens the garage with Race selected. `--race-smoke-test` verifies the
rolling start, rival rendering and result UI (with an accelerated finish setup).
`tests/race_tests.cpp` separately runs six complete 10 km simulations, checks
local-only perception, traffic coverage, rolling-start equality and same-step
finish ordering, boxed-in escape, temporary-gap overtaking and post-contact
recovery. Race sessions automatically write local diagnostic CSV data under
`build/qa/race_telemetry.csv`.

## Build and verify

The Windows x64 Raylib 5.5 static library and headers are included under
`third_party/raylib`, with their license. Use a Windows x64 MinGW-w64 C++17
compiler; the `g++` commands below must refer to that x64 toolchain.

Create a `build` directory, then compile from the repository root:

```text
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Ithird_party/raylib/include src/main.cpp -o build/NumberZero.exe -Lthird_party/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++
```

Build and run the offline checks:

```text
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/vehicle_tests.cpp -o build/vehicle_tests.exe
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/audio_tests.cpp -o build/audio_tests.exe
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/trail_tests.cpp -o build/trail_tests.exe
./build/vehicle_tests.exe
./build/audio_tests.exe
./build/trail_tests.exe
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/race_tests.cpp -o build/race_tests.exe
./build/race_tests.exe
```

Graphics smoke tests create a hidden native window, run a scripted drive and
brake sequence, and export captures under ignored `build/qa/`:

```text
./build/NumberZero.exe --smoke-test
./build/NumberZero.exe --smoke-test --civic
./build/NumberZero.exe --race-smoke-test --testarossa --ultrawide
```

See `docs/PERFORMANCE.md` for stock references, assumptions and measured model
results. These results are simulation estimates, not factory performance claims.

## Project layout

- `src/main.cpp`: garage, native controls, rendering, highway and HUD.
- `src/vehicle.h`: stock car data and fixed-step driving model.
- `src/race.h`: rolling start, reactive rival driver and 10 km race rules.
- `assets/cars/`: committed runtime car sprites, masks and manifests.
- `assets/shaders/`: car relighting and lit road shaders.
- `tests/vehicle_tests.cpp`: driving behaviour/performance checks.
- `third_party/raylib/`: native graphics dependency and license.

This checkout is synced to https://github.com/StephenRaider/Number_Zero.
Keep Blender models, render experiments, HTML labs and intermediate outputs in
`../Isometric pixel car racer/`. They are not runtime/build dependencies.
Review, commit and push game changes here; use fast-forward pulls and preserve
unrelated work. There is no background auto-sync service.

## Prototype polish

Driving faces northeast; steering right moves toward the right side of the road.
Paint uses a narrow clearcoat reflection; glazing has its own sharper finish.
Fixed world-space street lamps have a smooth extended falloff and stay active
beyond the viewport. Each lamp casts its own depth-map shadow; ambient and other
lamps remain independent. Shadow rays intersect the car atlas before marching,
so they have no fixed screen clipping rectangle or nearest-lamp switch.

Steering positioning is quicker at low/medium speed and calmer at highway speed.
Releasing steering recentres promptly. This is prototype control tuning, not a
validated comparison of real S13 and EK9 handling. Drivetrain, grip and braking
parameters remain separate. `src/scene.h` contains shared projection and lamp
selection math covered by the headless tests.

## Engine sound

Live procedural engine audio from the supplied `car_engine_sounds.zip`: SR20DET
turbo for the S13 and B16B/VTEC for the Civic. RPM follows the drivetrain;
throttle lifts during shifts, braking and the factory speed cut. The synth's
turbo, lift-off and VTEC layers follow its supplied profiles. These are
synthesised approximations, not recordings of the real cars.

Press **M** to mute/unmute during driving. Garage is silent; pausing or losing
focus fades the engine out. Audio runs on a dedicated device callback with
atomic input snapshots, so rendering stalls do not starve a frame-polled stream.
If no output device is available, the prototype continues with audio disabled.

The offline audio checks write diagnostic WAVs under
ignored `build/qa/`. `build/NumberZero.exe --audio-smoke-test` (optionally
`--civic`) exercises the real output callback during a scripted drive.
Ordinary `--smoke-test` stays silent and needs no output device.
See `third_party/car_engine_synth/README.md` for provenance and the BOV fix.

Closed-throttle coasting and braking retain an RPM-driven engine bed instead of
fading toward silence. Throttle changes the loaded tone and intake sound;
engine pitch still follows actual drivetrain RPM. Audio regressions compare
loaded/coast levels at 3000, 5000 and 7000 RPM for each car.

## View and instruments

The camera scales the complete shared highway corridor uniformly to the window,
with aligned depth/normal-map lighting and shadows. Forward visibility ends at
38 metres, fading into night fog over its final 8 metres on every display.
The same range fits all four lanes, including tall car roofs. Extra screen
width adds scenery rather than earlier visibility of hazards. Future traffic
must use `nz::VisibleAhead` for its visibility too.
Garage previews retain their original size; the studio key light orbits the
car roughly once every 11.4 seconds.

The compact bottom-left cluster combines central digital speed with an outer
RPM ring, fine ticks and redline markings. A narrow auxiliary strip holds gear,
tiny pedal ticks and lamp/audio status; trip distance sits just beneath the dial. It anchors to the current viewport when resizing.

Graphics checks support `--ultrawide` (2560x1080), `--superwide` (3840x1080),
and `--tall` (960x900), combined with `--smoke-test`. Use
`--resize-smoke-test` to exercise render-target resizing during a drive.
Captures include logical dimensions in their filenames under `build/qa/`.

## Lights, gearbox and suspension

- **H** toggles headlights and **T** toggles tail lamps in the garage or driving.
- **Z** toggles the left indicator, **C** the right; **X** toggles hazards.
  Garage buttons share the same state. Actual amber sections blink independently
  of the white headlights and red tail/brake sections.
- Two bumper-spaced headlight cones follow the rendered steering yaw, and their
  near-ground lamps share the same positions. Garage floor spill uses its
  front-view camera basis.
- Click the **AUTO / MANUAL** garage button to choose the gearbox. In manual,
  **Q** shifts down and **E** shifts up. There is no clutch pedal, neutral or
  reverse yet. An unsafe downshift is ignored; holding a gear reaches its rev
  limiter instead of automatically shifting. **M** remains audio mute.
- Acceleration lifts the nose/compresses the rear; braking dips the nose. A
  damped suspension state selects body poses from -3 to +3 degrees at 1.5-degree
  intervals, including all steering views. Wheels remain independently grounded
  and reuse their original animations. Rendered body masks, normals, front/far
  depth and lamp channels follow each pose. This is a small stylised motion,
  rather than a full tyre/suspension solver.

`--feature-smoke-test` captures steering, acceleration and braking frames with
hazards; add `--manual` to exercise scripted player-style sequential shifts.

## Refined camera and sprite presentation

The common lookahead is now 38 metres, with a 6-metre rear framing allowance.
This zooms out slightly and places the player farther southwest while preserving
the same forward visibility on every display. Twin headlight beams have wider
shoulders and a 40-metre reach, still rotating with the rendered car heading.

All neutral and pitched body views now receive dominant-material depth/normal
selection, crease-aware filtering and supported low-contrast paint-speckle
cleanup. Lamp, glass, plate, mask, emission and silhouette pixels are protected.
The renderer shades at the original 128-pixel resolution and uses exact area
coverage when shrinking the resulting colour image. Packed lighting maps remain
point-sampled. This avoids missing thin details and irregular nearest-neighbour
steps without blurring the full game image.

Headlights and tail/brake lamps have a restrained Gaussian halo in a separate
emission-only pass; it can spill over paint and road and follows each lamp switch.
Braking strengthens the red halo. Bright paint and HUD elements do not bloom.

## Glossy paint and glass

Paint and glazing have distinct runtime finish identities, stored in the spare
depth-texture alpha channel (albedo alpha still controls coverage). Paint uses
a tight clearcoat highlight; glazing uses a sharper lobe and much lower diffuse
response while retaining its dark grey base and a faint cool reflection.
The broad secondary specular lobe has been removed. Street-light reach and
road lighting are independent of this surface finish. Existing sprite cleanup,
area coverage filtering and bulb-only bloom remain active.

`--garage-smoke-test` captures a complete moving-light cycle; add `--black` and/or `--civic` to inspect high-contrast paint/glass reflections.

## Asset highway and scenery depth

The road uses the supplied Low road materials as two two-lane carriageways,
with asphalt grain at its proper scale and separately spaced worn lane markings.
Shoulders, four-metre guardrail modules, curved solar street lamps and periodic
overhead sign gantries replace the procedural line scenery. Source attribution
and modification notes are in `assets/highway/CREDITS.md`.

Ground, car and props are separate layers. Lamp/rail/gantry colour, normal,
front-depth and emission maps are rendered independently. The prop shader
compares each covered fragment with the car's depth, then hardware depth-tests
props against one another. A high lamp arm can cover the car while lower/behind
geometry stays behind it; no single whole-sprite foreground flag is used.
Street-light positions come from the exported lamp-head metadata and remain
active beyond visible bounds while their lighting can still contribute.

Only packed runtime textures, layout metadata and credits are in the checkout;
source GLBs, Blender scenes and render/packing scripts stay in the experiment
workspace. `--scenery-smoke-test` checks real GPU front/behind/split-depth cases
and drives the outer lane beneath the lamp arms. It can be combined with
`--civic`, `--manual`, `--ultrawide` or `--superwide`.

The driving HUD now follows the circular reference: a roughly 100-by-100 logical
pixel footprint at the bottom-left, a muted copper outer rim, mint instruments,
speed in the centre and RPM around the perimeter. Gear/pedals/status stay in
the narrow left strip. There is no rectangular enclosing panel or car-name row.

## Instruments and optional visual effects

The RPM dial has a fully transparent face. At the shift/red-zone threshold all
gauge numbers, speed and arcs turn amber. Instruments have restrained bloom;
turbo cars also show a compact **BOOST %** dial beside the RPM ring. Boost is
the drivetrain model's normalized spool/load value, not calibrated bar or PSI.
Car head/tail-lamp bloom is slightly stronger while paint highlights stay sharp.

Open **Esc / Pause** to compare **Tail-light trails** and **Background motion
blur** independently. Click their checkboxes or press **1** / **2** while paused.
Both start enabled and persist across garage visits during the session. Pause
freezes trail history for direct before/after comparison. The menu leaves the
car visible; Resume and Garage buttons are included.

Trails follow the actual composed left/right lamp and depth masks in world
space, strengthen under braking and fade within 0.14 seconds. Stopped/unlit cars
do not create new streaks; resets and car changes clear history.

Motion blur affects road and scenery in the travel direction, increasing mildly
with speed. The car and instruments are composited sharply. Separate prop
colour/depth layers preserve lamp/gantry occlusion, including blurred foreground
props. `--effects-smoke-test` captures the four toggle combinations on a frozen
frame, plus the amber red-zone gauge, and checks the compositor on the GPU.


## Garage decals

Open **DECALS / GRAPHICS** in the garage. The editor uses fixed diffuse studio
fill and softbox bar reflections instead of the orbiting point source.
Switch **FRONT VIEW / REAR VIEW**, choose one of three decal layers, then browse
32 vector-authored designs with the arrows or mouse wheel over the preview.
Drag on the car or use arrow keys/buttons to move the decal on its side plane;
adjust size, angle and single-ink colour. Racing designs retain their colours.
REMOVE clears the selected layer, RESET restores its transform, DONE / SAVE
returns to the garage. Esc also returns. Each car keeps its own three layers.
Settings persist in local, ignored `build/customisation.cfg`.

Decals are clipped to side-facing paint and composed into albedo before lighting.
Placement is in metres on the vehicle side, recovered from the existing depth
maps, with inverse yaw/roll/pitch transforms for all 30 body views per car.
The mirrored rear sprites represent the right rear quarter using the existing
body symmetry assumption; unique left/right geometry would need dedicated views.
No bonnet/roof placement or arbitrary surface wrapping is implemented.

`--decal-smoke-test` verifies all 30 poses, protected geometry/lamp/silhouette
maps, save/reload and captures front/rear editor and driving frames. Add
`--civic` for the EK9. Compile/run `tests/decal_tests.cpp` with the same standalone
compiler flags as the other unit checks to verify cross-view projection maths.

**Sponsor Overlay** is a transparent frame of sponsor blocks with a clear centre.
Choose it on layer 2 or 3 over another graphic; layers draw in order 1, 2, 3.

Base decals now exclude small sponsor stickers and numbers, preserving only
their main graphics and large wordmark. Numbers 07, 35 and 86 are separate
decals, each plain or in a white box with black border. **Sponsor Strip** adds
a single independent sponsor row. A useful stack is base graphic on layer 1,
sponsors on layer 2, number on layer 3. Number designs start at a smaller width;
switching between number variants keeps your adjusted placement and size.


Garage saves now include each car's paint colour, two-tone/hood settings and
colour-picker values, plus the selected car, gearbox and existing decals.
Previous decal-only save files remain compatible and upgrade on the next save.
Switching cars restores each car's own finish instead of resetting its paint.
Both cars use 2x steering response, release and lateral positioning rates while
retaining the existing speed-dependent handling curve.


## Ferrari Testarossa

The garage now includes the 1984-style Testarossa alongside the S13 and EK9.
It supports saved paint/hood/decal settings, independent wheels, steering and
all suspension views. H (or the garage light button) selects raised or retracted
pop-up headlight geometry as well as switching headlamp emission and beams.
Both popup states have all 30 body poses and reuse the same stock wheel frames.

The stock-style European setup uses a 4.9-litre flat-12 (390 PS / 490 Nm), rear
drive, five ratios and an engine rev limit. It has **no road-speed limiter**;
the current simplified drivetrain settles near 287 km/h through power and drag.
The S13 and EK9 have separate stock 180 km/h caps, removable in the garage. Engine sound uses a distinct
12-pulse procedural profile; it is an approximation, not a recorded engine.

Reference: [Ferrari Testarossa, Museo Nicolis](https://www.museonicolis.com/en/ferrari-1991-testarossa/)
lists roughly 290 km/h. Weight/gearbox/torque values are stock-period estimates;
the game simulation is not a calibrated vehicle dynamics model.

`--testarossa` selects the car (also works with graphics/audio smoke flags).
`--decal-smoke-test --testarossa` checks all popup poses and save migration.
Save version 4 includes a car count and per-car limiter-delete settings, and preserves older two-car/version-3 saves.

The formerly readable number decals now use invented broken-loop race marks
(A/B/C), with plain and boxed versions. Their stable IDs preserve old placements.


The Testarossa's exhaust mixes lower crank/bank harmonics with its twelve-cylinder
firing pulses for a fuller bass/midrange note. Broader, lower exhaust resonances
and softer high-frequency filtering reduce the thin buzz. The bass follows actual
RPM, load and shifts and stays audible on lift-off; it is not a constant loop.


Low-RPM audio now includes cycle-to-cycle combustion variation, coloured exhaust
pressure bursts and slow flywheel/governor hunting, fading out smoothly above
the low-speed range. This affects the audio engine only; drivetrain performance
and displayed RPM remain governed by the vehicle simulation.
Tail trails ignore centre lamps/engine-bay details and use the outer rear lamps,
including the exact sprite placement offset at the current render scale.

The driving camera is shifted 4.5 m farther ahead of the car (about one car length), keeping the same zoom. The shared visibility corridor is 42.5 m ahead / 1.5 m behind on every display.


## Highway traffic

All four lanes now flow in the player's direction. The traffic-only pool is
Corolla X Assista, Swift, Crown Comfort, Hiace and Fuso Rosa. These models are
independent of the garage roster; later eligible racer models can join the pool.
Traffic uses three steering views and four wheel phases, without squat samples.
Paint variants reuse those exports. Vehicles share the dynamic street lighting,
headlight cones, lamp glow and scene blur, with per-pixel traffic/prop/player depth.

The prototype road limit is **100 km/h**. Most drivers prefer 90-100 km/h,
with slower vehicles and occasional slower cars; traffic never exceeds the limit.
Drivers follow safe gaps, brake for vehicles including the player, signal for
1.3 seconds, and take 3.8 seconds to cross a lane (4.6 van / 5.0 minibus).
They reserve/check gaps ahead and behind, rather than changing lanes at random.
Spawning is outside the common visible corridor, with persistent offscreen
simulation and bounded recycling. Basic bumper/side contact slows and separates
the player; no damage system or game-over is introduced in this prototype.

`--traffic-smoke-test` runs a populated rendering/contact scenario; combine with
`--testarossa`, `--civic` or viewport flags. Existing isolated graphics smoke tests
keep traffic disabled for stable comparisons. Standalone `tests/traffic_tests.cpp`
checks speed caps, signalling, gradual movement, non-overlap, all five types,
bounded populations, deterministic seeds and player contact over a long drive.

Traffic wheel phases are checked to leave body pixels stationary outside the four
wheel regions. Reviewed axle pivots and tyre aspect correction prevent orbiting
parts and wheel-hop. Traffic shadows use front/back depth rays to each contributing
streetlight, replacing the earlier fixed screen-space ellipses.

## Evasive steering and traffic readability

Lift-off raises steering response by up to 30%; braking raises it by up to 80%.
Both blend through the pedal state. Maximum lateral movement gets smaller 12% /
28% assists, with braking retaining a lateral acceleration floor for evasive moves.
Steering release stays quick. This is deliberate game assistance, not a full
combined-grip vehicle simulation.

The stock brakeG coefficients are preserved; GameBrakeBoost applies 20% more
brake force and faster pedal take-up. Repeatable 100-0 km/h probes (third gear,
3000 initial rpm, immediate pedal command, no driver reaction delay) change from
S13 43.18m / EK9 41.03m / Testarossa 42.73m to 35.99m / 34.19m / 35.55m.
Baseline 70-0 mph probes were 54.39m (S13) and 53.88m (Testarossa). These were
already shorter than period Car and Driver results: 195ft/59.44m for a related
1990 240SX, and 210ft/64.01m for a 1985 Testarossa. Different tyres, variants and
test conditions preclude exact equivalence. No trustworthy measured EK9 stopping
distance was established. The new brakes are intentionally stronger for gameplay.
Sources:
https://www.caranddriver.com/reviews/a15141963/nissan-240sx-se-archived-test-review/
https://www.caranddriver.com/reviews/a15142218/1985-ferrari-testarossa-archived-road-test-review/

Amber exclamation triangles mark approaching traffic up to one second before
its centre enters the common visibility corridor. One marker per lane fades as
the sprite becomes visible. Receding traffic produces no entry alert. Markers
are drawn sharply above the scene, pause with simulation time and use the same
world lookahead for every aspect ratio.

Traffic factory palettes and estimated period-based spawn distributions are
recorded in assets/traffic/colours.md. Model-specific sales-by-colour data is not
claimed. Bus road-tyre faces use charcoal rubber; original silver hubs remain.

## Spatial traffic groups

Traffic supply now follows a moving spatial stream instead of attempting one
spawn every 1.8 seconds. Its layout is keyed by seed and 480m stream bands,
advecting at 24m/s (86.4km/h). Each band contains 8-11 staggered vehicles followed
by a quiet stretch. Crossing more stream distance replenishes more traffic, so
high speed cannot drain a fixed time-based supply. Ordinary relative-speed
behavior remains: faster driving still catches traffic more quickly.

Groups choose two, three or four occupied lanes (25/55/20 percent respectively),
with the subset shuffled per group. This leaves varying lanes open without
forcing AI vehicles to stay out of them. Existing following, indicators and
lane-change behavior continue normally, so moving vehicles can disperse groups.

New vehicles enter offscreen, at least 110m ahead or 90m behind, with coverage
maintained out to 420m ahead / 170m behind. Initial traffic stays at least 35m
from the player. The population budget remains 28; layout history is bounded.
The one-second entry warning and shared lookahead across aspect ratios are
unchanged. Long multi-seed tests compare density at 100/180/280km/h and check
quiet samples, open lanes and offscreen replenishment.

## Garage speed-limiter removal

The S13 and Civic each have a SPEED LIMITER: STOCK / DELETED button beside the
gearbox control. Choices save independently per car in NZ_GARAGE_4, alongside
existing paint and decals. Older saves load with stock governors. The Testarossa
shows NO FACTORY SPEED LIMITER rather than offering a redundant modification.

Removing the road-speed governor changes no power, ratios, engine rev limit or
traffic rules. A 120-second flat-road test reaches about 241km/h (S13) and
233km/h (EK9); those are simulation results, not claimed real-world specifications.
Restart/reset and manual/automatic modes all retain the selected limiter state.
`--limiter-delete` overrides the selected car for QA runs. Save roundtrip tests
check independent per-car settings and legacy migration; driving tests check
stock/deleted top speeds and retained first-gear rev protection.

Swift traffic sprites have charcoal wheel rubber and smaller rear lamp inserts
fitted within the body silhouette. All steering/spin maps were regenerated and
verified, including both indicator sides and stationary body pixels.

## Automatic kickdown

The automatic now compares wheel torque in usable lower gears when acceleration
is requested, instead of waiting for every engine to drop below 2600rpm. It needs
at least a 14% pulling-force benefit and keeps candidate RPM below 92% of the
normal upshift point. Light throttle/coasting use calmer model-relative RPM floors;
braking downshifts earlier for engine braking. Stock governors, rev protection
and manual gearbox controls are unchanged.

Automatic decisions take 0.20s for downshifts / 0.12s for upshifts. Shift cuts are
0.30s down (plus 0.04s per skipped gear) / 0.34s up, compared with immediate manual
commands and 0.22s / 0.28s shift cuts. Cooldowns and RPM/torque hysteresis prevent
hunting while preserving an advantage for well-timed manual selection.
Tests cover kickdown from 3500rpm, stable passing gears, calm lift-off, manual
shift timing, acceleration, limiter options and retained rev protection.

## Toyota Supra A80 SZ

The fourth garage car is the naturally aspirated 225PS Supra SZ, using a
five-speed gear set and an inline-six audio profile. Paint, black hood, three
decal layers, headlights, indicators, suspension pitch and saved limiter-delete
settings work like the other cars. Four garage entries fit above the paint panel.
Existing three-car version-4 saves migrate with their settings intact; the new
car starts with silver paint and its stock governor.

`--supra` selects it for ordinary play or any smoke mode. The runtime car README
records factory specification sources and prototype/visual approximations.
Build tests exercise all five vehicle and audio profiles. The audio snapshot
now reserves three car-ID bits, keeping the fifth car separate from active/mute
flags and guarding against aliasing as the roster grows.

Supra tone revision: retain phase-locked lower exhaust orders up to redline, reduce
bright induction/mechanical hiss, broaden low-mid pipe resonances and introduce
small pulse-strength variation. Regression checks now require a lower-order
exhaust energy floor at 3000/5000/6500rpm, in addition to clipping, lift-off and
RPM stability checks. This remains procedural audio; energy checks cannot certify
that a synthesized profile sounds like a recording of the real engine.

## Versioned car pipeline

Reusable asset tooling and the onboarding/sound process are now maintained in
[tools/car_pipeline](tools/car_pipeline/README.md). The portable runner provides
render, verify and sound-check commands with explicit external workspace paths.
Raw models, Blender scenes, caches, diagnostics and HTML experiments still stay
outside this game checkout. This versioned code folder is the deliberate exception
to the earlier experiment-only pipeline layout.

## Shared traffic lighting and horizon

Traffic cars and scenery no longer ramp their own sprite/depth/glow opacity near
the horizon. They keep opaque geometry and move through one longitudinal haze
applied after opaque world composition (with blur enabled or disabled). Culling
retains a padded contributor range beyond the horizon. The shared visible driving
corridor is unchanged across aspect ratios; the one-second traffic alert remains.

Traffic and player now cast road shadows through the SAME road shader. A native
point-sampled geometry atlas supplies traffic front/back depths at texture slot 8;
player maps keep slots 5-7. Each streetlight loses the same 80% direct contribution
when blocked, with ambient and other lights preserved. Shadowed lights are unioned
rather than stacking black overlays. Both paths clip rays to occupied geometry
bounds, preserving marching precision despite different canvas sizes. The old
traffic_shadow.fs overlay is removed.

Traffic still shares car.fs for paint/glass response and self-shadowing. Its own
head/tail/brake light spill now uses player-equivalent settings anchored to vehicle
dimensions, within the existing light budget. Imported traffic split normals are
properly inverse-transpose transformed during scale/flip and rotated during body,
steering and wheel animation. Corrected normal maps retain original colour, lamps,
depth, masks and silhouettes.

The traffic smoke test checks opaque geometry beyond the horizon and compares
identical player/traffic shadow volumes on the GPU: 1944 shadowed pixels, zero
mismatches in the synthetic two-light scene. Existing depth/foreground compositing,
scenery occlusion, glow, blur-toggle and widescreen checks also apply.

The base R34 uses a six-speed gearbox and AWD launch traction; the earlier cars
retain five gears. `--r34` selects it for play or smoke tests. Existing garage
saves retain their four cars' paint, decals and limiter settings when adding it.

## Wheelspin and traction assist

Driven wheels now have their own rotation speed and inertia. Too much torque
pushes the tyres beyond peak slip and reduces useful grip; engine RPM can flare
without equivalent road acceleration. Clutch engagement and axle load transfer
are smoothed. RWD, FWD and AWD use their respective driven-axle loads. Non-driven
wheels keep rolling at road speed; only the driven axle uses the slipping frames.
No extra car sprites were rendered.

**TRACTION ASSIST** is a game pedal helper available on every car from the garage
or **Esc > [3] Traction assist**. It feathers throttle with delayed feedback, so
some wheelspin remains. Hold **Shift+W** for manual gentler throttle.

**Factory TCS/ESC** is separate: only a car with the corresponding factory
hardware can enable it, from its garage button or **Y** during driving. Current
five selected period trims show **NOT FITTED**. R34 AWD stays active independently;
it is not switchable TCS/ESC. Future fitted cars select a documented road/sport
calibration: direct torque regulation for TCS, additional stability intervention
for ESC. The present ESC implementation is a highway-model approximation, not
an individual-wheel braking/yaw simulation. Both settings save per car; old
universal-aid saves migrate to traction assist. HUD: ASSIST, TC, ESC or SPIN.

Only sustained excess driven-wheel slip emits smoke: small low-opacity puffs
at the tyre contact patches, fading in under a second. Smoke stays in world
space, freezes on pause and clears on reset/car changes. The rival uses the same
physics and can feather its throttle as a driver rather than receiving hidden TCS.

The previous 4.9-second R34 calibration was measured before this tyre model.
It now measures about 5.08 s to 60 mph with TCS off; the floored Ferrari is about
6.35 s, game-assist about 5.84 s and a scripted feathered Ferrari about 5.17 s.
Driver technique is now part of the result. See docs/TRACTION.md for scope and
current limitations, and docs/PERFORMANCE.md for historical/current measurements.


## Racer contact

Player/rival bumps now share impact momentum according to mass, with grip-limited
resistance to gentle side pressure. Hard hits still move either car. Matched-speed
rubbing no longer applies a repeated forward-speed penalty, and road-edge contact
keeps both racers within bounds. Traffic collision penalties are unchanged.
See docs/CONTACT_PHYSICS.md and tests/racer_contact_tests.cpp for verification.
