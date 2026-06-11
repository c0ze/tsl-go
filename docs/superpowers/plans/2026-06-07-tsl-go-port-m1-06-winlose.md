# TSL Go Port — M1 Plan 6: Descent, Win, Lose, Morgue

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or executing-plans. Steps use checkbox (`- [ ]`).

**Goal:** Make the slice *winnable and losable*. The dungeon has multiple levels; descending the `>` stairs goes deeper, and descending from the deepest level wins. Death ends the game and writes a morgue file.

**Architecture:** `game` gains depth tracking, a `Descend()` method, win state, a death cause, an injected `LevelGen` (so it can build the next level without importing `gen` — `cmd` injects it), and a pure `MorgueText()`. `ui` adds a descend action and ends the loop on win/death. `cmd` wires the level generator and the start depth, and (the only I/O) writes `morgue.txt` and prints the outcome after the screen closes — keeping the engine I/O-free.

**Dependency direction:** unchanged — `cmd → gen`/`behaviors → game`; `game` imports no UI and no `gen` (the `LevelGen` func is injected).

**Scope note:** Save/resume (JSON) and configurable keymaps are deferred to an optional follow-up; this plan delivers the winnable/losable core that completes M1.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). No new deps.

## Working dir & environment
- Module in `go/`; `export GOTOOLCHAIN=local`. Branch `plan6-winlose`. No `go mod tidy`/`go get`.

---

## Task 1: Depth, descent, win, morgue (`game`)

**Files:** Modify `internal/game/game.go`, `internal/game/combat.go`; create `internal/game/descend.go`, `internal/game/descend_test.go`

- [ ] **Step 1** — add fields to the `Game` struct in `game.go` (after the item fields `Inventory/Weapon/Armor/Behaviors`):
```go
	Depth      int
	Won        bool
	DeathCause string
	NewLevelFn LevelGen
```

- [ ] **Step 2** — in `combat.go`, set the death cause when the player is killed. In `monsterAttacks`, change the death block to:
```go
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = m.Def.Name
		g.log("You die.")
	}
```

- [ ] **Step 3** — `internal/game/descend_test.go`:
```go
package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func descendGame() *Game {
	c := testContent()
	return &Game{
		Content: c, Level: NewLevel(8, 3, c.Tiles["floor"]),
		Player: Pos{1, 1}, PlayerHP: 10, PlayerMax: 10, Depth: 1,
	}
}

func putStairsUnderPlayer(g *Game) {
	g.Level.Set(g.Player, &content.TileDef{ID: "stairs_down", Glyph: ">", Passable: true, Transparent: true})
}

func TestDescendWinsAtMaxDepth(t *testing.T) {
	g := descendGame()
	g.Depth = MaxDepth
	putStairsUnderPlayer(g)
	g.Descend()
	if !g.Won {
		t.Error("descending at max depth should win")
	}
}

func TestDescendGeneratesNextLevel(t *testing.T) {
	g := descendGame()
	putStairsUnderPlayer(g)
	called := false
	g.NewLevelFn = func(depth int) (*Level, Pos, error) {
		called = true
		return NewLevel(5, 5, g.Content.Tiles["floor"]), Pos{2, 2}, nil
	}
	g.Descend()
	if !called {
		t.Fatal("NewLevelFn should be called to build the next level")
	}
	if g.Depth != 2 {
		t.Errorf("depth = %d, want 2", g.Depth)
	}
	if g.Player != (Pos{2, 2}) {
		t.Errorf("player = %v, want the new level's start {2 2}", g.Player)
	}
}

func TestDescendNeedsStairs(t *testing.T) {
	g := descendGame() // standing on plain floor
	g.Descend()
	if g.Won || g.Depth != 1 {
		t.Error("descending off the stairs should do nothing")
	}
}

func TestMorgueTextOnDeath(t *testing.T) {
	g := descendGame()
	g.Dead = true
	g.DeathCause = "ghoul"
	g.Depth = 2
	txt := g.MorgueText()
	if !strings.Contains(txt, "Killed by: ghoul") || !strings.Contains(txt, "Depth reached: 2") {
		t.Errorf("morgue missing expected lines:\n%s", txt)
	}
}
```

