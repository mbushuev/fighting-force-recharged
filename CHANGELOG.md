# Changelog

## Unreleased

- Prevent recessed track walls from covering the foreground platform in Subway
  and Subway Boss with custom cameras. Retain the rail mesh's authored winding
  and clip its hidden sides at the near platform edge. Preserve the top ledges,
  texture coordinates, native depth offset and draw distance. Coverage checks
  pass across 144 views and through both supported native projectors.
  Gameplay inspection confirms the Subway strip is resolved.

- Retain visible platform floor tiles when custom cameras turn or approach
  them, including Subway. Clip tiles at the near and far view limits instead
  of discarding a whole tile when one corner crosses either boundary. Apply
  tiled-floor repair to orbit and first-person views independently of eye-view
  availability; preserve native gaps, materials, fog and directed-camera play.
  Native-projector coverage checks pass across camera angles and distances;
  gameplay confirmation remains pending.

- Close the exposed shop return in the next Bronx block, following its brick
  courses and sloped upper window edges. Preserve the adjoining street and
  native geometry. Use the original draw distance for this local repair;
  only the extended rear street receives the longer fade. Retail/RIP handler
  and geometry checks pass; gameplay inspection remains pending.

- Complete exposed side returns on the Bronx brick frontage, recessed shop
  and native block ends. Match the installed wall courses and sloped window
  edges without closing the street or intersection. Follow Complete Rooms;
  preserve the original model and include the new faces in camera obstruction.
  Geometry and native-handler checks pass for retail and RIP executables;
  gameplay inspection remains pending.

- Complete the missing sides of six tiled pilasters in the Subway entrance
  shell, including its use in Subway Boss. Reuse native panel widths, materials
  and cornice profiles; keep the passage and both ends open. Complete Rooms
  controls the addition for custom cameras, including camera obstruction.
  Geometry coverage and native-loader checks pass with retail and RIP builds;
  in-game inspection remains pending.

- Extend the distance fade and clipping range for the completed Bronx street,
  keeping its facades visible beyond the rear intersection. Restore the native
  range before drawing actors and other models; preserve the fog toggle and
  Complete Rooms setting. Retail and RIP native-handler checks pass; visual
  gameplay confirmation remains pending.

- Show original English level names in campaign slots, Continue previews,
  previous-level choices and pause save status. Retain existing save formats
  and checkpoints. Correct the room-review name for BRONX (STAGE33A).

- Extend the Bronx area's rear street with brick and concrete buildings
  beyond the existing intersection, using installed facade materials at their
  native panel scale. Keep the cross street and central road open and retain
  the authored road surface without overlapping floor geometry. Follow Complete
  Rooms and custom-camera ownership. Installed-resource and native-handler tests
  pass for retail and RIP executables; gameplay inspection is pending.

- Complete the laboratory rear boundary with a native-scale entrance arch,
  recessed metal doors and connecting jambs, ceiling and floor. Complete the
  shopping center entrance atrium beneath its rear balcony with a shallow
  service vestibule, matching wall trim, ceiling lights and closed double doors.
  Continue the upper gallery behind the beam. Retain the native entrances.
  Installed-resource tests verify coverage, recess depth, native arch UVs and
  runtime routing; gameplay inspection of these revised additions is pending.

- Complete the ship rear bulkhead with native wall materials fitted to its
  sloped roof profile and a recessed steel double door with a striped frame
  and handles. Add matching wall courses beside the shopping center's
  later rear corridor while retaining the central passage.
  These additions follow Complete Rooms and custom-camera ownership; original
  assets, entrances and scripts remain unchanged. Gameplay review is pending.

- Extend Complete Rooms to the later laboratory lift. Continue the opposite
  shaft panels and cross-members across all six shell variants, using their
  installed materials and UVs. Preserve the moving platform, landings and
  structural openings. The lift has received gameplay inspection; a separate
  check of both travel endpoints remains pending.

