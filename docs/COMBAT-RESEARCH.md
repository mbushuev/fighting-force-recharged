# Native guard research

This is a static feasibility study for a player-controlled guard. It does not
implement a new action or change the installed game. Evidence comes from the
reviewed English retail executable, original character archives and the wrapper
input/animation boundaries. Addresses below are executable-relative RVAs.

## Findings

Enemy defense provides reusable entry, holding, impact and exit animations.
There are three useful generic families, rather than one universal pose.
The four hero animation archives contain no named `BLOCK` or `SBLOCK` clips.
The player keyboard decoder has no separate guard input.

Player attacks against guarding enemies and enemy attacks against players use
different hit handlers. Assigning an enemy guard action to a hero does **not**
by itself make incoming enemy attacks respect it. Pose adaptation, input/state
ownership and hit resolution are separate implementation requirements.

## Original resources

| Archive/member | Skeleton used for inspection | Joints | Entry / hold / impact / exit frames |
| --- | --- | --- | --- |
| `CHARS11.WAD/GENMALE.ADT` | `SUIT.ACH` | 20 | 7 / 14 / 15 / 10 |
| `CHARS11.WAD/GENBIGMA.ADT` | `BIGBLACK.ACH` | 21 | 8 / 11 / 15 / 8 |
| `CHARS15.WAD/GENFMALE.ADT` | `VAMP.ACH` | 30 | 9 / 11 / 13 / 6 |

The four names are `BLOCKIN1`, `BLOCKSB1`, `BLOCKHT1` and `BLOCKOT1`.
All decoded frame event flags in these twelve clips are zero. Their root
translations are not zero: even a holding clip can contain forward motion.
Repeatedly looping a donor clip without handling that motion can move the hero.

The reviewed retail and RIP manifests have identical hashes for these two stage
archives and all four hero archives. A future implementation can read the
installed originals in either edition. Do not distribute extracted poses or
write converted animation data into the original archives.

### Hero compatibility

| Hero | Joints | Candidate source | Observed compatibility |
| --- | --- | --- | --- |
| Hawk | 20 | Generic male / Suit | Same bone tree and indices, but eleven mapped bones differ in translation or fixed rotations. Matching joint count does not permit blind angle copying. |
| Mace | 30 | Generic female / Vamp | Same tree. The first twelve non-root bones have matching transforms; hair and leg transforms differ. |
| Alana | 32 | Generic female / Vamp | Related upper-body topology with different dimensions. The extra hair nodes shift lower-body indices; a full semantic mapping is still required. |
| Smasher | 27 | Generic large male / BigBlack | All twenty mapped non-root bones have matching translations, fixed rotations and mapped parents. Six extra hero nodes have no mesh vertices. The special head marker is attached to a different node. |

Smasher is the strongest initial prototype candidate. The candidate mapping,
written donor to hero, is `0..6 -> 0..6`, `7..9 -> 10..12`, and
`10..20 -> 16..26`. Preserve the hero's extra nodes and head behavior. This
structural match is not yet a visual acceptance test.

For Mace, the leg axes differ enough that copying the same angles can produce
the wrong pose even though the arm chain matches. Hawk also has differences
around the elbows and legs. Compare bind spaces and final mesh orientation,
not just joint positions. Preserve each hero's geometry, bone lengths and
accessory hierarchy. A conversion should solve the target local rotation from
the desired parent-relative orientation, including both pre- and post-rotation
matrices. Simply adding an Euler-angle offset is not sufficient in general.

The offline decoder successfully reads 461 hero clips containing 13,808 frames,
plus the twelve donor guard clips containing 127 frames, within their record
bounds. This establishes format coverage for these assets, not correct gameplay
or the appearance of a retargeted animation.

## Native animation and state map

| Responsibility | RVA | Observed behavior |
| --- | --- | --- |
| Action-name map | `0x6c8b0` | Twelve-byte name/metadata/action entries. Guard entries start at `0x6d234`. |
| ADT loader | `0x31ad0` | Installs named clips into the selected character/shared table and writes global action metadata. |
| Character animation table pointers | `0x879780` | Indexed by character type. The hero types are Hawk 0, Mace 1, Smasher 2 and Alana 3. |
| Action metadata | `0x8674c0` | Shared 16-bit values; high byte is consumed as an attack damage factor. |
| Action setter | `0x37a60` | Installs a table entry, resets animation frame and several action fields. It does not validate the chosen clip pointer. |
| Player action selector | `0x4e320` | Adds player/held-object action remapping before the common setter. |
| Animation decoder | `0x488f0` | Decodes three components per destination-model joint; first frame is absolute, later frames are signed deltas. |
| Common actor update | `0x38670` | Advances pose and runs movement/collision work. |
| Player update | `0x4e970` | Interprets player actions and buffered controls. |

