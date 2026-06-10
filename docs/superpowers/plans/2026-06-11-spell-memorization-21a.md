# Spell memorization (21a) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Port 0.40's real spellbook model (`reading.c read_book`): books are
**read once and consumed** — half the time you learn the spell permanently,
half the time the book turns on you — and casting comes from **what you
know**, not what you carry. Replaces our carry-and-cast convention (a known
deviation since #15i). The final #15 system.

## C grounding
- `reading.c read_book`: already known → "You already know this." (free, the
  book survives). Otherwise a coin flip (`maybe()`):
  - **Learn**: "You learn <Ability>.", the ability becomes permanent,
    **max EP +1**, and the book is consumed (`del_item`).
  - **Bad book**: "This book is difficult to understand!" then 1d3
    (`bad_book`): (1) "The book bites into your hand!" — the **hungry book**
    replaces your weapon for 100 turns (`HUNGRY_BOOK_LIFETIME`; its
    1/1/1/1/1/2 chain ≈ **1d2**, the max-preserving dice per the #68
    convention); (2) "Darkness falls around you!" — blind 33
    (`DEFAULT_BLIND_TIME`), falling through to (3) if already blind;
    (3) a monster **escapes from the book** — flame spirit from Breathe
    Fire, frostling from Frost Ray, else a 1-in-4 nameless horror or an imp,
    hostile, with the 500-tick summon lifetime. The book is consumed either
    way.
- Casting spends EP from the learned set; carrying the book grants nothing.

## Design
- `Game.Known map[string]bool` (book def IDs), saved like `Identified`.
- Books join `ReadableInventory`; `PlayerUse` on a spellbook runs the
  read_book flow (known/learn/bad-book, consumption, the turn).
- `SpellInventory()` returns synthetic `*Item{Def}` for known books in
  sorted-ID order — the entire cast path (`CastSpell`/`CastSpellAt`, the ui
  menu) is unchanged downstream.
- Effect `hungry_book` ("Hungry book") overrides `playerDamageSpec` like
  flame hands.
- Save: `known` field in the DTO; the fixed-point world gains a learned book.

## Task 1: read_book flow (TDD)
- Tests: learn (+1 EPMax, consumed, message), already-known (free, survives),
  each bad-book outcome (direct + seeded dispatch), hungry-book override.
- Commit: `feat(game): spell memorization — books are learned, not carried`

## Task 2: cast-from-known + save + ship
- Tests: an empty-handed caster with Known casts; carried-unread does not;
  save round-trips Known.
- Commit: `feat(game): cast from the learned set + save the known spells`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Reading is a gamble and knowledge is forever: the book burns up either way,
and `c)ast` lists what's in your head, not your pack. Suite green.