- Keep train carriage interiors and exteriors visible when a custom camera
  turns backward or moves inside a carriage. Replace the train's origin-only
  depth gate with the existing polygon clipping path, independently of room
  completion. Retain native camera behavior and inter-carriage passages.

- Add local room-review save sets for all 25 campaign stages, with an isolated
  test-copy launcher and progress backups. Document confirmed unfinished areas,
  train visibility investigation and outdoor scenery extensions separately from
  open model edges that still need inspection.

- Expand room geometry triage to singular archive names and small or low-polygon
  models. Report included and excluded archives explicitly; keep size thresholds
  as labels rather than excluding models from inspection.

- Add optional Complete Rooms in Gameplay Settings for custom-camera play in
  the elevator and first boss office. Build private wall extensions from the
  installed room materials: continue the lift side glazing and add a wooden
  office interior wall with a closed double door. Retain the lift entrance,
  original office geometry and native scripts. Include the added surfaces in
  camera obstruction capture. Keep native framing, demonstrations, the office
  intro and unrelated stages on their original models.

- Add a per-player Move List to pause and Options. Cover native
  strikes/combos, jumping and running attacks, hero-specific grab continuations,
  weapons, throws and item interactions. Show applied remapped buttons, follow
  the active solo device or each cooperative assignment, and identify disabled
  guard, unbound actions and incompatible simultaneous pad chords. Distinguish
  simultaneous commands from sequences without native shoulder-button glyphs,
  group item throws into one entry and retain native character-selection order.
  Use a readable backdrop without gameplay overlays. Preserve native combat
  timing and keep the game paused while browsing.

- Add remappable Camera Left/Right actions on D-pad left/right. Hold to turn at
  the configured horizontal stick speed in either view, with independent
  cooperative ownership. Retain D-pad zoom and raw menu navigation, suppress
  player movement, and preserve existing custom bindings on conflicts.

- Include all three native detachable scenery classes in Pickup and its prompt,
  including elevator rails. Reuse native trigger/facing eligibility and the
  original pull animation; retain its object conversion and weapon attachment
  frame. Punch no longer starts that interaction with Separate Pickup enabled.
  Hide consumed or replaced scenery prompts and keep cooperative ownership.

- Keep held items during Kick and Backfist when Separate Pickup is enabled;
  bypass the native combat-button throw/drop prepass and retain normal kick
  selection. Use Throw/Drop for intentional release. Turning the setting off
  restores the original combined combat/item behavior.

- Reserve the entire D-pad for assigned actions during gameplay; unassigned
  left/right directions no longer move the character. Retain raw D-pad menu
  navigation and up/down camera zoom.

- Show a compact device-aware pickup label beside a reachable item. Follow each
  cooperative player's current binding and render sample, suppress unavailable
  or stale objects, and batch each label into a single state-preserving draw.
  Retain edge prompts for off-screen items and objects at the first-person feet.

- Separate item pickup from Punch with a remappable F/RB action and an optional
  Separate Pickup setting in Gameplay Settings. Retain native pickup animations,
  reach, hero restrictions, object ownership and automatic consumables. Require
  a fresh press after interrupted pickup; empty pickup never becomes an attack.
- Use LT for Block, RT for Run, RB for Pickup and LB for Throw/Drop. Retain B
  for native Backfist and enemy grappling. Put camera zoom on D-pad up/down
  without also moving the player; retain raw D-pad navigation in menus.
- Add remappable Throw/Drop on R/LB, using native held-item poses and release
  frames. An empty-hand press does nothing. Retain the native left-hand drop
  fallback for items without a throw pose, and keep interrupted actions unqueued.
- Migrate unchanged older default pad layouts as a group; retain customized
  bindings and leave conflicting new actions unbound. Save the new layout as
  bindings version 2. Show the current pickup binding in pause and settings.

- Support remappable two-button XInput chords. A complete chord takes priority
  over its individual gameplay bindings; menu navigation remains unchanged.
  Capture and display chords in Controller Settings without losing saved layouts.

