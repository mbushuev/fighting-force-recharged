# Room completion survey

The Complete Rooms option reconstructs the elevator cabin
(`MODELS13`, `NEWZL2.MOD`) and first boss office (`MODELS15`, `ROOM15.MOD` and
`ROOM153.MOD`). Their added walls, cabin glazing and closed office door have
received gameplay inspection. It also extends the later laboratory lift's six
shaft/panel variants using rotated copies of the opposite panels and cross-members.
The moving platform and external landings are excluded. The later lift has
received gameplay inspection; a separate check of both travel endpoints remains
pending. Asset tests verify unaltered native geometry, materials, opening
coverage and stage routing.

The laboratory rear boundary now uses a native-scale entrance arch and a
recessed pair of metal doors. The shopping center entrance atrium (`MALL01`)
has a shallow service vestibule under its rear balcony, with matching wall trim,
ceiling lights and closed double doors. Its native entrance at the other end
remains open. The upper gallery continues behind the beam. The separate later
corridor (`MALL02A`) retains its central passage between added wall courses.
The ship rear bulkhead follows its existing roof arch and includes a recessed
steel double door, striped frame and handles. Asset tests verify
coverage, recess depth, native arch UV ordering, materials and runtime routing;
gameplay inspection of these revised interiors is pending. Added doors are
scenery, not new routes. Native assets and actor collision zones remain unchanged.

The Bronx area now has brick and concrete facade extensions beyond the
rear intersection. They reuse the BR01 materials and panel scale, with corner
returns outside the road and capped roofs. The existing ground map already
continues the roadway through and beyond the intersection. No duplicate ground
mesh is drawn, and neither street is blocked by a transverse wall. Tests verify
the open crossing, facade coverage, unchanged native records and ground tiles,
and dispatch through the native MOD loader and environment handler with both
supported executables. Gameplay inspection of this extension is pending.

This is **not full campaign coverage**. Live review has also confirmed exposed
boundaries in the later laboratory lift, laboratory, shopping center and ship,
train visibility failures, and a truncated road behind the Bronx area.
The train visibility failure is repaired independently of Complete Rooms. Its
existing carriage interiors and rear scenery received gameplay inspection with
custom views; no replacement carriage wall was necessary. The laboratory,
shopping center, ship and Bronx area have reconstruction awaiting gameplay
review.

## Confirmed next work

| Load Game stage | Area | Reconstruction constraint |
| --- | --- | --- |
| 24 / `STAGE72` | Later laboratory lift | Opposite panels and cross-members reconstructed in six shell variants. Moving cabin and landings preserved; gameplay inspection completed, separate review of both travel endpoints pending. |
| 23 / `STAGE71` | Laboratory | Rear arch and recessed metal doors replace the flat wall extension. Native entrance tunnel retained; revised geometry awaits gameplay inspection. |
| 8 / `STAGE31` | Shopping center | Entrance-atrium vestibule beneath the rear balcony, plus wall courses beside the later central corridor. Native passages retained; gameplay inspection pending. |
| 20 / `STAGE65` | Ship interior | Rear metal bulkhead with recessed steel double doors under the roof arch. Native entrance hatch retained; revised entrance awaits gameplay inspection. |
| 15 / `STAGE51` | Train | Repaired: retain both carriage meshes through polygon clipping. Existing rear geometry is visible and carriage connections remain open. |
| 12 / `STAGE33A` | Bronx | Added brick/concrete blocks beyond the rear intersection. Existing road and cross street retained; native-handler checks pass, gameplay inspection pending. |

The train requires separate visibility and geometry work. The native carriage
handler at `0x44BCE0` draws a model and its paired interior only when the transformed
origin depth is strictly between 256 and 4096. Types 307, 309, 311 and 313 share
that handler; their paired interiors are `TRAIN01R` through `TRAIN04R`. This is
an additional early rejection before polygon clipping, outside the existing
custom-camera object-origin hooks. Custom views now submit both meshes using
their unchanged object transforms and the existing near-plane/viewport clipping.
Tests cover all four pairs, both camera modes, cooperative ownership and native
fallbacks. Gameplay inspection confirmed the backward-view failure is resolved.

