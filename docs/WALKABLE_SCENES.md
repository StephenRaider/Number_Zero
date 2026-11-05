# Walkable scene prototype

From the Number Zero checkout:

```powershell
.\build.ps1
.\build\NumberZero.exe --walkable-scenes
```

Use WASD or the arrow keys to walk. Press E near a character to open or close
dialogue. Keys 1, 2, and 3 switch between garage, dealership, and parking area
(PA) blockouts. Escape closes the preview. Dialogue and room layouts are
placeholders in src/walkable_scene.h.

The prototype reads optional 640 by 360 transparent PNGs from
assets/scenes/{garage,dealer,pa}. The floor and rear scenery go in base.png.
prop_1.png and prop_2.png are full-canvas layers aligned to that base. Each prop
draws at its ground-depth key among the actors. Collision and conversation range
use metres on the ground plane. Absent layers fall back to the blockout.

Use --walkable-scenes-smoke-test to check floor bounds, collision and conversation
range, then render a frame to build/walkable-scenes-smoke.png.