## 0.0.131 - 2026-11-02

- Add optional per-player target focus on the existing Focus/Recenter binding,
  with bounded camera tracking, directional target switching, view-relative
  ground movement and independent dynamic cooperative views. Preserve native
  attack and damage-reaction ownership; release dead or persistently hidden
  targets without automatic retargeting.
- Mark the selected enemy with short outlined corners around its actual
  projected pose, including lying enemies. Allow the marker to be disabled
  independently of the focus mechanic.
- Attach a first-person laser to the rendered pistol/shotgun barrel, including
  lowered locomotion poses and intermediate 60 FPS frames. Use blue with
  ammunition and red when empty, with a visual obstruction dot. Exclude launchers
  and leave native firing, damage, spread and cadence unchanged.
- Move Player Block into Options > Gameplay Settings and add Target Focus,
  Target Marker, Weapon Laser and Throw Trajectory switches. Apply drafts only
  with Save and Back; preserve unrelated input preferences and original files.
- Preview the flight of carried grenades and heavy throw-only items with a
  dashed arc and first-contact ring in either camera view. Use native throw
  speed, gravity and drag; stop at captured scenery, approximate actor volumes
  or the native floor. Add an independent Throw Trajectory switch to Gameplay
  Settings. Leave native throws, collisions, damage and item lifetimes unchanged.
- Include the combat focus and aiming-preview guide in setup and the Windows
  package so links from the bundled controls and quick-start guides resolve.

## 0.0.130 - 2026-11-01

First public release for the supported English Windows retail and RIP editions.

### Installer refresh - 2026-11-02

- State minimum installation requirements consistently in the quick start,
  setup guide and maintainer documentation. Label complete reference inventories
  as test evidence or optional diagnostics rather than installation gates.
- Limit installation checks to the supported executable, nonempty main data
  and safe compatibility writes. Accept non-reference resources and missing
  optional files; retain the complete reference audit only as `--verify`.
- Accept readable disc cabinets with the supported extracted retail executable.
  Extract only fixed game filenames and verify copies against actual source
  bytes, including extra data, without requiring a reference disc inventory.
- Finish successful setup with a single Finish action instead of a disabled
  Installed button. Route Enter and Escape to close the completed window, keep
  installation inputs locked, and reject further installation commands. Retain
  retries after cancellation or failure.
- Leave existing published release assets untouched when a reviewed tag is
  rebuilt; require a separate maintainer upload for an authorized refresh.

### Release preparation

- Support stable release metadata with matching README labels and dated
  changelog sections. Keep version increments independent of commit counts.
- Connect the prepared GitHub repository while preserving both histories and
  license notices. Add source CI and manually requested candidate builds.
- Fetch reviewed dependencies from immutable sources without committing vendor
  binaries. Gate candidates on VERSION, matching notes, safe defaults and exact
  source provenance; independently verify downloaded archives and checksums.
- Limit future tag-triggered release preparation to unpublished drafts after
  verification. Manual candidate runs create Actions artifacts only.

- Name the project Fighting Force Recharged and license project-authored code
  under MIT, with separate third-party notices. Use the project name in setup.
- Add audited source and Windows candidate packages with checksums and source
  revision metadata. Verify the installer payload against an exact allowlist
  and pinned dependency hashes; exclude local configuration and validation
  reports. Include quick-start instructions and all required license notices.
- Extend repository checks to disc-image formats, private tool directories,
  credential-like values and conversation metadata in source files. Audit
  distinct historical file versions without repeatedly reading unchanged blobs.
- Keep deterministic test temporary files under ignored build output so native
  atomic-save checks use a writable workspace destination.

### Frontend controls

- Route single-player character selection to one native cursor while accepting
  both keyboard and controller input. Prevent duplicate confirmations from
  selecting both cursors and stalling a new game after menu transitions.

### Campaign checkpoints

- Keep three independent campaign slots, each with a previous-level checkpoint
  and verified file backup. Import the earlier single-slot save into slot 1
  without modifying the legacy file. Isolate damaged slots and recover valid
  backups automatically.
