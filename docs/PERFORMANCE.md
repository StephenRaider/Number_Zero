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
