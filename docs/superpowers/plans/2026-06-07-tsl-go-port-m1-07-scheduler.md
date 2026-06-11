# TSL Go Port — Plan 7: Speed / Energy Scheduler

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or executing-plans. Steps use checkbox (`- [ ]`).

**Goal:** Close the biggest gameplay-parity gap from review: faster creatures act more often. Port the C's energy model (`game.c`: each creature's `move_counter += speed`, acts when it crosses `TURN_TIME`) into the turn-ender.

**Architecture:** Minimal-invasive. The player still acts once per input (one turn = `TurnCost` of "time"). The existing `monstersAct()` — already called by every player action — is changed from "each monster acts once" to **energy-based**: each monster gains `speedOf(m)` energy and acts while it holds `≥ TurnCost`. A monster's speed comes from its def (`MonsterDef.Speed`, default 100). Default 100 == `TurnCost`, so it reproduces today's behavior exactly (existing tests untouched); a faster monster acts twice, a slower one every other turn. No `ui.Run` change, no method-signature changes.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). No new deps.

## Working dir & environment
- Module in `go/`; `export GOTOOLCHAIN=local`. Branch `plan7-scheduler`. No `go mod tidy`/`go get`.

---

## Task 1: Monster speed in content (`content`, `data`)

**Files:** Modify `internal/content/content.go`, `data/monsters.toml`

- [ ] **Step 1** — add a `Speed` field to `MonsterDef` in `content.go` (after `Damage`):
```go
	Speed  int    `toml:"speed"` // energy gained per turn; <= 0 defaults to 100
```

- [ ] **Step 2** — give the data monsters distinct speeds in `data/monsters.toml`: add `speed = 130` to the `[monster.rat]` table and `speed = 80` to the `[monster.ghoul]` table (rats are quick, ghouls shamble).

- [ ] **Step 3** — from `go/`: `export GOTOOLCHAIN=local && go build ./... && go test ./internal/content/` → PASS (the field is plain data; existing tests still pass). **Commit:**
```bash
git add internal/content/content.go data/monsters.toml
git commit -m "feat(content): MonsterDef speed (rat fast, ghoul slow)"
```

---

## Task 2: Energy-based turn scheduler (`game`)

**Files:** Modify `internal/game/creature.go`, `internal/game/combat.go`, `internal/game/combat_test.go`

- [ ] **Step 1** — add an `Energy` field to the `Creature` struct in `creature.go`:
```go
// Creature is a monster on the level (the player is modelled separately on Game
// for now). HP is current hit points; Def carries its stats and glyph.
type Creature struct {
	Def     *content.MonsterDef
	Pos     Pos
	HP      int
	Faction Faction
	Energy  int
}
```

- [ ] **Step 2** — append a test to `internal/game/combat_test.go`:
```go
func TestFasterMonsterActsMultipleTimes(t *testing.T) {
	g := combatGame() // 10x3 floor, player at (1,1)
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 1, Dodge: 1, Damage: "1d1", Speed: 250}, Pos: Pos{8, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	before := rat.Pos.X
	g.monstersAct() // speed 250 → 250 energy → acts twice (250-100-100=50)
	if before-rat.Pos.X != 2 {
		t.Errorf("speed-250 monster stepped %d tiles in one turn, want 2", before-rat.Pos.X)
	}
}

func TestDefaultSpeedActsOnce(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 9, Attack: 1, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 9} // Speed 0 → default 100
	g.Level.Creatures = append(g.Level.Creatures, rat)
	before := rat.Pos.X
	g.monstersAct()
	if before-rat.Pos.X != 1 {
		t.Errorf("default-speed monster stepped %d tiles, want 1", before-rat.Pos.X)
	}
}
```

- [ ] **Step 3** — in `combat.go`, add the constants and `speedOf` helper (near the existing `senseRange` const), and rewrite `monstersAct` to be energy-based, extracting the single-action logic into `monsterAct`.

Add near the top (with `senseRange`):
```go
const (
	turnCost     = 100 // energy needed for one action
	defaultSpeed = 100 // energy per turn for a monster with no speed set
)

func speedOf(m *Creature) int {
	if m.Def.Speed > 0 {
		return m.Def.Speed
	}
	return defaultSpeed
}
```

Replace the existing `monstersAct` function with:
```go
// monstersAct advances the world after the player's turn. Each living monster
// gains energy equal to its speed and takes an action for every full turn's
// worth it holds, so faster monsters act more often (the C's move_counter /
// TURN_TIME model). A monster's leftover energy carries to the next turn.
func (g *Game) monstersAct() {
	snapshot := make([]*Creature, len(g.Level.Creatures))
	copy(snapshot, g.Level.Creatures)
	for _, m := range snapshot {
		if g.Dead {
			return
		}
		if g.Level.CreatureAt(m.Pos) != m {
			continue // already removed this turn
		}
		m.Energy += speedOf(m)
		for m.Energy >= turnCost {
			m.Energy -= turnCost
			g.monsterAct(m)
			if g.Dead {
				return
			}
			if g.Level.CreatureAt(m.Pos) != m {
				break
			}
		}
	}
}

// monsterAct is a single monster action: attack the player if adjacent, else
// step toward the player if within sense range.
func (g *Game) monsterAct(m *Creature) {
	if chebyshev(m.Pos, g.Player) == 1 {
		g.monsterAttacks(m)
		return
	}
	if chebyshev(m.Pos, g.Player) <= senseRange {
		g.stepToward(m, g.Player)
	}
}
```

- [ ] **Step 4** — from `go/`: `export GOTOOLCHAIN=local && gofmt -l . && go build ./... && go test ./... -count=1 && go vet ./...` → all clean (existing combat tests still pass; the two new tests pass). **Commit:**
```bash
git add internal/game/creature.go internal/game/combat.go internal/game/combat_test.go
git commit -m "feat(game): energy/speed scheduler — faster monsters act more often"
```

---

## Done criteria
- Monsters carry a per-def `Speed`; the rat (130) acts slightly more often than the player, the ghoul (80) slightly less.
- `monstersAct` is energy-based: a monster acts `floor((carried + speed) / TurnCost)` times per player turn, leftover energy carried over.
- Default speed (100) exactly reproduces the old one-action-per-turn behavior (all prior tests pass unchanged).
- `go test ./...`, `go vet`, gofmt all clean.

## Deferred
- Variable player speed (the player is fixed at one turn per input); per-action time costs; haste/slow effects on `attr_speed`. These extend the same `Energy` model.
