# Car engine synthesiser

Imported from the user-supplied `car_engine_sounds.zip` (2026-11-03).
Only the runtime header is required. Reference WAVs, demo and offline renderer
stay in the experiment workspace, outside this game checkout.

Upstream credit and permission (from section 7 of the supplied README):

> Everything here is original synthesis code written for the Raylib Racers 2 project; no samples or third-party audio. Use it freely
> in your game. Car names are used only as descriptive labels of the sound being imitated.

Local correction: BOV lift detection now detects the smoothed throttle crossing
0.35 downwards while boosted. Upstream required the previous sample to exceed
0.55 and the next sample to be below 0.35, which its 30 ms smoothing prevents.
Additional local tuning: closed-throttle engine presence now follows RPM.
The upstream pulse-load, overrun and output-level reductions multiplied into a
near-silent coast. Removed the extra overrun pulse reduction, raised the
RPM-dependent closed-load/body/output floors and retained more high-RPM exhaust
bandwidth. Full-throttle settings and profile parameter values are unchanged;
true throttle still reaches turbo, BOV, VTEC and overrun logic. This is sound
design for the prototype, not a calibrated real-car acoustic measurement.
