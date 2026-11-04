# Future transmission design

Status: agreed design note, 2026-11-05. Do not implement until requested.

Separate the car's actual transmission hardware from the game's shifting aid,
just as universal traction assist is separate from factory TCS/ESC.

- Manual-transmission cars: allow player-controlled manual shifts or optional
  game shifting assist. Assisted manual remains a manual gearbox operated by a
  game helper; it is not an automatic-transmission conversion.
- Automatic-transmission cars: use the actual automatic gearbox. Do not expose
  manual gear selection unless that specific gearbox supports driver-selected
  shifts (for example an appropriate sequential/manual-selection mode).
- General performance hierarchy: well-executed player manual shifting fastest,
  actual automatic shifting next, game-assisted manual shifting slowest.
- This ranks good manual execution, not every manual attempt: poor timing should
  still cost performance. Do not guarantee a bonus merely for selecting manual.
- Allow model-specific exceptions for genuinely superior automatic gearboxes;
  the hierarchy is a general game tuning goal, not a universal hardware claim.

Future implementation should identify transmission type and permitted shift
controls per exact car configuration, independently from whether shifting assist
is enabled. The current generic manual/automatic selector has not been changed
by this note.
