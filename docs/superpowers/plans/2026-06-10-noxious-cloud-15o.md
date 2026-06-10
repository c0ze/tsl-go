# Noxious Cloud Spell (15o) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** A **noxious cloud** spell — a self-centred area-of-effect that
**poisons** every nearby creature (damage-over-time area denial), rounding out
the offensive spell kit alongside flash (AoE blind), frost ray (beam), force
bolt (single-target), and first aid (heal). Advances #15.

## Design
- Generalise the flash spell's area logic into a shared helper
  `affectNearby(radius, kind, turns) int` — applies a status effect to every
  creature within Chebyshev `radius` of the player, returning the count.
  `FlashBlind` becomes a thin wrapper (`affectNearby(r, "blind", t)`); a new
  `PoisonNearby` wraps `affectNearby(r, "poison", t)`. No behaviour change to
  flash; the existing FlashBlind test still passes.
- Poison already damages creatures 1 HP/turn and kills via `killCreature`
  (`tickCreatureEffects`), so the cloud deals real, lingering damage.
- A `noxious_cloud` behaviour mirrors `flash`: casts `PoisonNearby(NoxiousRadius,
  Power)`, reporting how many creatures were caught (or that none were near).
- Data: `spellbook_noxious_cloud` (kind spellbook, use `noxious_cloud`, an EP
  cost, power = poison turns). `NoxiousRadius = 3` (tighter than flash's 4,
  since it does damage, not just control).

## Task 1: engine helper (TDD)
**Files:** `internal/game/spell.go`, `internal/game/spell_test.go`
- Test first: `PoisonNearby` poisons creatures in radius (and a far one is
  spared); a poisoned creature loses HP on the next `monstersAct`.
- Implement `affectNearby`; rewrite `FlashBlind` over it; add `PoisonNearby` +
  `NoxiousRadius`.
- Commit: `feat(game): AoE poison via shared affectNearby helper`

## Task 2: behaviour + data + golden + ship
**Files:** `internal/behaviors/behaviors.go`, `internal/behaviors/behaviors_test.go`,
`data/items.toml`, `data/data_test.go`
- Test first: the behaviour poisons nearby creatures and reports the count; the
  embedded spellbook loads with its cost.
- Implement `noxious_cloud`, register it; add `spellbook_noxious_cloud`; golden.
- Commit: `feat(content): spellbook of noxious cloud (AoE poison)`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Cast noxious cloud and the pack around you starts bleeding HP each turn until the
poison fades or kills them. Suite green through CodeRabbit.
