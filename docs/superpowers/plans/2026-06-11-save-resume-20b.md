# Save/resume, the behavior shell (20b) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Save/resume arc 2/2: wire #77's engine round-trip to the C's
player-facing behavior — **S)ave is save-and-quit**, a savefile resumes at
startup and is **deleted on load** (no save-scumming). Completes the arc.

## C grounding
- `saveload.c try_to_save_game`: on success "Game saved." then exit; on
  failure "Couldn't save game!" and play resumes — never crash a live game.
- `try_to_load_game` at startup; the savefile is deleted after a successful
  restore (`delete_savefile`); a corrupt savefile aborts *without* deleting,
  so nothing is silently lost. `SAVE_FILENAME "TSL-SAVE"` (`main.h:15`) —
  ours: `~/.tsl-save.json`.
- Resume order: `play()`'s `goto creature_turn` — the player acts first;
  the zero-surplus `playerEnergy` already encodes this.

## Design
- ui: `ActSave` on `S` (shift-s); `Run` returns the sentinel
  `ErrSaveRequested` — the ui layer never touches files; cmd owns the path.
- cmd: `savePath()` (home dotfile), `saveTo(path, g)`,
  `loadFrom(path, content)` (behaviors and the lazy level builder are
  constructed inside) — load success deletes the
  file, load failure reports and exits without deleting. main: resume if a
  savefile exists, else new game; on `ErrSaveRequested` save, print
  "Game saved.", exit 0; a failed save logs "Couldn't save game!" and
  resumes the loop.

## Task: the shell (TDD)
**Files:** `internal/ui/ui.go` + test, `internal/ui/tcell/screen.go` + test,
`cmd/tsl/main.go` + test.
- Tests first: `S` maps to ActSave; Run surfaces ErrSaveRequested; a cmd
  round-trip through a temp file resumes the same state and deletes the
  file; a corrupt file errors without deletion.
- Commit: `feat(cmd): S)ave-and-quit + resume-on-start (no save-scumming)`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Press S, see "Game saved.", relaunch, and the dungeon is exactly where you
left it — once, because the savefile is gone. Suite green.
