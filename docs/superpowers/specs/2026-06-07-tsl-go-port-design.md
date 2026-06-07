# The Slimy Lichmummy — Go Port Design

- **Date:** 2026-06-07
- **Status:** Approved (design); pending implementation plan
- **Author:** Arda Karaduman (with Claude)
- **Branch:** `go-port`

## 1. Background

The Slimy Lichmummy (TSL) is a turn-based, ASCII roguelike by Ulf Åström
(happyponyland.net), last released as **v0.40 (2012-09-26)**. This repo
(`c0ze/tsl`) is a personal fork of the original C source: ~50k lines of C, ~43k
of it game logic in `common/`, behind a narrow UI boundary (`common/ui.h`) with
two front-ends — curses (`console/`, ~480 LOC) and Allegro (`gui/`, ~1.8k LOC).

Upstream is dormant: 0.40 is the last release and development stalled ~2017.
This fork is already at 0.40; there are no upstream updates to pull.

**Goal:** rewrite TSL in **Go** as a **faithful port** — the end result looks
and plays like 0.40 — while writing **idiomatic, readable Go**. We aim for
behavior parity, *not* line-by-line translation. Many patterns in the original C
are deliberately dropped (see §3).

### Licensing note
TSL is explicitly **not open source** (`LICENSE.TXT`): source is provided to
compile and verify; unmodified copies may be redistributed; the author "doesn't
mind unofficial ports but prefers you contact him first" and forbids selling.
A personal rewrite is fine. **If this port is ever published/distributed, email
the author first** (ulf.astrom@gmail.com).

## 2. Decisions (locked)

| Decision | Choice |
|---|---|
| Language | Go |
| Fidelity | Faithful behavior parity with 0.40; idiomatic Go, not transliteration |
| Architecture | **Approach A** — layered idiomatic engine + data-driven content + behavior registry |
| Content | **Fully data-driven** — all *data* in files; inherently-code parts (spell/ability effects, level-gen algorithms) stay in Go behind a named registry the data references |
| Data format | **TOML** |
| Front-end | **Console first** (tcell); graphical tileset front-end (ebiten) is a later phase |
| First milestone (M1) | **One winnable vertical slice** (console-only) |
| Repo layout | New `go/` subdirectory in this repo; C kept alongside as the reference spec |

## 3. Architecture & package layout

The Go module lives in `go/` (module path `github.com/c0ze/tsl`). The C source is
untouched and serves as the behavior reference.

```text
go/
  cmd/tsl/            main: load config + content, wire registry, run loop
  internal/
    game/             World/Game state, Level, Tile, turn loop   ← core
    creature/         creature model + attribute set
    item/             item model
    fov/              field of view
    ai/               monster AI
    combat/           combat resolution
    gen/              dungeon/area generation algorithms
    content/          data-schema structs + TOML loader (reads data/)
    behaviors/        named spell/ability/effect funcs + named gen funcs (registry)
    rng/              Mersenne Twister port
    save/             versioned JSON snapshot
    ui/               UI interface + shared view logic (status, log, viewport)
      tcell/          terminal implementation (M1)
      # ebiten/       graphical implementation (later phase)
  data/               TOML content files (monsters, items, attributes, areas, tiles)
  assets/             (later, GUI) tileset/font derived from original
```

**Dependency rule (the one hard boundary):** engine packages do **zero I/O** and
**never import `ui`**, so they are fully testable in isolation. The UI **never
mutates game state** — it renders a read-only view and submits player *actions*.
One-way flow: `cmd → ui → game ← behaviors`, with `content`/`rng` as leaves.
Behaviors act *on* the world (they are game logic); the `Behavior`/`Generator`
types are declared in the engine core and the engine holds the registry as
**injected data** (wired by `cmd` at startup), so there is no `game`↔`behaviors`
import cycle. Data references behaviors by name.

### C patterns deliberately dropped

| Original C pattern | Go replacement |
|---|---|
| Global mutable state in headers (`view_top`, `terminal[]`, `map_test`, `mem_alloc`) | State owned by an explicit `Game`/`World` struct; no package-level mutable globals |
| Manual alloc tracking (`mem_alloc`, `check_unfree_memory`) | Deleted — GC |
| `blean_t` tri-state bool | Plain `bool`; explicit typed enum where "unknown" is real |
| Function pointers in data tables (`attr_info[].invoke`) | Named behavior registry (`map[string]Behavior`) + small interfaces |
| Raw struct-dump savefiles (32/64-bit fragile) | Versioned JSON snapshot |
| One flat 200-wide `attr_index_t` enum mixing stats/resistances/abilities/spells/item-props | Keep the generic-attribute *concept* (a real feature: effects modify attributes uniformly) but as a typed attribute set keyed by string IDs from data |

## 4. The UI boundary

The clean version of `ui.h`. Instead of ~40 low-level cursor primitives, the
engine emits a read-only snapshot and the UI draws it. **Two interfaces, defined
in the engine, implemented by the UI** (dependency inversion — `game` imports
neither `ui` nor tcell):

