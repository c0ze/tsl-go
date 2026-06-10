# Scroll of magic weapon (15v) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Scroll 9 of 10: **scroll of magic weapon** (`magic.c magic_weapon`)
— your hands ignite for 22 turns, overriding whatever you wield. Only the
familiar scroll (needs summons) remains after this. Advances #15.

## C grounding
- `magic.c:92`: `set_temp_weapon(caster, virtual_flame_hands,
  MAGIC_WEAPON_LIFETIME)` (22, `rules.h:135`); "Your hands seem to be on
  fire.". A temp weapon supersedes the wielded one while it lasts.
- `vweapon.c:163`: flame hands deal fire damage in a 3/5/5/5 attack
  sequence; our flat-dice model approximates that as **2d4+1**.
- `treasure.c:1135`: named "scroll of magic weapon" (with the prefix,
  unlike blink/mark/recall/trap-detection).

## Design
- Effect `flame_hands` (HUD "Flaming hands"), 22 turns.
- `playerDamageSpec()`: while `flame_hands` is active, return the flame
  spec — overriding the wielded weapon, like the C's temp weapon.
- Behavior `magic_weapon` + `scroll_magic_weapon` def; joins the appearance
  pool.

## Task: effect + override + scroll (TDD)
**Files:** `internal/game/combat.go` + test, behaviors + data + goldens.
- Tests first: with a dagger wielded and flame hands active, the damage spec
  is the flame spec; expired → the dagger's returns; behavior message; golden.
- Commit: `feat(content): scroll of magic weapon (flame hands)`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
One read and your fists burn hotter than your blade for 22 turns. Suite green.
