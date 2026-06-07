# TSL Go Port — M1 Plan 5: Items, Inventory, Equip, Behavior Registry

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or executing-plans. Steps use checkbox (`- [ ]`).

**Goal:** Items lie in the dungeon; the player picks them up, opens an inventory menu, equips a weapon/armor (which change combat), and quaffs a healing potion driven through the **behavior registry** (the design's data↔code bridge).

**Architecture:** `content` gains `ItemDef` + optional `items.toml`. `game` gains an `Item` type, a ground-item list, player inventory + equipped weapon/armor, a `Behavior` func type, and an injected `Behaviors` registry; combat reads equipped stats. New leaf-ish package `internal/behaviors` *implements* effects as `game.Behavior`s and exposes `Registry()`; `cmd` injects it (so `game` never imports `behaviors` — no cycle). `gen` places items. `ui` adds a `Menu` prompt + item rendering; `ui/tcell` implements the menu.

**Scope note:** potion (heal, via registry), weapon, armor. Wands + interactive targeting are deferred to a small follow-up — the registry pattern is fully demonstrated by `heal`, and equip is demonstrated by weapon/armor.

**Dependency direction:** `cmd → behaviors → game`; `cmd → ui → game`; `game → {content, rng, fov}`. No `game`↔`behaviors` cycle.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). No new deps.

## Working dir & environment
- Module in `go/`; `export GOTOOLCHAIN=local`. Branch `plan5-items`. No `go mod tidy`/`go get`.

---

## Task 1: Item content (`content`)

**Files:** Modify `go/internal/content/content.go`; create `go/data/items.toml`; modify `go/internal/content/content_test.go`

- [ ] **Step 1** — create `go/data/items.toml`:
```toml
# Item definitions. [item.<id>] → ItemDef keyed by <id>.
[item.healing_potion]
name = "healing potion"
glyph = "!"
color = "red"
kind = "potion"
use = "heal"
power = 8

[item.dagger]
name = "dagger"
glyph = ")"
color = "normal"
kind = "weapon"
attack = 2
damage = "1d4"

[item.leather_armor]
name = "leather armor"
glyph = "["
color = "brown"
kind = "armor"
dodge = 2
```

- [ ] **Step 2** — append to `go/internal/content/content_test.go`:
```go
func TestLoadItems(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "items.toml"), []byte("[item.healing_potion]\nname=\"healing potion\"\nglyph=\"!\"\ncolor=\"red\"\nkind=\"potion\"\nuse=\"heal\"\npower=8\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	p, ok := c.Items["healing_potion"]
	if !ok {
		t.Fatal("healing_potion missing")
	}
	if p.Kind != "potion" || p.Use != "heal" || p.Power != 8 || p.Rune() != '!' {
		t.Errorf("unexpected potion def: %+v", p)
	}
}

func TestLoadWithoutItemsFileIsOK(t *testing.T) {
	dir := writeTiles(t, "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n")
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if len(c.Items) != 0 {
		t.Errorf("expected no items, got %d", len(c.Items))
	}
}

func TestLoadRejectsBadItemKind(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "items.toml"), []byte("[item.x]\nname=\"x\"\nglyph=\"x\"\ncolor=\"normal\"\nkind=\"banana\"\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if _, err := Load(dir); err == nil {
		t.Fatal("expected error for invalid item kind")
	}
}
```

- [ ] **Step 3** — in `go/internal/content/content.go`: add the `ItemDef` type, the `Items` map on `Content`, `itemsFile`, optional loading mirroring monsters, and `validateItem`.

