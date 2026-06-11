package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

func TestTeleportMovesToPassableTile(t *testing.T) {
	g := combatGame() // 10x3 all floor, player at (1,1)
	start := g.Player
	if !g.Teleport() {
		t.Fatal("teleport should succeed on an open level")
	}
	if g.Player == start {
		t.Error("teleport should move the player off the starting tile")
	}
	if !g.Level.Passable(g.Player) {
		t.Errorf("teleport landed on an impassable tile %v", g.Player)
	}
	if g.Level.CreatureAt(g.Player) != nil {
		t.Error("teleport should not land on a creature")
	}
}

func TestTeleportLandsOnFloorAmongWalls(t *testing.T) {
	c := testContent()
	rows := []string{
		"#####",
		"#@.##",
		"##..#",
		"#####",
	}
	lvl, start, err := ParseLevel(c, rows, map[rune]string{'#': "wall", '@': "floor", '.': "floor"})
	if err != nil {
		t.Fatal(err)
	}
	g := &Game{Content: c, Level: lvl, Player: start, RNG: rng.NewWithSeed(1)}
	for i := 0; i < 30; i++ {
		g.Teleport()
		if !g.Level.Passable(g.Player) {
			t.Fatalf("teleport landed on a wall at %v", g.Player)
		}
	}
}

func TestRevealMapMarksAllSeen(t *testing.T) {
	g := combatGame()
	g.RevealMap()
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			if !g.Level.At(Pos{X: x, Y: y}).Seen {
				t.Fatalf("tile %d,%d not marked seen after RevealMap", x, y)
			}
		}
	}
}

func TestReadableInventoryFiltersScrolls(t *testing.T) {
	g := combatGame()
	g.Inventory = []*Item{
		{Def: &content.ItemDef{Name: "potion", Kind: "potion", Use: "heal"}},
		{Def: &content.ItemDef{Name: "scroll", Kind: "scroll", Use: "teleport"}},
	}
	r := g.ReadableInventory()
	if len(r) != 1 || r[0].Def.Kind != "scroll" {
		t.Errorf("ReadableInventory = %v, want exactly one scroll", r)
	}
}

func TestPlayerUseScrollConsumesAndRuns(t *testing.T) {
	g := combatGame()
	ran := false
	g.Behaviors = map[string]Behavior{"teleport": func(gg *Game, it *Item) []string {
		ran = true
		return []string{"zap"}
	}}
	sc := &Item{Def: &content.ItemDef{Name: "scroll", Kind: "scroll", Use: "teleport"}}
	g.Inventory = append(g.Inventory, sc)

	g.PlayerUse(sc)

	if !ran {
		t.Error("reading a scroll should invoke its behavior")
	}
	if g.hasInventoryItem(sc) {
		t.Error("a read scroll should be consumed")
	}
}

func TestBlinkLandingSpringsTrap(t *testing.T) {
	g := combatGame()
	trap := &content.TileDef{ID: "dart_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	// Every tile the blink can land on is a trap, so the landing is certain.
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			p := Pos{X: x, Y: y}
			if p != g.Player && g.Level.Passable(p) {
				g.Level.Set(p, trap)
			}
		}
	}
	if !g.Teleport() {
		t.Fatal("expected the blink to land somewhere")
	}
	if !g.HasEffect("poison") {
		t.Error("a blink springs the trap it lands on (C cast_blink/activate_trap)")
	}
}

func TestFloatingBlinkSkipsLandingTrap(t *testing.T) {
	g := combatGame()
	trap := &content.TileDef{ID: "dart_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			p := Pos{X: x, Y: y}
			if p != g.Player && g.Level.Passable(p) {
				g.Level.Set(p, trap)
			}
		}
	}
	g.AddEffect("levitate", 10)
	g.Teleport()
	if g.HasEffect("poison") {
		t.Error("a floater blinks over the trap below (C activate_trap is_floating)")
	}
}

func TestForgetMapClearsSeen(t *testing.T) {
	g := combatGame()
	g.RevealMap()
	g.ForgetMap()
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			if g.Level.At(Pos{X: x, Y: y}).Seen {
				t.Fatal("ForgetMap should wipe the automap (C magic.c amnesia)")
			}
		}
	}
}

func TestMarkAndRecallSameLevel(t *testing.T) {
	g := fakeDungeon(t)
	g.MarkRecall()
	mark := g.Player
	g.Player = Pos{2, 1} // wander off
	if !g.Recall() {
		t.Fatal("a marked recall should succeed")
	}
	if g.Player != mark {
		t.Errorf("recall should return to the mark %v, got %v", mark, g.Player)
	}
}

func TestRecallAcrossLevels(t *testing.T) {
	g := fakeDungeon(t)
	mark := g.Player
	g.MarkRecall()
	g.Player = Pos{3, 1}
	g.Travel() // descend to B
	if g.Dungeon.current != "b" {
		t.Fatal("setup: expected to be on B")
	}
	if !g.Recall() {
		t.Fatal("recall should pull the player back across levels (C change_level)")
	}
	if g.Dungeon.current != "a" || g.Player != mark {
		t.Errorf("recall should land on A at %v, got level %q at %v", mark, g.Dungeon.current, g.Player)
	}
}

func TestUnmarkedRecallFizzles(t *testing.T) {
	g := fakeDungeon(t)
	if g.Recall() {
		t.Error("recall with no mark set must fizzle (we diverge from the C's zero-init landing)")
	}
}

func TestRecallNudgesOffOccupiedMark(t *testing.T) {
	g := fakeDungeon(t)
	g.MarkRecall()
	mark := g.Player
	g.Player = Pos{3, 1}
	squatter := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3}, Pos: mark, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, squatter)
	if !g.Recall() {
		t.Fatal("an occupied mark should still recall, one tile off")
	}
	if g.Player == mark {
		t.Error("the player must not land inside the squatter")
	}
	if d := chebyshev(g.Player, mark); d != 1 {
		t.Errorf("expected a one-ring nudge, landed %d away", d)
	}
}

func TestBuriedMarkRollsBackLevelSwitch(t *testing.T) {
	g := fakeDungeon(t)
	g.MarkRecall()
	mark := g.Player
	// Bury the mark and its whole ring in bodies on level A.
	for dy := -1; dy <= 1; dy++ {
		for dx := -1; dx <= 1; dx++ {
			p := Pos{X: mark.X + dx, Y: mark.Y + dy}
			g.Level.Creatures = append(g.Level.Creatures, &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 1}, Pos: p, HP: 1})
		}
	}
	g.Player = Pos{3, 1}
	g.Travel() // over to B
	before := g.Player
	if g.Recall() {
		t.Fatal("a fully buried mark must fizzle")
	}
	if g.Dungeon.current != "b" || g.Player != before {
		t.Errorf("a fizzled recall must leave no trace: on %q at %v", g.Dungeon.current, g.Player)
	}
}
