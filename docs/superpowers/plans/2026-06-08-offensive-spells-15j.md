# Offensive Spells 15j Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Give the spell system teeth: a **targeted attack spell** you aim with
the cursor. Ships **force bolt** — cast it (`c`), aim, and blast a creature for
EP. Reuses the cast loop (15i) plus the `Target` cursor and `lineOfSight` already
built for wands/bows.

## Design
- A spellbook is now either a **utility** spell (a `use` behavior, e.g. first aid)
  or a **targeted** spell (`ranged` + `damage`, no behavior). Relax spellbook
  validation to require `use` **or** (`ranged > 0` and a valid `damage` spec);
  `cost > 0` always.
- **`Game.CastSpellAt(book, target)`**: refuse (no EP, no turn) when EP is too
  low, the target is out of range, or `lineOfSight` is blocked; otherwise spend
  EP, roll `damage` against the creature there (kill via `killCreature`), and
  pass a turn.
- **`ActCast`**: after the spell menu, if the chosen spell has `Ranged > 0`,
  prompt `Target` and `CastSpellAt`; otherwise `CastSpell` (unchanged).

**Scope:** single-target bolt. Line/beam spells (frost ray) and status/AoE spells
are later — they layer on the same cast+target flow.

## Task 1: content + game — targeted casting (TDD)
**Files:** `internal/content/content.go` (spellbook validation),
`internal/content/content_test.go`, `internal/game/spell.go` (CastSpellAt), tests
- Validation: a damage spellbook (ranged + damage, no use) loads; a spellbook with
  neither use nor ranged-damage is rejected.
- `CastSpellAt` with the EP/range/LOS gates + damage.
- Tests first: a ranged-damage spellbook validates; casting at a creature in
  range/LOS spends EP + damages/kills + passes a turn; out of range / blocked /
  too little EP each refuse (no EP spent).
- Commit: `feat(game): targeted attack spells (CastSpellAt)`

## Task 2: ui — aim on cast + data + ship
**Files:** `internal/ui/ui.go`, `internal/ui/ui_test.go`, `data/items.toml`,
`data/data_test.go`
- `ActCast`: target when the spell has range, else cast in place.
- `spellbook_force_bolt` (ranged, damage, cost; no use). Golden test.
- Tests first: Run casts force bolt at a creature via a fake Target (creature dies,
  EP drops).
- Commit: `feat(data): spellbook of force bolt`
- Full gate; push, PR, CodeRabbit loop → merge. Advances #15 (magic).

## Done when
Press `c`, pick force bolt, aim down a corridor, and blast a monster for EP —
duck behind a wall and the spell has no shot. Suite green.
