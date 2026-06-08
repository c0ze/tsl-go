# Dungeon Full Map 2b Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Complete the faithful 10-level branching map on the 2a engine. Pure data (`levels.toml`) plus a golden-test update — no engine changes.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). Commands run from the `go/` directory at the repo root.

## The map (faithful to `common/places.c`)

| level (id) | links to | win |
|---|---|---|
| the Dungeon (`dungeon`, start) | catacombs, ominous_cave | |
| the Catacombs (`catacombs`) | dungeon, laboratory | |
| the Ominous Cave (`ominous_cave`) | dungeon, comm_hub, underpass | |
| the Laboratory (`laboratory`) | catacombs, drowned_city | |
| the Communications Hub (`comm_hub`) | ominous_cave, frozen_vault | |
| the Underpass (`underpass`) | ominous_cave, drowned_city | |
| the Drowned City (`drowned_city`) | laboratory, dragons_lair, underpass | |
| the Vault of the Frozen Saint (`frozen_vault`) | comm_hub, dragons_lair | |
| the Dragons Lair (`dragons_lair`) | chapel, drowned_city, frozen_vault | |
| the Chapel of Fallen Stars (`chapel`) | dragons_lair | **win** |

All links are reciprocal, so the graph is fully connected and bidirectional.

## Notes / divergences
- **Win moves to the Chapel** (was temporarily the Ominous Cave in 2a). The Chapel keeps the temporary `win = true` until 2c replaces it with the real ascension altar + the mummylich/dragon.
- **Sizes stay ~60x24** for every level (fits an 80-col terminal; the engine renders the whole level with no camera yet). The faithful wide Chapel (160 cols) waits for a viewport.
- **Spawn tables** draw from the current 8-monster roster (ratman, ghoul, graveling, gnoblin, crypt_vermin, merman, scarecrow, slime), themed and tougher the deeper you go (e.g. the Drowned City favors mermen; the Dragons Lair and Chapel are scarecrow/slime/ghoul-heavy). Special-AI monsters + the Dragon/mummylich bosses come with later increments.

## Task 1: golden test (failing first)
**File:** `data/data_test.go`
- Update `TestEmbeddedDungeon`: assert all 10 level ids present, exactly one `Start` (dungeon), exactly one `Win` (chapel).
- Run: expect FAIL (only 3 levels exist).

## Task 2: rewrite `levels.toml`
**File:** `data/levels.toml`
- Define all 10 levels per the table, each with a themed weighted spawn table and a `monsters` count (~8 early, ~12–14 deep). Exactly one `start` (dungeon), one `win` (chapel).
- Run: `GOTOOLCHAIN=local go test ./data/ ./internal/content/ ./cmd/...` — expect PASS (validation: links/spawns resolve, one start).

## Task 3: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Commit plan + data. Push `dungeon-full-map`; PR notes 2b completes the map; 2c (altar + bosses) next. CodeRabbit loop → merge → pull master.

## Done when
All 10 named levels are reachable via the graph, the Chapel is the win, the shipped data validates, and `TestNewGameFromShippedData` still boots. Suite green.