- Add Load Game with slot previews, previous-stage confirmation and replacement
  confirmation for new campaigns. Default new games to an empty slot and retain
  existing progress when character selection is canceled. Continue selects the
  most recently started and successfully saved campaign.
- Show the saved slot and stage on pause. Add confirmed level restart through
  native cleanup and loading. Preserve keyboard/controller navigation, neutral
  highlights, blue instructions and release gates; prevent save-menu attract
  timeouts and accidental confirmation from held input.

- Automatically save the beginning of each campaign level in a separate,
  versioned checkpoint file. Preserve stage, difficulty, player slots,
  characters, health, lives, scores and extra-life thresholds for one or two
  players. Exclude attract demonstrations and retain the level-entry checkpoint
  during a retry or mid-level exit.
- Add Continue as the first native main-menu row when valid progress exists.
  Show its stage and player count, preserve native new-game/Options actions,
  and resume through normal resource loading without modifying original files.
- Validate checkpoint length, version, checksum and value ranges before use.
  Flush writes before atomic replacement; retain the previous checkpoint and
  report write failures. Keep input assignments and wrapper settings separate.

### Controller feedback

- Increase pistol recoil motor strength and extend its pulse from 140 to 180 ms.
  Keep the shotgun, heavy-weapon and combat-event profiles unchanged.

- Strengthen weapon recoil and select distinct pistol, shotgun, heavy-gun and
  projectile-launcher profiles from the native firing path and held weapon.
  Give shotguns a heavier, longer pulse than pistols. Refresh recoil for each
  new shot in a burst and replace the old profile when switching weapons,
  without stacking motor strength or changing hit, damage and block feedback.

- Add a distinct controller recoil pulse for each native weapon discharge,
  including shots that miss. Emit one pulse before tracing or launching the
  projectile; additional aiming rays do not repeat it. Preserve native firing
  behavior and the existing Vibration setting, player routing and stop rules.

- Add hit feedback for accepted melee, held-weapon, slide and firearm contacts
  with vehicles and destructible scenery. Route the pulse to the attacking
  player's controller, preserving native contact, durability and destruction
  results. Exclude misses, repeated contacts and AI aiming probes from hit feedback.

- Strengthen combat vibration and slightly lengthen its pulses. Keep distinct
  hit, damage and block profiles, bounded overlap and immediate stop on pause
  or focus loss.

- Show device-aware blue instructions in wrapper settings and quit confirmation.
  Track deliberate keyboard, mouse and controller input; held buttons, idle
  controllers and small stick noise do not repeatedly change the displayed device.
- Show current core combat and camera bindings on pause, including remapped
  buttons and alternate-only keyboard bindings. Use independent assignments for
  both cooperative players and omit Block when the optional mechanic is disabled.
- Allow X to clear a selected binding outside capture in the controls menu;
  X remains assignable during capture. Menu confirm/back remain A/B regardless
  of gameplay bindings.

- Add short, distinct XInput vibration pulses for confirmed damage dealt to
  characters, damage received and successful player blocks. Preserve native
  combat results; missed melee attacks and holding Block do not trigger feedback.
- Route each event to its player's assigned controller, including mixed-device
  cooperative layouts. Bound overlapping pulses without accumulating strength
  or extending a contact indefinitely. Expire output independently of rendering.
- Add **Vibration ON/OFF** to **Options > Controller Settings**, enabled by
  default and persisted with **Save and Back**. Stop output on pause, quit
  prompts, focus loss, transitions, reassignment and normal process exit.


### Experimental player guard

- Preserve an in-progress backstep turn when Block is pressed. Queue guard
  until the native turn transfers its final direction to the character, then
  enter protection without a second press or an extra idle tick.
- Preserve held Block input through standing hit reactions and enter guard at
  native recovery completion, without an additional unprotected idle tick.
  Keep damage already taken and native reaction duration; releasing Block
  cancels the pending entry. Falls, grabs and death retain native transitions.
