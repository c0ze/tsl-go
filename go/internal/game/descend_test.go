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
