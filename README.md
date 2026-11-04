# Number Zero

A native Raylib highway-driving prototype with isometric pixel cars and dynamic
lighting. Choose the Silvia S13 or Civic EK9, choose its paint, then drive a
straight four-lane highway at night.

## Play on this machine

Run `build/NumberZero.exe` after building. Convenience scripts such as
`build.ps1` and `Play.ps1` may be present on this machine, but are local-only
and intentionally excluded from Git alongside `AGENTS.md`.

The executable finds the committed assets relative to its location. It needs no
Blender, Python, browser, network connection or audio device.

In the garage, select a car and paint swatch, or use the colour picker. The S13
also offers two-tone paint, and either car can use a black bonnet. Click Start
Night Drive or press Enter.

| Control | Action |
| --- | --- |
| W / Up | Accelerate |
| S / Down / Space | Brake |
| A / D or Left / Right | Continuous lateral steering |
| H | Toggle headlights and beams |
| T | Toggle tail lights; braking still activates brake lights |
| Escape | Pause/resume; exit from garage |
| Tab | Return to garage |
| R | Reset the drive |

Steering is continuous rather than lane snapping. Its response depends on speed,
car setup and available tyre grip. The car settles after releasing steering.
Choose AUTO or MANUAL in the garage. Manual uses Q down / E up with an assisted
clutch, shift delay and over-rev downshift protection; AUTO shifts itself.
The game pauses on focus loss. The window can be resized; rendering uses a
dynamic pixel canvas with nearest-neighbour integer scaling. The road camera fits a shared world-space visibility corridor into every
aspect ratio, with the same forward fog limit for all players. The garage stays centred.
Press **F11** to toggle borderless fullscreen.

## Implemented

- Garage with both cars, paint selection, optional two-tone/black hood.
- Scrolling straight highway, lane markings, barriers and street lamps.
- GPU normal-map lighting, highlights, depth-aware self/ground shadows,
  headlight beams, tail-light spill and brake-light emission.
- Separate depth-composited body/wheel layers, steering poses and wheel spin.
- Fixed-120-Hz drivetrain, drag, rolling resistance, braking and lateral motion.
- Speed, gear, RPM, pedal inputs and distance HUD.

The highway is currently a driving test. Traffic, rivals, parking areas,
and damage are future work.
The stock wheels remain the car-specific prototype exports; the planned shared
standard-size wheel library has not been implemented.

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
```

Graphics smoke tests create a hidden native window, run a scripted drive and
brake sequence, and export captures under ignored `build/qa/`:

```text
./build/NumberZero.exe --smoke-test
./build/NumberZero.exe --smoke-test --civic
```

See `docs/PERFORMANCE.md` for stock references, assumptions and measured model
results. These results are simulation estimates, not factory performance claims.

## Project layout

- `src/main.cpp`: garage, native controls, rendering, highway and HUD.
- `src/vehicle.h`: stock car data and fixed-step driving model.
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
25 vector-authored designs with the arrows or mouse wheel over the preview.
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
