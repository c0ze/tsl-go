# Scroll of Summon Familiar (15w) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The tenth and final 0.40 scroll: **scroll of Summon Familiar**
(`magic.c summon_familiar`) — an imp ally that follows you and fights your
enemies. Brings the port's first ally AI and monster-vs-monster combat.
Completes the scroll list. Advances #15.

## C grounding
- `magic.c:585`: always an **imp** (the 1d4 switch has only a default),
  `charm()`ed — `attr_player_ally = 1`, ai_offensive — with
  `lifetime = DEFAULT_SUMMON_LIFETIME` (500, `rules.h:70`); placed at the
  nearest free spot; "The imp has arrived.".
- `game.c:123-146`: lifetime ticks down once per creature-turn; at 0 the
  creature "disappears." (no corpse, no drops).
- `ai.c basic_ai`: allies and hostiles target each other (the `enemies()`
  split). Bounded port: an ally attacks an adjacent hostile, pursues the
  nearest hostile in sense range, else heels to the player; a hostile
  prefers an adjacent ally over a distant player. (Full mutual targeting at
  range is out of scope — noted.)
- `treasure.c:1171`: named "scroll of Summon Familiar" (capitals in 0.40).

## Design
- `Creature.Ally bool` + `Creature.Lifetime int` (0 = permanent). worldTick
  decrements a positive lifetime each tick (≈ a creature-turn at base speed);
  at 0 the creature disappears, corpse-free.
- Ally turn in `monsterAct`: adjacent hostile → `monsterFights` (the first
  creature-vs-creature swing: attack vs dodge, damage dice, `killCreature`);
  nearest hostile within sense range → step toward it; otherwise follow the
  player (step toward when more than 2 away).
- Hostile turn: an adjacent ally is attacked before a distant player.
- Behavior `summon_familiar`: clones the imp def into an Ally with the 500
  lifetime at a free spot near the player; the C arrival message. Item
  `scroll_familiar`, the C's capitalized name.

## Task 1: ally AI + lifetime + creature combat (TDD)
**Files:** `internal/game/{creature,combat}.go` + new `ally_test.go`.
- Tests first: an ally adjacent to a rat bites it (rat HP drops, player HP
  untouched); an ally pursues a rat in range; an idle ally heels toward a
  distant player; lifetime 2 expires ("disappears.", no corpse); a hostile
  adjacent to both targets the ally.
- Commit: `feat(game): ally AI — creature-vs-creature combat + summon lifetimes`

## Task 2: behavior + data + ship
- `summon_familiar` behavior + `scroll_familiar` def + goldens; full gate;
  push, PR, CodeRabbit loop → merge.
- Commit: `feat(content): scroll of Summon Familiar — the scroll list complete`

## Done when
Read the scroll and an imp pads after you for 500 turns, throwing itself at
whatever you're fighting — all ten 0.40 scrolls shipped. Suite green.
