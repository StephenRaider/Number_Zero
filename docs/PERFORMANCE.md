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

Original import baseline: 0-100 km/h 5.48 s, 100-0 34.9 m in 2.48 s with the shared
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

Pre-calibration snapshot (before the 4.9-second launch request), measured using
stock power/torque/gearing, automatic shift assist,
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

## Roster audit and R34 launch calibration (5 November 2026)

At the user's requested approximately 4.9-second **0-60 mph** target, the R34
now measures **4.941 s at 120 Hz / 4.936 s at 240 Hz**. The distinct 0-100 km/h
result is 5.218 s with interpolated crossings (5.23 s on the tick-based timer).
Its reference engine output remains 280 PS / 392 Nm, mass remains 1540 kg,
and neither grip, boost multiplier, torque curve, ratios nor drag was raised.

The assisted launch RPM rises from 3800 to 4400, putting initial clutch slip
near the torque peak. The R34's assisted upshift torque interruption changes
from 0.34 to 0.20 s, and decision dwell from 0.12 to 0.04 s. Its manual upshift
interruption is 0.18 s. These are game driver/clutch calibration parameters,
not claims of measured Getrag shift times. Earlier cars retain their existing
0.28 s manual / 0.34 s assisted upshift and 0.12 s assisted dwell. Gearbox ratios,
rev protection, factory governor and the quicker-manual option are preserved.
Quicker shifts also modestly improve rolling acceleration; this is not confined
to a special 0-60 multiplier. R34 50-100 km/h in second falls from 3.88 to 3.74 s.

All-car measurements, full throttle on level road, 75 kg driver, no rollout,
reaction time, traffic or steering. The manual column is a scripted ideal driver
shifting at the configured shift RPM, rather than a human reaction benchmark.

| Car | 0-60 mph assisted | 0-60 mph ideal manual | 0-100 km/h assisted | Assessment |
| --- | --- | --- | --- | --- |
| S13 K's SR20DET | 6.32 s | 6.26 s | 6.60 s | Rated inputs match the selected catalogue; no strong reason to retune from this audit. |
| Civic EK9 | 6.98 s | 6.87 s | 7.28 s | Conservative. Published launch results/weights vary; no retune to a single fastest claim. |
| Testarossa | 4.85 s | 4.79 s | 5.06 s | Quick end of period results; close to C/D's measured 5.0 s to 60 mph. |
| Supra SZ (NA) | 6.50 s | 6.47 s | 6.81 s | Configuration is the 225 PS five-speed NA car, not the much quicker Turbo. |
| R34 GT-R | 4.94 s | 4.91 s | 5.22 s | Meets the requested approximate 4.9-second target. |

The other four cars' acceleration results are unchanged. This is a sanity check
of rated inputs and model outputs, not certification of exact factory times.
The following primary sources anchor model/variant selection:

- [1991 S13 K's catalogue, Toyota GAZOO](https://gazoo.com/catalog/maker/NISSAN/SILVIA/198805/1502068/):
  SR20DET, 205 PS, 1150 kg. Do not substitute CA18DET/NA/US 240SX results.
- [Honda's original EK9 factbook](https://www.honda.co.jp/factbook/auto/CIVIC/19970822/cv97-013.html):
  B16B, 185 PS and equipment-dependent mass. The game uses the 1070 kg base.
  [Evo's retrospective](https://www.evo.co.uk/honda/civic-type-r/208642/honda-civic-type-r-ek9-1997-2000-review-1990s-jdm-icon-makes-a-peugeot)
  describes an aggressive 5.7-second test result with a 1040 kg specification;
  its prose/table also mix 0-60 and 0-62 labels. That is insufficient evidence
  to force this heavier, assisted-launch model to the same quickest number.
- [Toyota's original A80 launch release](https://global.toyota/en/detail/7868203):
  SZ is 225 PS / 29.0 kg-m normally aspirated, distinct from 280 PS twin-turbo RZ.
- [C/D's original Testarossa road test](https://www.caranddriver.com/reviews/a15142218/1985-ferrari-testarossa-archived-road-test-review/):
  5.0 seconds to 60 mph. Market/weight/test protocol differ from the Euro-power
  reference model, so this is a plausibility anchor, not an exact fitting target.

No well-matched instrumented stock S13 K's or base SZ acceleration test was
established in this pass; their checks are lower-confidence than the Ferrari's.
Do not promote third-party simulator estimates to measured evidence. Unrestricted
speeds (roughly S13 241, EK9 233, SZ 247 and R34 254 km/h in the existing
90-second runs) remain estimates requiring drag/gear/dyno validation. Japanese
cars retain the 180 km/h stock governor. Brake distances around 35-36 m from
100 km/h include the deliberate common game brake boost, not factory brake data.

Reproduce the assisted table with tools/performance_sanity.cpp, or pass
--manual for the ideal manual trace. The vehicle regression now explicitly
checks 60 mph (96.56064 km/h), the R34 target at both 120/240 Hz, sixth gear,
manual advantage, AWD launch traction, limiters and all earlier cars' behavior.