Guard action IDs are entry `0x39`, exit `0x3a`, impact `0x3b`, and hold `0x3c`.
Their action-name metadata is zero. A separate family uses `0xd0..0xd2`,
including `SBLOCKIN`, `SBLOCKOT` and `SBLOCKSB`. It participates in special
defense decisions and should not be treated as an interchangeable ordinary
guard without another audit.

The AI decision at `0x35270` depends on target attack classification, distance,
facing, random probability, cooldown and AI parameter data. Its facing checks
accept approximately +/-67.5 degrees for attacker and defender. Its lifecycle
handler at `0x355d0` transitions entry to hold, impact to hold, hold to exit,
and exit back to AI control when each animation completes. The AI decision can
renew defense while a threat remains. This is not a button-held player loop.

Useful actor fields, relative to each `0xf4`-byte record at `0x86a500`:

| Offset | Observed use |
| --- | --- |
| `0x00`, `0x04`, `0x0c` | Animation pointer, frame index and action ID. A completed frame index is -1. |
| `0x10`, `0x14`, `0x18` | Animation root displacement components. |
| `0x1c` | Current frame event flags. |
| `0x28..0x30` | World position. |
| `0x34`, `0x38` | Character type and body heading. |
| `0x42`, `0x4a` | Health and accumulated damage used by hit reactions. |
| `0x52` | Hit bookkeeping used by several attack paths. |
| `0x7d`, `0x7e` | Held-object references. |

The decoder trusts the model's joint count. Feeding a 21-joint compressed clip
directly to the 27-joint Smasher model is unsafe even with a known bone map.
Build a complete target-joint stream in private memory first. Do not call the
native loader merely to import donor poses: its table selection and global
metadata writes are additional side effects.

## Hit resolution

| Responsibility | RVA | Guard implications |
| --- | --- | --- |
| Player melee contact wrapper | `0x3a720` | Gets an attack contact and calls the multi-target player-hit path. |
| Player-hit path | `0x39a60` | Recognizes target actions `0x39`, `0x3b`, `0x3c`; guard branch begins at `0x39d1e`. |
| Enemy melee contact and reaction | `0x2b060` | Searches the two player slots, accepts contact, then applies effects, damage and ordinary player reactions. No corresponding guard branch is present. |
| Enemy melee accepted contact | `0x2b1bb` | Candidate interception boundary before normal impact effects and hit-side effects. This is a research location, not an installed hook. |
| Action-based damage | `0x3c4e0` | Reads the attack action's metadata factor and calls the shared damage helper. Enemy melee calls it at `0x2b28b`. |
| Shared damage application | `0x3c380` | Applies attack/defense scaling, damage accumulation, attribution and health changes; clamps damage to at least one. |
| Other contact paths | `0x39440`, `0x2b4d0`, `0x3c520` | Thrown/projectile and weapon-related paths need separate coverage. They cannot be assumed to inherit the melee guard behavior. |

The existing enemy guard response uses impact effect `0x362`, selects the
impact pose `0x3b`, and calls the damage helper with factor 16. That is a
fixed-point input to several calculations, not a universal percentage reduction
of the final ordinary damage. Native guard can still lose health. Passing a
factor of zero to the helper also does not implement zero damage because of its
minimum-damage clamp.

The player-hit guard branch itself has no facing rejection; enemy AI checks
facing when choosing defense. A player can hold a guard indefinitely, so the
new contact policy must explicitly reject rear attacks. Otherwise reusing the
branch would effectively provide protection behind the hero as well.

The enemy contact path produces an ordinary impact effect, additional effects,
damage, knockback/hit/death actions and relationship cleanup. A damage-only
hook or a health refund after that handler is too late to implement a correct
block. Intercept an accepted eligible contact before those effects, produce the
guard response, and preserve the caller's successful-contact return semantics.
Also account for repeated limb tests in one attack window; a blocked hit must
not deal damage repeatedly because the defender remains in an eligible pose.

## Recommended integration

