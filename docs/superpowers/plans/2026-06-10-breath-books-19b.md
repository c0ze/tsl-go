# Breath spellbooks (19b) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Breath arc 2/2: the player exhales too. 0.40's first two spellbooks
(`treasure.h:131`) are **book of Noxious Breath** and **book of Breathe
Fire** — both cost 5 EP (`attrs.c:434/438`) and ride the cone machinery from
#70. Advances #19 + #15.

## C grounding
- `treasure.h:132-133` + `treasure.c:1080/1086`: the books exist and are
  named "book of Noxious Breath" / "book of Breathe Fire" (the C's "book
  of" + capitals; our older five books say "spellbook of x" — a pre-existing
  local convention left alone).
- `attrs.c`: both invoke the same `breath_weapon` the monsters use, cost 5.
- `breath.c`: the player picks a direction; "You breathe fire!"/"You breathe
  poison!"; everything in the cone is hit, friendly fire (your imp!)
  included, per-target rolls.

## Design
- `ItemDef.Breath` ("fire"/"poison", validated) mirroring MonsterDef.
- `playerBreathe(kind, target)` in breath.go: direction from
  `signOf(target − player)`, the #70 cone/damage/poison constants, kills via
  `killCreature`.
- `CastSpellAt`: a breath book spends its EP and exhales (before the beam
  branch); the ui's cast wiring targets when `Ranged > 0 || Breath != ""` —
  the existing Target prompt supplies the direction, no Prompter change.
- Data: the two books, cost 5, C names; goldens.

## Task: playerBreathe + cast wiring + data (TDD)
**Files:** `internal/game/{breath,spell}.go` + tests, `internal/ui/ui.go`,
`internal/content/content.go`, data + goldens.
- Tests first: casting Breathe Fire at a rat three east burns it and a
  flank bystander, spares one beyond the cone, spends 5 EP; Noxious Breath
  poisons; insufficient EP refuses without a turn.
- Commit: `feat(content): breath spellbooks — the player exhales too`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Five EP buys you a cone of your own — point it away from the imp. Suite green.
