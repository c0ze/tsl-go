package ui

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func testGame(t *testing.T, rows []string) *game.Game {
	t.Helper()
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
	lvl, start, err := game.ParseLevel(c, rows, map[rune]string{'.': "floor", '#': "wall", '@': "floor"})
	if err != nil {
		t.Fatal(err)
	}
	return &game.Game{Content: c, Level: lvl, Player: start}
}

// scriptPrompter returns a fixed sequence of actions, then ActQuit forever.
type scriptPrompter struct {
	actions []Action
	i       int
}

func (s *scriptPrompter) NextAction() (Action, error) {
	if s.i >= len(s.actions) {
		return Action{Kind: ActQuit}, nil
	}
	a := s.actions[s.i]
	s.i++
	return a, nil
}

func (s *scriptPrompter) Menu(MenuSpec) (int, bool) { return 0, false }

type nullRenderer struct{ frames int }

func (n *nullRenderer) Render(View) { n.frames++ }

func TestBuildViewStatusLine(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.PlayerHP, g.PlayerMax = 14, 20
	g.Weapon = &game.Item{Def: &content.ItemDef{Name: "dagger"}}
	v := BuildView(g)
	for _, want := range []string{"HP 14/20", "Wield: dagger", "Wear: none"} {
		if !strings.Contains(v.Status, want) {
			t.Errorf("status %q missing %q", v.Status, want)
		}
	}
}

func TestBuildViewShowsEffects(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.PlayerHP, g.PlayerMax = 10, 20
	g.AddEffect("poison", 3)
	v := BuildView(g)
	if !strings.Contains(v.Status, "Poisoned") {
		t.Errorf("status %q should show Poisoned", v.Status)
	}
}

func TestBuildViewPlacesPlayer(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	g.UpdateFOV()
	v := BuildView(g)
	if v.W != 3 || v.H != 3 {
		t.Fatalf("view size = %dx%d, want 3x3", v.W, v.H)
	}
	if got := v.At(1, 1).Glyph; got != '@' {
		t.Errorf("center glyph = %q, want '@'", got)
	}
	if got := v.At(0, 0).Glyph; got != '.' {
		t.Errorf("corner glyph = %q, want '.'", got)
	}
}

func TestBuildViewVisibilityStates(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			tl := g.Level.At(game.Pos{X: x, Y: y})
			tl.Visible, tl.Seen = false, false
		}
	}
	g.Level.At(game.Pos{X: 1, Y: 1}).Visible = true // player tile
	vis := g.Level.At(game.Pos{X: 0, Y: 1})
	vis.Visible, vis.Seen = true, true
	g.Level.At(game.Pos{X: 0, Y: 0}).Seen = true // remembered only
	// (2,2) stays fully unseen

	v := BuildView(g)
	if c := v.At(1, 1); c.Glyph != '@' {
		t.Errorf("player cell = %q, want '@'", c.Glyph)
	}
	if c := v.At(0, 1); c.Glyph != '.' || c.Dim {
		t.Errorf("visible floor = %q dim=%v, want '.' bright", c.Glyph, c.Dim)
	}
	if c := v.At(0, 0); c.Glyph != '.' || !c.Dim {
		t.Errorf("remembered floor = %q dim=%v, want '.' dim", c.Glyph, c.Dim)
	}
	if c := v.At(2, 2); c.Glyph != ' ' {
		t.Errorf("unseen cell = %q, want blank space", c.Glyph)
	}
}

func TestRunAppliesActionsUntilQuit(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	p := &scriptPrompter{actions: []Action{
		{Kind: ActMove, Dir: game.DirE},
		{Kind: ActMove, Dir: game.DirE},
		{Kind: ActQuit},
	}}
	r := &nullRenderer{}
	if err := Run(g, p, r); err != nil {
		t.Fatalf("Run: %v", err)
	}
	if g.Player != (game.Pos{X: 3, Y: 1}) {
		t.Errorf("player = %v, want {3 1}", g.Player)
	}
	if r.frames == 0 {
		t.Error("expected at least one rendered frame")
	}
}

func TestBuildViewShowsVisibleItem(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.UpdateFOV()
	g.Level.Items = append(g.Level.Items, &game.Item{
		Def: &content.ItemDef{ID: "potion", Glyph: "!", Color: content.ColorRed}, Pos: game.Pos{X: 3, Y: 1},
	})
	v := BuildView(g)
	if c := v.At(3, 1); c.Glyph != '!' {
		t.Errorf("visible item glyph = %q, want '!'", c.Glyph)
	}
}

