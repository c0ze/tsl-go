# Blink scroll rename + scroll of amnesia (15t) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Two scroll-kit fixes from the 0.40 source. First, a faithfulness
correction: 0.40 has **no teleportation scroll** — its random-relocation
scroll is the **blink scroll** (`treasure.h` scroll list; `treasure.c:1178`),
which is exactly the mechanic our mislabeled "scroll of teleportation"
shipped. Second, the **scroll of amnesia** — the map-eraser, the inverse of
magic mapping. Advances #15.

## C grounding
- `treasure.h:119-129` is the whole 0.40 scroll list: identify, magic mapping,
  trap detection, recharge, **blink**, familiar, mark, recall, **amnesia**,
  magic weapon. No teleportation scroll exists; the C's controlled
  `cast_teleport` is spell-side and "computer controlled teleport works just
  like blink" (`teleport.c:261`).
- `teleport.c cast_blink`: move to a random free spot; "You blink away..." /
  "Suddenly, you are somewhere else."; springs any trap at the destination.
- `magic.c amnesia`: wipe the automap (`memory[y][x] = gent_blank`);
  "You suddenly feel very forgetful.".
- `treasure.c:1178`: the item is named "blink scroll" (not "scroll of blink").

## Design
- Rename `scroll_teleport` → `scroll_blink`, name "blink scroll"; the
  behavior key stays `teleport` internally but the read messages become the
  C's blink lines, and the landing applies the destination tile's trap
  effect (cast_blink's `activate_trap`). Tests referencing the old id move.
- New `Game.ForgetMap()` (inverse of the reveal used by magic mapping:
  Seen=false everywhere); behavior `amnesia` returns the C line;
  `scroll_amnesia` joins the scroll appearance pool — an unidentified read
  can now cost you your map.

## Task 1: rename + landing trap + ForgetMap (TDD)
**Files:** `internal/game/teleport.go` + tests, `internal/behaviors/behaviors.go`
+ tests, `data/items.toml`, `data/data_test.go`.
- Tests first: blink onto a trap tile springs it; ForgetMap clears Seen;
  amnesia behavior wipes the map with the C message; golden defs
  (`scroll_blink` named "blink scroll", `scroll_amnesia`).
- Commit: `feat(content): blink scroll (0.40's true name) + scroll of amnesia`

## Task 2: gate + ship
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
The scroll kit matches `treasure.h`: the random hop is honestly called a blink
scroll and lands you on whatever was waiting there, and one unlucky read wipes
your hard-earned map. Suite green.