Add after `MonsterDef`'s `Rune`:
```go
// ItemDef defines a kind of item.
type ItemDef struct {
	ID     string `toml:"-"`
	Name   string `toml:"name"`
	Glyph  string `toml:"glyph"`
	Color  Color  `toml:"color"`
	Kind   string `toml:"kind"`   // "potion", "weapon", or "armor"
	Use    string `toml:"use"`    // behavior name (potions)
	Power  int    `toml:"power"`  // potion magnitude (heal amount)
	Attack int    `toml:"attack"` // weapon attack bonus
	Dodge  int    `toml:"dodge"`  // armor dodge bonus
	Damage string `toml:"damage"` // weapon damage spec
}

// Rune returns the item's glyph as a rune.
func (i *ItemDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(i.Glyph)
	return r
}

var validItemKinds = map[string]bool{"potion": true, "weapon": true, "armor": true}
```
Add `Items map[string]*ItemDef` to `Content`:
```go
type Content struct {
	Tiles    map[string]*TileDef
	Monsters map[string]*MonsterDef
	Items    map[string]*ItemDef
}
```
Add the file struct:
```go
type itemsFile struct {
	Item map[string]*ItemDef `toml:"item"`
}
```
In `Load`, after the monsters block (before `return c, nil`), add an items block mirroring monsters:
```go
	c.Items = map[string]*ItemDef{}
	ipath := filepath.Join(dir, "items.toml")
	if _, err := os.Stat(ipath); err == nil {
		var inf itemsFile
		if _, err := toml.DecodeFile(ipath, &inf); err != nil {
			return nil, fmt.Errorf("loading %s: %w", ipath, err)
		}
		for id, def := range inf.Item {
			def.ID = id
			if err := validateItem(def); err != nil {
				return nil, fmt.Errorf("item %q in %s: %w", id, ipath, err)
			}
			c.Items[id] = def
		}
	} else if !errors.Is(err, os.ErrNotExist) {
		return nil, fmt.Errorf("stat %s: %w", ipath, err)
	}
```
Add the validator:
```go
func validateItem(i *ItemDef) error {
	if utf8.RuneCountInString(i.Glyph) != 1 {
		return fmt.Errorf("glyph must be exactly one character, got %q", i.Glyph)
	}
	if !validColors[i.Color] {
		return fmt.Errorf("invalid color %q", i.Color)
	}
	if !validItemKinds[i.Kind] {
		return fmt.Errorf("invalid kind %q", i.Kind)
	}
	return nil
}
```

- [ ] **Step 4** — `go test ./internal/content/` → PASS. **Commit:**
```bash
git add go/internal/content/ go/data/items.toml
git commit -m "feat(content): ItemDef + optional items.toml loading"
```

---

## Task 2: Item type, inventory state, behavior type (`game`)

**Files:** Create `go/internal/game/item.go`, `go/internal/game/item_test.go`; modify `go/internal/game/game.go`

- [ ] **Step 1** — extend the `Game` struct in `game.go` to add item fields (after `Dead`):
```go
	Inventory []*Item
	Weapon    *Item
	Armor     *Item
	Behaviors map[string]Behavior
```
and add the `Items` field to `Level`:
```go
type Level struct {
	W, H      int
	tiles     []Tile
	Creatures []*Creature
	Items     []*Item
}
```

- [ ] **Step 2** — `go/internal/game/item_test.go`:
```go
package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestItemAtAndRemove(t *testing.T) {
	l := NewLevel(5, 5, testContent().Tiles["floor"])
	it := &Item{Def: &content.ItemDef{ID: "potion", Glyph: "!"}, Pos: Pos{2, 2}}
	l.Items = append(l.Items, it)
	if l.ItemAt(Pos{2, 2}) != it {
		t.Fatal("ItemAt(2,2) should find the item")
	}
	if l.ItemAt(Pos{0, 0}) != nil {
		t.Error("ItemAt(0,0) should be nil")
	}
	l.RemoveItem(it)
	if l.ItemAt(Pos{2, 2}) != nil {
		t.Error("item should be gone after RemoveItem")
	}
}
```

- [ ] **Step 3** — `go/internal/game/item.go`:
```go
package game

import "github.com/c0ze/tsl/internal/content"

// Item is an item instance (on the ground while Pos is meaningful, or carried).
type Item struct {
	Def *content.ItemDef
	Pos Pos
}

// Behavior is an item effect, looked up by name from Game.Behaviors and invoked
// when an item is used. It returns log messages. Declared here (the engine
// core) so behaviors can be implemented elsewhere and injected without a cycle.
type Behavior func(g *Game, it *Item) []string

// ItemAt returns the first item lying on p, or nil.
func (l *Level) ItemAt(p Pos) *Item {
	for _, it := range l.Items {
		if it.Pos == p {
			return it
		}
	}
	return nil
}

// RemoveItem removes it from the ground (no-op if absent).
func (l *Level) RemoveItem(it *Item) {
	for i, x := range l.Items {
		if x == it {
			l.Items = append(l.Items[:i], l.Items[i+1:]...)
			return
		}
	}
}
```

