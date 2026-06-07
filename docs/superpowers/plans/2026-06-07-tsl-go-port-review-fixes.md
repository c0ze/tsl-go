# TSL Go Port — Review Fixes (fail-fast validation, UI bounds, portable binary)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or executing-plans. Steps use checkbox (`- [ ]`).

**Goal:** Address a post-M1 code review: (1) reject malformed dice specs at load, (2) reject unknown potion `use` references at startup, (3) guard UI rendering against out-of-bounds positions, (4) embed `data/` so the binary runs from any directory.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). No new deps (uses stdlib `embed`/`io/fs`).

## Working dir & environment
- Module in `go/`; `export GOTOOLCHAIN=local`. Branch `review-fixes`. No `go mod tidy`/`go get`.

---

## Task 1: Embed data + load through `fs.FS` (`content`, `data`, `cmd`)

**Files:** Create `go/data/data.go`; modify `go/internal/content/content.go`, `go/internal/content/content_test.go`, `go/cmd/tsl/main.go`

- [ ] **Step 1** — create `go/data/data.go`:
```go
// Package data embeds the game's TOML content so the binary is self-contained
// and runs from any working directory.
package data

import "embed"

//go:embed *.toml
var Files embed.FS
```

- [ ] **Step 2** — in `content.go`, change the import block to (drop `os`/`path/filepath`; add `io/fs`):
```go
import (
	"errors"
	"fmt"
	"io/fs"
	"strconv"
	"strings"
	"unicode/utf8"

	"github.com/BurntSushi/toml"
)
```

- [ ] **Step 3** — replace the entire `Load` function (from `func Load(dir string) (*Content, error) {` through its closing `}`) with this `fs.FS`-based version plus two helpers:
```go
// Load reads and validates all content from fsys (tiles.toml is required;
// monsters.toml and items.toml are optional).
func Load(fsys fs.FS) (*Content, error) {
	c := &Content{
		Tiles:    map[string]*TileDef{},
		Monsters: map[string]*MonsterDef{},
		Items:    map[string]*ItemDef{},
	}

	var tf tilesFile
	if err := decodeTOML(fsys, "tiles.toml", &tf); err != nil {
		return nil, err
	}
	for id, def := range tf.Tile {
		def.ID = id
		if err := validateTile(def); err != nil {
			return nil, fmt.Errorf("tile %q: %w", id, err)
		}
		c.Tiles[id] = def
	}
	if len(c.Tiles) == 0 {
		return nil, fmt.Errorf("tiles.toml: no tiles defined")
	}

	var mf monstersFile
	if ok, err := decodeOptionalTOML(fsys, "monsters.toml", &mf); err != nil {
		return nil, err
	} else if ok {
		for id, def := range mf.Monster {
			def.ID = id
			if err := validateMonster(def); err != nil {
				return nil, fmt.Errorf("monster %q: %w", id, err)
			}
			c.Monsters[id] = def
		}
	}

	var inf itemsFile
	if ok, err := decodeOptionalTOML(fsys, "items.toml", &inf); err != nil {
		return nil, err
	} else if ok {
		for id, def := range inf.Item {
			def.ID = id
			if err := validateItem(def); err != nil {
				return nil, fmt.Errorf("item %q: %w", id, err)
			}
			c.Items[id] = def
		}
	}
	return c, nil
}

func decodeTOML(fsys fs.FS, name string, v any) error {
	b, err := fs.ReadFile(fsys, name)
	if err != nil {
		return fmt.Errorf("reading %s: %w", name, err)
	}
	if _, err := toml.Decode(string(b), v); err != nil {
		return fmt.Errorf("parsing %s: %w", name, err)
	}
	return nil
}

// decodeOptionalTOML decodes name if present; (false, nil) if it does not exist.
func decodeOptionalTOML(fsys fs.FS, name string, v any) (bool, error) {
	b, err := fs.ReadFile(fsys, name)
	if errors.Is(err, fs.ErrNotExist) {
		return false, nil
	}
	if err != nil {
		return false, fmt.Errorf("reading %s: %w", name, err)
	}
	if _, err := toml.Decode(string(b), v); err != nil {
		return false, fmt.Errorf("parsing %s: %w", name, err)
	}
	return true, nil
}
```

