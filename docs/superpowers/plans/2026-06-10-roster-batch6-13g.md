# Roster batch 6 (13g) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Five more canonical 0.40 monsters (batch 6): **electric snake,
sludge dweller, severed hand, floating brain, flame spirit** — all from
`common/monster.c`, glyphs from `common/glyph.c`, spawn levels from
`common/places.c` `std_enemy` tables. Advances #13. (The **mimic** waits: its
essence is the item-disguise `ai_mimic` machinery, its own increment.)

## C grounding (stats `monster.c`, glyph `glyph.c`, levels `places.c`)
- **electric snake** `S` cyan: HP 3, speed 35, fangs; Dungeon/Ominous
  Cave/Underpass. (m_shock/m_recharge abilities out of scope.)
- **sludge dweller** `s` brown: HP 2, speed 60, **mudball caster** → ranged;
  everywhere mid-game (Dungeon, Ominous Cave, Laboratory, Comm Hub,
  Underpass, Drowned City).
- **severed hand** `p` normal: HP 4, speed 80, fast strangler; Catacombs.
  (0.40 itself reuses `p` — goatman also wears it; colors differ.)
- **floating brain** `b` magenta: HP 15, speed 70, **force-bolt caster** →
  ranged; the Frozen Vault. (blink out of scope.)
- **flame spirit** `j` red: HP 20, speed 60, burning melee; the Dragons Lair.
- Glyph note: `b/j/s/S` are currently worn by bat/jackal/skeleton/cave_snake —
  pre-parity filler absent from 0.40 (no `monster.c`/`glyph.c` entries).
  Batch 6 takes the canonical glyphs with distinct colors; filler retirement
  is flagged as its own follow-up task.

## Design
Data-only, the batch-1..5 pattern: five `[monster.*]` defs (attack/dodge/
damage scaled like prior batches since the C's attack sequences don't map
1:1), spawn-table entries on the C-faithful levels, `min_depth` tiering for
generic generation, golden data test.

## Task: defs + spawns + golden (TDD)
**Files:** `data/monsters.toml`, `data/levels.toml`, `data/data_test.go`.
- Golden test first (red): five defs with canonical glyph/HP/speed (+ranged
  for sludge dweller and floating brain); spawn entries present.
- Fill TOML (green); full gate; push, PR, CodeRabbit loop → merge.
- Commit: `feat(content): roster batch 6 (electric snake, sludge dweller, severed hand, floating brain, flame spirit)`

## Done when
The Dungeon hisses with slow electric snakes, sludge dwellers lob mud across
half the game, severed hands scuttle the Catacombs, a floating brain hurls
bolts in the Frozen Vault, and a flame spirit guards the Dragons Lair. Suite
green.
