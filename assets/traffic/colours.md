# Traffic paint provenance

Traffic uses a separate palette per source model/trim. RGB values are artistic
sRGB approximations for small sprites, not measured paint samples. Garage paint
remains player-selectable.

## Availability

- Corolla X Assista, 2004-06: Super White II and Silver Mica Metallic only.
  Period trim catalogue: https://www.o-cross.net/catalog/car/toyota/corolla/F004/M005G017/
- Swift, November 2004: Pearl White, Silky Silver Metallic, Bluish Black Pearl 3,
  Azure Grey Pearl Metallic, Kashmir Blue Pearl Metallic, Supreme Red Pearl 2,
  Lavender Metallic and Garnet Orange Metallic. Toyota's period catalogue:
  https://gazoo.com/catalog/maker/SUZUKI/SWIFT/200411/10024570/
  Manufacturer launch record: https://www.suzuki.co.jp/release/a/a041101.htm
- Crown Comfort, 2001-02: Super White II and Black.
  https://www.carsensor.net/catalog/toyota/crown_comfort/F001/M005G007/
- Hiace van DX, August 1995: White and Dark Blue. Conservative single-tone
  subset, avoiding later colours and unimplemented two-tone factory schemes.
  https://www.goo-net.com/catalog/TOYOTA/HIACE_VAN/102933/
- Rosa: plain white is a conservative provisional fleet finish, not a verified
  complete factory palette. Exact source model year/trim and period brochure
  have not been established. No random bright fleet liveries are generated.
  A period white vehicle is documented here, but used listings cannot establish
  original factory paint: https://www.goo-net.com/usedcar/spread/goo/19/700080443330260410001.html

## Spawn weights are estimates, not model sales statistics

No reliable sales-by-colour breakdown for these specific models/trims was found.
DuPont's 2006 Japan market figures give silver 27%, white/pearl 24%, black 16%,
grey 12%, blue 10%. This is aggregate manufactured-vehicle colour popularity,
not sales for a particular model. Period DuPont press release reproduced at:
https://www.theautochannel.com/news/2006/11/29/030181.html

The general passenger-car prior is restricted and renormalized to each palette.
Assista: 47% white / 53% silver; Comfort: 60% white / 40% black.
Swift: white 24, silver 27, black 16, grey 12, blue 10, red 6, lavender 3,
orange 2 percent. The latter three are design estimates for the remaining 11%.
Hiace: 90% white / 10% blue is a commercial-fleet design estimate, not measured
sales. Rosa: 100% provisional white. Both fleet assumptions are intentionally
separate from passenger-car market data. Update these priors when reliable
model/year/market-specific production or sales data becomes available.

Runtime palette definitions and weights: src/traffic_colours.h.
