package data

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

// TestEmbeddedRoster validates the actually-shipped content: the roster is
// present and the canon names/glyphs are correct.
func TestEmbeddedRoster(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load shipped content: %v", err)
	}
	if _, ok := c.Monsters["rat"]; ok {
		t.Error("monster id \"rat\" should be renamed to \"ratman\"")
	}
	for _, id := range []string{"ratman", "ghoul", "graveling", "gnoblin", "crypt_vermin", "merman"} {
		if _, ok := c.Monsters[id]; !ok {
			t.Errorf("missing monster %q", id)
		}
	}
	if g := c.Monsters["ghoul"]; g == nil || g.Rune() != 'Z' {
		t.Error("ghoul glyph should be 'Z' (canon)")
	}
	if r := c.Monsters["ratman"]; r == nil || r.Rune() != 'r' {
		t.Error("ratman glyph should be 'r'")
	}
}

// TestDepthGatedTanks checks the tougher monsters only appear on deeper floors.
func TestDepthGatedTanks(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Monsters["scarecrow"]; s == nil || s.MinDepth != 2 {
		t.Errorf("scarecrow should be gated to depth 2, got %+v", s)
	}
	if s := c.Monsters["slime"]; s == nil || s.MinDepth != 3 {
		t.Errorf("slime should be gated to depth 3, got %+v", s)
	}
}

// TestEmbeddedDungeon validates the shipped level graph loads with exactly one
// start and the expected starter levels.
func TestEmbeddedDungeon(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	all := []string{"dungeon", "catacombs", "ominous_cave", "laboratory", "comm_hub", "underpass", "drowned_city", "frozen_vault", "dragons_lair", "chapel"}
	for _, id := range all {
		if _, ok := c.Levels[id]; !ok {
			t.Errorf("missing level %q", id)
		}
	}
	if len(c.Levels) != len(all) {
		t.Errorf("level count = %d, want %d", len(c.Levels), len(all))
	}
	starts := 0
	for _, l := range c.Levels {
		if l.Start {
			starts++
		}
	}
	if d := c.Levels["dungeon"]; starts != 1 || d == nil || !d.Start {
		t.Errorf("want exactly one start (the Dungeon), got %d", starts)
	}
	if ch := c.Levels["chapel"]; ch == nil || !ch.Altar || ch.Boss != "elder_mummylich" {
		t.Errorf("Chapel should have the altar + mummylich boss, got %+v", ch)
	}
	if dl := c.Levels["dragons_lair"]; dl == nil || dl.Boss != "dragon" {
		t.Errorf("Dragons Lair should have the dragon boss, got %+v", dl)
	}
	if a := c.Tiles["altar"]; a == nil || !a.Win {
		t.Error("altar tile should be a win tile")
	}
}