- [ ] **Step 4** — `internal/game/descend.go`:
```go
package game

import (
	"fmt"
	"strings"
)

// LevelGen builds the level for a given depth. It is injected by cmd (which has
// access to the gen package), so game never imports gen.
type LevelGen func(depth int) (*Level, Pos, error)

// MaxDepth is how deep the dungeon goes; descending from the deepest level wins.
const MaxDepth = 3

// Descend takes the down-stairs under the player to the next level. Descending
// from the deepest level wins the game. It passes no monster turn (it changes
// levels), and does not itself recompute FOV — the game loop does that.
func (g *Game) Descend() {
	if g.Dead || g.Won {
		return
	}
	if g.Level.At(g.Player).Def.ID != "stairs_down" {
		g.log("There are no stairs to descend here.")
		return
	}
	if g.Depth >= MaxDepth {
		g.Won = true
		g.log("You descend the final stairs and escape the dungeon. You win!")
		return
	}
	if g.NewLevelFn == nil {
		g.log("The way down is sealed.")
		return
	}
	lvl, start, err := g.NewLevelFn(g.Depth + 1)
	if err != nil {
		g.log("The way down is blocked.")
		return
	}
	g.Depth++
	g.Level = lvl
	g.Player = start
	g.log("You descend to depth %d.", g.Depth)
}

// MorgueText returns a plain-text summary of the run. It performs no I/O.
func (g *Game) MorgueText() string {
	var b strings.Builder
	fmt.Fprintf(&b, "The Slimy Lichmummy — morgue\n\n")
	fmt.Fprintf(&b, "Depth reached: %d\n", g.Depth)
	fmt.Fprintf(&b, "HP: %d/%d\n", g.PlayerHP, g.PlayerMax)
	switch {
	case g.Dead:
		cause := g.DeathCause
		if cause == "" {
			cause = "unknown causes"
		}
		fmt.Fprintf(&b, "Killed by: %s\n", cause)
	case g.Won:
		fmt.Fprintf(&b, "Result: escaped victorious\n")
	}
	wield, worn := "nothing", "nothing"
	if g.Weapon != nil {
		wield = g.Weapon.Def.Name
	}
	if g.Armor != nil {
		worn = g.Armor.Def.Name
	}
	fmt.Fprintf(&b, "Wielding: %s\nWearing: %s\n", wield, worn)
	fmt.Fprintf(&b, "Inventory (%d):\n", len(g.Inventory))
	for _, it := range g.Inventory {
		fmt.Fprintf(&b, "  - %s\n", it.Def.Name)
	}
	return b.String()
}
```

- [ ] **Step 5** — `go test ./internal/game/` → PASS. **Commit:**
```bash
git add internal/game/game.go internal/game/combat.go internal/game/descend.go internal/game/descend_test.go
git commit -m "feat(game): depth, descent, win condition, death cause, morgue text"
```

---

## Task 2: Descend action + endgame (`ui`, `ui/tcell`)

**Files:** Modify `internal/ui/ui.go`, `internal/ui/ui_test.go`, `internal/ui/tcell/screen.go`

- [ ] **Step 1** — append a test to `internal/ui/ui_test.go`:
```go
func TestRunDescendToWin(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	g.Depth = game.MaxDepth
	g.Level.Set(g.Player, &content.TileDef{ID: "stairs_down", Glyph: ">", Passable: true, Transparent: true})
	p := &scriptPrompter{actions: []Action{{Kind: ActDescend}}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if !g.Won {
		t.Error("descending at max depth should win and end the loop")
	}
}
```

- [ ] **Step 2** — in `internal/ui/ui.go`:

(a) add `ActDescend` to the `ActionKind` constants (after `ActInventory`):
```go
	ActDescend
```

(b) end the loop on win as well as death — change the post-render check in `Run` from `if g.Dead {` to:
```go
		if g.Dead || g.Won {
			return nil
		}
```

(c) handle the action in `Run`'s switch (after `ActInventory`'s block):
```go
		case ActDescend:
			g.Descend()
```

- [ ] **Step 3** — in `internal/ui/tcell/screen.go`, map `>` to descend in `keyToAction` (with the other rune cases):
```go
	case '>':
		return ui.Action{Kind: ui.ActDescend}, true
```

- [ ] **Step 4** — `go test ./internal/ui/ ./internal/ui/tcell/` → PASS. **Commit:**
```bash
git add internal/ui/ui.go internal/ui/ui_test.go internal/ui/tcell/screen.go
git commit -m "feat(ui): descend action and win/lose end-of-game"
```

---

## Task 3: Wire descent + morgue + outcome (`cmd/tsl`)