These are proposed implementation boundaries, not supported features yet.

1. **Prepare poses offline, starting with Smasher.** Decode the original donor,
   remap to the complete hero joint set, and preserve native idle values for
   unmapped accessory nodes. Convert basis rotations where necessary. Calibrate
   root height and feet contact against the hero's own idle pose. Give holding
   poses zero unintended horizontal movement; keep any impact recoil explicit
   and collision-aware. Check entry/hold/impact/exit continuity in a pose preview.
2. **Install an isolated player guard state.** Keep state independently for
   each player slot and update it once per native simulation tick. Start from
   grounded, allowed neutral actions; do not cancel attacks, jumps, slides,
   grabs, hit reactions or death just because the button is held. Own the guard
   lifecycle while preserving common collision/gravity and stage updates.
   Restart/replace completed clips before another decoder call with frame -1.
   Define release and interrupted-entry behavior and clear stale action buffers.
   The native player update loops over both slots; guarding one player must not
   bypass the other player's update or shared bookkeeping.
3. **Resolve guarded melee contacts.** Use the defender's body heading and the
   incoming attack direction, not an unrelated orbit-camera yaw. A frontal
   sector comparable to the native AI's 135-degree total is a starting point.
   Preserve contact consumption and attacker feedback. Apply the guard impact
   pose/effect and an explicit chip-damage policy; allow normal hits from behind.
   Treat grabs, explosions, bullets and large thrown objects as separate rules.
4. **Integrate controls and presentation.** Add a distinct remappable `Block`
   action. LB and Q are unassigned defaults in the reviewed layout. Consume the
   action only during gameplay, respect per-player device ownership, and clear
   it on focus/menu transitions. Publish the resulting actor pose through the
   existing 30 Hz simulation / 60 Hz rendering transaction. Guard logic must
   never advance a second time for an interpolated frame or another split view.
5. **Handle resource lifetime.** Prepare private clip memory outside combat,
   install only validated hero entries after the appropriate table load, and
   retain them while simulation or rendering can reference them. Recheck table
   reloads on stage changes, restart and character selection. Preserve shared
   action metadata and enemy tables. Disable the optional feature cleanly if
   an asset, executable signature or attachment boundary does not match.

A first playable milestone should use an unarmed, stationary held guard against
ordinary frontal punches/kicks. Movement while guarding, carried props, guns,
special attacks and zero-damage versus chip-damage balance should have explicit
rules before being enabled. The existing input menu can expose the binding;
the guard should not silently replace Backfist, Run or the camera controls.

In first person the actual head/upper body moves with the guard pose. Keep that
physical relationship, check the forearms' screen coverage, and constrain camera
roll/impact jolts if necessary. A third-person pose that looks correct can still
obscure the entire first-person view.
The current first-person body-alignment helper accepts idle and locomotion
actions only. It must deliberately include the new allowed guard phases if
turning the view is to turn the blocking direction as well. Preserve the
existing heading restrictions during attacks, knockdowns and jumps.

## Validation and remaining uncertainty

Reproduce the asset checks without modifying game data:

```powershell
python tools/analyze_guard.py
python tests/guard_assets_test.py
python tools/inspect_retail.py range 0x42b060 0x42b4c5
python tools/inspect_retail.py range 0x4488f0 0x448a72
```

The asset analyzer verifies original hashes against both reviewed manifests,
checks archive/record bounds, decodes hero and donor frames and compares the
candidate skeleton mappings. `--assets` selects another installed WAD folder.
Its output is structural evidence, not a rendered preview or gameplay test.
The inspection tool accepts virtual addresses including the retail image base;
the tables in this document use RVAs.

The native-code findings are from the supported retail image. Asset equality
does not establish identical executable hook sites in the RIP; check both
executable profiles and instruction signatures before enabling a runtime hook.
Full Alana retargeting, final pose appearance, sound selection, exact block
timing, weapon/special-attack coverage and interrupted-action rules remain to
be validated during implementation.

Runtime acceptance needs all four heroes, front/side/rear attacks, multiple
attackers, hold/release during transitions, lethal chip damage if enabled,
held objects, first/third person, both player slots and split/shared cameras.
Verify that one attack window produces one intended response at both native
and interpolated display rates. Repeat after focus loss, death/respawn, level
reload and character changes, with original-file integrity checks for both
supported editions. No runtime acceptance result is claimed by this study.
