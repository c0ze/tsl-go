# Eating & Corpses Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Killing a monster drops an edible corpse; an `e` command lets the player eat corpses/food to heal. (GitHub issue #12.)

**Architecture:** Stay data-driven. Food is a new item `kind` consumed through the existing behavior registry (a new `eat` behavior, mirroring `heal`). Monsters gain an optional `corpse` field naming the food item they drop on death; death is centralised in a new `Game.killCreature`. A new `ActEat` UI action filters the inventory to food and reuses the existing menu→`PlayerUse` plumbing. Corpses are flagged `nospawn` so they only appear via drops, never as floor loot. All content validates fail-fast at load.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`), BurntSushi/toml, gdamore/tcell v2.7.4. Module root: `/Users/arda/projects/tsl/go`.

**Conventions:**
- All commands run from `/Users/arda/projects/tsl/go`.
- Prefix every go command with `GOTOOLCHAIN=local`. If you see a GOROOT mismatch error, also prepend `env -u GOROOT`.
- TDD: write the failing test, watch it fail, implement, watch it pass, commit. One concern per commit.
- Branch: `eating-corpses` off `master`. Do **not** commit to `master`.

**Faithfulness note (C reference):** `common/eat.c` heals corpse/carcass +1, butchered food (meat/bread/cheese) +3, red mushroom +4 then poison. `common/monster.c` sets `creature->corpse`; `common/creature.c:494` drops it via `build_item`. We port the heal-on-eat + corpse-drop core; the mushroom's **poison** and joke-eats (sword→death) are deferred to the status-effect work in #15/#18.

---

## Task 0: Branch

**Step 1:** Create the working branch.
```bash
cd /Users/arda/projects/tsl/go && git checkout master && git pull --ff-only && git checkout -b eating-corpses
```
Expected: "Switched to a new branch 'eating-corpses'".

---

## Task 1: `food` item kind + `nospawn` flag (content)

**Files:**
- Modify: `internal/content/content.go`
- Test: `internal/content/content_test.go`

**Step 1: Write failing tests.** Append to `content_test.go`:
```go
func TestLoadFoodItem(t *testing.T) {
	dir := t.TempDir()
	must := func(name, body string) {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	must("tiles.toml", "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n")
	must("items.toml", "[item.ration]\nname=\"ration\"\nglyph=\"%\"\ncolor=\"brown\"\nkind=\"food\"\nuse=\"eat\"\npower=5\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	r, ok := c.Items["ration"]
	if !ok || r.Kind != "food" || r.Use != "eat" || r.Power != 5 {
		t.Errorf("unexpected food def: %+v", r)
	}
}

func TestLoadRejectsFoodWithoutUse(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "items.toml"), []byte("[item.bad]\nname=\"bad\"\nglyph=\"%\"\ncolor=\"brown\"\nkind=\"food\"\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for food with empty use")
	}
}
```

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/content/ -run TestLoadFood -v
```
Expected: compile error / FAIL (kind "food" rejected, `nospawn` field absent).

**Step 3: Implement.** In `content.go`:
- Add the `NoSpawn` field to `ItemDef` (after `Damage`):
```go
	Damage  string `toml:"damage"`  // weapon damage spec
	NoSpawn bool   `toml:"nospawn"` // exclude from random floor loot (e.g. corpses)
```
- Add `"food"` to `validItemKinds`:
```go
var validItemKinds = map[string]bool{"potion": true, "weapon": true, "armor": true, "food": true}
```
- In `validateItem`, add a `food` branch to the `switch i.Kind`:
```go
	case "food":
		if strings.TrimSpace(i.Use) == "" {
			return fmt.Errorf("food must have a non-empty use")
		}
		if i.Power < 0 {
			return fmt.Errorf("food power must be >= 0, got %d", i.Power)
		}
```

**Step 4: Run, expect PASS.**
```bash
GOTOOLCHAIN=local go test ./internal/content/ -run TestLoadFood -v
```

**Step 5: Commit.**
```bash
git add internal/content/content.go internal/content/content_test.go
git commit -m "feat(content): food item kind + nospawn flag"
```

---

## Task 2: Monster `corpse` field + corpse-reference validation (content)

**Files:**
- Modify: `internal/content/content.go`
- Test: `internal/content/content_test.go`

