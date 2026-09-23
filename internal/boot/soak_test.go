package boot_test

import (
	"errors"
	"fmt"
	"os"
	"runtime/debug"
	"strconv"
	"strings"
	"testing"

	"github.com/c0ze/tsl-go/data"
	"github.com/c0ze/tsl-go/internal/behaviors"
	"github.com/c0ze/tsl-go/internal/boot"
	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/gen"
	"github.com/c0ze/tsl-go/internal/rng"
	"github.com/c0ze/tsl-go/internal/ui"
)

// errBudget ends a soak run that neither died nor won within its action cap.
var errBudget = errors.New("action budget spent")

// bot is a seeded random player: it mostly heads for the nearest stairs (so
// runs reach the deep levels), and otherwise mashes every verb with random
// menu picks and targets — including cancels and out-of-range indices.
type bot struct {
	g       *game.Game
	r       *rng.MT
	actions int
	budget  int
}

func (b *bot) NextAction() (ui.Action, error) {
	b.actions++
	if b.actions > b.budget {
		return ui.Action{}, errBudget
	}
	if b.g.Level.PortalAt(b.g.Player) != nil && b.r.Intn(4) != 0 {
		return ui.Action{Kind: ui.ActTravel}, nil
	}
	if b.r.Intn(10) < 6 {
		if d, ok := b.towardStairs(); ok {
			return ui.Action{Kind: ui.ActMove, Dir: d}, nil
		}
	}
	verbs := []ui.ActionKind{ui.ActMove, ui.ActMove, ui.ActMove, ui.ActPickup, ui.ActInventory,
		ui.ActEat, ui.ActZap, ui.ActRead, ui.ActFire, ui.ActCast, ui.ActTalk, ui.ActClose, ui.ActSave}
	k := verbs[b.r.Intn(len(verbs))]
	return ui.Action{Kind: k, Dir: game.Direction(1 + b.r.Intn(8))}, nil
}

func (b *bot) Menu(m ui.MenuSpec) (int, bool) {
	if b.r.Intn(8) == 0 {
		return 0, false
	}
	return b.r.Intn(len(m.Items)+1) - b.r.Intn(2), true // occasionally -1 or len
}

func (b *bot) Target(o game.Pos) (game.Pos, bool) {
	if b.r.Intn(8) == 0 {
		return game.Pos{}, false
	}
	return game.Pos{X: o.X + b.r.Intn(15) - 7, Y: o.Y + b.r.Intn(15) - 7}, true
}

func (b *bot) Render(ui.View) {}

// towardStairs is one BFS step toward the nearest portal over passable tiles.
func (b *bot) towardStairs() (game.Direction, bool) {
	l := b.g.Level
	if len(l.Portals) == 0 {
		return 0, false
	}
	dirs := []game.Pos{{X: 0, Y: -1}, {X: 0, Y: 1}, {X: 1, Y: 0}, {X: -1, Y: 0},
		{X: -1, Y: -1}, {X: 1, Y: -1}, {X: -1, Y: 1}, {X: 1, Y: 1}}
	first := map[game.Pos]int{}
	queue := []game.Pos{}
	for i, d := range dirs {
		p := game.Pos{X: b.g.Player.X + d.X, Y: b.g.Player.Y + d.Y}
		if l.Passable(p) {
			first[p] = i
			queue = append(queue, p)
		}
	}
	for len(queue) > 0 {
		p := queue[0]
		queue = queue[1:]
		if l.PortalAt(p) != nil {
			return dirFor(dirs[first[p]]), true
		}
		for _, d := range dirs {
			n := game.Pos{X: p.X + d.X, Y: p.Y + d.Y}
			if _, seen := first[n]; !seen && l.Passable(n) {
				first[n] = first[p]
				queue = append(queue, n)
			}
		}
	}
	return 0, false
}

func dirFor(d game.Pos) game.Direction {
	for _, dir := range []game.Direction{game.DirN, game.DirS, game.DirE, game.DirW,
		game.DirNW, game.DirNE, game.DirSW, game.DirSE} {
		dx, dy := dir.Delta()
		if dx == d.X && dy == d.Y {
			return dir
		}
	}
	return game.DirN
}

// soak plays one seeded run to death, victory, or the action budget, saving
// and reloading whenever the bot asks to, and returns how it ended.
func soak(c *content.Content, seed uint32, budget int) (end string, err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("panic: %v\n%s", r, debug.Stack())
		}
	}()
	g, err := boot.NewGame(c, seed)
	if err != nil {
		return "", err
	}
	b := &bot{g: g, r: rng.NewWithSeed(seed ^ 0x9e3779b9), budget: budget}
	for {
		err := ui.Run(g, b, b)
		switch {
		case errors.Is(err, ui.ErrSaveRequested):
			var sb strings.Builder
			if err := g.Save(&sb); err != nil {
				return "", fmt.Errorf("save: %w", err)
			}
			var loaded *game.Game
			build := func(def *content.LevelDef) (*game.Level, error) { return gen.LevelFromDef(loaded.RNG, c, def) }
			loaded, err = game.LoadGame(strings.NewReader(sb.String()), c, behaviors.Registry(), build)
			if err != nil {
				return "", fmt.Errorf("load after save: %w", err)
			}
			g, b.g = loaded, loaded
			continue
		case errors.Is(err, errBudget):
			return "budget", nil
		case err != nil:
			return "", err
		}
		switch {
		case g.Won:
			return "won", nil
		case g.Dead:
			return "dead", nil
		}
		return "quit", nil
	}
}

// TestSoakRandomPlay drives full random runs through the real ui.Run loop and
// content, with save/load round-trips mid-run: any panic or load failure over
// the seeds fails the test with the seed that reproduces it.
func TestSoakRandomPlay(t *testing.T) {
	c, err := content.Load(data.Files)
	if err != nil {
		t.Fatal(err)
	}
	seeds, budget := 60, 4000
	if n, err := strconv.Atoi(os.Getenv("TSL_SOAK_SEEDS")); err == nil && n > 0 {
		seeds = n // a deeper local sweep: TSL_SOAK_SEEDS=2000 go test ./internal/boot
	} else if testing.Short() {
		seeds, budget = 10, 1500
	}
	ends := map[string]int{}
	for s := 1; s <= seeds; s++ {
		end, err := soak(c, uint32(s), budget)
		if err != nil {
			t.Fatalf("seed %d: %v", s, err)
		}
		ends[end]++
	}
	t.Logf("run endings over %d seeds: %v", seeds, ends)
}
