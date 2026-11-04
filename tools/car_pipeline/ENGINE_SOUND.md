# Engine sound acceptance standard

The current game uses procedural profiles in `src/engine_sound.h` and the DSP in
`third_party/car_engine_synth/`. Tests live in `tests/audio_tests.cpp`. Keep these
as the single sources of truth; do not copy forks into the asset tool folder.

## Required behavior

- Match cylinder count, aspiration, idle/redline and broad engine character.
- Preserve low/middle exhaust body through high RPM instead of collapsing into
  one thin rising firing-frequency note. Avoid sustained DC saturation.
- Give low RPM combustion variation, with variation settling at higher RPM.
- Let throttle change exhaust/intake character as well as loudness. Intake hiss,
  mechanical noise and turbo whistle should support the exhaust, not overwhelm it.
- Keep lift-off audible at high RPM, with a distinct unloaded character. Follow
  gear changes and pause/mute/car-switch controls without clicks or dropouts.
- Retain headroom. A louder profile is not automatically a better profile.

## Automated gates currently implemented

The sound-check runner compiles and runs the existing audio suite: finite/clipped
sample checks, output presence, lift-off/load contrast, idle versus high-RPM
presence, mute/pause/switching, atomic car-ID publication, low-RPM variation and
VTEC/turbo behavior. Ferrari has low-body checks; Supra additionally requires a
lower-exhaust-order energy proxy above 0.28 at 3000/5000/6500rpm. This is a
profile-specific two-pole 250Hz lowpass/25Hz rejection measurement, not a universal
bass percentage or a rule that every engine must be as bassy as a flat twelve.
New engine families need appropriate limits and reference review.

## Listening gate

Audition at consistent playback level: idle, 1500-3000rpm acceleration, a full RPM
sweep, sustained high RPM, lift-off and downshift. Compare with an approved profile
of the same family and a suitable real-engine reference when available. Reject
vacuum-cleaner hiss, an isolated mosquito-like tone, missing exhaust weight, large
loudness holes or artificial abrupt changes. Numerical tests are necessary but
cannot certify convincing timbre. Do not mark sound approved solely because the
suite passed. Document listening feedback and required revisions.

The current Supra and Ferrari are tuned baselines, not recordings. Formal approved
reference clips and profiles for every engine family remain to be established.
Keep listening WAVs and spectral diagnostics in the external workspace.

The R34 RB26 six-cylinder twin-turbo profile also has low-order body and RMS
checks at 3000/5500/7500 RPM, plus the common low-RPM/load/coast tests. Fifth-car
IDs use three bits in the atomic audio snapshot; active/mute bits follow them.
