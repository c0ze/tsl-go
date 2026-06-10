# Save/resume, engine round-trip (20a) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The save/resume arc, PR 1 of 2: a complete, tested **engine
round-trip** — `Game.Save(io.Writer)` and `LoadGame(...)` that reconstruct
the whole world (dungeon cache included) so play continues exactly where it
stopped. PR 2 (20b) wires the S)ave key, load-on-start, and the C's
delete-on-load rule. Advances the last big roadmap arc.

## C grounding
- `saveload.c try_to_save_game`: serialize everything, "Game saved.", then
  **exit** — saving is quitting. `try_to_load_game` at startup restores and
  the savefile is deleted (no save-scumming); `play()`'s opening
  `goto creature_turn` means the player always acts first on resume —
  "saving is a free action". Our zero-surplus `playerEnergy` convention
  already encodes that invariant.
- The C dumps raw structs through a pointer table (`build_saveload_table`);
  the *format* is an implementation detail — the faithful part is the
  behavior. We use JSON DTOs.

## Design
- `internal/rng`: `Snapshot() []uint32` / `Restore(snapshot)` exposing the
  MT state, so the post-load dice sequence continues exactly — full
  determinism across the save boundary.
- `internal/game/save.go`: DTO types with JSON tags mirroring Game, Level,
  Creature, Item, Tile (def pointers ↔ content IDs; equipped slots ↔
  inventory indices; `Disguise`/`DisguiseAs` ↔ IDs; all unexported clocks —
  `playerEnergy`, `swimFatigue`, `epTurn`, recall pin — included).
- `Game.Save(w io.Writer) error` walks the Dungeon's level cache;
  `LoadGame(r, content, behaviors, build) (*Game, error)` rebuilds the
  Dungeon around the restored cache (defs/build re-injected, like cmd does
  at boot).
- Unknown content IDs in a savefile fail the load with a clear error
  (the fail-fast convention).

## Task 1: RNG snapshot (TDD)
**Files:** `internal/rng/mt.go` + test.
- Tests first: snapshot → draw N values → restore → the same N values again.
- Commit: `feat(rng): MT state snapshot/restore for save games`

## Task 2: the round-trip (TDD)
**Files:** `internal/game/save.go` + `save_test.go`.
- Tests first: (1) the **fixed-point invariant** — save(load(save(g))) ==
  save(g) on a world salted with every kind of state (effects, identify +
  appearances, recall pin, equipped gear, ally with lifetime, disguised
  mimic, revealed trap, wand/ammo charges, two visited levels);
  (2) RNG continuity — a worldTick after load moves a rat exactly as the
  unsaved twin does; (3) equipped identity — the loaded Weapon IS the
  inventory entry; (4) a marked recall still works cross-level after load;
  (5) an unknown item ID fails loudly.
- Commit: `feat(game): save/load — the full world round-trips through JSON`
- Full gate; push, PR, CodeRabbit loop → merge. 20b follows.

## Done when
A buffer holds the entire dungeon — every clock, glamour, and pinned recall —
and a game loaded from it is indistinguishable from one that never stopped.
Suite green.