- [ ] **Step 4** — update `content_test.go`: every call to `Load(dir)` must now pass an `fs.FS`. Wrap each directory with `os.DirFS`, i.e. change `Load(dir)` → `Load(os.DirFS(dir))` in **all** test functions (`TestLoadTiles`, `TestLoadRejectsBadColor`, `TestLoadRejectsMultiRuneGlyph`, `TestLoadRejectsEmpty`, `TestLoadMonsters`, `TestLoadWithoutMonstersFileIsOK`, `TestLoadItems`, `TestLoadWithoutItemsFileIsOK`, `TestLoadRejectsBadItemKind`). `os` is already imported in the test file.

- [ ] **Step 5** — in `cmd/tsl/main.go`: import the data package and load from it. Add `"github.com/c0ze/tsl/data"` to imports, and change the content load in `run()` from:
```go
	// content.Load reads ./data relative to the working directory, so tsl must
	// be run from the go/ module root for now.
	c, err := content.Load("data")
```
to:
```go
	c, err := content.Load(data.Files)
```

- [ ] **Step 6** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/content/ ./cmd/tsl/` → PASS. **Commit:**
```bash
git add go/data/data.go go/internal/content/content.go go/internal/content/content_test.go go/cmd/tsl/main.go
git commit -m "feat(content): embed data via fs.FS so the binary runs anywhere"
```

---

## Task 2: Validate dice specs at load (`content`)

**Files:** Modify `go/internal/content/content.go`, `go/internal/content/content_test.go`

- [ ] **Step 1** — add a `validDamageSpec` helper at the end of `content.go`:
```go
// validDamageSpec reports whether s is a well-formed dice spec "NdS" or
// "NdS+M" / "NdS-M" (matching rng.RollSpec's grammar).
func validDamageSpec(s string) bool {
	d := strings.IndexByte(s, 'd')
	if d <= 0 || d >= len(s)-1 {
		return false
	}
	if n, err := strconv.Atoi(s[:d]); err != nil || n < 0 {
		return false
	}
	rest := s[d+1:]
	if rest[0] == '+' || rest[0] == '-' {
		return false
	}
	if i := strings.IndexAny(rest[1:], "+-"); i >= 0 {
		i++ // account for the rest[1:] offset
		if _, err := strconv.Atoi(rest[i:]); err != nil {
			return false
		}
		rest = rest[:i]
	}
	if sides, err := strconv.Atoi(rest); err != nil || sides < 1 {
		return false
	}
	return true
}
```

- [ ] **Step 2** — in `validateMonster`, replace the existing damage check (`if strings.TrimSpace(m.Damage) == "" { ... }`) with:
```go
	if !validDamageSpec(m.Damage) {
		return fmt.Errorf("damage %q is not a valid dice spec", m.Damage)
	}
```

- [ ] **Step 3** — in `validateItem`, replace the weapon damage check inside the `case "weapon":` (`if strings.TrimSpace(i.Damage) == "" { ... }`) with:
```go
		if !validDamageSpec(i.Damage) {
			return fmt.Errorf("weapon damage %q is not a valid dice spec", i.Damage)
		}
