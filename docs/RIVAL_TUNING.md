# Rival driving diagnostics

The initial driver rejected adjacent lanes with fixed distance/closing-speed
thresholds, required almost complete lateral settling before another decision,
and evaluated only one lane change. It also braked for the car it was leaving
behind, without predicting when the lateral overlap would end. A farther car
in the passing lane could therefore veto a pass that the Testarossa could
complete and return from safely. The previous gap-opening fix did not solve
that limitation.

The replacement uses short physics rollouts (2.6 s, 50 ms samples) with the
configured CarSpec: steering response, available grip, throttle, gearbox and
brakes. Candidate paths include holding, an adjacent lane, and a subsequent
adjacent change/return after settling. Continuous relative rectangle sweeps
check traffic between prediction samples. Fast and braking strategies are
compared by predicted progress, terminal speed and clearance, with small
maneuver costs to discourage oscillation. The first action is applied, then
local observations are refreshed about every 115 ms. This is deliberately
short-range tactical prediction, not knowledge of traffic-generation patterns.

RivalDriver::Configure(car, skill) supports different physical cars and driver
confidence. Lower skill increases reaction delay and preferred clearance; grip
and steering capability come from the actual vehicle rather than a speed-class
buff. The current opponent uses the stock Testarossa with skill 0.9. This does
not assert that every Ferrari has better lateral grip than every Civic.

A post-contact buffer bug was also found while logging: two cars could stop
close together and reject every prediction because their initial positions
were already inside the preferred clearance. Predictions now allow increasing
an existing small clearance while retaining the physical collision envelope.

## Traffic retention across a race gap

Traffic remains alive throughout the corridor from the trailing racer to the
leader, not just in two disconnected windows around them. Spawn history covers
the same corridor, so cars generated for the leader are still present when the
follower arrives. Only traffic behind both racers or ahead of both retention
limits is removed. The population budget grows by stream band with separation
and remains capped; single-car Drive keeps its previous 28-car budget.

The previous two-window cleanup deleted traffic between the racers while
retaining its emitted-slot history. This caused empty road for the follower
once the lead grew to roughly a kilometre. Multi-minute regression runs now
check both lead directions at 1-3.6 km gaps over three seeds, retaining roughly
3 cars per 145 m ahead of the trailing racer, including quiet stretches. A
separate test verifies a specific car survives between both local windows.

## Reproducible comparisons

The table below was captured at commit 4fe1191, before the subsequent traffic
retention fix. It records that tuning comparison rather than current race times.

Both cars were driven by the same version of the local driver, using a stock,
limited Civic against the stock Testarossa, six fixed traffic seeds and rolling
50 km/h starts. This is a diagnostic opponent, not recorded human play, and
cannot explain the precise unrecorded player encounter. After the change the
Civic driver also receives its own CarSpec for prediction. Both racers share
the same traffic simulation, without engine/handling buffs or catch-up.

| Seed | Previous race time / winner | New race time / winner | Previous rival braking | New rival braking |
| --- | --- | --- | --- | --- |
| 11 | 232.1 s / Ferrari | 180.7 s / Ferrari | 58.9 s | 6.0 s |
| 42 | 225.9 s / Ferrari | 179.9 s / Ferrari | 28.3 s | 7.8 s |
| 197 | 228.4 s / Ferrari | 181.4 s / Ferrari | 42.9 s | 7.2 s |
| 7 | 256.3 s / Civic | 176.3 s / Ferrari | 78.5 s | 4.7 s |
| 12345 | 204.5 s / Ferrari | 157.7 s / Ferrari | 25.9 s | 3.3 s |
| 901 | 217.4 s / Ferrari | 169.1 s / Ferrari | 34.1 s | 5.1 s |

The new runs still had 2–4 rival contacts per 10 km: confidence improved without
making it flawless. A separate controlled scenario verifies passing a nearby
car via a lane containing a farther car, then returning without contact. Other
checks cover creating a gap, recovering after contact, full-throttle clear-road
behavior, bounded perception, finish order and six complete 10 km races.

Build the standalone comparison with the same C++17 toolchain as the game:

```text
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tools/race_benchmark.cpp -o build/race_benchmark.exe
./build/race_benchmark.exe current
```

It writes build/qa/rival_current.csv. Historical local captures from this
comparison are rival_baseline.csv and rival_predictive.csv. The files are
ignored; source tools and this report are committed. A missing finish fails the
benchmark rather than being reported as a Ferrari win.

Live races draw a fresh traffic seed at each garage start, R restart or Rematch,
from a generator initialized with launch-time system/clock entropy. Consecutive
seeds cannot repeat. Smoke tests retain seed 12345 for stable comparisons, and
standalone tests continue to accept explicit seeds.

Live play writes build/qa/race_telemetry.csv at 4 Hz: session, seed, race phase/time,
both cars' speeds/distances, lead, throttle/brake, lateral position, target lane,
front gap/closing speed, left/stay/right path scores, rejected predictions,
gap-preparation state and contacts. Scores of -1e9 mean no permitted safe path
in that direction. Logs reset per launch and contain successive sessions.

## Civic top-end audio

Only the Civic runtime profile changed. Broad, lower-Q exhaust resonances,
reduced high-cam brightness, wider/quieter induction and restrained mechanical
noise replace the narrow high-RPM drone. Lower exhaust orders preserve body;
the VTEC state, pitch/RPM tracking, shifts and lift-off remain active.

At full load, 8300 RPM output RMS fell from 0.1767 to 0.1431 (about 19%). In the
steady diagnostic sample, energy above 1 kHz fell from 7.3% to 0.35%. These are
signal measurements, not proof of recorded-engine authenticity; this remains a
procedural synth and needs player listening feedback. Audio checks enforce a
controlled top-end level, low-RPM variation, cam engagement/release, load/coast
contrast, silence/mute and finite unclipped output.

```text
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tools/audio_profile_probe.cpp -o build/audio_profile_probe.exe
./build/audio_profile_probe.exe current
```

This exports build/qa/civic_top_end_current.wav at 5000/6500/7500/8300 RPM.
Before/after captures remain local in build/qa.
