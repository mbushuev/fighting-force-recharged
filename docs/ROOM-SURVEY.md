# Room completion survey

The Complete Rooms option currently reconstructs only the elevator cabin
(`MODELS13`, `NEWZL2.MOD`) and first boss office (`MODELS15`, `ROOM15.MOD` and
`ROOM153.MOD`). Their added walls, cabin glazing and closed office door have
received gameplay inspection. Native assets remain unchanged.

## Review queue

The following are geometry candidates, not confirmed missing walls. Model
boundaries must be checked with the assembled scene, original entrances,
camera-dependent selectors and scripted transitions before extending coverage.

| Area | Resources to inspect | Evidence and next check |
| --- | --- | --- |
| Later laboratory lift complex | `MODELS72`: `LIFTBOT`, `LIFTBOT2`, `LIFTTOP2` | Broad open positive-Z side in the lower/upper room pieces. Check adjoining shaft meshes, cabin travel and both landings. |
| Laboratory passages | `MODELS71`: `LAB01S` | Far end lacks broad wall coverage. Determine whether adjoining room pieces or a working doorway occupy it. |
| Shopping center | `MODELS31`: `MALL02A` | Three enclosed sides and an open positive-Z frontage. Check storefront/connection geometry and traversal before adding a wall. |
| Ship interior | `MODELS65`: `NAVEL05` | Long three-sided shell with an open positive-Z end. Check entry/exit and model placement. |
| Train interiors | `MODELS51`: `TRAIN01R` through `TRAIN04R` | Open ends alternate with model orientation. Treat them as potential carriage connections, not automatic end-wall repairs. |

Outdoor fences, building facades and thin arena walls can produce the same
coverage pattern. Examples include `RAIL03` and `DARKWALL`; a three-sided score
does not establish an enclosed room. `ROOM14` has sampled geometry in all four
directions and is not evidence of another wholly omitted side. Local gaps and
runtime omissions remain possible there.

## Reproducing the triage

Run `python tools/audit_room_shells.py game/Wads` against an installed original
resource directory. The tool reads model archives without changing them and
prints JSON containing model names, counts and directional coverage. It skips
front-end and recorded-scene archives. It does not export vertices, faces,
textures or proprietary file contents.

The scan considers large models, converts coordinates using the native loader's
arithmetic half-scale rule, and samples horizontal rays at three heights and
nine positions per side. Only intersections in the model's outer quarter count,
so central furniture cannot substitute for an outer wall. The review flag
requires one broadly open side and substantial coverage on the other three.

This is per-model triage. It does not combine object transforms, evaluate texture
opacity, distinguish doors from holes, or reproduce dynamic scene selection.
It cannot establish full campaign coverage or justify filling every open side.