```go
// What the UI draws each frame — a pure value snapshot.
type View struct {
    Map      [][]Cell    // visible/remembered tiles: glyph + color + state
    Player   StatusView  // HP, EP, attributes, position, conditions
    Messages []Message   // recent log lines
    Viewport Rect        // scroll window over the level
}

// What the engine calls when it needs input mid-turn.
type Prompter interface {
    NextAction() (Action, error)            // main loop: "what do you do?"
    Target(kind TargetKind) (Pos, bool)     // pick a tile/direction
    Menu(m MenuSpec) (choice int, ok bool)  // inventory, abilities, options
    Confirm(prompt string) bool             // y/n
    More()                                  // [MORE] paging
    ReadText(prompt string) string          // name, labels
}
```

Game loop: engine advances until it needs the player → `Prompter.NextAction()` →
apply `Action` → re-render `View`. Mid-action sub-decisions (targeting a spell,
choosing from inventory) are `Prompter` callbacks.

**Good parts of the C kept:**
- **Glyph + color is data**, carried on each monster/item/tile def. The UI reads
  `Cell.Glyph`/`Cell.Color`. This is what makes a later ebiten tileset front-end a
  drop-in: rune → tile index, same `View`.
- **Abstract actions, remappable keys** (vi-keys default + Dvorak + full rebind,
  per the original `bind`/`bindn`/`dvorak` config). Mapping lives in `ui`; the
  engine only sees `Action`.
- **Screen regions** (map viewport, status panel, 4-line message log) are a UI
  concern laid into tcell's cell grid. The GUI's old 80×24 emulation is gone;
  each front-end lays out regions natively.

## 5. Data schema, loader, behavior registry

**Content types (data).** C enums + parallel info-tables become TOML keyed by id:

```toml
[monster.ghoul]
name = "ghoul"
glyph = "g"
color = "brown"
ai = "melee"                 # → registered AI behavior
gore = "claw"
attrs = { health = 8, speed = 100, attack = 2, vision = 6 }
abilities = ["leap"]         # → registered behavior names

[item.wand_of_fireball]
name = "wand of fireball"
glyph = "/"
color = "red"
kind = "wand"
charges = 5
invoke = "fireball"          # ← data↔code bridge
```

Same shape for `attribute` defs (mirroring `attr_info_t`: min/max/default/
percent/morgue), `tile` defs (glyph, color, passable, transparent-for-FOV,
swimmable), and `area` defs (generator name + parameters: depth range, size,
monster/item spawn tables referencing ids with weights, glyph theme).

**Loader.** At startup, read all of `data/` into a `Content` struct of maps keyed
by id, then **validate and fail fast**: every behavior/ai/gen/spawn/attr
reference must resolve, with a clear message ("ghoul references unknown ability
'leep' in monsters.toml"). After load, string ids resolve once into
pointers/handles so hot loops never hash strings.

**Behavior registry — the data↔code bridge.** The `Behavior` function type is
declared in the **engine core** (cleaned-up version of C's
`invoke(caster, source, param)`), so the engine can hold and call a registry
without importing the implementations:

```go
// declared in the engine core (game package)
type Behavior func(g *game.Context, caster *creature.Creature, src *item.Item, target game.Pos) (used bool)
```

The `behaviors` and `gen` packages *implement* effects/generators (importing
`game`) and expose constructors returning the populated `map[string]Behavior` /
`map[string]Generator` (keys like `"fireball"`, `"teleport"`, `"force_bolt"`,
`"gen_cavern"`). `cmd` builds these registries at startup and **injects** them
into the `Game`. This keeps the dependency strictly one-way (`behaviors → game`,
no cycle); data only ever names behaviors, and the *effects* stay testable Go
code.

**Code vs data enums.** Mechanical vocabulary stays Go constants (directions
`dir_t`, damage types, gore types). Only *content* enums (monsters, items,
attributes, areas, tiles) move to data.

## 6. Engine: world model, turn loop, systems

**State model — one owner, no globals.** A top-level `Game` struct owns the
current `Level`, the player, the `rng.MT`, the message log, turn counter,
options/config, and the loaded `Content`.
- `Level` = slice-backed 2D grid of `Tile` (width×height in one `[]Tile`) + the
  level's creatures/items/traps, stairs/portals, and per-tile visibility/memory
  state for rendering.
- `Creature` = position, the **attribute set** (TSL's generic attribute system
  kept intact, as a clean typed set), inventory, equipment, AI state, faction
  (player/ally/enemy).
- `Item` = content def handle + stack count, charges, identification state,
  enchantment, equipped slot.

**Turn loop — energy/speed scheduler.** TSL is speed-based (`attr_speed`), not
round-robin. Actors accrue energy by speed; whoever is ready acts — player via
`Prompter.NextAction()`, monsters via `ai`. **Port TSL's actual speed math from
the C** so monster cadence matches 0.40.

**Actions & systems.** A decision becomes an `Action` (move, melee, pick-up,
use/invoke, cast, descend, wait…); applying it mutates the world and emits
messages. Movement into an enemy routes to combat. Split by parity-criticality:

| Translate *closely* from C (parity-critical math) | Write fresh idiomatic Go (structure only) |
|---|---|
| `fov`; `combat` damage math incl. damage-sequence weapons + Fudge-like rolls; speed/energy timing; `gen` core algorithms | game loop, action dispatch, inventory/equip, message log, AI state-machine scaffolding, world/level plumbing |

**RNG / save / errors:**
- **RNG:** port Mersenne Twister (`mt19937ar.c`) into `rng`; `Game` owns exactly
  one seedable generator. No global `math/rand`.
- **Save:** versioned JSON snapshot of `Game` (world + RNG state + turn). Keep
  TSL's **suspend/resume** semantics (save on quit, load-and-delete on resume) —
  faithful and permadeath-correct.
- **Errors, three kinds:** data/IO → returned `error`, fail fast at startup;
  in-game "can't do that" → log feedback, *not* errors; broken invariants →
  `panic`, recovered at top level to restore the terminal and print a bug report
  (clean version of the C `bug_string`).

## 7. Testing strategy

The architecture is built for deterministic testing (I/O-free engine, UI behind
interfaces, seedable RNG):

1. **Unit tests** (table-driven) on engine logic: attribute/effect application,
   inventory/equip, action dispatch, scheduler.
2. **Differential tests vs. the C** for parity-critical cores:
   - **RNG** — reproduce the C's exact MT sequence for a seed (mt19937ar ships
     known test vectors).
   - **FOV & combat** — capture C outputs for fixed inputs as golden vectors;
     assert Go matches.
   - **Dungeon gen** — assert structural invariants (connected, reachable stairs,
     in bounds). Exact same-seed-same-level parity is a stretch goal.
