# Dungeon Graph 2a Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Stand up the data-driven dungeon graph engine — `levels.toml`, a `Dungeon` (graph + generated-level cache), portal-stair travel with persistence, and per-level spawn tables — proven with 3 real levels. See `docs/superpowers/specs/2026-06-08-tsl-dungeon-graph-design.md`.

**Architecture:** `content.LevelDef` (loaded from `levels.toml`) describes each level; `gen.LevelFromDef` builds one; `game.Dungeon` holds the graph + cache + current id and is injected with the `gen` builder by `cmd`; `Game.Travel` moves between levels via `Portal` tiles, persisting visited levels. Replaces `Depth`/`Descend`/`NewLevelFn`/`gen.Rooms(...,depth)`.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). Commands run from the `go/` directory at the repo root, prefixed `GOTOOLCHAIN=local`; prepend `env -u GOROOT` on a GOROOT error. TDD, commit per task, suite green between commits.

---

## Task 1: `content.LevelDef` + `levels.toml` (content)
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- Types (in `content.go`):

  ```go
  type SpawnEntry struct {
      Monster string `toml:"monster"`
      Weight  int    `toml:"weight"`
  }
  type LevelDef struct {
      ID       string       `toml:"-"`
      Name     string       `toml:"name"`
      W        int          `toml:"width"`
      H        int          `toml:"height"`
      Start    bool         `toml:"start"`
      Links    []string     `toml:"links"`
      Monsters int          `toml:"monsters"`
      Spawn    []SpawnEntry `toml:"spawn"`
      Win      bool         `toml:"win"`
  }
  ```

  Plus `Content.Levels map[string]*LevelDef` and a `levelsFile` wrapper (`Level map[string]*LevelDef`, tagged `level`).
- `Load`: decode optional `levels.toml`; set IDs; then `validateLevels(c)`: exactly one `Start`; each `Links` target ∈ Levels; each `Spawn.Monster` ∈ Monsters; `Weight >= 1`; `W >= 12 && H >= 8`; `Monsters >= 0`.
- **Tests:** loads a 2-level graph; rejects (a) no start, (b) two starts, (c) link to unknown level, (d) spawn referencing unknown monster, (e) weight 0.
- **Red→green→commit:** `feat(content): LevelDef + levels.toml (graph, spawn tables)`

## Task 2: engine types — `Dungeon`, `Level` fields, `Travel` (game)
**Files:** `internal/game/dungeon.go` (new), `internal/game/game.go`, `internal/game/dungeon_test.go` (new)
- `Portal{Pos Pos; Target string}`. Add to `Level`: `ID string`, `Start Pos`, `Return Pos`, `entered bool`, `Portals []Portal`. Add `PortalAt(p Pos) *Portal`.
- `Dungeon{defs map[string]*content.LevelDef; cache map[string]*Level; current string; build func(*content.LevelDef) (*Level, error)}` with `NewDungeon(defs, startID, build)`, `enter(id)` (generate+cache on first visit; set current), `Current() *Level`.
- `Game` gains `Dungeon *Dungeon` (keep `Level *Level` = current). `Game.Travel()`: `p := g.Level.PortalAt(g.Player)`; nil → log + return; else `g.Level.Return = g.Player`, `enter(target)`, `g.Level = g.Dungeon.Current()`, place player (`Start` first time + set `entered`; else `Return`), `g.log("You enter %s.", def.Name)`, and if target def `Win` → `g.Won = true`.
- **Tests (fake `build`):** travel A→B places player at B.Start; kill a creature on B, travel back to A then again to B → creature still gone (persistence) and player at B.Return; travelling onto a `Win` level sets `Won`.
- **Commit:** `feat(game): Dungeon graph + portal Travel with persistence`

## Task 3: `gen.LevelFromDef` (gen)
**Files:** `internal/gen/gen.go`, `internal/gen/gen_test.go`
- `func LevelFromDef(r *rng.MT, c *content.Content, def *content.LevelDef) (*game.Level, error)`: carve rooms (reuse `carveRoom`/`carveCorridor`) sized `def.W×def.H`; set `Start` = first room center; for each `def.Links[i]` place a `Portal` (a `stairs_down` tile + `Portals` entry) in a distinct later room; scatter `def.Monsters` monsters drawn from the weighted `def.Spawn` table; place items as today.
- Add a weighted pick helper over `def.Spawn`.
- **Tests:** a level built from a 2-link def has 2 portals with the right targets, all reachable; monsters come only from the spawn table; deterministic for a seed.
- **Commit:** `feat(gen): LevelFromDef — build a level from a LevelDef + spawn table`

## Task 4: wire the graph; retire the linear model (game/ui/cmd)
**Files:** `internal/game/descend.go` (→ delete linear bits / repurpose), `internal/game/game.go`, `internal/ui/ui.go`, `internal/ui/ui.go` HUD, `internal/ui/tcell/screen.go`, `cmd/tsl/main.go`, and touched tests.
- Replace `ActDescend`→`ActTravel` (keep `>` key); `Run` calls `g.Travel()`.
- Remove `Depth`, `MaxDepth`, `Descend`, `LevelGen`/`NewLevelFn`; update `MorgueText` (current level name instead of depth); HUD `statusLine` shows `g.Level.Name()`/current level def name instead of `Depth`.
- `cmd/main.go`: build `content.Levels` → `game.NewDungeon(defs, start, func(def){ return gen.LevelFromDef(r, c, def) })`; set `g.Dungeon`, `g.Level = dungeon.Current()`, `g.Player = g.Level.Start`.
- Fix/replace broken tests: `descend_test.go` (→ travel/win semantics), `ui_test.go` win test (→ travel onto win level), any `Depth` references.
- **Commit:** `refactor: route the game through the Dungeon graph; retire linear depth`

## Task 5: `levels.toml` data + golden test (data)
**Files:** `data/levels.toml` (new — embedded by `data/data.go`'s `*.toml`), `data/data_test.go`
- 3 levels: `dungeon` (start; links catacombs, ominous_cave), `catacombs` (links dungeon, ominous_cave; `win = true` as the temporary goal), `ominous_cave` (links dungeon, catacombs). Spawn tables from the current roster (ratman/graveling/gnoblin/crypt_vermin/merman; tougher ones deeper).
- Golden test: `content.Load(data.Files)` yields the 3 levels, exactly one start, all links/ spawns valid.
- **Commit:** `feat(data): 3-level starter dungeon graph (Dungeon, Catacombs, Ominous Cave)`

## Task 6: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Commit spec + plan. Push `dungeon-graph`; PR documents 2a scope + that 2b (full map) and 2c (altar/bosses) follow. CodeRabbit loop → merge `--delete-branch` → pull master.

## Done when
You start in the Dungeon, find stairs to the Catacombs / Ominous Cave, travel between them with state persisted, reaching the flagged level wins; HUD shows the level name; suite green.
