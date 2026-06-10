# Hidden traps + trap detection scroll (18d) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Port 0.40's trap concealment: traps start **unrevealed** and render
as plain floor until they spring, are spotted, or a **trap detection scroll**
exposes them. Fixes a known divergence (our traps were always visible) and
ships scroll 8 of 10. Advances #18 + #15.

## C grounding
- `traps.c`: a trap starts `revealed = false` with `difficulty = roll(2,6)`;
  activating reveals it (`traps.c:330`).
- `game.c try_to_detect_traps` (passive, per turn): a *visible* unrevealed
  trap is spotted when `perception + s_trap_detection > difficulty`; the
  player's perception is 3 (`player.c:1265`), so only the flimsiest traps
  betray themselves on sight.
- `magic.c reveal_traps` (the scroll): every unrevealed trap on the level is
  exposed and written to the automap; "You sense the presence of traps.".
- `treasure.c:1164`: named "trap detection scroll" (prefix-less, like blink/
  mark/recall).

## Design
- `game.Tile` gains `Disguise *content.TileDef`, `Revealed bool`, and
  `TrapDifficulty int` (instance state alongside Visible/Seen). New
  `Tile.Appears()` returns the Disguise while unrevealed, else the real def;
  `ui.BuildView` renders `Appears()` — the seam also fits future secret
  doors. Zero values keep every existing tile untouched.
- gen `placeTraps`: traps go down disguised as floor with difficulty 2d6.
- A shared `g.springTrapAt(p)` (reveal + effect + "You trigger a trap!")
  replaces the three duplicated trigger sites (step, landing, blink).
- Passive spotting in `passTurn`: visible disguised tiles reveal when
  `playerPerception (3) > TrapDifficulty`, the C's strict inequality.
- `Game.RevealTraps() int` exposes (and maps) every disguised tile; behavior
  `detect_traps` + `scroll_trap_detection` with the C message.

## Task 1: disguise/reveal engine + passive spotting (TDD)
**Files:** `internal/game/{game,combat,effects,teleport,fov}.go` + tests,
`internal/ui/ui.go` + test.
- Tests first: a disguised trap renders as floor and springs+reveals on step
  (then renders as itself); difficulty 12 stays unseen while visible,
  difficulty 2 is spotted on sight; landing and blink reveal too;
  `RevealTraps` exposes all and returns the count.
- Commit: `feat(game): hidden traps — disguise, spring-reveal, passive spotting`

## Task 2: gen difficulty + scroll + ship
**Files:** `internal/gen/gen.go` + test, behaviors + data + goldens.
- gen rolls 2d6 per trap and disguises it; scroll behavior + def.
- Commit: `feat(content): trap detection scroll`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
The floor ahead looks innocent — until your boot finds the plate, your landing
springs it, or one scroll lights up every needle on the level. Suite green.