- Hand movement, attack and jump input back to native combat immediately after
  Block is released, without waiting for the arm-lowering animation. Re-pressing
  Block during lowering starts a new entry. Holding Block still suppresses
  attacks; native attacks and damage reactions remain non-cancellable.
- Continue native target checks after a repeated blocked contact. One player's
  guard no longer prevents the same attack window from testing the other player;
  each defender keeps independent protection and one impact response per window.
- Invalidate guard on every focus-loss notification, including switches shorter
  than a simulation tick. Require an observed release after focus or scripted
  control suspension; masked input no longer re-arms a held Block button.
  Reject stale protection during focus, menu and scripted-control suspension.
- Add a saved **Player Block ON/OFF** option directly in the native **Options** menu,
  using its navigation, font and selection highlight. Keep only the Block button
  assignments in Controller Settings. Save the choice immediately without changing
  device assignments or other settings.
  Apply it to both players without restarting; OFF restores native combat without
  added guard poses or protection. Release active guards safely, preserve held
  items and require a fresh Block press when re-enabled. Retain existing INI
  choices and default new installations to OFF. Failed saves leave the active
  setting unchanged.
- Restrict guard to empty hands or light items that allow native running. Reject
  heavy objects such as vehicle wheels in either hand while preserving native
  carrying, attacks and throws; revoke protection if the held load becomes heavy.
  Gameplay checks confirm rejection with a carried wheel and continued guard
  availability with a pistol or melee stick.
- Allow guarding while carrying firearms, melee weapons or props. Preserve hand
  attachments and inventory state, restore the native held-item stance on release,
  and keep shooting, swinging, pickup and throw animations non-cancellable.
  Verify release through the original retail/RIP action selector; held-item
  appearance and subsequent attacks still require gameplay acceptance.
- Allow camera-relative turning in place with movement keys or the left stick
  while guarding, including each player's independent cooperative camera.
- Shorten each hero's guard entry to four simulation frames (about 0.13 seconds)
  while preserving the converted motion endpoints and immediate frontal protection.
- Add an opt-in unarmed Smasher guard through `Combat/PlayerGuard`, disabled by
  default. Reuse validated original large-male poses through private 27-joint
  clips and independently own entry, hold, impact and release for each player.
- Add remappable Q/LB Block bindings without overriding existing assignments.
  Stop ordinary frontal melee before native damage and knockback; keep rear,
  weapon, projectile, grab and special attack paths native. Match the actor's
  forward axis and preserve one response per continuous attack window.
- Check hook signatures, lifecycle, input migration, native stack boundaries
  and all converted animation frames with the original decoder. Smasher's
  frontal block and ordinary rear damage have been verified in gameplay.
- Extend the optional guard to Hawk, Mace and Alana. Adapt enemy upper-body
  rotations through each hero's bind/post transforms while preserving native
  idle roots, legs and accessories. Keep private clips and table attachment
  separate for every hero and player, including character/table reloads.
  Native decoder and isolated runtime checks cover all four heroes; full
  visual, first-person and level/cooperative acceptance remains pending.

### Control settings

- Add per-player keyboard/mouse and XInput action layouts in the wrapper INI,
  including camera, pause, quit and zoom bindings. Reserve distinct controller
  slots, support keyboard/mouse ownership by either cooperative player, and
  prevent native gameplay bindings from leaking across modern assignments.
- Select the original shared cooperative camera or the dynamic shared/split
  camera independently of modern input. Retain a single native render pass and
  stage-directed camera in original mode.
- Replace the native Controller Settings screen with wrapper settings and add
  an F6 main-menu shortcut. Edit devices, slots, primary/alternate keyboard and
  mouse bindings, gamepad buttons/triggers and cooperative camera mode. Validate
  button conflicts, support defaults/cancel, and save atomically without changing
  original game settings or assets. Keep menu confirm/back controls available.