- [ ] **Step 4** — `go test ./internal/game/` → PASS. **Commit:**
```bash
git add go/internal/game/item.go go/internal/game/item_test.go go/internal/game/game.go
git commit -m "feat(game): Item type, ground-item list, Behavior func, inventory fields"
```

---

## Task 3: Pickup, use/equip, equipment-aware combat (`game`)

**Files:** Modify `go/internal/game/combat.go`; create `go/internal/game/use.go`, `go/internal/game/use_test.go`

- [ ] **Step 1** — in `combat.go`, make the player's melee read equipped gear. Add helpers and use them in `playerAttacks`/`monsterAttacks`:
```go
func (g *Game) playerAttackStat() int {
	if g.Weapon != nil {
		return playerAttack + g.Weapon.Def.Attack
	}
	return playerAttack
}

func (g *Game) playerDamageSpec() string {
	if g.Weapon != nil && g.Weapon.Def.Damage != "" {
		return g.Weapon.Def.Damage
	}
	return playerDamage
}

func (g *Game) playerDodgeStat() int {
	if g.Armor != nil {
		return playerDodge + g.Armor.Def.Dodge
	}
	return playerDodge
}
```
Then change `playerAttacks` to use `g.playerAttackStat()` and `g.playerDamageSpec()` (replace the `playerAttack` and `playerDamage` references in that function), and change `monsterAttacks` to use `g.playerDodgeStat()` (replace `playerDodge` there).

- [ ] **Step 2** — `go/internal/game/use_test.go`:
```go
package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func useGame() *Game {
	c := testContent()
	return &Game{
		Content: c, Level: NewLevel(8, 3, c.Tiles["floor"]), Player: Pos{1, 1},
		RNG: rng.NewWithSeed(1), PlayerHP: 10, PlayerMax: 20,
		Behaviors: map[string]Behavior{
			"heal": func(g *Game, it *Item) []string {
				g.PlayerHP += it.Def.Power
				if g.PlayerHP > g.PlayerMax {
					g.PlayerHP = g.PlayerMax
				}
				return []string{"healed"}
			},
		},
	}
}

func TestPickupMovesItemToInventory(t *testing.T) {
	g := useGame()
	it := &Item{Def: &content.ItemDef{ID: "dagger", Name: "dagger", Glyph: ")", Kind: "weapon"}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, it)
	g.PlayerPickup()
	if len(g.Inventory) != 1 || g.Inventory[0] != it {
		t.Fatal("item should be in inventory")
	}
	if g.Level.ItemAt(g.Player) != nil {
		t.Error("item should be off the ground")
	}
}

func TestUseWeaponEquipsAndBoostsDamage(t *testing.T) {
	g := useGame()
	dagger := &Item{Def: &content.ItemDef{ID: "dagger", Name: "dagger", Kind: "weapon", Attack: 3, Damage: "5d1"}}
	g.Inventory = append(g.Inventory, dagger)
	g.PlayerUse(dagger)
	if g.Weapon != dagger {
		t.Fatal("dagger should be equipped")
	}
	if g.playerDamageSpec() != "5d1" {
		t.Errorf("damage spec = %q, want 5d1 from weapon", g.playerDamageSpec())
	}
}

func TestUsePotionHealsAndIsConsumed(t *testing.T) {
	g := useGame()
	potion := &Item{Def: &content.ItemDef{ID: "healing_potion", Name: "healing potion", Kind: "potion", Use: "heal", Power: 5}}
	g.Inventory = append(g.Inventory, potion)
	g.PlayerUse(potion)
	if g.PlayerHP != 15 {
		t.Errorf("HP = %d, want 15 (10+5)", g.PlayerHP)
	}
	if len(g.Inventory) != 0 {
		t.Error("potion should be consumed")
	}
}
```