**Step 1: Write failing tests.** Append to `content_test.go`:
```go
func writeCorpseFixture(t *testing.T, monsterBody string) string {
	t.Helper()
	dir := t.TempDir()
	w := func(name, body string) {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	w("tiles.toml", "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n")
	w("items.toml", "[item.rat_corpse]\nname=\"rat corpse\"\nglyph=\"%\"\ncolor=\"brown\"\nkind=\"food\"\nuse=\"eat\"\npower=3\nnospawn=true\n\n[item.dagger]\nname=\"dagger\"\nglyph=\")\"\ncolor=\"normal\"\nkind=\"weapon\"\nattack=2\ndamage=\"1d4\"\n")
	w("monsters.toml", monsterBody)
	return dir
}

func TestLoadMonsterCorpseRef(t *testing.T) {
	dir := writeCorpseFixture(t, "[monster.rat]\nname=\"rat\"\nglyph=\"r\"\ncolor=\"brown\"\nhp=3\nattack=2\ndodge=1\ndamage=\"1d2\"\ncorpse=\"rat_corpse\"\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if c.Monsters["rat"].Corpse != "rat_corpse" {
		t.Errorf("corpse = %q, want rat_corpse", c.Monsters["rat"].Corpse)
	}
}

func TestLoadRejectsUnknownCorpse(t *testing.T) {
	dir := writeCorpseFixture(t, "[monster.rat]\nname=\"rat\"\nglyph=\"r\"\ncolor=\"brown\"\nhp=3\nattack=2\ndodge=1\ndamage=\"1d2\"\ncorpse=\"nope\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for unknown corpse item")
	}
}

func TestLoadRejectsNonFoodCorpse(t *testing.T) {
	dir := writeCorpseFixture(t, "[monster.rat]\nname=\"rat\"\nglyph=\"r\"\ncolor=\"brown\"\nhp=3\nattack=2\ndodge=1\ndamage=\"1d2\"\ncorpse=\"dagger\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: corpse must reference a food item")
	}
}
```

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/content/ -run 'TestLoad.*Corpse' -v
```

**Step 3: Implement.** In `content.go`:
- Add the `Corpse` field to `MonsterDef` (after `Speed`):
```go
	Speed  int    `toml:"speed"`  // energy gained per turn; <= 0 defaults to 100
	Corpse string `toml:"corpse"` // item id dropped on death ("" = none); must be a food item
```
- At the end of `Load`, just before `return c, nil`, add the cross-reference check:
```go
	if err := validateCorpseRefs(c); err != nil {
		return nil, err
	}
	return c, nil
}

// validateCorpseRefs checks that every monster's corpse (when set) names a
// defined food item, so bad content fails at load instead of at the kill.
func validateCorpseRefs(c *Content) error {
	for id, m := range c.Monsters {
		if m.Corpse == "" {
			continue
		}
		it, ok := c.Items[m.Corpse]
		if !ok {
			return fmt.Errorf("monster %q: corpse %q is not a defined item", id, m.Corpse)
		}
		if it.Kind != "food" {
			return fmt.Errorf("monster %q: corpse %q must be a food item, got kind %q", id, m.Corpse, it.Kind)
		}
	}
	return nil
}
```
(Replace the existing bare `return c, nil` with the block above.)

**Step 4: Run, expect PASS** (and full content package green):
```bash
GOTOOLCHAIN=local go test ./internal/content/ -v
```

**Step 5: Commit.**
```bash
git add internal/content/content.go internal/content/content_test.go
git commit -m "feat(content): monster corpse field + fail-fast corpse-ref validation"
```

---

## Task 3: `eat` behavior (behaviors)

**Files:**
- Modify: `internal/behaviors/behaviors.go`
- Test: `internal/behaviors/behaviors_test.go`

**Step 1: Write failing test.** Append to `behaviors_test.go`:
```go
func TestEatRestoresHPClamped(t *testing.T) {
	reg := Registry()
	eat, ok := reg["eat"]
	if !ok {
		t.Fatal("eat behavior not registered")
	}
	g := &game.Game{PlayerHP: 18, PlayerMax: 20}
	it := &game.Item{Def: &content.ItemDef{Name: "rat corpse", Power: 5}}
	msgs := eat(g, it)
	if g.PlayerHP != 20 {
		t.Errorf("HP = %d, want 20 (clamped)", g.PlayerHP)
	}
	if len(msgs) == 0 {
		t.Error("expected an eat message")
	}
}
```
(Match the existing import block in `behaviors_test.go`; it already imports `game` and `content`.)

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/behaviors/ -run TestEat -v
```

