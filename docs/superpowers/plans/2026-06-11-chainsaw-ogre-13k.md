# The chainsaw ogre (13k) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Auditing issue #13 against the C surfaced one missed spawnable
regular: the **chainsaw ogre** (`tsl-0.40/common/monster.c:408`). The last
bestiary gap closes. Advances #13 to completion.

## C grounding
- HP 8, **speed 120**, glyph `O` (`glyph.c:110`); `virtual_chainsaw` melee —
  a 3/3/5/5/7/7 chain (avg 5, max 7) ≈ **1d5+2**, exact on avg/max/min per
  the #68 dice convention. Sometimes drops an ogre corpse.
- Homes (`places.c:97/245`): lead `std_enemy` of the Ominous Cave and the
  Underpass.
- Out of scope (noted): grenade throwing (`treasure_grenade` — the explode
  subsystem), aimed shot, the chainsaw's wound rider (`attr_i_wound` — wounds
  unported), per-monster vision 3 (sense range is global).
- Our `[monster.ogre]` ('O') is pre-parity filler — no plain ogre exists in
  0.40; it joins the filler-retirement chip. The C reuses glyphs itself, so
  the shared `O` differs by color.

## Task: def + spawns + golden (TDD)
- Golden first (red): def (O / 8 / speed 120 / 1d5+2) + spawn presence on
  ominous_cave and underpass; fill TOML (green); full gate; PR; CodeRabbit
  loop → merge.
- Commit: `feat(content): the chainsaw ogre — the last 0.40 bestiary regular`
