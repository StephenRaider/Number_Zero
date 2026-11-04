# Driven-wheel slip, clutch and TCS

## Physics contract

Vehicle stores driven tread speed (m/s), integrated tread distance, signed slip
ratio, clutch transfer, smoothed axle-load transfer and TCS intervention. Road
speed/distance remain independent. Wheel inertia is equivalent to 4% of car plus
driver mass and replaces the old blanket 4% longitudinal inertia multiplier.

Slip is (wheelSpeed-roadSpeed)/max(roadSpeed,3). The low-speed denominator avoids
a singularity at rest. Grip rises to its peak near 6% slip, then drops toward
70% of that peak as excessive slip develops. Torque left after tyre force
accelerates the driven wheels instead of being silently discarded. The tyre/
vehicle momentum equations run at at least 900 Hz; engine/clutch/gear coupling
runs at at least 120 Hz, including larger AI prediction steps. Engine torque
ramps down approaching redline to avoid a timestep-dependent on/off discontinuity.

Axle normal load accounts for drivetrain and smoothed acceleration transfer.
The Ferrari has a 0.94 longitudinal grip scale as an explicit older-road-tyre
modelling choice. This is not a measured tyre specification. Lateral response
loses some capacity under excessive powered slip; the existing highway steering
assist remains. This is not a full four-wheel yaw, differential, tyre temperature,
clutch thermal or ABS/locked-wheel model. Service braking retains the existing
assisted formulation. Do not market it as a validated vehicle simulator.

Clutch transfer ramps from rest and disengages/re-engages during shifts. The
existing assisted launch RPM still approximates controlled clutch slip. Engine
RPM tracks driven-wheel motion, so wheelspin creates a real RPM flare; it is
not just a wheel animation effect. Coasting/braking let wheel speed recover.

## TCS and driver control

CarSpec.factoryTcs defines a future car's default; the present five configurations
use false. Each player car also has an independently saved optional assist flag.
This lets the prototype demonstrate both behaviors without inventing factory
hardware for an old model. In particular, ATTESA-style AWD traction allocation
is distinct from torque-cut TCS (see [Haltech's ATTESA description](https://support.haltech.com/portal/en/kb/articles/nissan-attesa-control)).

With assist on, drive torque is reduced before/when slip exceeds the useful
region. No grip or horsepower is added. tcsCut feeds the engine audio load and
tcsActive feeds the HUD. Y toggles it while driving or in the garage; the garage
button exposes the same state. Shift+W requests 65% throttle, allowing a keyboard
player to launch gently. A skilled driver can outperform conservative TCS.

The rival predicts and drives the same physics and uses observed slip to feather
its pedal. Its torque cut is a driver input, not hidden TCS or a power advantage.
It may abort a still-in-progress merge if a newly visible blocker makes the
current path unsafe; all alternatives still pass the swept collision checks.

## Rendering and smoke

The existing wheel maps are partitioned by recovered front/rear surface position.
The driven axle uses tread-distance animation/blur; the other axle uses road
rotation. All-wheel-drive cars use the driven signal on both axles. The cache
includes both wheel frames and keeps categorical normal/depth data point sampled.

Smoke requires >20% slip and >1.5 m/s excess tread speed for at least 0.12 s,
positive throttle and no braking. Puffs are emitted only at driven axle contact
patches (front for FWD, rear for RWD, both for AWD), at a bounded rate. Each
emitter keeps at most 48 puffs. Radius, low opacity and 0.42-0.56 s lifetime keep
it restrained. Positions are world anchored, drawn behind car/prop surfaces;
pause freezes age and reset/garage clears history. No smoke on normal rolling,
brief slip noise, standing with no input or a clean TCS-assisted launch.

## Saves and telemetry

NZ_GARAGE_5 adds a TCS flag after each car's limiter flag. Versions 1-4 still
load their paints, decals, gearbox and limiter settings, with TCS defaults.
Native save checks exercise on/off flags on different cars and old-save migration.

Race CSV logs additionally include player car ID, player/rival slip and TCS
states, player clutch/tread speed and player throttle. Thus a wheelspin loss or
TCS intervention can be distinguished from deliberate opponent hesitation.

## Verification

```text
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/traction_tests.cpp -o build/traction_tests.exe
./build/traction_tests.exe
./build/NumberZero.exe --wheelspin-smoke-test --testarossa
./build/NumberZero.exe --wheelspin-smoke-test --testarossa --tcs
./build/NumberZero.exe --decal-smoke-test --testarossa
```

The physics suite checks excess torque causing slip/RPM flare, falling grip,
TCS recovery and its traction benefit, driver feathering, AWD advantage,
timestep stability, coasting/braking recovery, smoke axle positions, lifetime,
bounds, pause and reset. Native checks assert independent axle animation and
smoke emission with TCS off, and intervention with bounded slip/no sustained
smoke with it on. Existing braking, limiter, gearbox, sound and full-distance
race tests remain required. Captures/logs stay in ignored build/qa.