```

- [ ] **Step 4** — append a test to `content_test.go`:
```go
func TestLoadRejectsBadMonsterDamage(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "monsters.toml"), []byte("[monster.bad]\nname=\"bad\"\nglyph=\"b\"\ncolor=\"normal\"\nhp=3\nattack=1\ndodge=1\ndamage=\"1x4\"\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for malformed monster damage spec")
	}
}
```

- [ ] **Step 5** — `go test ./internal/content/` → PASS. **Commit:**
```bash
git add go/internal/content/content.go go/internal/content/content_test.go
git commit -m "feat(content): reject malformed dice specs at load"
```

---

## Task 3: Validate potion behavior references (`game`, `cmd`)

**Files:** Modify `go/internal/game/item.go`, `go/internal/game/item_test.go`, `go/cmd/tsl/main.go`

- [ ] **Step 1** — add `ValidateItemUses` to `item.go` (add `"fmt"` and keep the existing `content` import):
```go
import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
)
```
(adjust the import block of `item.go` to include `fmt`), then add at the end of the file:
```go
// ValidateItemUses checks that every consumable item's `use` names a registered
// behavior, so malformed content fails fast at startup instead of silently
// doing nothing at runtime.
func ValidateItemUses(c *content.Content, reg map[string]Behavior) error {
	for id, it := range c.Items {
		if it.Kind == "potion" {
			if _, ok := reg[it.Use]; !ok {
				return fmt.Errorf("item %q references unknown behavior %q", id, it.Use)
			}
		}
	}
	return nil
}
```

- [ ] **Step 2** — append a test to `item_test.go`:
```go
func TestValidateItemUsesRejectsUnknown(t *testing.T) {
	reg := map[string]Behavior{"heal": func(*Game, *Item) []string { return nil }}
	good := &content.Content{Items: map[string]*content.ItemDef{
		"potion": {ID: "potion", Kind: "potion", Use: "heal"},
	}}
	if err := ValidateItemUses(good, reg); err != nil {
		t.Errorf("valid use should pass: %v", err)
	}
	bad := &content.Content{Items: map[string]*content.ItemDef{
		"potion": {ID: "potion", Kind: "potion", Use: "hael"},
	}}
	if err := ValidateItemUses(bad, reg); err == nil {
		t.Error("unknown behavior reference should be rejected")
	}
}
```

- [ ] **Step 3** — in `cmd/tsl/main.go`'s `newGame`, after the `g := &game.Game{...}` literal (which sets `Behaviors: behaviors.Registry()`), validate references before returning:
```go
	if err := game.ValidateItemUses(c, g.Behaviors); err != nil {
		return nil, err
	}
```
(Place it just before `g.NewLevelFn = ...` or before `return g, nil` — anywhere after `g` is constructed.)

- [ ] **Step 4** — from `go/`: `go test ./internal/game/ ./cmd/tsl/` → PASS. **Commit:**
```bash
git add go/internal/game/item.go go/internal/game/item_test.go go/cmd/tsl/main.go
git commit -m "feat(game): fail fast on unknown potion behavior references"
```

---

## Task 4: Defensive bounds in rendering (`ui`)

**Files:** Modify `go/internal/ui/ui.go`

- [ ] **Step 1** — in `BuildView`, guard the item, creature, and player draws with `l.InBounds(...)` so a malformed game state skips instead of panicking. The item loop becomes:
```go
	for _, it := range l.Items {
		if l.InBounds(it.Pos) && l.At(it.Pos).Visible {
			*v.At(it.Pos.X, it.Pos.Y) = Cell{Glyph: it.Def.Rune(), Color: it.Def.Color}
		}
	}
```
the creature loop becomes:
```go
	for _, m := range l.Creatures {
		if l.InBounds(m.Pos) && l.At(m.Pos).Visible {
			*v.At(m.Pos.X, m.Pos.Y) = Cell{Glyph: m.Def.Rune(), Color: m.Def.Color}
		}
	}
```
and guard the player draw:
```go
	if l.InBounds(g.Player) {
		*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
	}
```

- [ ] **Step 2** — Verify whole module from `go/`:
```bash
export GOTOOLCHAIN=local
gofmt -l .
go build ./... && go test ./... -count=1 && go vet ./...
go build -o /tmp/tsl_rf ./cmd/tsl && (cd /tmp && /tmp/tsl_rf </dev/null >/dev/null 2>&1; echo "ran from /tmp exit $?") ; rm -f /tmp/tsl_rf
```
Expected: gofmt clean, all packages `ok`, vet clean, binary builds. (Running from `/tmp` will fail to open a real terminal — that's fine; the point is the embedded data loads with no cwd dependency, so the error is a tcell/terminal error, not a "data not found" error.)

- [ ] **Step 3** — Commit:
```bash
git add go/internal/ui/ui.go
git commit -m "fix(ui): guard rendering against out-of-bounds positions"
```

---

## Done criteria
- `data/*.toml` is embedded; `go run`/the built binary load content regardless of working directory.
- Malformed monster/weapon dice specs are rejected at content load.
- A potion whose `use` is not a registered behavior fails the game at startup.
- `BuildView` never indexes out of bounds.
- `go test ./...`, `go vet`, gofmt all clean.