- [ ] **Step 3** — `go/internal/game/use.go`:
```go
package game

// PlayerPickup picks up the item under the player, then passes the turn.
func (g *Game) PlayerPickup() {
	if g.Dead {
		return
	}
	it := g.Level.ItemAt(g.Player)
	if it == nil {
		g.log("There is nothing here to pick up.")
		return
	}
	g.Level.RemoveItem(it)
	g.Inventory = append(g.Inventory, it)
	g.log("You pick up the %s.", it.Def.Name)
	g.monstersAct()
}

// PlayerUse equips a weapon/armor or invokes a consumable's behavior, then
// passes the turn.
func (g *Game) PlayerUse(it *Item) {
	if g.Dead {
		return
	}
	switch it.Def.Kind {
	case "weapon":
		g.Weapon = it
		g.log("You wield the %s.", it.Def.Name)
	case "armor":
		g.Armor = it
		g.log("You wear the %s.", it.Def.Name)
	case "potion":
		if b, ok := g.Behaviors[it.Def.Use]; ok {
			g.Messages = append(g.Messages, b(g, it)...)
		} else {
			g.log("Nothing happens.")
		}
		g.removeInventory(it)
	}
	g.monstersAct()
}

func (g *Game) removeInventory(it *Item) {
	for i, x := range g.Inventory {
		if x == it {
			g.Inventory = append(g.Inventory[:i], g.Inventory[i+1:]...)
			return
		}
	}
}
```

- [ ] **Step 4** — `go test ./internal/game/` → PASS. **Commit:**
```bash
git add go/internal/game/combat.go go/internal/game/use.go go/internal/game/use_test.go
git commit -m "feat(game): pickup, use/equip, and equipment-aware combat"
```

---

## Task 4: Behavior registry (`behaviors`)

**Files:** Create `go/internal/behaviors/behaviors.go`, `go/internal/behaviors/behaviors_test.go`

- [ ] **Step 1** — `go/internal/behaviors/behaviors_test.go`:
```go
package behaviors

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func TestHealCapsAtMax(t *testing.T) {
	reg := Registry()
	heal, ok := reg["heal"]
	if !ok {
		t.Fatal("registry missing heal")
	}
	g := &game.Game{PlayerHP: 18, PlayerMax: 20}
	it := &game.Item{Def: &content.ItemDef{Name: "healing potion", Power: 8}}
	msgs := heal(g, it)
	if g.PlayerHP != 20 {
		t.Errorf("HP = %d, want capped at 20", g.PlayerHP)
	}
	if len(msgs) == 0 {
		t.Error("heal should return a message")
	}
}
```

- [ ] **Step 2** — `go/internal/behaviors/behaviors.go`:
```go
// Package behaviors implements named item/ability effects as game.Behaviors and
// exposes them via Registry(). It imports game (to act on the world); game does
// not import it — cmd injects the registry into the Game, avoiding a cycle.
package behaviors

import (
	"fmt"

	"github.com/c0ze/tsl/internal/game"
)

// Registry returns the name→behavior map referenced by item `use` fields.
func Registry() map[string]game.Behavior {
	return map[string]game.Behavior{
		"heal": heal,
	}
}

func heal(g *game.Game, it *game.Item) []string {
	amt := it.Def.Power
	g.PlayerHP += amt
	if g.PlayerHP > g.PlayerMax {
		g.PlayerHP = g.PlayerMax
	}
	return []string{fmt.Sprintf("You quaff the %s and recover %d HP.", it.Def.Name, amt)}
}
```

- [ ] **Step 3** — `go test ./internal/behaviors/` → PASS. **Commit:**
```bash
git add go/internal/behaviors/
git commit -m "feat(behaviors): registry with heal effect"
```

---

## Task 5: Place items during generation (`gen`)

**Files:** Modify `go/internal/gen/gen.go`, `go/internal/gen/gen_test.go`

- [ ] **Step 1** — append to `go/internal/gen/gen_test.go`:
```go
func TestRoomsPlacesItems(t *testing.T) {
	c := testContent()
	c.Items = map[string]*content.ItemDef{
		"potion": {ID: "potion", Name: "potion", Glyph: "!", Color: content.ColorRed, Kind: "potion", Use: "heal", Power: 8},
	}
	l, _, _, err := Rooms(rng.NewWithSeed(3), c, 60, 24)
	if err != nil {
		t.Fatal(err)
	}
	if len(l.Items) == 0 {
		t.Error("expected at least one item placed")
	}
	for _, it := range l.Items {
		if !l.Passable(it.Pos) {
			t.Errorf("item on impassable tile at %v", it.Pos)
		}
	}
}
```

