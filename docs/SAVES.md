# Campaign checkpoints

The wrapper automatically saves at the beginning of each campaign level,
including the first level of a new game. Completing a level replaces the
checkpoint when the next level finishes its native initialization. The saved
state includes the destination level, one- or two-player mode, difficulty,
character selection, health, remaining lives, scores and extra-life thresholds.
End-of-level bonuses use the original game logic before the next checkpoint.

Select **Continue** at the top of the main menu to resume the saved level from
its beginning. The selected row shows its stage number and player count. The
entry uses the original menu renderer, highlight, navigation and confirmation
buttons. It appears only when a valid checkpoint is available.

There is one automatic checkpoint per installation, stored as
`fightingforce-progress.sav` beside the compatibility DLL. Copy that file to
back it up or transfer it between the supported English retail and reviewed
English RIP installations. Device assignments, button bindings and wrapper
settings remain in their existing configuration file.

Starting a new game replaces the previous campaign checkpoint when its first
level loads. Leaving a level, losing a life or retrying does not update its
checkpoint. After completing the campaign, the last level's checkpoint remains
available for replay. Attract demonstrations and non-campaign screens never
overwrite progress. Two-player saves retain player slots, including a surviving
second player when the first has been eliminated.

This is level-entry saving, not a snapshot of an ongoing fight. Enemy state,
destroyed scenery, held objects, animations and camera positions are initialized
by the normal level loader. Earlier movie transitions are not replayed when
resuming a later level. Current wrapper control and camera settings still apply.

The save uses a versioned, fixed-size record with bounds checking and a checksum.
Writes are flushed to a temporary file before replacing the existing checkpoint.
A failed write retains the previous file and reports **Progress Save Failed**
on the next main menu. Invalid or unsupported data disables Continue and shows
**Save Data Invalid**; starting a new game can create a replacement. The game
folder must be writable. Original executables, libraries and assets are never
modified.