## Review queue

The following are geometry candidates, not confirmed missing walls. Model
boundaries must be checked with the assembled scene, original entrances,
camera-dependent selectors and scripted transitions before extending coverage.

| Area | Resources to inspect | Evidence and next check |
| --- | --- | --- |
| Early connecting interiors, stages 2 and 3 | `MODELS12/13`: `CORRIDOR` | Narrow models excluded by the earlier size filter have open opposite ends. Check room joins and the elevator entrance, not only the large cabin shell. |

Outdoor fences, building facades and thin arena walls can produce the same
coverage pattern. Examples include `BUILD5`, `RAIL03` and `DARKWALL`; a three-sided score
does not establish an enclosed room. `ROOM14` has sampled geometry in all four
directions and is not evidence of another wholly omitted side. Local gaps and
runtime omissions remain possible there.

## Reproducing the triage

Run `python tools/audit_room_shells.py game/Wads` against an installed original
resource directory. The tool reads model archives without changing them and
prints JSON containing archive/model names, counts, spans and directional
coverage. It inventories both MODEL and MODELS names and lists excluded
front-end/recorded-scene archives explicitly. It does not export vertices,
faces, textures or proprietary file contents.

The expanded installed-resource scan inventories 2,681 model entries across
26 gameplay/arena archives and excludes 10 front-end/recorded-scene archives.
119 models meet the large-shell size label; small and low-polygon models remain
in the inventory instead of silently disappearing. Planar models have no
volumetric side score. Archive totals are not campaign-stage totals.

The scan converts coordinates using the native loader's
arithmetic half-scale rule, and samples horizontal rays at three heights and
nine positions per side. Only intersections in the model's outer quarter count,
so central furniture cannot substitute for an outer wall. The review flag
requires one broadly open side and substantial coverage on the other three.

This is per-model triage. It does not combine object transforms, evaluate texture
opacity, distinguish doors from holes, or reproduce dynamic scene selection.
It cannot establish full campaign coverage or justify filling every open side.

## Isolated review saves

Create a new test copy from an existing patched installation:

```powershell
python tools/prepare_room_review.py --game game --output work/room-review
```

Open `work/room-review/Review-Rooms.cmd`, select a set and choose **Load set and
start game**, then use **Load Game**. The first three sets are:

| Set | Slot 1 | Slot 2 | Slot 3 |
| --- | --- | --- | --- |
| 01 | Later lift, stage 24 | Laboratory, stage 23 | Shopping center, stage 8 |
| 02 | Ship interior, stage 20 | Train, stage 15 | Bronx, stage 12 |
| 03 | Completed lift, stage 3 | Completed office, stage 5 | Adjoining interior, stage 4 |

Nine further sets cover stages 1 through 25 in campaign-index order. Presets
contain only wrapper checkpoint values: Alana, normal difficulty, full health,
nine lives and zero score. They start at the native level entrance; enemies,
doors, scripts and later sections still require normal play. They are not
mid-level snapshots or proof that every room has been inspected.

The tool refuses an existing output directory and leaves source saves intact.
Switching sets requires the game to be closed and backs up both primary and
recovery saves under the test copy's `history` directory. Unused slots and stale
recovery files are removed from that copy. Immutable presets stay in `presets`.
The copied native files are verified byte for byte. Generated data remains
outside Git and must not be distributed with the patch.

The mapping below follows the native STAGE resource table; sorting model
archives gives an incorrect order, especially for alternate stages.

| Menu stage | STAGE suffixes, in order |
| --- | --- |
| 1-5 | `11`, `12`, `13`, `14`, `15` |
| 6-10 | `21`, `22`, `31`, `32A`, `32B` |
| 11-15 | `33`, `33A`, `41`, `42`, `51` |
| 16-20 | `61`, `62`, `63`, `64`, `65` |
| 21-25 | `66`, `67`, `71`, `72`, `73` |

For each room, inspect full horizontal turns and low/high pitch in both camera
views, then repeat after section changes, destruction, doors and lift travel.
Check multi-model joins and camera clipping before deciding to add geometry.
Do not fill an opening solely because its individual model lacks a wall.