- [ ] **Step 2** — in `gen.go`, after `placeMonsters(...)`, add `placeItems(r, c, lvl, rooms, start)`, and add the helper:
```go
// placeItems drops up to one item into each room except the starting room.
func placeItems(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, start game.Pos) {
	ids := make([]string, 0, len(c.Items))
	for id := range c.Items {
		ids = append(ids, id)
	}
	if len(ids) == 0 {
		return
	}
	sort.Strings(ids)
	for i, room := range rooms {
		if i == 0 || r.Intn(2) == 0 { // ~half the rooms, never the start
			continue
		}
		pos := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if pos == start || !lvl.Passable(pos) || lvl.ItemAt(pos) != nil {
			continue
		}
		def := c.Items[ids[r.Intn(len(ids))]]
		lvl.Items = append(lvl.Items, &game.Item{Def: def, Pos: pos})
	}
}
```

- [ ] **Step 3** — `go test ./internal/gen/` → PASS. **Commit:**
```bash
git add go/internal/gen/
git commit -m "feat(gen): scatter items in generated rooms"
```

---

## Task 6: Inventory menu + item rendering (`ui`)

**Files:** Modify `go/internal/ui/ui.go`, `go/internal/ui/ui_test.go`

- [ ] **Step 1** — append to `go/internal/ui/ui_test.go` (and note: the existing `scriptPrompter` must gain a `Menu` method — add it):
```go
// Add to scriptPrompter (existing type) a Menu method via this helper type used
// in the new test; if scriptPrompter lacks Menu, add: func (s *scriptPrompter) Menu(MenuSpec) (int, bool) { return 0, false }

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
```
Also add a `Menu` method to the existing `scriptPrompter` so it still satisfies `Prompter`:
```go
func (s *scriptPrompter) Menu(MenuSpec) (int, bool) { return 0, false }
```

- [ ] **Step 2** — in `go/internal/ui/ui.go`:

(a) add `MenuSpec` and extend `Prompter`:
```go
// MenuSpec describes a selectable list for the front-end to present.
type MenuSpec struct {
	Title string
	Items []string
}

// Prompter supplies player actions and menu selections.
type Prompter interface {
	NextAction() (Action, error)
	Menu(MenuSpec) (index int, ok bool)
}
```

(b) extend `ActionKind` with pickup/inventory:
```go
const (
	ActNone ActionKind = iota
	ActMove
	ActQuit
	ActPickup
	ActInventory
)
```

(c) in `BuildView`, render ground items just before monsters (so monster/player draw on top). Insert the items loop immediately before the `for _, m := range l.Creatures` loop:
```go
	for _, it := range l.Items {
		if l.At(it.Pos).Visible {
			*v.At(it.Pos.X, it.Pos.Y) = Cell{Glyph: it.Def.Rune(), Color: it.Def.Color}
		}
	}
```

(d) handle the new actions in `Run`'s switch:
```go
		case ActPickup:
			g.PlayerPickup()
		case ActInventory:
			if len(g.Inventory) > 0 {
				names := make([]string, len(g.Inventory))
				for i, it := range g.Inventory {
					names[i] = it.Def.Name
				}
				if idx, ok := p.Menu(MenuSpec{Title: "Inventory", Items: names}); ok {
					g.PlayerUse(g.Inventory[idx])
				}
			}
```

- [ ] **Step 3** — `go test ./internal/ui/` → PASS. **Commit:**
```bash
git add go/internal/ui/ui.go go/internal/ui/ui_test.go
git commit -m "feat(ui): inventory menu prompt, pickup/inventory actions, item rendering"
```

---

## Task 7: Menu + keys in the tcell front-end (`ui/tcell`)

**Files:** Modify `go/internal/ui/tcell/screen.go`

