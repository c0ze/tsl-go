# Trap variants: web, flash, polymorph, plate (18f) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Issue #18's audited trap remainder — all verified LIVE in 0.40
(flash at encounter weight 12, web at 10, the heaviest hazards in
`places.c`).

## C grounding
- **Web** (`web.c`): `effect_web` 27 turns (`WEB_DURATION`); movement is
  replaced by `struggle_web` — each attempt knocks **6** off the clock
  (`WEB_STRUGGLE`); under 6, "You break free of the web." Attacking an
  adjacent enemy stays allowed (the C checks enemies before the web).
- **Flash** (`traps.c`): "You are blinded by a bright flash!" — blind 33
  (`DEFAULT_BLIND_TIME`); already-blind targets are immune (so is a worn
  blindfold, by commitment).
- **Polymorph**: "You step on a polymorph trap!" → `shapeshift_random` (the
  potion's roll, 85 turns).
- **Plate**: "You step on an electrified plate!" — `1d4+1` (`PLATE_DAMAGE`)
  straight damage, death cause "electricity".

## Design
- `TileDef.Damage` (the plate's dice); `springTrapAt` dispatches per trap:
  damage plates, web/flash with their C lines, `Effect: "polymorph"` calling
  a new `Game.PolymorphRandom` (extracted from the behavior, which now
  delegates). New `web` effect ("Webbed"): the player's move/bump-move turns
  into a struggle; webbed monsters struggle in `monsterAct`.
- Four new trap tiles (all `Disguise`-hidden like darts); `placeTraps` deals
  uniformly across the defined trap tiles (the C's per-level encounter
  weights noted as an approximation).

## Tasks (TDD)
1. Web effect + struggle (player + monster), plate damage, flash, polymorph
   trap; `PolymorphRandom` extraction.
2. Tiles + gen mix + goldens; full gate; PR; CodeRabbit loop → merge.
