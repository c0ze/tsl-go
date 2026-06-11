# Rings & Amulets 14c Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** **Rings and amulets** as passive accessory equip slots (#14) — two new
worn slots, *separate* from weapon and armor, whose bonuses **stack** onto your
hit and defense. The new mechanic is "two more equip slots," faithful to 0.40
where items have an `equipped` flag and `item_type` covers body/feet/head/cloak
plus rings and amulets (`tsl-0.40/common/item.h`, `tsl-0.40/common/gent.h: gent_amulet/gent_ring`).

## Design
- **Content:** two new item kinds, `ring` and `amulet`. Each carries passive
  `attack` and/or `dodge` bonuses (reusing the existing `ItemDef.Attack`/`Dodge`
  fields). Validation: a ring/amulet must grant at least one bonus (attack > 0
  **or** dodge > 0) — analogous to "wand needs damage or effect".
- **Equip slots:** `Game.Ring` and `Game.Amulet`, mirroring `Weapon`/`Armor`.
  - `autoEquip` fills an empty slot on pickup (faithful default, never downgrades).
  - `PlayerUse` on a ring/amulet "puts it on" (swaps into the slot); logs
    `You put on the %s.`
- **Stat folding:** generalise `playerAttackStat`/`playerDodgeStat` to sum the
  relevant bonus across **all** equipped gear (weapon, armor, ring, amulet) via
  a small `equippedGear()` helper. Weapons carry no dodge and armor no attack
  today, so summing both fields across all slots is correct and extensible.
  Damage dice stay weapon-only.
- **Surfacing:** morgue lists worn ring/amulet; HUD status line shows a `Worn:`
  segment when an accessory is equipped.
- **Identify:** rings/amulets are identified-by-default this slice (not added to
  `unidentifiable`), keeping the slice about the *equip-slot mechanic*. Shuffled
  ring/amulet *appearances* are a faithful follow-up (slice 2), as is an
  amulet-of-vitality **max-HP** bonus (needs an effective-max refactor).

## Task 1: content kinds + validation (TDD)
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- Tests first: `ring`/`amulet` are valid kinds; a ring/amulet with no attack and
  no dodge is rejected; one with a dodge bonus loads.
- Implement: add `ring`/`amulet` to `validItemKinds`; add a `validateItem` case
  requiring `Attack > 0 || Dodge > 0`.
- Commit: `feat(content): ring and amulet accessory kinds`

## Task 2: equip slots + stacked stats (TDD)
**Files:** `internal/game/game.go`, `internal/game/use.go`,
`internal/game/combat.go`, `internal/game/morgue.go`, and their `_test.go`.
- Tests first: `PlayerUse` on a ring sets `g.Ring` and logs "put on"; `autoEquip`
  fills an empty ring/amulet slot on pickup; `playerDodgeStat` reflects armor +
  ring + amulet **stacked**; `playerAttackStat` reflects weapon + accessory;
  re-using a ring swaps the slot; morgue lists the worn accessory.
- Implement: `Ring`/`Amulet` fields; `equippedGear()`; fold the stat sums;
  `autoEquip`/`PlayerUse` cases; morgue lines.
- Commit: `feat(game): ring and amulet equip slots with stacked bonuses`

## Task 3: HUD + data + golden + ship
**Files:** `internal/ui/ui.go`, `internal/ui/ui_test.go`, `data/items.toml`,
`data/data_test.go`.
- Tests first: status line shows the worn accessory; the embedded rings/amulet
  load with their bonuses.
- Implement: `Worn:` HUD segment; add `ring_of_accuracy` (attack), 
  `ring_of_protection` (dodge), `amulet_of_warding` (dodge); seed a couple into
  spawn/loot as appropriate; golden test.
- Commit: `feat(content): rings of accuracy/protection and amulet of warding`
- Full gate (`go test ./...`, `gofmt`); push, PR, CodeRabbit loop → merge.
  Advances #14.

## Done when
You can wear a ring **and** an amulet alongside your weapon and armor, their
bonuses stack onto your hit/defense, the HUD and morgue show them, and the suite
is green through CodeRabbit.
