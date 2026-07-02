package ui

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/rng"
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

func (s *scriptPrompter) Target(game.Pos) (game.Pos, bool) { return game.Pos{}, false }

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

func TestBuildViewShowsWornAccessories(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.Ring = &game.Item{Def: &content.ItemDef{Name: "ring of protection", Kind: "ring", Dodge: 2}}
	g.Amulet = &game.Item{Def: &content.ItemDef{Name: "amulet of warding", Kind: "amulet", Dodge: 3}}
	v := BuildView(g)
	if !strings.Contains(v.Status, "Worn:") {
		t.Fatalf("status %q should include the Worn segment", v.Status)
	}
	for _, want := range []string{"ring of protection", "amulet of warding"} {
		if !strings.Contains(v.Status, want) {
			t.Errorf("status %q should show worn accessory %q", v.Status, want)
		}
	}
}

func TestBuildViewShowsEP(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.EP, g.EPMax = 7, 10
	v := BuildView(g)
	if !strings.Contains(v.Status, "EP 7/10") {
		t.Errorf("status %q should show EP", v.Status)
	}
}

func TestBuildViewHUDFields(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.PlayerHP, g.PlayerMax = 14, 20
	g.EP, g.EPMax = 7, 10
	g.Weapon = &game.Item{Def: &content.ItemDef{Name: "dagger"}}
	g.AddEffect("poison", 3)
	v := BuildView(g)
	h := v.HUD
	if h.HP != 14 || h.HPMax != 20 {
		t.Errorf("HUD HP = %d/%d, want 14/20", h.HP, h.HPMax)
	}
	if h.EP != 7 || h.EPMax != 10 {
		t.Errorf("HUD EP = %d/%d, want 7/10", h.EP, h.EPMax)
	}
	if h.Location != "the dungeon" {
		t.Errorf("HUD location = %q, want the no-dungeon fallback", h.Location)
	}
	if h.Wield != "dagger" || h.Wear != "none" {
		t.Errorf("HUD gear = wield %q wear %q, want dagger/none", h.Wield, h.Wear)
	}
	if !strings.Contains(h.Effects, "Poisoned") {
		t.Errorf("HUD effects %q should include Poisoned", h.Effects)
	}
	if v.Status != h.Line() {
		t.Errorf("Status %q must be exactly HUD.Line() %q", v.Status, h.Line())
	}
}

func TestRunCastSpell(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.EP, g.EPMax = 10, 10
	cast := false
	g.Behaviors = map[string]game.Behavior{"first_aid": func(gg *game.Game, it *game.Item) []string { cast = true; return []string{"mend"} }}
	g.Content.Items = map[string]*content.ItemDef{"book_aid": {ID: "book_aid", Name: "spellbook of first aid", Kind: "spellbook", Use: "first_aid", Cost: 4}}
	g.Known = map[string]bool{"book_aid": true} // learned, not carried (C read_book)
	p := &menuPrompter{actions: []Action{{Kind: ActCast}, {Kind: ActQuit}}, pick: 0}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if !cast {
		t.Error("casting should run the spell")
	}
	if g.EP != 6 {
		t.Errorf("EP = %d, want 6 after casting a cost-4 spell", g.EP)
	}
}

