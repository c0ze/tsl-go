# Book of Deathspell + manual of pharmacy (15x) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Two more 0.40 books: the **Deathspell** (a 1-EP coin flip — the
target dies, or you do) and the **manual of pharmacy** (one read identifies
every potion). Advances #15.

## C grounding
- `magic.c:1301 deathspell`: player-only, touch range (a direction); no
  creature there → "No one is there!" and **no turn, no EP** (the C comments
  that the death risk stops tile-probing abuse). Otherwise "Death..." then
  `roll(1,2)`: caster dies ("Yours!", morgue "failed a Deathspell" → our
  noun: "deathspell") or the target is killed outright ("Theirs!"/"dies.").
  Cost 1 EP (`attrs.c:355`).
- `reading.c:127`: the manual of pharmacy identifies the whole potion table —
  "You learn how to identify all potions." Idempotent, so modelling it as a
  castable book is effect-equivalent to the C's read-once.
- `treasure.c:1116`: the **manual of camouflage is commented out in 0.40**
  (`reading.c:145-149` too) — omitting it entirely is the faithful choice.
- Names: "book of Deathspell" (capitals like Breathe Fire), "manual of
  pharmacy" (lowercase in the C).

## Design
- `ItemDef.Deathspell bool`; `CastSpellAt` branch: target must hold an
  adjacent creature (else the C refusal, free); spend 1 EP, flip:
  `killCreature(target)` or player death (cause "deathspell").
- `Game.IdentifyAllPotions() int` marks every potion def identified;
  behavior `pharmacy` returns the C line. Both books join the data.

## Task: deathspell + pharmacy (TDD)
**Files:** `internal/game/{spell,identify}.go` + tests, behaviors + data +
goldens.
- Tests first: seeded flips — one seed kills the adjacent rat outright, a
  another kills the caster (cause "deathspell"); a non-adjacent target or
  empty tile refuses free; pharmacy identifies an unidentified potion's
  display name; goldens.
- Commit: `feat(content): book of Deathspell + manual of pharmacy`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
One book ends fights instantly — half the time in your favor — and the other
ends potion roulette forever. Suite green.