- [ ] **Step 1** — add `'g'`→`ActPickup` and `'i'`→`ActInventory` to `keyToAction` (alongside the existing rune cases):
```go
	case 'g':
		return ui.Action{Kind: ui.ActPickup}, true
	case 'i':
		return ui.Action{Kind: ui.ActInventory}, true
```

- [ ] **Step 2** — implement `Menu` on `*Screen` (so it satisfies the extended `ui.Prompter`):
```go
// Menu presents a blocking list; arrows/jk move, Enter/letter selects, Esc/q cancels.
func (sc *Screen) Menu(m ui.MenuSpec) (int, bool) {
	if len(m.Items) == 0 {
		return 0, false
	}
	sel := 0
	for {
		sc.s.Clear()
		drawString(sc.s, 0, 0, m.Title)
		for i, it := range m.Items {
			prefix := "  "
			if i == sel {
				prefix = "> "
			}
			drawString(sc.s, 0, i+1, fmt.Sprintf("%s%c) %s", prefix, 'a'+i, it))
		}
		sc.s.Show()
		ev, ok := sc.s.PollEvent().(*tc.EventKey)
		if !ok {
			continue
		}
		switch {
		case ev.Key() == tc.KeyUp || ev.Rune() == 'k':
			sel = (sel - 1 + len(m.Items)) % len(m.Items)
		case ev.Key() == tc.KeyDown || ev.Rune() == 'j':
			sel = (sel + 1) % len(m.Items)
		case ev.Key() == tc.KeyEnter:
			return sel, true
		case ev.Key() == tc.KeyEscape || ev.Rune() == 'q':
			return 0, false
		case ev.Rune() >= 'a' && int(ev.Rune()-'a') < len(m.Items):
			return int(ev.Rune() - 'a'), true
		}
	}
}
```
Add `"fmt"` to the imports of `screen.go`.

- [ ] **Step 3** — Verify whole module from `go/`:
```bash
export GOTOOLCHAIN=local
gofmt -l .
go build ./... && go test ./... -count=1 && go vet ./...
go build -o /tmp/tsl_p5 ./cmd/tsl && echo "binary OK" && rm -f /tmp/tsl_p5
```
Expected: gofmt clean, all packages `ok`, vet clean, binary builds.

- [ ] **Step 4** — Commit:
```bash
git add go/internal/ui/tcell/screen.go
git commit -m "feat(ui/tcell): inventory menu and pickup/inventory keys"
```

---

## Task 8: Wire the behavior registry (`cmd/tsl`)

**Files:** Modify `go/cmd/tsl/main.go`, `go/cmd/tsl/main_test.go`

- [ ] **Step 1** — append to `go/cmd/tsl/main_test.go`:
```go
func TestNewGameHasBehaviors(t *testing.T) {
	g, err := newGame(testTiles(), 1)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if g.Behaviors["heal"] == nil {
		t.Error("expected heal behavior wired into the game")
	}
}
```

- [ ] **Step 2** — in `newGame` (`main.go`), inject the registry. Add `"github.com/c0ze/tsl/internal/behaviors"` to imports and set the field in the returned `&game.Game{...}`:
```go
		PlayerHP:  startHP,
		PlayerMax: startHP,
		Behaviors: behaviors.Registry(),
```

- [ ] **Step 3** — Whole module green from `go/`: `export GOTOOLCHAIN=local && go build ./... && go test ./... -count=1 && go vet ./...`. **Commit:**
```bash
git add go/cmd/tsl/
git commit -m "feat(cmd): inject the behavior registry into the game"
```

---

## Done criteria (Plan 5)
- Items load from `data/items.toml`; `gen` scatters them; they render on visible tiles.
- `g` picks up the item underfoot; `i` opens an inventory menu.
- Selecting a weapon/armor equips it (combat uses its attack/damage/dodge); selecting a potion invokes its `use` behavior via the injected registry and consumes it.
- `behaviors.Registry()` supplies `heal`; `cmd` injects it; `game` never imports `behaviors`.
- `go test ./...`, `go vet`, gofmt all clean.

## Deferred
- Wands + interactive targeting (the `Target` prompt); stacking; identification; richer item kinds and effects — follow-ups.
- A real HP/status bar (HP is currently only surfaced via messages).
