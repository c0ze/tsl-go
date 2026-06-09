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

// TestEmbeddedGear checks the shipped weapon/armor table loaded.
func TestEmbeddedGear(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["crystal_sword"]; w == nil || w.Kind != "weapon" || w.Attack != 4 || w.Damage != "2d4" {
		t.Errorf("crystal_sword def unexpected: %+v", w)
	}
	if a := c.Items["chainmail"]; a == nil || a.Kind != "armor" || a.Dodge != 4 {
		t.Errorf("chainmail def unexpected: %+v", a)
	}
}

// TestEmbeddedConsumables checks the status-effect consumables loaded.
func TestEmbeddedConsumables(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if m := c.Items["red_mushroom"]; m == nil || m.Kind != "food" || m.Use != "eat_mushroom" {
		t.Errorf("red_mushroom def unexpected: %+v", m)
	}
	if p := c.Items["potion_regeneration"]; p == nil || p.Kind != "potion" || p.Use != "regenerate" {
		t.Errorf("potion_regeneration def unexpected: %+v", p)
	}
}

// TestEmbeddedTraps checks the dart trap tile + that some levels use traps.
func TestEmbeddedTraps(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if tr := c.Tiles["dart_trap"]; tr == nil || tr.Effect != "poison" || tr.EffectTurns <= 0 {
		t.Errorf("dart_trap tile unexpected: %+v", tr)
	}
	trapped := false
	for _, l := range c.Levels {
		if l.Traps > 0 {
			trapped = true
		}
	}
	if !trapped {
		t.Error("expected at least one level with traps")
	}
}

// TestEmbeddedWand checks the force-bolt wand loaded.
func TestEmbeddedWand(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["wand_force_bolt"]; w == nil || w.Kind != "wand" || w.Damage != "2d6" || w.Power != 5 {
		t.Errorf("wand_force_bolt def unexpected: %+v", w)
	}
}

// TestEmbeddedVenomWand checks the status-effect (venom) wand loaded: a pure
// poison wand with charges and no direct damage.
func TestEmbeddedVenomWand(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	w := c.Items["wand_venom"]
	if w == nil || w.Kind != "wand" || w.Effect != "poison" || w.EffectTurns <= 0 || w.Power <= 0 {
		t.Errorf("wand_venom def unexpected: %+v", w)
	}
	if w != nil && w.Damage != "" {
		t.Errorf("wand_venom should be a pure status wand (no damage), got %q", w.Damage)
	}
}

// TestEmbeddedDoors checks the door tiles loaded and that some levels use them.
func TestEmbeddedDoors(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	dc := c.Tiles["door_closed"]
	if dc == nil || dc.Passable || dc.Transparent || dc.OpensTo != "door_open" {
		t.Errorf("door_closed tile unexpected: %+v", dc)
	}
	if do := c.Tiles["door_open"]; do == nil || !do.Passable || !do.Transparent {
		t.Errorf("door_open tile unexpected: %+v", do)
	}
	doored := false
	for _, l := range c.Levels {
		if l.Doors {
			doored = true
		}
	}
	if !doored {
		t.Error("expected at least one level with doors enabled")
	}
}

// TestEmbeddedScrolls checks the read-verb scrolls loaded.
func TestEmbeddedScrolls(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["scroll_teleport"]; s == nil || s.Kind != "scroll" || s.Use != "teleport" {
		t.Errorf("scroll_teleport def unexpected: %+v", s)
	}
	if s := c.Items["scroll_magic_mapping"]; s == nil || s.Kind != "scroll" || s.Use != "reveal" {
		t.Errorf("scroll_magic_mapping def unexpected: %+v", s)
	}
}

// TestEmbeddedSlowingWand checks the control (slowing) wand loaded.
func TestEmbeddedSlowingWand(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	w := c.Items["wand_slowing"]
	if w == nil || w.Kind != "wand" || w.Effect != "slow" || w.EffectTurns <= 0 || w.Power <= 0 {
		t.Errorf("wand_slowing def unexpected: %+v", w)
	}
	if w != nil && w.Damage != "" {
		t.Errorf("wand_slowing should be a pure status wand (no damage), got %q", w.Damage)
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