- Read menu navigation and key capture from the existing DirectInput keyboard,
  including reacquisition after focus loss. Keep shared frontend menus accessible
  from keyboard regardless of cooperative device assignment; restrict per-player
  ownership to character selection and gameplay. Distinguish footer instructions
  with a separate color while retaining the native selected-row highlight.
- Keep keyboard and XInput navigation available together in solo character
  selection, regardless of cooperative assignments. Use the requested game
  mode for selection ownership rather than its temporary displayed actor count;
  keep native device indices separate from logical player ownership.

### Game folder compatibility

- Recognize the reviewed English RIP by its executable hash and BIOS probe
  signatures. Restore the BIOS fallback only in process memory, resolve native
  data paths against the executable folder, and skip the empty intro before
  invoking the movie decoder. Preserve all supplied files on disk.
- Supply primary Direct3D HAL defaults when no legacy video settings exist;
  retain saved settings and avoid setup execution or registry writes.
- Avoid optical-device calls and the native six-second track retry cycle when
  the reviewed RIP has no mounted audio CD. Run native track bookkeeping with
  music temporarily disabled for that call, then restore its saved preference.
  Retain normal CD audio when original media is available.

### Disc installation

- Add a standalone English retail disc installer for 64-bit Windows 10/11.
  Extract original InstallShield data without running legacy setup/DirectX,
  verify copied files against their source, and add separate compatibility files.
  Detect mounted discs independently of drive letter, reject existing targets,
  and roll back private staging on cancellation or failed verification.
  Include a graphical setup, optional per-user desktop shortcut and CLI verify.
- Scale setup from a 96-DPI baseline, wrap descriptions and status text, and
  size buttons to their labels. Scroll content in compact windows while keeping
  installation and cancellation actions visible.
- Add the fix directly to existing retail folders and the reviewed English RIP
  after checking executable compatibility and main data. Keep game files in place,
  preserve settings/saves, and roll back wrapper changes on cancellation or
  failure. Track installed compatibility hashes for later updates; refuse
  unknown or locally changed replacement files. Add `--apply --target` and a
  dedicated GUI mode with no copy destination. Disc installation still creates
  a new folder. Keep the existing disc CLI options as aliases.

### Rendering

- Match equivalent Euler rotation branches before blending skeletal joints at
  60 Hz. Keep the native root/body orientation pair together during exact
  half-turn handoffs, preventing transient sideways poses on direction reversal
  while retaining position, limb animation and the native simulation cadence.
- Add optional 60 Hz gameplay presentation with interpolated actor, skeleton,
  camera, loose-object and effect samples while retaining one native simulation
  step per two images. Restore temporary render-side state before simulation,
  retain native menu/movie/pause cadence and pace completed split images once.
  Set `GameplayFPS=30` under `[Rendering]` to restore native rendering.
- Restart optional frame-rate diagnostics across stage/tick resets and long
  gaps, preventing false update-rate spikes without changing gameplay timing.

### Gameplay camera

- Defer vehicle and prop origin visibility to face clipping in active custom
  orbit and eye views, including cooperative cameras. Retain partially visible
  meshes when their origin leaves the view. Install this with scene geometry
  independently of first-person support; preserve native object state,
  simulation checks and original directed-camera visibility.
- Retain the surviving cooperative player's orbit, eye view and camera controls
  after either actor is defeated. Use one full-width view owned by that player
  and restore the normal two-player camera policy when both actors are alive.
- Add an experimental two-player shared camera with gradual split/merge based
  on actor screen occupancy and captured opaque obstructions. Retain separate
  right-stick orbits, trigger distances and L3 eye views, per-player movement
  heading, native cooperative simulation and one final presentation per frame.
- Pace a completed cooperative frame once. Skip native frame-clock updates and
  the busy wait for the hidden intermediate view, retaining the original limit.
- Default cooperative input to keyboard/mouse for player one and a controller
  for player two. The controls menu also supports the reverse assignment or two
  distinct controllers and saves the selection in the wrapper configuration.
