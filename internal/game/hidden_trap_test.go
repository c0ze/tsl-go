package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

var testTrapDef = &content.TileDef{ID: "dart_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
var testFloorDef = &content.TileDef{ID: "floor", Glyph: ".", Passable: true, Transparent: true}

// hideTrap plants a disguised trap of the given difficulty at p.
func hideTrap(g *Game, p Pos, difficulty int) *Tile {
	g.Level.Set(p, testTrapDef)
	t := g.Level.At(p)
	t.Disguise = testFloorDef
	t.TrapDifficulty = difficulty
	return t
}

func TestDisguisedTrapAppearsAsFloor(t *testing.T) {
	g := combatGame()
	tile := hideTrap(g, Pos{2, 1}, 8)
	if tile.Appears() != testFloorDef {
		t.Error("an unrevealed trap wears the floor's face (C: traps render only once revealed)")
	}
	tile.Revealed = true
	if tile.Appears() != testTrapDef {
		t.Error("a revealed trap shows itself")
	}
}

func TestSteppingSpringsAndReveals(t *testing.T) {
	g := combatGame()
	tile := hideTrap(g, Pos{2, 1}, 8)
	g.PlayerStep(DirE)
	if !g.HasEffect("poison") {
		t.Error("stepping on the hidden trap springs it")
	}
	if !tile.Revealed {
		t.Error("springing reveals the trap (C traps.c:330)")
	}
}

func TestPassiveSpottingRespectsDifficulty(t *testing.T) {
	g := combatGame()
	easy := hideTrap(g, Pos{4, 1}, 2)  // perception 3 > 2: spotted on sight
	hard := hideTrap(g, Pos{6, 1}, 12) // 3 > 12 never holds
	easy.Visible, hard.Visible = true, true
	g.advanceWorld()
	if !easy.Revealed {
		t.Error("a flimsy visible trap is spotted (C try_to_detect_traps)")
	}
	if hard.Revealed {
		t.Error("a well-made trap stays hidden in plain sight")
	}
}

func TestUnseenTrapNotSpotted(t *testing.T) {
	g := combatGame()
	easy := hideTrap(g, Pos{4, 1}, 2)
	easy.Visible = false // out of the FOV
	g.advanceWorld()
	if easy.Revealed {
		t.Error("spotting requires line of sight (C: can_see gate)")
	}
}

func TestRevealTrapsExposesAll(t *testing.T) {
	g := combatGame()
	a := hideTrap(g, Pos{4, 1}, 12)
	b := hideTrap(g, Pos{6, 1}, 12)
	b.Revealed = true // already known: not counted again
	if n := g.RevealTraps(); n != 1 {
		t.Errorf("RevealTraps should count newly exposed traps, got %d", n)
	}
	if !a.Revealed || !a.Seen {
		t.Error("the scroll exposes and maps every trap (C reveal_traps writes to memory)")
	}
}

func TestBlinkLandingRevealsTrap(t *testing.T) {
	g := combatGame()
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			p := Pos{X: x, Y: y}
			if p != g.Player && g.Level.Passable(p) {
				hideTrap(g, p, 12)
			}
		}
	}
	g.Teleport()
	if !g.Level.At(g.Player).Revealed {
		t.Error("a blink landing springs and reveals the trap under it")
	}
}
