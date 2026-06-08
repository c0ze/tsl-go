# Gear Breadth 14a Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a real weapon + body-armor table so the dagger isn't the only option (and the bosses become beatable). First slice of #14 — **pure data**; the engine already equips weapons/armor and uses their stats in combat.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). Commands run from the `go/` directory at the repo root. TDD: golden test first, then data.

## Scope
- Melee weapons (kind `weapon`, `attack` bonus + `damage` dice): machete (1d6), quarterstaff (1d6), cleaver (1d8), crystal sword (2d4), doomblade (2d6). The dagger (1d4) stays the weak baseline.
- Body armors (kind `armor`, `dodge` bonus): filthy rags (1), fish-scale (3), chainmail (4), rune armor (5). Leather (2) stays.
- These spawn as floor loot and equip through the existing inventory flow (`g` to pick up, `i` → select → equip). No engine changes.
- **Out of scope (later 14 increments):** equip slots beyond body (feet/head/cloak), autoequip-on-pickup, ranged weapons + ammo.

## Task 1: golden test (failing first)
**File:** `data/data_test.go` — add `TestEmbeddedGear`: assert e.g. `crystal_sword` (kind weapon, attack 4, damage `2d4`) and `chainmail` (kind armor, dodge 4) load from the shipped content. Run: expect FAIL.

## Task 2: data
**File:** `data/items.toml` — add the weapons + armors above. Run `go test ./data/ ./internal/content/ ./cmd/...` — expect PASS (content validation: weapon damage specs are well-formed).

## Task 3: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Commit plan + data. Push `gear-breadth`; PR notes this is the data slice of #14 (slots/autoequip/ranged follow). CodeRabbit loop → merge → pull master.

## Done when
The dungeon scatters a variety of weapons/armor; equipping a crystal sword + chainmail makes the mummylich winnable. Suite green.
