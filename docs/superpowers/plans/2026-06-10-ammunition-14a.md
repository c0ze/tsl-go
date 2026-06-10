# Ammunition (14a) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The shortbow stops firing for free: **arrows** as a stackable
ammunition item, consumed per shot, with the C's "Out of ammo!" refusal.
Closes #14's last noted TODO. Advances #14.

## C grounding
- `treasure.c:1332`: "crude arrow"/"crude arrows" (plus sharp/masterwork
  tiers — one type bounds this PR, the quality tiers noted out of scope),
  `item_type_ammo`, glyph `:` (`glyph.c:214`), weight 1 (`WEIGHT_MISSILE`),
  found singly and in piles (`treasure_a_crude_pile`).
- `player.c:1882-1887`: firing pulls one round from the equipped stack;
  an empty quiver prints "Out of ammo!".

## Design
- Item kind `ammo` (validation + `kindWeights` 1); `arrow` def ("crude
  arrow", `:`, power 8 = bundle size). Gen seeds `Charges` from Power like
  wands; loot bundles land on floors.
- **Stacking**: picking up ammo merges into an existing inventory stack of
  the same def (Charges add; the duplicate item vanishes).
- `FireWeapon`: a wielded bow needs an arrow stack — none or empty →
  "Out of ammo!" free refusal; a shot consumes one, the stack vanishing at
  zero.

## Task: ammo kind + stacking + consumption (TDD)
**Files:** `internal/content/content.go`, `internal/game/{fire,use}.go` +
tests, `data/items.toml`, goldens.
- Tests first: a shot consumes one arrow; the empty quiver refuses free with
  the C line; pickup merges bundles; the stack disappears at zero; golden.
- Commit: `feat(content): ammunition — arrows, stacking, Out of ammo!`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Every shot counts — literally — and an empty quiver sends you back to the
dagger. Suite green.