**Step 3: Implement.** Rewrite `behaviors.go` body so `heal` and `eat` share a clamped restore helper (DRY):
```go
// Registry returns the name→behavior map referenced by item `use` fields.
func Registry() map[string]game.Behavior {
	return map[string]game.Behavior{
		"heal": heal,
		"eat":  eat,
	}
}

// restoreHP adds amount to the player's HP, clamped to PlayerMax, and reports
// how much was actually recovered.
func restoreHP(g *game.Game, amount int) int {
	before := g.PlayerHP
	g.PlayerHP += amount
	if g.PlayerHP > g.PlayerMax {
		g.PlayerHP = g.PlayerMax
	}
	return g.PlayerHP - before
}

func heal(g *game.Game, it *game.Item) []string {
	n := restoreHP(g, it.Def.Power)
	return []string{fmt.Sprintf("You quaff the %s and recover %d HP.", it.Def.Name, n)}
}

func eat(g *game.Game, it *game.Item) []string {
	n := restoreHP(g, it.Def.Power)
	if n > 0 {
		return []string{fmt.Sprintf("You eat the %s and recover %d HP.", it.Def.Name, n)}
	}
	return []string{fmt.Sprintf("You eat the %s.", it.Def.Name)}
}
```

**Step 4: Run, expect PASS.**
```bash
GOTOOLCHAIN=local go test ./internal/behaviors/ -v
```

**Step 5: Commit.**
```bash
git add internal/behaviors/behaviors.go internal/behaviors/behaviors_test.go
git commit -m "feat(behaviors): eat behavior (shared clamped restoreHP)"
```

---

## Task 4: Corpse drop on death — `killCreature` (engine)

**Files:**
- Modify: `internal/game/combat.go`
- Test: `internal/game/combat_test.go`

**Step 1: Write failing test.** Append to `combat_test.go`:
```go
func TestKillCreatureDropsCorpse(t *testing.T) {
	food := &content.ItemDef{ID: "rat_corpse", Name: "rat corpse", Glyph: "%", Color: content.ColorBrown, Kind: "food", Use: "eat", Power: 3}
	c := &content.Content{Items: map[string]*content.ItemDef{"rat_corpse": food}}
	lvl := game.NewLevel(3, 1, &content.TileDef{ID: "floor", Glyph: ".", Passable: true, Transparent: true})
	g := &game.Game{Content: c, Level: lvl}
	m := &game.Creature{Def: &content.MonsterDef{Name: "rat", Corpse: "rat_corpse"}, Pos: game.Pos{X: 1, Y: 0}, HP: 1}
	lvl.Creatures = append(lvl.Creatures, m)

	g.KillCreatureForTest(m) // thin exported wrapper added below

	if lvl.CreatureAt(game.Pos{X: 1, Y: 0}) != nil {
		t.Error("creature should be removed")
	}
	it := lvl.ItemAt(game.Pos{X: 1, Y: 0})
	if it == nil || it.Def.ID != "rat_corpse" {
		t.Errorf("expected rat_corpse dropped at kill site, got %v", it)
	}
}

func TestKillCreatureNoCorpse(t *testing.T) {
	lvl := game.NewLevel(3, 1, &content.TileDef{ID: "floor", Glyph: ".", Passable: true, Transparent: true})
	g := &game.Game{Level: lvl}
	m := &game.Creature{Def: &content.MonsterDef{Name: "ghost"}, Pos: game.Pos{X: 1, Y: 0}, HP: 1}
	lvl.Creatures = append(lvl.Creatures, m)
	g.KillCreatureForTest(m)
	if lvl.ItemAt(game.Pos{X: 1, Y: 0}) != nil {
		t.Error("monster with no corpse should drop nothing")
	}
}
```

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/game/ -run TestKillCreature -v
```

**Step 3: Implement.** In `combat.go`, replace the kill branch of `playerAttacks`:
```go
	if m.HP <= 0 {
		g.killCreature(m)
	}
