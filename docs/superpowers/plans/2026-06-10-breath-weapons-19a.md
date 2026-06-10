# Breath weapons (19a) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The breath subsystem (`breath.c`), monster side: **cone attacks**
for the six 0.40 breathers already in our roster — the Dragon and flame
spirit/burning skull breathe fire, the Lurker, giant slimy toad (and kin)
noxious fumes. Settles the Lurker's noted TODO and gives the Dragon its
signature. Player-side breath spellbooks are the arc's PR 2. Advances #19
(breath/explode) + #13.

## C grounding
- `area.c area_cone`: walk `range` steps along the direction; each step the
  cone swells, throwing two perpendicular side-rays of the swell length;
  stops at walls. Fire range 3, poison range 4 (`missile.c get_spell_range`).
- `breath.c breath_weapon`: every creature in the cone is hit — friendly
  fire included. Fire: `2d4` (`BREATHE_FIRE_DAMAGE`). Poison: `1d2` +
  poisoned 12 turns (`NOXIOUS_BREATH_*`), skipped for nonbreathers. Messages:
  "breathes fire!"/"breathes poison!", the player "You get burned!" /
  "You inhale the vile fumes!".
- Breathers (`monster.c`/`unique.c`): Dragon, flame spirit, burning skull →
  fire; Lurker, giant slimy toad, hellhound → per their C attrs (verified at
  implementation against each block).
- Bounded AI (the C picks via `cone_direction` + EP costs; our monsters have
  no EP): breathe when the player is within breath range with line of sight,
  along the dominant direction toward them; otherwise act as before.

## Design
- `game.breathCone(origin Pos, dx, dy, rng int) []Pos` — the C walk-and-swell,
  wall-stopped.
- `MonsterDef.Breath` ("" | "fire" | "poison") with ranges/damages as engine
  consts; validation restricts the values.
- `monsterAct`: a breather with the player in range + LOS exhales before
  other options; every creature in the cone takes the roll (allies and
  hostiles alike — C friendly fire), the player via `HurtPlayer`
  (cause "<name>'s fiery breath" → noun "dragonfire"? keep cause = monster
  name, consistent with melee).
- Data: breath flags on the six defs + goldens.

## Task 1: cone geometry + breath resolution (TDD)
**Files:** `internal/game/{combat,los}.go` + new `breath_test.go`.
- Tests first: cone shape (range 3 east = widening triangle, wall-clipped);
  a fire breath burns the player and a bystander rat in the cone, spares one
  outside; poison breath poisons; a breather out of range closes in instead.
- Commit: `feat(game): breath weapons — C cone geometry + fire/poison breath`

## Task 2: data flags + ship
- Six defs flagged; goldens; full gate; PR; CodeRabbit loop → merge.
- Commit: `feat(content): the Dragon (and friends) breathe`

## Done when
Standing three tiles from the Dragon is no longer safe: the cone lights you,
your imp, and the unlucky kobold beside you. Suite green.
