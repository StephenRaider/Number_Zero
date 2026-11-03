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
The DSP, profiles and parameter values otherwise match the supplied header.