func TestRunCastForceBoltAtCreature(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.RNG = rng.NewWithSeed(1)
	g.EP, g.EPMax = 10, 10
	g.Content.Items = map[string]*content.ItemDef{"book_force": {ID: "book_force", Name: "spellbook of force bolt", Kind: "spellbook", Ranged: 6, Damage: "10d1", Cost: 5}}
	g.Known = map[string]bool{"book_force": true}
	g.Level.Creatures = append(g.Level.Creatures, &game.Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3})
	g.UpdateFOV()
	p := &zapPrompter{actions: []Action{{Kind: ActCast}, {Kind: ActQuit}}, target: game.Pos{X: 3, Y: 1}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if len(g.Level.Creatures) != 0 {
		t.Error("casting force bolt should have killed the rat")
	}
	if g.EP != 5 {
		t.Errorf("EP = %d, want 5 after casting a cost-5 spell", g.EP)
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
	if got := v.At(0, 0).Glyph; got != '·' {
		t.Errorf("corner glyph = %q, want '·'", got)
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
	if c := v.At(0, 1); c.Glyph != '·' || c.Dim {
		t.Errorf("visible floor = %q dim=%v, want '·' bright", c.Glyph, c.Dim)
	}
	if c := v.At(0, 0); c.Glyph != '·' || !c.Dim {
		t.Errorf("remembered floor = %q dim=%v, want '·' dim", c.Glyph, c.Dim)
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

// capturePrompter records the last menu it was shown, then cancels it.
type capturePrompter struct {
	actions  []Action
	i        int
	lastMenu MenuSpec
}

func (c *capturePrompter) NextAction() (Action, error) {
	if c.i >= len(c.actions) {
		return Action{Kind: ActQuit}, nil
	}
	a := c.actions[c.i]
	c.i++
	return a, nil
}
func (c *capturePrompter) Menu(m MenuSpec) (int, bool)      { c.lastMenu = m; return 0, false }
func (c *capturePrompter) Target(game.Pos) (game.Pos, bool) { return game.Pos{}, false }

func TestInventoryMenuShowsAppearance(t *testing.T) {
	g := testGame(t, []string{".@."})
	g.Content.Items = map[string]*content.ItemDef{
		"healing": {ID: "healing", Name: "potion of healing", Kind: "potion", Use: "heal"},
	}
	g.RNG = rng.NewWithSeed(1)
	g.AssignAppearances()
	it := &game.Item{Def: g.Content.Items["healing"]}
	g.Inventory = append(g.Inventory, it)
	want := g.DisplayName(it)
	if want == "potion of healing" {
		t.Fatal("expected the potion to start unidentified")
	}

	p := &capturePrompter{actions: []Action{{Kind: ActInventory}}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}

	if len(p.lastMenu.Items) == 0 || p.lastMenu.Items[0] != want {
		t.Errorf("inventory menu should show the appearance %q, got %v", want, p.lastMenu.Items)
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

func (m *menuPrompter) Target(game.Pos) (game.Pos, bool) { return game.Pos{}, false }

// zapPrompter scripts actions, picks the first wand, and targets a fixed tile.
type zapPrompter struct {
	actions []Action
	i       int
	target  game.Pos
}

func (z *zapPrompter) NextAction() (Action, error) {
	if z.i >= len(z.actions) {
		return Action{Kind: ActQuit}, nil
	}
	a := z.actions[z.i]
	z.i++
	return a, nil
}
func (z *zapPrompter) Menu(MenuSpec) (int, bool)        { return 0, true }
func (z *zapPrompter) Target(game.Pos) (game.Pos, bool) { return z.target, true }

func TestRunZapWandDamagesCreature(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.RNG = rng.NewWithSeed(1)
	g.UpdateFOV()
	g.Level.Creatures = append(g.Level.Creatures, &game.Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3})
	g.Inventory = append(g.Inventory, &game.Item{Def: &content.ItemDef{Name: "wand", Kind: "wand", Damage: "5d1"}, Charges: 3})
	p := &zapPrompter{actions: []Action{{Kind: ActZap}, {Kind: ActQuit}}, target: game.Pos{X: 3, Y: 1}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if g.Level.CreatureAt(game.Pos{X: 3, Y: 1}) != nil {
		t.Error("zapping the wand should have killed the rat")
	}
}

func TestRunFireWeaponAtCreature(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.RNG = rng.NewWithSeed(1)
	g.Weapon = &game.Item{Def: &content.ItemDef{Name: "shortbow", Kind: "weapon", Damage: "10d1", Ranged: 6}}
	g.Inventory = append(g.Inventory, &game.Item{Def: &content.ItemDef{ID: "arrow", Name: "crude arrow", Kind: "ammo"}, Charges: 3})
	g.Level.Creatures = append(g.Level.Creatures, &game.Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3})
	g.UpdateFOV()
	p := &zapPrompter{actions: []Action{{Kind: ActFire}, {Kind: ActQuit}}, target: game.Pos{X: 3, Y: 1}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if g.Level.CreatureAt(game.Pos{X: 3, Y: 1}) != nil {
		t.Error("firing the shortbow should have killed the rat")
	}
}

func TestRunFireWithNoRangedWeaponReports(t *testing.T) {
	g := testGame(t, []string{".@."})
	p := &scriptPrompter{actions: []Action{{Kind: ActFire}, {Kind: ActQuit}}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if len(g.Messages) == 0 || !strings.Contains(g.Messages[len(g.Messages)-1], "no ranged weapon") {
		t.Errorf("expected a 'no ranged weapon' message, got %v", g.Messages)
	}
}

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

func TestRunReadScrollFromInventory(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	read := false
	g.Behaviors = map[string]game.Behavior{"teleport": func(gg *game.Game, it *game.Item) []string {
		read = true
		return []string{"blink"}
	}}
	g.Inventory = append(g.Inventory, &game.Item{Def: &content.ItemDef{Name: "scroll of teleportation", Kind: "scroll", Use: "teleport"}})
	p := &menuPrompter{actions: []Action{{Kind: ActRead}, {Kind: ActQuit}}, pick: 0}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if !read {
		t.Error("reading a scroll should invoke its behavior")
	}
	if len(g.Inventory) != 0 {
		t.Errorf("the scroll should be consumed, inventory = %v", g.Inventory)
	}
}

func TestRunReadWithNoScrollReports(t *testing.T) {
	g := testGame(t, []string{".@."})
	p := &scriptPrompter{actions: []Action{{Kind: ActRead}, {Kind: ActQuit}}}
	if err := Run(g, p, &nullRenderer{}); err != nil {
		t.Fatal(err)
	}
	if len(g.Messages) == 0 || !strings.Contains(g.Messages[len(g.Messages)-1], "nothing to read") {
		t.Errorf("expected a 'nothing to read' message, got %v", g.Messages)
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

func TestBuildViewHidesDisguisedTraps(t *testing.T) {
	g := testGame(t, []string{".@."})
	trap := &content.TileDef{ID: "dart_trap", Glyph: "^", Color: content.ColorRed, Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	pos := game.Pos{X: 2, Y: 0}
	g.Level.Set(pos, trap)
	tile := g.Level.At(pos)
	tile.Disguise = floor
	tile.Visible = true
	v := BuildView(g)
	if got := v.At(2, 0).Glyph; got != '·' {
		t.Errorf("an unrevealed trap should render as floor, got %q", got)
	}
	tile.Revealed = true
	v = BuildView(g)
	if got := v.At(2, 0).Glyph; got != '^' {
		t.Errorf("a revealed trap should show itself, got %q", got)
	}
}

func TestBuildViewShowsMimicGlamour(t *testing.T) {
	g := testGame(t, []string{".@..."})
	m := &game.Creature{
		Def: &content.MonsterDef{ID: "mimic", Name: "mimic", Glyph: "m", Color: content.ColorNormal, HP: 5},
		Pos: game.Pos{X: 3, Y: 0}, HP: 5,
		Disguised:  true,
		DisguiseAs: &content.ItemDef{ID: "healing_potion", Name: "healing potion", Glyph: "!", Color: content.ColorRed, Kind: "potion"},
	}
	g.Level.Creatures = append(g.Level.Creatures, m)
	g.Level.At(game.Pos{X: 3, Y: 0}).Visible = true
	v := BuildView(g)
	if got := v.At(3, 0).Glyph; got != '!' {
		t.Errorf("a disguised mimic renders as its loot, got %q", got)
	}
	m.Disguised = false
	v = BuildView(g)
	if got := v.At(3, 0).Glyph; got != 'm' {
		t.Errorf("a revealed mimic shows itself, got %q", got)
	}
}

func TestRunSurfacesSaveRequest(t *testing.T) {
	g := testGame(t, []string{".@."})
	p := &zapPrompter{actions: []Action{{Kind: ActSave}}}
	if err := Run(g, p, &nullRenderer{}); err != ErrSaveRequested {
		t.Errorf("Run should surface the save sentinel, got %v", err)
	}
}

func TestCloseKeyBinding(t *testing.T) {
	// 'O' is the C's own default binding for action_close (keymap.c:219).
	a, ok := ActionForRune('O')
	if !ok || a.Kind != ActClose {
		t.Errorf("ActionForRune('O') = %+v, %v; want ActClose", a, ok)
	}
}