- Clip the dedicated ground-grid pass at the first-person eye plane. Reuse its
  native tile selection and materials, preserve absent cells, opaque coverage
  and painter-ordered far depth, and retain ordinary floor rendering in orbit.
- Retain visible scene-model and shadow triangles crossing the custom camera's
  near plane, using the native scene projector boundary and material dispatcher.
  Preserve scene winding, original geometry and native directed-camera rendering.
  Keep the animated eyes at least 16 units above the actor's foot level during
  low slide and knockdown poses.
- Use the continuous native walking gait for first-person retreat, including
  held firearms. Avoid repeatedly restarting the finite backstep and its final
  turn while retaining view-relative displacement and native collisions.
- Clip local held-object triangles at the first-person eye plane instead of
  discarding a whole face when one corner crosses it. Preserve texture mapping,
  native materials, depth and sorted-polygon lifetime; show reverse prop faces
  only in first person. Keep the native hand attachment and original meshes.
- Give first-person look independent mouse and right-stick sensitivity, with
  default rates approximately 25% above orbit. Retain the shared vertical inversion.
- Apply first-person diagonal, backward and sideways movement with held
  firearms as well as empty hands. Keep the native firearm step speed and
  collision response; align native idle/weapon stances with the viewing yaw.
  Leave firing actions and other carried-object movement native.
- Preserve backward and sideways first-person movement while the body faces
  the view. Reorient native ground-animation displacement before collision,
  retaining its speed, analog walking control and native solver corrections.
  Cover walk/run/backstep variants and keyboard-only configurations.
- Add an animated first-person view for live single-player gameplay, toggled
  with Tab or L3. Follow the animated skull position through movement, jumps
  and combat while retaining user-controlled look and the original arms/body.
  Exclude only local head faces using temporary render records. Preserve the
  third-person distance when switching back; reserve L3 for view selection
  and retain RB as the controller run button.
- Preserve intact destructible hall partitions when crossing into the next
  section during free orbit. Suppress only the transition's bulk removal;
  retain native hit/breakage paths and the state of already broken partitions.
- Retain the second stage's hall doorway wall across its section boundary and
  native view-yaw gate during free orbit. Keep it in camera obstruction geometry
  without restoring broken partitions or changing native progression records.
- Recenter R3 and the middle mouse button behind the actor by converting
  actor heading to the opposite-facing native view yaw.
- Preserve reverse-side opaque level walls during free orbit using temporary
  two-sided face records at the native CPU projector. Retain native material
  rules and leave original model buffers, props and attract scenes unchanged.
- Increase the default trigger zoom rate threefold and expose
  `TriggerZoomUnitsPerSecond` without changing mouse wheel steps.
- Retain collision geometry for unchanged live solid objects when their render
  model is temporarily omitted. Invalidate it on movement, carrying or removal,
  and delay outward boom recovery until clearance is stable.
- Retain environment-record vehicle contacts across the native origin visibility
  gate as well as the separate object pass. Invalidate removed, moved or replaced
  records without publishing a temporarily empty obstacle.
- Add an experimental independent 360-degree player-centered orbit for live
  single-player gameplay, with mouse/right-stick look, wheel distance and
  middle-button/R3 recentering. Preserve stage progression, camera-relative
  movement, native scene playback and cooperative framing.
- Retain native render-pose publication and discard mouse movement after
  focus changes.
- Bound the lowest orbit angle by distance and local ground clearance, rounded
  conservatively to the native angle grid, to prevent below-feet viewpoints.
- Sweep a camera sphere against opaque environment and destructible-object
  triangles captured before native visibility clipping. Contract the camera
  boom immediately at obstructions and restore distance smoothly afterward.
  Exclude transparent material rectangles, characters and transient debris.
- Publish environment and solid-object collision geometry as one complete
  frame so vehicles cannot disappear between native rendering passes.
- Grow the obstruction probe near the player so a hood touching the pivot's
  comfort radius cannot collapse an otherwise clear camera view. Keep full
  sphere clearance farther along the boom and test nearby walls continuously.

