# Prototype stock performance

## Selected stock variants

| Parameter | Silvia S13 K's | Civic Type R EK9 |
| --- | --- | --- |
| Reference variant | Japan, 1991-93, SR20DET, 5MT | Japan, base EK9 Type R, B16B, 5MT |
| Rated crank power | 205 PS at 6,000 rpm | 185 PS at 8,200 rpm |
| Peak torque | 275 Nm at 4,000 rpm | 160 Nm at 7,500 rpm |
| Curb mass used | 1,150 kg | 1,070 kg |
| Driver added in simulation | 75 kg | 75 kg |
| Driven wheels | Rear | Front |
| Gear ratios 1-5 | 3.321 / 1.902 / 1.308 / 1.000 / 0.759 | 3.230 / 2.105 / 1.458 / 1.107 / 0.848 |
| Final drive | 4.083 | 4.400 |
| Stock tyre dimensions used | 205/60R15 | 195/55R15 |
| Wheel rolling-radius approximation | 0.3135 m | 0.29775 m |

The visual S13 does not uniquely establish an engine/trim, so this prototype
explicitly chooses the later turbo K's, rather than an earlier CA18DET or an
NA Q's. EK9 mass varies by equipment; the model uses the 1,070 kg base reference,
not the lighter race-base or heavier comfort-equipped Type Rx.

## Model and uncertainty

Longitudinal force comes from an interpolated engine torque curve, the selected
gear, final drive and wheel radius. Traction is capped using driven-axle load and
acceleration load transfer. Aerodynamic drag, rolling resistance, engine braking,
service braking and gear-change torque interruptions oppose/interrupt drive.
The model adds a 75 kg driver and a 4% equivalent rotating-inertia allowance.

Published rated power, torque, mass, tyre dimensions and reference gearing anchor
the model. Full torque curves, drag area, drivetrain loss, tyre grip, brake grip,
turbo response and lateral response are engineering approximations, not measured
curves for these exact vehicles. Do not describe this as a validated simulator.
The tyre radius uses nominal dimensions, rather than a measured loaded radius.

The five-speed manual gearsets have an automatic driver: throttle/braking are
manual, but clutch launch and shifts are assisted. Both cars use an assumed
Japanese-market 180 km/h factory limiter. It cuts drive progressively around the
limit; this is separate from the unrestricted aerodynamic/gearing top speed.

Lateral movement uses speed-dependent steering response, a tyre-acceleration
limit shared with longitudinal acceleration/braking, and road-edge bounds. This
is a simplified highway handling model, not a full bicycle/suspension simulation.
Rendered body roll/yaw uses the existing three steering poses. Body squat/dive
has not been faked through rotating a whole sprite.

## Deterministic results

With a fixed 1/120 s timestep, full throttle, automatic clutch/shifts, level road,
normal grip and one 75 kg driver, the first prototype produces:

| Result | S13 | EK9 |
| --- | --- | --- |
| Model 0-100 km/h | 6.56 s | 7.21 s |
| Model 100-0 km/h distance | 43.7 m | 41.4 m |
| Model 100-0 km/h time | 3.11 s | 2.95 s |
| Governed speed | Approximately 180 km/h | Approximately 180 km/h |

The automated checks also cover no-input rest, shifting, steering symmetry,
continuous lateral movement, road bounds and timestep stability. Native graphics
smoke tests exercised acceleration, steering, animated wheels and braking with
both cars; shader compilation is checked and fails startup instead of silently
falling back to unlit rendering. Test captures/logs stay under `build/`.

## Sources

- Honda's 1997 Type R launch announcement (official rated power):
  https://global.honda/en/newsroom/worldnews/1997/4970822b.html
- Honda's original EK9 chassis/handling factbook:
  https://www.honda.co.jp/factbook/auto/CIVIC/19970822/cv97-008.html
- EK9 catalogue specification, including mass, torque, tyres and gear ratios:
  https://www.goo-net.com/catalog/HONDA/CIVIC/2003826/
- 1991-93 S13 K's catalogue specification, including SR20DET, mass and tyres:
  https://www.carsensor.net/catalog/nissan/silvia/F001/M002G001/
- Nissan's S13 heritage page distinguishes the earlier CA18DET and the 1991
  transition to the SR20 engine family; its 175 PS figure is the earlier car and
  is not used for this SR20DET prototype:
  https://www.nissan-global.com/EN/HERITAGE_COLLECTION/silvia_ks.html
- HKS explanation of the typical Japanese-market 180 km/h factory limiter:
  https://www.hks-power.co.jp/en/product/electronics/speed/sld/

Gearbox values for the S13 are reference FS5W71C/SR20DET five-speed values. Stock
variant, equipment and measured tyre/drivetrain data should be reviewed again
before introducing a larger roster or making comparative performance claims.

## Revised highway steering

Full steering uses a speed-dependent lateral velocity envelope in m/s:
`min(speed * 0.32, 1.7 + 3 / (1 + (speed / 21)^2))` with speed in m/s.
It ramps from zero at rest, reaches roughly 3.8 m/s at 50 km/h, then falls to
2.55 m/s at 120 km/h and 2.15 m/s at 180 km/h. Steering input and velocity
responses also become calmer with speed. Release uses quicker damping to limit
unwanted lateral drift. Available tyre grip still caps lateral acceleration,
including under braking/acceleration.