```
and add:
```go
// killCreature resolves a monster's death: announce it, drop its corpse (when
// the def names one), and remove it from the level. Centralising death here
// keeps every kill site (player, future hazards) dropping corpses consistently.
func (g *Game) killCreature(m *Creature) {
	g.log("The %s dies.", m.Def.Name)
	if m.Def.Corpse != "" && g.Content != nil {
		if def, ok := g.Content.Items[m.Def.Corpse]; ok {
			g.Level.Items = append(g.Level.Items, &Item{Def: def, Pos: m.Pos})
		}
	}
	g.Level.RemoveCreature(m)
}

// KillCreatureForTest exposes killCreature to package-external tests.
func (g *Game) KillCreatureForTest(m *Creature) { g.killCreature(m) }
```
Remove the now-duplicated `g.log("The %s dies.", ...)` and `g.Level.RemoveCreature(m)` lines that were inline in `playerAttacks`.

**Step 4: Run, expect PASS** (and the whole game package — existing combat tests still pass because their monsters have `Corpse == ""`):
```bash
GOTOOLCHAIN=local go test ./internal/game/ -run 'TestKill|TestPlayer|TestMonster' -v
```

**Step 5: Commit.**
```bash
git add internal/game/combat.go internal/game/combat_test.go
git commit -m "feat(game): centralise death in killCreature; drop corpses on kill"
```

---

## Task 5: Consume food + extend startup validation (engine)

**Files:**
- Modify: `internal/game/use.go`
- Test: `internal/game/use_test.go`

**Step 1: Write failing test.** Append to `use_test.go` (mirror the existing potion test):
```go
func TestPlayerUseFoodEatsAndRemoves(t *testing.T) {
	g := newUseTestGame(t) // reuse whatever helper use_test.go already defines; otherwise build a Game with a 1-tile level
	g.PlayerHP, g.PlayerMax = 4, 20
	g.Behaviors = map[string]game.Behavior{"eat": func(gg *game.Game, it *game.Item) []string {
		gg.PlayerHP += it.Def.Power
		return []string{"munch"}
	}}
	food := &game.Item{Def: &content.ItemDef{Name: "rat corpse", Kind: "food", Use: "eat", Power: 3}}
	g.Inventory = append(g.Inventory, food)
	g.PlayerUse(food)
	if g.PlayerHP != 7 {
		t.Errorf("HP = %d, want 7", g.PlayerHP)
	}
	if len(g.Inventory) != 0 {
		t.Errorf("food should be consumed, inventory = %v", g.Inventory)
	}
}

func TestValidateItemUsesCoversFood(t *testing.T) {
	c := &content.Content{Items: map[string]*content.ItemDef{
		"corpse": {ID: "corpse", Kind: "food", Use: "no_such_behavior"},
	}}
	if err := game.ValidateItemUses(c, map[string]game.Behavior{}); err == nil {
		t.Fatal("expected error: food references unknown behavior")
	}
}
```
> If `use_test.go` has no `newUseTestGame` helper, construct the Game inline the same way the existing potion test in that file does (look at `TestRunInventoryUsesSelectedItem` style / the existing use_test setup) and drop the helper call.

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/game/ -run 'TestPlayerUseFood|TestValidateItemUsesCoversFood' -v
```

**Step 3: Implement.**
- In `use.go` `PlayerUse`, change the potion case to also handle food:
```go
	case "potion", "food":
		if b, ok := g.Behaviors[it.Def.Use]; ok {
			g.Messages = append(g.Messages, b(g, it)...)
		} else {
			g.log("Nothing happens.")
		}
		g.removeInventory(it)
```
- Add the edible-inventory query (used by the UI next task):
```go
// EdibleInventory returns the food items the player is carrying, in inventory
// order.
func (g *Game) EdibleInventory() []*Item {
	var food []*Item
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.Kind == "food" {
			food = append(food, it)
		}
	}
	return food
}
```
- In `internal/game/item.go`, extend `ValidateItemUses` to cover food:
```go
		if it.Kind == "potion" || it.Kind == "food" {
			if _, ok := reg[it.Use]; !ok {
				return fmt.Errorf("item %q references unknown behavior %q", id, it.Use)
			}
		}
```

**Step 4: Run, expect PASS.**
```bash
GOTOOLCHAIN=local go test ./internal/game/ -v
```

**Step 5: Commit.**
```bash
git add internal/game/use.go internal/game/item.go internal/game/use_test.go
git commit -m "feat(game): eat food via PlayerUse; EdibleInventory; validate food uses"
```

---

## Task 6: Eat command — `ActEat` + key (UI)