func TestRunInventoryUsesSelectedItem(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.PlayerHP, g.PlayerMax = 5, 20
	g.Behaviors = map[string]game.Behavior{"heal": func(gg *game.Game, it *game.Item) []string {
		gg.PlayerHP += it.Def.Power
		return []string{"healed"}
	}}
	g.Inventory = append(g.Inventory, &game.Item{Def: &content.ItemDef{Name: "potion", Kind: "potion", Use: "heal", Power: 5}})
	p := &menuPrompter{actions: []Action{{Kind: ActInventory}, {Kind: ActQuit}}, pick: 0}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if g.PlayerHP != 10 {
		t.Errorf("HP = %d, want 10 after quaffing", g.PlayerHP)
	}
}

// menuPrompter scripts actions and always picks index `pick` from any menu.
type menuPrompter struct {
	actions []Action
	i       int
	pick    int
}

func (m *menuPrompter) NextAction() (Action, error) {
	if m.i >= len(m.actions) {
		return Action{Kind: ActQuit}, nil
	}
	a := m.actions[m.i]
	m.i++
	return a, nil
}
func (m *menuPrompter) Menu(MenuSpec) (int, bool) { return m.pick, true }

func TestBuildViewShowsVisibleMonsterAndMessages(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.UpdateFOV()
	g.Level.Creatures = append(g.Level.Creatures, &game.Creature{
		Def: &content.MonsterDef{ID: "rat", Glyph: "r", Color: content.ColorBrown, HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3,
	})
	g.Messages = []string{"You hit the rat for 2.", "The rat dies."}
	v := BuildView(g)
	if c := v.At(3, 1); c.Glyph != 'r' {
		t.Errorf("visible monster glyph = %q, want 'r'", c.Glyph)
	}
	if len(v.Messages) == 0 || v.Messages[len(v.Messages)-1] != "The rat dies." {
		t.Errorf("view should carry recent messages, got %v", v.Messages)
	}
}

func travelGame(t *testing.T) *game.Game {
	t.Helper()
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	defs := map[string]*content.LevelDef{
		"start": {ID: "start", Name: "the Start", W: 5, H: 3, Start: true, Links: []string{"end"}},
		"end":   {ID: "end", Name: "the End", W: 5, H: 3, Links: []string{"start"}},
	}
	build := func(def *content.LevelDef) (*game.Level, error) {
		l := game.NewLevel(def.W, def.H, floor)
		l.Start = game.Pos{X: 1, Y: 1}
		l.Portals = []game.Portal{{Pos: game.Pos{X: 3, Y: 1}, Target: def.Links[0]}}
		return l, nil
	}
	d, err := game.NewDungeon(defs, "start", build)
	if err != nil {
		t.Fatal(err)
	}
	g := &game.Game{Content: &content.Content{Levels: defs}, Dungeon: d, Level: d.Current(), PlayerHP: 10, PlayerMax: 10}
	g.EnterStart()
	return g
}

func TestRunTravelChangesLevel(t *testing.T) {
	g := travelGame(t)
	g.Player = game.Pos{X: 3, Y: 1} // on the start portal
	p := &scriptPrompter{actions: []Action{{Kind: ActTravel}, {Kind: ActQuit}}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if g.LocationName() != "the End" {
		t.Errorf("after travel, location = %q, want the End", g.LocationName())
	}
}

func TestRunEatHealsFromInventory(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.PlayerHP, g.PlayerMax = 5, 20
	g.Behaviors = map[string]game.Behavior{"eat": func(gg *game.Game, it *game.Item) []string {
		gg.PlayerHP += it.Def.Power
		return []string{"munch"}
	}}
	g.Inventory = append(g.Inventory, &game.Item{Def: &content.ItemDef{Name: "rat corpse", Kind: "food", Use: "eat", Power: 3}})
	p := &menuPrompter{actions: []Action{{Kind: ActEat}, {Kind: ActQuit}}, pick: 0}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if g.PlayerHP != 8 {
		t.Errorf("HP = %d, want 8 after eating", g.PlayerHP)
	}
}

func TestRunEatWithNoFoodReports(t *testing.T) {
	g := testGame(t, []string{".@."})
	p := &scriptPrompter{actions: []Action{{Kind: ActEat}, {Kind: ActQuit}}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if len(g.Messages) == 0 || !strings.Contains(g.Messages[len(g.Messages)-1], "nothing to eat") {
		t.Errorf("expected a 'nothing to eat' message, got %v", g.Messages)
	}
}