### Compatibility and input

- Keep CD music playing when the retail closes its control session immediately
  after playback or status commands. Retain native explicit stop commands and
  report successful playback requests in the local runtime log.
- Open main-menu quit confirmation with Esc. Retain Esc as submenu Back and
  gameplay pause, and require release after returning from a submenu or cancelling.
- Poll the selected controller during native movie playback. A, B or Start
  requests the native skip/cleanup path, with focus and release gates between clips.
- Replace Q with F10 for the modern keyboard quit shortcut. Add B to the main
  menu quit prompt while preserving B as back/cancel in submenus and gameplay.
- Require RB/L3 for controller running. Full stick deflection stays within walking
  speed and cannot synthesize run attacks; releasing run immediately clears its action.
- Add continuous left-stick walking speed with button-controlled running, using each
  character's native animation displacement and a short acceleration ramp.
  Apply speed before the original actor collision solver; retain native combat,
  airborne, carrying, keyboard/D-pad, cooperative and demo movement.
- Add single-player WASD movement, Space jump, Shift run, E backfist and J/K
  attacks alongside mouse combat. Move pause from Space to Esc/P and support
  WASD navigation and Enter/Esc quit confirmation without changing saved bindings.
- Map XInput controllers to gameplay actions, frontend menus, pause and quit.
  Share automatic controller selection between movement and right-stick orbit,
  add trigger zoom, radial deadzones and reconnect/focus/scene neutral gates,
  and suppress duplicate legacy joystick actions from the selected input path.
- Start the supported original Windows executable through a separate wrapper
  without editing retail files.
- Discover the mounted original disc independently of its drive letter.
- Reacquire keyboard input and retry the same poll; clear stale state when a
  read fails.
- Add left-mouse punch and right-mouse kick to live single-player gameplay at
  the native action boundary, retaining keyboard/joystick bindings and combos.
  Keep mouse attacks inactive in menus, demos and while another app has focus.
- Reuse the original CD audio session across scene transitions while handling
  remounted discs and failed commands.

### Rendering and presentation

- Cover loading at its native entry and skip the fixed three-second disc-spin
  busy wait. Preserve actual media/file reads and expose `ShortLoadingWait`.
- Use nonblocking native surface flips and suspend background game updates while
  continuing the original Windows message dispatcher, including native pause
  loops that omit message polling. Reset relative mouse,
  controller release gates and camera timing after focus loss.
- Retain an in-memory copy of the completed native loading image across its
  immediate renderer teardown. Display it with preserved proportions until
  replacement game frames are ready, then fade into the next scene.
- Initialize DPI awareness early for correctly sized borderless presentation.
- Stabilize the native far-plane depth marker without changing texture
  filtering, texture coordinates or original vertex buffers.
- Correct filtered transparent edges while preserving opaque geometry,
  vehicle wheels and native translucent passes.
- Fade across movie, window and renderer replacement without desktop flashes;
  preserve the original copyright image and proportional movie framing.
- Expand the horizontal gameplay view using supported widescreen display
  modes while keeping vertical field of view and UI proportions.
- Keep world polygons and shadows outside normalized UI mapping, avoiding
  displaced texture fragments, missing side geometry and flickering shadows.

### Development

- Scope animated eye poses, head exclusion and first-person movement to an
  explicit player, including the second actor's native animation palette.
- Prepare distinct cooperative XInput slots, independent action words and
  second-player menu input, with shared pause controls and focus release gates.
- Add tested cooperative screen-occupancy, split/merge hysteresis and off-axis
  projection primitives for a shared camera that can separate into two views.
- Document native camera ownership, input coupling and implementation boundaries.
- Add a read-only, hash-checked camera reference analysis tool.
- Decode inspection ranges from the requested address to avoid carrying
  instruction alignment across embedded executable data.
- Establish a source-only repository with focused rollback checkpoints,
  portable build-tool discovery and checks for private or generated content.