The earlier S13 response disadvantage was a prototype tuning choice, not a
validated factory comparison. The revised base response is 9.5 for the S13 and
9.0 for the EK9, a small preference for prompt S13 input. RWD does not establish
superior handling by itself; real comparative handling needs measured data.

## Base Skyline GT-R R34 (BNR34)

The fifth playable car is the base Japanese-market GT-R, not a V-spec/Nur or a
modified engine. It uses the advertised 280 PS at 6800 RPM, 392 Nm at 4400 RPM,
1540 kg curb mass plus the shared 75 kg driver, 2.665 m wheelbase, 245/40R18
nominal tyre radius (0.3266 m), six Getrag ratios
3.827/2.360/1.685/1.312/1.000/0.793 and a 3.545 final drive. Idle is modelled at
950 RPM, shifts at 7800 and redline at 8000. The 180 km/h JDM governor is enabled
until the garage limiter-delete option is selected; rev protection remains.

Sources: [Nissan's GT-R specifications](https://global.nissannews.com/en/releases/skyline-and-skyline-gt-r-undergo-minor-model-change)
support the shared RB26DETT, dimensions, tyres and six-speed ratios. That press
release's specification table is for the 1560 kg V-spec II; the base car's
1540 kg mass is from the [1999 base GT-R catalogue](https://www.goo-net.com/catalog/NISSAN/SKYLINE/1501420/).
The earlier JGTC competition-car heritage page is not used for stock tuning.

The torque curve between rated points, 0.83 drivetrain efficiency, 0.70 m2 CdA,
weight distribution and steering/grip parameters are modelling assumptions.
AWD uses both axles' traction budget rather than the single driven axle limit;
it does not simulate ATTESA's hydraulic control/torque split or HICAS. Steering
uses the same speed-sensitive highway assists as the rest of the roster. AUTO
is the game's shifting assist, not a claim that the stock GT-R had an automatic.

Current simulation: 0-100 km/h 5.48 s, 100-0 34.9 m in 2.48 s with the shared
game braking boost, governed maximum about 180 km/h. In the existing 90-second
limiter-delete test it reaches 253.6 km/h. These are simulation results, not
factory measurements. Older cars retain their five-speed gearbox limits.

RB26 audio uses a six-cylinder turbo profile, with restrained whistle/noise,
low-order exhaust body and continuous lift-off. It is procedural rather than a
recording. Audio tests cover fifth-car atomic IDs, load/coast, headroom, low-RPM
variation and high-RPM exhaust body; player listening remains the timbre check.

## R34 versus Testarossa sanity check

A standing launch and our race's 50 km/h rolling start are different tests. AWD
can help launch traction but provides much less benefit once both cars are
moving without wheelspin. A mid/rear-engine Testarossa also has substantial
rear driven-axle load. Its extra power matters increasingly with speed.

The [Car and Driver original Testarossa test](https://www.caranddriver.com/reviews/a15142218/1985-ferrari-testarossa-archived-road-test-review/)
measured 0-60 mph in 5.0 seconds. [Evo's R34 review](https://www.evo.co.uk/nissan/skyline/gt-r-r34)
lists a claimed 4.8-second 0-60 mph figure and describes the car's strong AWD
traction. These were not a controlled head-to-head test, and the Evo feature
concerns a UK V-spec rather than the base JDM configuration in this prototype.
They support a close contest and a plausible R34 launch edge, not a guaranteed
large margin or dominance in a rolling race. Do not compare 60 mph with 100 km/h
as if they were the same endpoint.

Measured using current stock power/torque/gearing, automatic shift assist,
no traffic/steering, 120 Hz integration and interpolated speed crossings:

| Test | Testarossa | R34 GT-R |
| --- | --- | --- |
| 0-30 km/h | 1.40 s | 1.35 s |
| 0-50 km/h | 2.26 s | 2.09 s |
| 0-60 mph | 4.85 s | 5.20 s |
| 0-100 km/h | 5.06 s | 5.48 s |
| 0-160 km/h | 10.54 s | 13.32 s |
| 50-100 km/h rolling, both starting in second | 3.26 s | 3.88 s |
| 50-160 km/h rolling, both starting in second | 8.74 s | 11.73 s |

Assessment: the model already gives the R34 the initial launch advantage. It
then loses to the more powerful Ferrari, including in the rolling-start race.
Its 0-60 result is conservative relative to Evo's claimed figure; the Ferrari
is near the quick end of period results. Our simple clutch/turbo launch model
has no driver-selectable preloaded-boost launch, and the R34 uses the nominal
280 PS rating rather than assuming an unverified higher output. Acceleration
curves, boost and shift calibration are approximations; these numbers are not
proof of an exact real-world ranking. No physics values were changed for this
check. Revisit launch/clutch/boost calibration against comparable measured
stock-car tests before adding power simply to force a desired winner.

Reproduce the model comparison:

```text
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tools/performance_sanity.cpp -o build/performance_sanity.exe
./build/performance_sanity.exe
```

The governor is deleted in this diagnostic only so it can also report 0-200
figures; it does not affect the sub-180 comparisons above. Times of zero mean a
threshold was at/below starting speed or not reached within the diagnostic.
