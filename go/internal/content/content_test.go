package content

import (
	"os"
	"path/filepath"
	"testing"
)

func writeTiles(t *testing.T, body string) string {
	t.Helper()
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte(body), 0o644); err != nil {
		t.Fatal(err)
	}
	return dir
}

func TestLoadTiles(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true

[tile.wall]
glyph = "#"
color = "normal"
passable = false
transparent = false
`)
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	floor, ok := c.Tiles["floor"]
	if !ok {
		t.Fatal("floor tile missing")
	}
	if floor.ID != "floor" {
		t.Errorf("ID = %q, want floor", floor.ID)
	}
	if floor.Rune() != '.' {
		t.Errorf("Rune = %q, want '.'", floor.Rune())
	}
	if !floor.Passable {
		t.Error("floor should be passable")
	}
	if c.Tiles["wall"].Passable {
		t.Error("wall should not be passable")
	}
}

func TestLoadRejectsBadColor(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = "."
color = "chartreuse"
passable = true
transparent = true
`)
	if _, err := Load(dir); err == nil {
		t.Fatal("expected error for invalid color, got nil")
	}
}

func TestLoadRejectsMultiRuneGlyph(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = ".."
color = "normal"
passable = true
transparent = true
`)
	if _, err := Load(dir); err == nil {
		t.Fatal("expected error for multi-rune glyph, got nil")
	}
}

// A file that parses but defines no tiles must be rejected (a level needs tiles).
func TestLoadRejectsEmpty(t *testing.T) {
	dir := writeTiles(t, "# valid TOML, but no [tile.*] tables\n")
	if _, err := Load(dir); err == nil {
		t.Fatal("expected error when no tiles are defined, got nil")
	}
}

func TestLoadMonsters(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte(`
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true
`), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "monsters.toml"), []byte(`
[monster.rat]
name = "rat"
glyph = "r"
color = "brown"
hp = 3
attack = 2
dodge = 1
damage = "1d2"
`), 0o644); err != nil {
		t.Fatal(err)
	}
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	rat, ok := c.Monsters["rat"]
	if !ok {
		t.Fatal("rat monster missing")
	}
	if rat.ID != "rat" || rat.HP != 3 || rat.Rune() != 'r' {
		t.Errorf("unexpected rat def: %+v", rat)
	}
}

func TestLoadWithoutMonstersFileIsOK(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true
`)
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if len(c.Monsters) != 0 {
		t.Errorf("expected no monsters, got %d", len(c.Monsters))
	}
}
