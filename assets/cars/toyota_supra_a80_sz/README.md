# Toyota Supra A80 SZ

Playable naturally aspirated SZ baseline: 2JZ-GE, 225 PS at 6000rpm, 284.4Nm at
4800rpm, 1410kg, rear-wheel drive, 2550mm wheelbase, 225/50ZR16 stock tyre radius.
Five W58 ratios: 3.285 / 1.894 / 1.275 / 1.000 / 0.783; final drive 4.083.
The automatic mode is the prototype's shift assistant for that five-speed gear
set, not a simulation of Toyota's optional four-speed factory automatic.

Toyota 1993 SZ manual catalogue:
https://toyota.jp/ucar/catalog/brand-TOYOTA/car-SUPRA/199305/1000911/
Toyota launch description:
https://global.toyota/en/detail/7868203
Factory W58 reference (Toyota repair manual mirror):
https://www.supras.org.nz/techinfo/TSRM/14_MT.pdf

Intermediate torque-curve points, drag, traction, suspension and brake response
are prototype estimates; the quoted peak output/torque and mechanical dimensions
anchor the model. The shared gameplay brake and steering assists also apply.
A 180km/h prototype road governor supports the garage limiter-delete option.

Visual source: user-supplied toyota_supra_mk4__supra_1997__free_download.glb.
This stock-style A80 mesh represents the SZ; it is not an exact reconstruction
of all 1993 trim/year details. Its existing factory-style spoiler is retained.
The source, prepared GLB, raw renders, pipeline and timing diagnostics remain in
the separate experiment workspace. This folder contains runtime exports only.

522 PNG maps: 30 body views including pitch/steering, separate eight-phase wheel
animation plus blur for both camera quarters and all steering poses, paint/hood
masks, dark-grey glazing, blank white plates and independent lamps/indicators.
Native 128px, 26 pixels/metre, rendered at 4x resolution. Paint-surface cleanup
preserves glazing, lamps, plates, masks and silhouette. Inline-six audio is a
procedural approximation with no turbo or VTEC layers; DC is removed before
saturation for this profile to keep overlapping exhaust pulses audible under load.
