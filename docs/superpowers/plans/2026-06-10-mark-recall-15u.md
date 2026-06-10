# Mark & recall scrolls (15u) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The teleport-home pair from 0.40's scroll list: the **mark scroll**
pins a spot (level + position), the **recall scroll** snaps you back to it —
across levels. Advances #15.

## C grounding
- `teleport.c cast_mark`: store `recall_location/recall_y/recall_x`
  (level + coordinates); "Destination marked.".
- `teleport.c cast_recall`: `change_level` + place the caster at the mark;
  "You find yourself back in <level name>.". The C's unmarked recall would
  land at its zero-initialized location — we fizzle instead ("nothing
  happens"), the deliberate divergence here.
- Names (`treasure.c:1192/1199`): "mark scroll" / "recall scroll" — like the
  blink scroll, no "scroll of" prefix.

## Design
- `Game.recallLevel string` / `recallPos Pos`; `MarkRecall()` records
  `Level.ID` + position; `Recall() bool` re-enters the marked level through
  the Dungeon (persisted cache — same machinery as stairs Travel) and places
  the player at the mark, nudging one ring outward if a creature now stands
  there (the C stacks creatures; we don't). Unmarked or no-dungeon → false.
- Behaviors `mark` / `recall` with the C messages; recall names the level via
  `LocationName()`. Items `scroll_mark` / `scroll_recall` join the pool.

## Task 1: MarkRecall/Recall engine (TDD)
**Files:** `internal/game/teleport.go` + tests.
- Tests first: mark here, walk away, recall returns exactly here (same
  level); recall across levels via a real two-level dungeon (mark, take
  stairs, recall → back on the original level at the mark); unmarked recall
  reports false; an occupied mark nudges to an adjacent tile.
- Commit: `feat(game): mark/recall — cross-level return to a pinned spot`

## Task 2: behaviors + data + ship
- Behaviors with C messages; goldens for both defs; full gate; PR; CodeRabbit
  loop → merge.
- Commit: `feat(content): mark & recall scrolls`

## Done when
Read the mark scroll beside the shop-quality loot you can't carry (you're
burdened, after all), dive deeper, then recall straight back to it from three
levels down. Suite green.