**Files:**
- Modify: `internal/ui/ui.go`, `internal/ui/tcell/screen.go`
- Test: `internal/ui/ui_test.go`, `internal/ui/tcell/screen_test.go`

**Step 1: Write failing tests.**
`ui_test.go`:
```go
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
```
`screen_test.go` — add a case to the `TestKeyToAction` table:
```go
		{tc.NewEventKey(tc.KeyRune, 'e', tc.ModNone), ui.Action{Kind: ui.ActEat}, true},
```

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/ui/... -run 'TestRunEat|TestKeyToAction' -v
```

**Step 3: Implement.**
- `ui.go`: add `ActEat` to the `ActionKind` const block (after `ActDescend`):
```go
	ActDescend
	ActEat
```
- `ui.go`: in `Run`'s action switch, add:
```go
		case ActEat:
			food := g.EdibleInventory()
			if len(food) == 0 {
				g.Messages = append(g.Messages, "You have nothing to eat.")
				break
			}
			names := make([]string, len(food))
			for i, it := range food {
				names[i] = it.Def.Name
			}
			if idx, ok := p.Menu(MenuSpec{Title: "Eat what?", Items: names}); ok && idx >= 0 && idx < len(food) {
				g.PlayerUse(food[idx])
			}
```
- `screen.go`: in `keyToAction`'s rune switch, add (next to `'i'`):
```go
		case 'e':
			return ui.Action{Kind: ui.ActEat}, true
```

**Step 4: Run, expect PASS.**
```bash
GOTOOLCHAIN=local go test ./internal/ui/... -v
```

**Step 5: Commit.**
```bash
git add internal/ui/ui.go internal/ui/tcell/screen.go internal/ui/ui_test.go internal/ui/tcell/screen_test.go
git commit -m "feat(ui): e command eats food from inventory"
```

---

## Task 7: Generator skips non-spawnable items (gen)

**Files:**
- Modify: `internal/gen/gen.go`
- Test: `internal/gen/gen_test.go`

**Step 1: Write failing test.** Append to `gen_test.go` a check that a `nospawn` item is never placed. (Use the existing test helpers in that file for building a Content + RNG; mirror an existing gen test's setup.) Minimal assertion:
```go
func TestPlaceItemsSkipsNoSpawn(t *testing.T) {
	c := genTestContent() // existing helper if present; else build tiles+items inline like other gen tests
	for _, it := range c.Items {
		it.NoSpawn = true // make every item non-spawnable
	}
	r := rng.NewWithSeed(1)
	lvl, _, _, err := Rooms(r, c, 40, 20)
	if err != nil {
		t.Fatal(err)
	}
	if len(lvl.Items) != 0 {
		t.Errorf("no items should spawn when all are nospawn, got %d", len(lvl.Items))
	}
}
```
> Adapt `genTestContent()` to whatever the existing gen tests use to construct `*content.Content`.

**Step 2: Run, expect FAIL.**
```bash
GOTOOLCHAIN=local go test ./internal/gen/ -run TestPlaceItemsSkipsNoSpawn -v
```

**Step 3: Implement.** In `gen.go` `placeItems`, skip non-spawnable items when building `ids`:
```go
	ids := make([]string, 0, len(c.Items))
	for id, it := range c.Items {
		if it.NoSpawn {
			continue
		}
		ids = append(ids, id)
	}
```

**Step 4: Run, expect PASS.**
```bash
GOTOOLCHAIN=local go test ./internal/gen/ -v
```

**Step 5: Commit.**
```bash
git add internal/gen/gen.go internal/gen/gen_test.go
git commit -m "feat(gen): exclude nospawn items (corpses) from floor loot"
```

---

## Task 8: Content data + gitignore

**Files:**
- Modify: `data/items.toml`, `data/monsters.toml`, repo-root `.gitignore`

**Step 1: Add food items.** Append to `data/items.toml`:
```toml
[item.ration]
name = "ration"
glyph = "%"
color = "brown"
kind = "food"
use = "eat"
power = 5

[item.rat_corpse]
name = "rat corpse"
glyph = "%"
color = "brown"
kind = "food"
use = "eat"
power = 3
nospawn = true

