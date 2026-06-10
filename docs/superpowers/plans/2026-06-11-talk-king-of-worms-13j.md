# Talk verb + the King of Worms (13j) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The C's interaction verb (`actions.c:790 interact`) and the unique it
was deferred for: **the King of Worms**, boss of the Ominous Cave
(`places.c` gives that level `boss = encounter_king_of_worms`). Advances #13.

## C grounding
- `actions.c interact`: talking to a sleeping creature reports "appears to be
  asleep." without a turn; otherwise per-creature flavor — the gnoblin
  "snarls at you!", the ghoul "appears to desire your brain.", the imp
  "giggles impishly!", the slime "oozes around, indifferent to your
  inquiries." — and the King's lore line: "He keeps the wisdom that you need,
  the password that you want." A successful chat passes the turn.
- The C's no-line default is literally "BUG: Undefined generic interaction!"
  — a debug marker, not content; we default to "You get no reply."
  (deliberate, gentle deviation).
- `unique.c:97`: the King — `W` (`glyph.c:113`), HP 30, **speed 10** (the
  Necromancer's glacial turret pace), sleep-immune, no spells: a lore fixture
  you can still pick a fight with. The password quest itself needs systems
  0.40 never finished; the line is the content.

## Design
- `MonsterDef.Chat` — the full rendered line. `Game.Talk()`: first adjacent
  creature; none → free refusal; asleep → free "appears to be asleep.";
  chat → the line + a turn; silent → the gentle default + a turn.
- ui `ActTalk` on `t` (free key) through the Run loop and tcell map.
- Data: chat lines for gnoblin/ghoul/imp/slime; `king_of_worms` def
  (boss-only) + `ominous_cave` boss wiring; goldens.

## Task: Talk + the King (TDD)
**Files:** `internal/game/` (new talk.go + test), `internal/ui/ui.go` +
tcell map + tests, data + goldens.
- Tests first: chatting an adjacent gnoblin logs its line and passes a turn
  (rat closes in elsewhere); no neighbor refuses free; a sleeping neighbor
  reports free; a silent monster gets the default; the King's def + boss
  wiring goldens.
- Commit: `feat(game): t)alk — the C interaction verb + the King of Worms`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Deep in the Ominous Cave a `W` waits, barely moving, and answers exactly one
question with the line 0.40 players remember. Suite green.