**Files:** Modify `cmd/tsl/main.go`, `cmd/tsl/main_test.go`

- [ ] **Step 1** — append to `cmd/tsl/main_test.go`:
```go
func TestNewGameStartsAtDepthOne(t *testing.T) {
	g, err := newGame(testTiles(), 1)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if g.Depth != 1 {
		t.Errorf("depth = %d, want 1", g.Depth)
	}
	if g.NewLevelFn == nil {
		t.Error("NewLevelFn should be wired so the player can descend")
	}
}
```

- [ ] **Step 2** — rewrite `cmd/tsl/main.go` so `run` returns an outcome string, `main` prints it after the screen closes, and a morgue is written on death. Replace `main`, `run`, and `newGame` with:
```go
func main() {
	outcome, err := run()
	if err != nil {
		fmt.Fprintln(os.Stderr, "tsl:", err)
		os.Exit(1)
	}
	if outcome != "" {
		fmt.Println(outcome)
	}
}

func run() (string, error) {
	// content.Load reads ./data relative to the working directory, so tsl must
	// be run from the go/ module root for now.
	c, err := content.Load("data")
	if err != nil {
		return "", err
	}
	g, err := newGame(c, uint32(time.Now().UnixNano()))
	if err != nil {
		return "", err
	}
	screen, err := tcellui.New()
	if err != nil {
		return "", err
	}
	defer screen.Close()
	if err := ui.Run(g, screen, screen); err != nil {
		return "", err
	}
	switch {
	case g.Won:
		return "You escaped the dungeon victorious!", nil
	case g.Dead:
		_ = os.WriteFile("morgue.txt", []byte(g.MorgueText()), 0o644)
		return "You have died. A morgue was written to morgue.txt.", nil
	default:
		return "You leave the dungeon. Farewell.", nil
	}
}

// newGame builds a fresh, procedurally generated dungeon seeded by seed, and
// wires a level generator so the player can descend.
func newGame(c *content.Content, seed uint32) (*game.Game, error) {
	r := rng.NewWithSeed(seed)
	lvl, start, _, err := gen.Rooms(r, c, mapW, mapH)
	if err != nil {
		return nil, err
	}
	const startHP = 20
	g := &game.Game{
		Content:   c,
		Level:     lvl,
		Player:    start,
		RNG:       r,
		PlayerHP:  startHP,
		PlayerMax: startHP,
		Behaviors: behaviors.Registry(),
		Depth:     1,
	}
	g.NewLevelFn = func(depth int) (*game.Level, game.Pos, error) {
		l, s, _, err := gen.Rooms(r, c, mapW, mapH)
		return l, s, err
	}
	return g, nil
}
```
Ensure `main.go`'s import block includes `"fmt"`, `"os"`, `"time"`, and the internal `content`/`game`/`gen`/`rng`/`ui`/`behaviors`/`tcellui` packages (most are already imported from earlier plans; add any missing).

- [ ] **Step 3** — Verify the whole module from `go/`:
```bash
export GOTOOLCHAIN=local
gofmt -l .
go build ./... && go test ./... -count=1 && go vet ./...
go build -o /tmp/tsl_p6 ./cmd/tsl && echo "binary OK" && rm -f /tmp/tsl_p6
```
Expected: gofmt clean, all packages `ok`, vet clean, binary builds. (`go run ./cmd/tsl`: descend with `>`, reach the bottom to win, or die and get a morgue.)

- [ ] **Step 4** — Commit:
```bash
git add cmd/tsl/
git commit -m "feat(cmd): wire descent generator, write morgue, report outcome"
```

---

## Done criteria (Plan 6)
- The player descends `>` stairs to deeper levels (carrying HP, inventory, equipment); descending from depth `MaxDepth` wins.
- Death ends the game and writes `morgue.txt` (depth, HP, killer, gear, inventory).
- The game loop ends cleanly on win or death; `cmd` prints the outcome after restoring the terminal.
- `go test ./...`, `go vet`, gofmt all clean.
- **M1 is a complete winnable vertical slice:** generate → explore (FOV) → fight → loot/equip → descend → win or die.

## Deferred (optional follow-ups, beyond the M1 winnable slice)
- Save/resume (JSON snapshot with content-id relinking + RNG state).
- Configurable keymap (`.tsl_conf`: vi/Dvorak/rebind), an on-screen HP/status bar, and wands + interactive targeting.