3. **Content validation test** — load real `data/` in CI; every reference must
   resolve.
4. **UI snapshot tests** — render a known `View` via tcell's in-memory
   `SimulationScreen`; snapshot the text grid.
5. **Scripted headless playthroughs** — feed a canned `Prompter` into a seeded
   game; assert end state. The C stays buildable for side-by-side human checks.

Implementation is expected to be TDD-driven.

## 8. M1 — winnable vertical slice (console only)

**In scope:**
- `cmd/tsl` boots: loads config + TOML content (fail-fast validation), opens the
  tcell UI.
- One generator → a small dungeon (default ~3 levels) with stairs; reaching the
  exit ⇒ **win screen**. Death ⇒ game over + a basic morgue dump.
- Player rendering, movement (vi-keys + arrows), FOV, bump-to-attack.
- ~3 monsters (e.g. ghoul, slime, ratman) with a sleep/hunt melee AI state
  machine and speed-based turns.
- Melee combat (damage-sequence model + Fudge-like rolls), HP, death, scrolling
  message log.
- A handful of items — a weapon, armor, a healing item, one wand — with pickup,
  inventory menu, equip, and use/invoke via the behavior registry.
- Basic suspend/resume save (validates the JSON snapshot early); seedable RNG.

**Out of scope (later milestones):**
- The ebiten graphical/tileset front-end.
- The rest of 0.40's content and depth: full monster/item/area roster, uniques,
  the full spell list, shapeshifting, crafting, augmentation/facets, traps
  variety, wounds/poison depth, swimming/lava, bestiary — ported incrementally
  once M1 proves the architecture end-to-end.

## 9. C reference map (source of truth per system)

For the implementer, the C files to consult for behavior parity:

| Go target | C reference |
|---|---|
| `rng` | `common/mt19937ar.c` |
| `fov` | `common/fov.c` |
| `combat` | `common/combat.c`, `common/rolls.c`, `common/wounds.c` |
| `gen` | `common/modbuild.c`, `common/level.c`, `common/area.c` |
| `ai` | `common/ai.c` |
| `creature` / attributes | `common/creature.c`, `common/attrs.c`, `common/ability.c`, `main.h` (`attr_index_t`, `attr_info_t`) |
| `item` | `common/item.c`, `common/itemprop.c`, `common/equip.c`, `common/inventory.c`, `common/treasure.c` |
| `behaviors` (spells/abilities) | `common/magic.c`, `common/effect.c`, `common/balls.c`, `common/missile.c`, `common/teleport.c` |
| `game` loop / actions | `common/game.c`, `common/actions.c` |
| `ui` (View/Prompter) | `common/ui.h`, `common/ui.c`, `console/console.c`, `common/menuitem.c`, `common/select.c` |
| `save` | `common/saveload.c` (semantics only; format is redesigned) |
| config / keymap | `common/options.c`, `common/keymap.c`, `common/input.c`, `tsl_conf_example` |
```
