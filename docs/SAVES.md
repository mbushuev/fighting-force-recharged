# Campaign saves

The wrapper provides three independent campaign slots. Each slot saves the
current level's starting state and retains the previous level's checkpoint.
Progress includes stage, one- or two-player mode, difficulty, character slots,
health, lives, scores and extra-life thresholds. End-of-level bonuses remain
native. Input assignments and camera settings remain in the wrapper INI.

## Main menu

**Continue** immediately loads the last successfully started/saved campaign.
Its preview shows characters, stage, difficulty and player count. This entry
appears when a usable save exists. Browsing slots does not change this choice.

**Load Game** lists all three slots, with characters, difficulty, lives and
stage information for the selected slot. Select an occupied slot, then choose
**Continue** or **Previous Stage**. Loading the previous checkpoint asks for
confirmation and rewinds only that campaign when its level loads.

**1 Player Game** and **2 Player Game** first open a slot selector, defaulting
to the first empty slot. Replacing an occupied or damaged slot requires an
explicit **Yes Replace**; **No** is selected initially. Character selection
then uses the original game screen. Canceling before the new level loads does
not replace any save. An inaccessible slot cannot be selected for replacement.

The menus support arrows/WASD and Enter/Esc, or controller D-pad/left stick and
A/B. Confirmation waits for held buttons to be released after opening a screen.
Selected rows use the native bright highlight; instructions use blue text.
Slot menus do not time out into an attract demonstration.

## During play

The current campaign saves automatically when each new level finishes native
initialization. Leaving midway, dying or retrying does not advance its checkpoint.
The pause screen shows the saved slot and stage, with **Continue**, **Move List**,
**Restart Level** and **Quit to Menu** in the development build. Move List opens
the per-player combat guide without resuming play. Pause Continue resumes the current fight. Restart
asks for confirmation and reloads that level's starting state, including health,
lives and scores, through normal engine cleanup and loading.

These are level-entry saves, not mid-fight snapshots. Enemies, destroyed scenery,
held objects, poses and camera positions follow the native fresh-level setup.
After completing the campaign, its last level remains available for replay.
Attract demonstrations never overwrite progress. Eliminated cooperative players
retain their original slots, including a surviving second player.

## Files, migration and recovery

Slots are stored beside the compatibility DLL as `fightingforce-save-1.sav`,
`fightingforce-save-2.sav` and `fightingforce-save-3.sav`. Each has an independent
`.sav.bak` recovery file. Copy the slot files and their backups to preserve or
transfer them between the supported English retail and reviewed English RIP
installations. The game folder must be writable.

When all three slots are absent, an existing `fightingforce-progress.sav` from
the earlier single-slot wrapper is imported into slot 1. The legacy file is
retained unchanged and is not updated by subsequent play.

Versioned records validate length, checksum and value ranges. A verified current
file is backed up before replacement. Writes flush a temporary file before atomic
replacement; failure preserves the current file and reports **Progress Save
Failed**. A damaged or missing primary can recover from its own valid backup,
with **Backup Recovered** shown in the slot list. Invalid data in one slot does
not hide or overwrite other campaigns. If both copies are unusable, that slot
is marked damaged and requires explicit replacement for a new game.

Original executables, libraries and game assets remain unchanged on disk.

## Level names

The development build displays the original English level-select names instead
of bare stage numbers in save menus and pause status. The names come from the
game's level-select table; only its repeated `LEVEL` suffix is omitted. Save
files still use the same internal indices and require no conversion.

This reference maps the former one-based menu numbers to those names. It is
resource order, not a single mandatory route through the branching campaign.

| Former number | Original name | Former number | Original name |
| --- | --- | --- | --- |
| 1 | Car Park | 14 | Subway Boss |
| 2 | Reception | 15 | Train |
| 3 | Lift | 16 | Air Base |
| 4 | Corridor | 17 | Naval Base 2 |
| 5 | Zengs Office | 18 | Flying Wing |
| 6 | High Street | 19 | Hovercraft |
| 7 | High Street Boss | 20 | Naval Base 1 |
| 8 | Mall | 21 | Air Base Boss |
| 9 | Park 1 | 22 | Naval Base Boss |
| 10 | Park 2 | 23 | Island Lab |
| 11 | Bridge | 24 | Island Lift |
| 12 | Bronx | 25 | Final |
| 13 | Subway | | |
