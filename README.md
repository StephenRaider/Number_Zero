# Number Zero

A native Raylib highway-driving prototype with isometric pixel cars and dynamic
lighting. Choose the Silvia S13 or Civic EK9, choose its paint, then drive a
straight four-lane highway at night.

## Play on this machine

Run `build/NumberZero.exe`, or build and launch with:

```powershell
.\build.ps1 -Run
```

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
Five-speed gear ratios are simulated with automatic shifts for this prototype.
The game pauses on focus loss. The window can be resized; rendering uses a
dynamic pixel canvas with nearest-neighbour integer scaling. Widescreen reveals
more highway; taller windows gain vertical space. The garage stays centred.
Press **F11** to toggle borderless fullscreen.

## Implemented

- Garage with both cars, paint selection, optional two-tone/black hood.
- Scrolling straight highway, lane markings, barriers and street lamps.
- GPU normal-map lighting, highlights, depth-aware self/ground shadows,
  headlight beams, tail-light spill and brake-light emission.
- Separate depth-composited body/wheel layers, steering poses and wheel spin.
- Fixed-120-Hz drivetrain, drag, rolling resistance, braking and lateral motion.
- Speed, gear, RPM, pedal inputs and distance HUD.

The road is currently an empty driving test: traffic, rivals, parking areas,
damage, and acceleration/braking body-pitch poses are future work.
The stock wheels remain the car-specific prototype exports; the planned shared
standard-size wheel library has not been implemented.

## Build and verify

The Windows x64 Raylib 5.5 static library and headers are included under
`third_party/raylib`, with their license. `build.ps1` first uses the sibling
`../toolchain/w64devkit/bin/g++.exe`, then falls back to `g++` on PATH.
Use a Windows x64 MinGW-w64 C++17 compiler with this library.

```powershell
.\build.ps1 -Test
.\build\NumberZero.exe --smoke-test
.\build\NumberZero.exe --smoke-test --civic
```

`-Test` builds and runs deterministic drivetrain/handling tests. Graphics smoke
tests create a hidden native window, run a scripted drive and brake sequence,
and export garage/driving/braking frames to ignored `build/qa/`.
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
Paint uses stronger clearcoat highlights with a narrow and a broad specular lobe.
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

`build.ps1 -Test` includes offline sound checks and writes diagnostic WAVs under
ignored `build/qa/`. `build/NumberZero.exe --audio-smoke-test` (optionally
`--civic`) exercises the real output callback during a scripted drive.
Ordinary `--smoke-test` stays silent and needs no output device.
See `third_party/car_engine_synth/README.md` for provenance and the BOV fix.

Closed-throttle coasting and braking retain an RPM-driven engine bed instead of
fading toward silence. Throttle changes the loaded tone and intake sound;
engine pitch still follows actual drivetrain RPM. Audio regressions compare
loaded/coast levels at 3000, 5000 and 7000 RPM for each car.

## View and instruments

The driving camera uses 19.5 pixels/metre instead of 26, so the car/road are 25%
smaller with correctly aligned depth/normal-map lighting and shadows. The car
sits farther back toward the southwest, leaving more road visible ahead.
Garage previews retain their original size; the studio key light orbits the
car roughly once every 11.4 seconds.

The bottom-right cluster groups a needle RPM dial with redline markings,
digital speed, gear, trip distance, throttle/brake bars, lamp/audio status and
warnings. It anchors to the current viewport when resizing.

Graphics checks support `--ultrawide` (2560x1080), `--superwide` (3840x1080),
and `--tall` (960x900), combined with `--smoke-test`. Use
`--resize-smoke-test` to exercise render-target resizing during a drive.
Captures include logical dimensions in their filenames under `build/qa/`.
