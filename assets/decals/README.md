# Runtime decals

25 vector-authored decal sets exported from reviewed experiment studies. Dragon
designs were rejected and are excluded. PNGs are transparent, cropped to ink
bounds and reduced to at most 1024 x 512 for native 128px car compositing.
Original vector drawings and full-resolution review sheets remain in the
experiment workspace. Reference photographs are not runtime assets.

catalog.txt: stable ID | display label | single-ink flag | PNG filename.
Single-ink assets use alpha as coverage; colour is selected in the editor.
Multicolour racing sets retain their original RGB. Decals are composited into
body albedo before dynamic lighting; no light or shadow is baked into them.

## Small-scale readability

The reviewed revision replaces hairlines with bold vector strokes and filled
leaves/petals, and uses fewer, larger sponsor blocks. Honeycomb Fade has a solid
vertical left edge and filled hexagons that thin out toward the right.
Red Slash Racing, Orange Endurance, Heritage Tricolour and Touring Sunset are
authored mirrored for right-side placement. Mint Club Racer remains unchanged.
Stable catalogue IDs preserve existing saved selections and transforms.

## Racing layouts and sponsor overlay

Directional racing stripes run from the top to the bottom of the decal bounds.
Sponsor blocks are drawn last over the stripes, rather than in a separate strip.
Honeycomb Fade retains its straight solid left edge and a diagonal fade at right.
`20_sponsor_overlay` is a single-ink perimeter of bold sponsor blocks with an
open centre. Apply it on layer 2 or 3 above another design; layer 3 is topmost.
