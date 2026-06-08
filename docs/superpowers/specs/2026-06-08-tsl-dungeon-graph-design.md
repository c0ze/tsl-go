# TSL Dungeon Graph — Design (#16)

**Status:** approved (in-chat, 2026-06-08). Replaces the linear `Depth`/`Descend` model with a faithful, data-driven branching graph of named levels, ending at the Chapel ascension altar — so the dungeon "plays like 0.40".

## Decisions (approved)
1. **Persisted levels.** A level is generated on first visit and cached; revisiting (the graph is bidirectional) shows the *same* level — dead monsters stay dead, dropped items stay. Sets up save/resume later.
2. **Portal stairs, one per link.** A stair tile carries its *target level id*. A level has 2–3 stairs in different rooms; you explore to find the exit you want. Arrival names the destination ("You enter the Catacombs.").
3. **Temporary win through 2a/2b.** A level def may set `win = true`; arriving there wins (a placeholder). The real **ascension altar** + bosses land in 2c (with #17).

## Non-goals (this design)
The full bestiary (#13), item/equipment tables (#14/#15), and boss AI (#17). The dungeon *places* monsters via per-level spawn tables, but their definitions/behaviors come from those epics.

## Data model — `levels.toml`
```toml
[level.dungeon]
name = "the Dungeon"
width = 60
height = 24
start = true                  # exactly one level is the entry point
links = ["catacombs", "ominous_cave"]
monsters = 8                  # how many to scatter
[[level.dungeon.spawn]]       # weighted spawn table
monster = "ratman"
weight = 3
[[level.dungeon.spawn]]
monster = "graveling"
weight = 1
```
- `content.LevelDef{ID, Name, W, H, Start bool, Links []string, Monsters int, Spawn []SpawnEntry{Monster string, Weight int}, Win bool}`.
- `content.Content.Levels map[string]*LevelDef`.
- **Validation (fail-fast):** exactly one `start`; every `links` target is a defined level; every `spawn.monster` is a defined monster; `weight >= 1`; `width/height` large enough for rooms.

## Engine
- `Portal{Pos, Target string}` — a stair tile position and the level id it leads to.
- `Level` gains: `ID string`, `Start Pos` (first-arrival spawn), `Return Pos` (saved leave position), `entered bool`, `Portals []Portal`.
- `Dungeon{defs map[string]*LevelDef, cache map[string]*Level, current string, build func(*LevelDef)(*Level,error)}` — the graph + generated-level cache; `build` is injected by `cmd` (the `gen` package) so `game` never imports `gen`.
  - `enter(id)`: generate-and-cache on first visit, set `current`.
  - First arrival → player at `Start` (set `entered`); re-arrival → player at `Return`.
- `Game` holds `Dungeon *Dungeon`; `Game.Level` is the current level (`dungeon.cache[current]`), so FOV/combat/UI are unchanged.
- `Game.Travel()` (replaces `Descend`): if the player stands on a `Portal`, save `Return`, `enter(target)`, swap `Game.Level`, place the player, and win if the target def has `Win`.
- **Generation:** `gen.LevelFromDef(r, c, def)` carves rooms (reusing the existing carve helpers), places one portal per link in distinct rooms, and scatters `def.Monsters` monsters drawn from the weighted spawn table. Returns a `*game.Level` with `ID/Start/Portals` set.

## Removed / reworked (migration)
- Out: `Game.Depth`, `MaxDepth`, `Descend`, `LevelGen`/`NewLevelFn`, the depth-based win, `gen.Rooms(...,depth)` and its global-pool + `min_depth` spawning (superseded by per-level spawn tables; `min_depth` stays on `MonsterDef` but is unused by graph levels).
- HUD: show the **level name** ("the Catacombs") instead of "Depth N".

## The faithful map (for 2b, from `places.c`)
`Dungeon` →(Catacombs, Ominous Cave); Catacombs→Laboratory; Ominous Cave→(Comm Hub, Underpass); Laboratory→Drowned City; Comm Hub→Frozen Vault; Underpass→Drowned City; Drowned City→Dragons Lair; Frozen Vault→Dragons Lair; Dragons Lair→Chapel (🐉); **Chapel of Fallen Stars** (⭐ ascension = win). All links bidirectional.

## Increment plan
- **2a (this PR):** the engine + `levels.toml` + portal travel + persistence + per-level spawn tables, proven with **3 levels** (Dungeon[start], Catacombs, Ominous Cave). One flagged `win` so the game stays winnable. HUD shows level name.
- **2b:** all 10 levels + links + spawn tables + themes (mostly data).
- **2c:** the ascension altar win + the Dragon + the mummylich (with #17).
