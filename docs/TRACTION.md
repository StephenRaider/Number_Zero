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

## Game assist and factory electronics are independent

Vehicle.tractionAssist is the universal game aid. It limits the driver's pedal
using imperfect slip feedback (target starts reacting above 7.5%, 35/s cut,
3/s recovery, minimum 18% pedal). It has no tyre force budget or brake control.
It permits transient slip and is deliberately less efficient than a well-tuned
factory controller or skilled pedal control. Garage TRACTION ASSIST or pause
[3] toggles it. Shift+W is a separate manual 65% pedal request.

Vehicle.tcsEnabled requests the installed factory electronics package.
CarSpec.factoryTcs/factoryEsc gate it in physics, UI and save loading; a saved
true flag or a command-line request cannot give an unequipped car the hardware.
Only equipped cars expose the garage toggle and Y driving hotkey. TCS changes
delivered axle torque after the pedal/engine calculation, leaving pedal state
independent. ESC additionally reacts to path/steering disagreement and powered
slip with a torque demand, corrective lateral damping and modest deceleration.

The current highway model has no independent yaw or four brake actuators, so
ESC is explicitly a stability approximation. Actual systems compare yaw and
steering and brake individual wheels; see [Bosch ESP](https://www.bosch-mobility.com/en/solutions/braking-systems/electronic-stability-program/).
Do not claim this implementation reproduces those actuators or an OEM ECU.

ElectronicsTune selects explicit Road/Sport calibration templates. Road aims at
4.5% slip with slower torque recovery and earlier stability intervention; Sport
aims at 8.5%, responds and recovers faster and tolerates more lateral disturbance.
Sport makes better progress in the launch test without granting more tyre grip
or power. These are game calibration assumptions, not measured OEM constants.
Assign by exact model/year/trim and documented system character; do not infer
capability purely from price or a 'sports car' label. Future specific cars may
need different profiles. TCS-only cars never receive ESC authority.

### Current selected trims

All five current configurations are treated as not fitted: S13 K's SR20DET,
EK9 Type R, original F113 Testarossa, early JDM A80 SZ 5MT, base BNR34 GT-R.
Do not confuse the original Ferrari with the new 849 Testarossa or transfer
later/turbo trim equipment to the base SZ. The equipment audit references:

- [Honda 1997 EK9 equipment](https://www.honda.co.jp/factbook/auto/CIVIC/19970822/cv97-012.html): helical mechanical LSD and ABS, no VSA/TCS listed.
- [Toyota 1993 JDM release](https://global.toyota/en/detail/7868203): distinguishes SZ from turbo electronic-throttle models. [Toyota UK turbo pack](https://media.toyota.co.uk/wp-content/uploads/sites/5/pdf/Supra-Gen4-UK-launch-pack-1993.pdf) describes TRC using rear brakes, sub-throttle and ignition; that turbo equipment must not be assigned to SZ.
- [Haltech ATTESA description](https://support.haltech.com/portal/en/kb/articles/nissan-attesa-control): front axle clutch torque allocation, distinct from a torque-cut/stability switch. Base R34 keeps the current AWD traction approximation; no ATTESA-disable option is added.

No road/sport factory electronics are fabricated for the existing roster to
showcase the implementation. Synthetic equipped specifications in tests exercise
the controllers; these do not appear as selectable vehicles or hidden upgrades.

The rival predicts and drives the same physics and feathers its own pedal using
observed slip. Its driver input is separate from either player assistance flag.
Its local swept collision checks and abortable merges remain unchanged.

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

NZ_GARAGE_6 stores separate per-car game-assist and factory-package flags after
the limiter flag. Versions 1-4 retain cosmetics/gearbox/limiter with defaults;
version 5's universal aid migrates to the game-assist field. Factory requests
are sanitized against the exact car's equipment. Paint and decals are unchanged.

Race CSV logs retain slip, factory requests, clutch, tread speed and pedal;
append game-assist request/intervention and factory TCS/ESC intervention flags.
Thus deliberate pedal reduction can be distinguished from wheel-torque cuts.

## Verification

```text
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/traction_tests.cpp -o build/traction_tests.exe
./build/traction_tests.exe
./build/NumberZero.exe --wheelspin-smoke-test --testarossa
./build/NumberZero.exe --wheelspin-smoke-test --testarossa --traction-assist
./build/NumberZero.exe --decal-smoke-test --testarossa
```

The physics suite checks hardware gating even with a forged true request,
independent imperfect game assist, equipped synthetic road/sport controllers,
TCS-only versus ESC authority, plus slip/RPM/animation, smoke and timestep checks.
Native checks cover assist launches, axle animation, current unequipped garage UI,
v5-to-v6 migration, cosmetics preservation and independent v6 flags. Existing
braking, limiter, gearbox, sound and full-distance race tests remain required.
Captures/logs stay in ignored build/qa.