[item.ghoul_corpse]
name = "ghoul corpse"
glyph = "%"
color = "green"
kind = "food"
use = "eat"
power = 1
nospawn = true
```

**Step 2: Give monsters corpses.** In `data/monsters.toml`, add to `[monster.rat]`:
```toml
corpse = "rat_corpse"
```
and to `[monster.ghoul]`:
```toml
corpse = "ghoul_corpse"
```

**Step 3: Ignore the morgue artifact.** Append to repo-root `/Users/arda/projects/tsl/.gitignore`:
```
morgue.txt
```

**Step 4: Validate content loads (fail-fast paths exercised) + build.**
```bash
GOTOOLCHAIN=local go build ./... && GOTOOLCHAIN=local go run ./cmd/tsl --help 2>/dev/null; echo "build ok"
```
> The binary has no `--help`; the point is that `content.Load` + `ValidateItemUses` + `validateCorpseRefs` run at startup. A cleaner check: add a tiny throwaway `go test` that calls `content.Load(data.Files)` — or rely on `main_test.go` if it already does. If `cmd/tsl/main_test.go` constructs a game, run it:
```bash
GOTOOLCHAIN=local go test ./cmd/... -v
```

**Step 5: Commit.**
```bash
git add data/items.toml data/monsters.toml ../.gitignore
git commit -m "feat(data): edible rations + rat/ghoul corpses; gitignore morgue.txt"
```
> Note the `../.gitignore` path because you are in `go/`. Alternatively `git -C /Users/arda/projects/tsl add .gitignore`.

---

## Task 9: Full verification, manual smoke, push & PR

**Step 1: Whole suite + vet + build.**
```bash
cd /Users/arda/projects/tsl/go && GOTOOLCHAIN=local go vet ./... && GOTOOLCHAIN=local go test ./... && GOTOOLCHAIN=local go build -o /tmp/tsl ./cmd/tsl && echo ALL_GREEN
```
Expected: `ALL_GREEN`.

**Step 2: Relocatable-binary smoke (proves embed.FS still works).**
```bash
cd /tmp && ./tsl < /dev/null; echo "exit: $?"
```
Expected: it starts (tcell may complain about no TTY under `/dev/null`, which is fine — we only need it not to fail on content loading). If it needs a TTY, skip; the `go test ./cmd/...` already exercised content load.

**Step 3: Manual playtest checklist (if a TTY is available).**
- Start the game, kill a rat, confirm a `%` (rat corpse) appears on its tile.
- Walk onto it, press `g` to pick up, press `e`, select "rat corpse", confirm HP rises and the corpse is consumed.
- Press `e` with no food and confirm "You have nothing to eat."

**Step 4: Push the branch.** (Network — controller must pass `dangerouslyDisableSandbox: true`.)
```bash
git push -u origin eating-corpses
```

**Step 5: Open the PR** linking issue #12:
```bash
gh pr create --title "Eating & corpses (#12): drop corpses on death, e to eat" \
  --body "$(cat <<'EOF'
Implements #12.

## What
- Monsters drop an edible **corpse** on death (`MonsterDef.corpse` → a food item).
- New item `kind = "food"`, consumed through the behavior registry via a new `eat` behavior (shares a clamped `restoreHP` helper with `heal`).
- New `e` command: pick a food item from the inventory and eat it to heal.
- Corpses are flagged `nospawn` so they only appear via drops, never as random floor loot.
- Fail-fast content validation: food must name a registered `use`; a monster's `corpse` must reference a defined **food** item.
- Data: edible `ration` (findable), `rat_corpse` (+3), `ghoul_corpse` (+1); rat & ghoul now drop them.
- Chore: gitignore `morgue.txt`.

## Faithfulness
Ports the heal-on-eat + corpse-drop core from `common/eat.c` / `common/monster.c`. The red mushroom's **poison** and joke-eats (sword→death) are deferred to the status-effect work (#15/#18).

## Tests
Content (food kind, corpse refs, validation), behaviors (eat clamp), engine (corpse drop on kill, eat via PlayerUse, food-use validation), gen (nospawn excluded), UI (`e` eats / "nothing to eat", keymap). Closes #12.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

**Step 6: Hand off to the CodeRabbit loop** (receiving-code-review skill): start the background watcher polling `gh pr view <n> --json statusCheckRollup`, address findings on the same branch, re-review, merge with `gh pr merge <n> --merge --delete-branch` when green, then `git checkout master && git pull`.

---

## Done when
- `#12` merged to `master`, suite green.
- In-game: kill a rat → `%` corpse drops → `e` eats it → HP rises. `e` with no food says so.
