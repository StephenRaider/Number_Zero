# Racer contact tuning

Player-versus-rival contact uses `src/racer_contact.h`; traffic retains the
existing `TrafficWorld::ResolveContact` response unchanged. This is a game-tuned
highway contact model, not a full rotating rigid-body/crash simulation.

## Response

- Keep the shared 4.50 m longitudinal / 1.96 m lateral contact extents.
- Resolve the shallowest rectangular overlap axis rather than using a fixed
  2.5 m longitudinal threshold to classify all impacts.
- Use car mass plus the existing 75 kg driver allowance. Hard-hit penetration
  correction is inverse-mass weighted, so a lighter car moves more.
- Closing normal velocity produces an inelastic, mass-weighted impulse. Rear
  impacts exchange momentum rather than clamping only the following car's
  speed to the leader's. No bounce or arbitrary engine/handling advantage.
- A gently squeezed car can support itself through its tyres if it is holding
  position or steering against the impact. The external road reaction budget
  is bounded by mass, grip and timestep. A 1.20 static-contact firmness factor
  multiplies the existing assisted lateral acceleration scale. This is explicit
  game tuning applied equally to player and rival, not an OEM tyre measurement.
- That support reduces both collision-induced lateral drift and positional
  ratcheting. A strong side impact exceeds the budget and still moves either car.
- Tangential rubbing uses a small friction impulse capped at 8% of the normal
  impact impulse, dependent on relative tangent velocity. Remove the old
  repeated 0.997 speed multiplier: matched-speed rubbing no longer drains both
  racers' forward pace every collision tick.
- Separating bodies receive penetration correction without another velocity
  impulse or cancellation of their lateral escape velocity.
- At either road boundary, remaining penetration correction goes into the car
  with free room. Outward pair velocity is stopped when one body is pinned;
  neither car is forced beyond the verge or left overlapping.
- Numerical separation is 2 mm at 120 Hz and decreases quadratically for smaller
  steps, avoiding large repeated micro run-ups in resting contact.

The same function and rules serve both roles. No rival-only mass/grip multiplier,
no changes to traffic impact severity, and no new defensive/aggressive AI tactics.
Existing contact event logs capture the resulting velocity/position changes.

## Controlled checks

`tests/racer_contact_tests.cpp` covers rear-impact momentum sharing, lighter-body
movement, grip-supported squeeze resistance, finite response to hard impacts,
no collision energy gain in the checked impacts, no repeated scrape drag,
escape velocity, pinning at both road edges, role/left-right symmetry, sustained
pressure at 120/240 Hz, and the existing traffic rear/scrape penalties.

A two-second isolated test starts an R34 beside a Testarossa, both at 180 km/h.
The R34 continuously steers into it; the Ferrari holds its original lateral
position through a simple steering controller. This is a reproducible contact
fixture, not a claim about the live rival's strategy. The new response displaces
the Ferrari about 0.168 m at 120 Hz (0.152 m at 240 Hz), retaining about 176.9 km/h.
A local replay of the previous solver displaced it about 0.307 m and reduced it
to about 154.4 km/h. The previous replay remains ignored QA data; the supported
new thresholds are checked by the source test. This is primarily a reduction
in easy side-pressure speed suppression, not immunity to being blocked.

```text
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc tests/racer_contact_tests.cpp -o build/racer_contact_tests.exe
./build/racer_contact_tests.exe
./build/NumberZero.exe --race-smoke-test --r34
```

Existing full-distance rival/race tests and contact telemetry tests remain
required. No claims of contact-free AI or complete protection from intentional
boxing-in: smarter tactical escape is separate future work.
