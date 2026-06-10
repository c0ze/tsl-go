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

// TestEmbeddedRosterBatch2 checks the second monster batch loaded with the
// expected glyphs and edible corpses, broadening the bestiary beyond rats/ghouls.
func TestEmbeddedRosterBatch2(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	want := map[string]rune{
		"bat": 'b', "kobold": 'k', "zombie": 'z',
		"skeleton": 's', "wraith": 'W', "ogre": 'O',
	}
	for id, glyph := range want {
		m := c.Monsters[id]
		if m == nil {
			t.Errorf("missing monster %q", id)
			continue
		}
		if m.Rune() != glyph {
			t.Errorf("%s glyph = %q, want %q", id, m.Rune(), glyph)
		}
	}
	// At least one new monster appears in a spawn table, so it can actually show up.
	spawned := false
	for _, l := range c.Levels {
		for _, s := range l.Spawn {
			if _, ok := want[s.Monster]; ok {
				spawned = true
			}
		}
	}
	if !spawned {
		t.Error("expected at least one batch-2 monster placed in a spawn table")
	}
}

// TestEmbeddedImp checks the ranged caster loaded and appears in a spawn table.
func TestEmbeddedImp(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	imp := c.Monsters["imp"]
	if imp == nil || imp.Rune() != 'i' || imp.Ranged <= 0 {
		t.Errorf("imp def unexpected: %+v", imp)
	}
	spawned := false
	for _, l := range c.Levels {
		for _, s := range l.Spawn {
			if s.Monster == "imp" {
				spawned = true
			}
		}
	}
	if !spawned {
		t.Error("expected the imp placed in a spawn table")
	}
}

// TestEmbeddedRosterBatch3 checks the third monster batch loaded with canon glyphs
// and that at least one appears in a spawn table.
func TestEmbeddedRosterBatch3(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	want := map[string]rune{
		"jackal": 'j', "cave_snake": 'S', "giant_spider": 'a',
		"dire_wolf": 'd', "wisp": 'w', "troll": 'T',
	}
	for id, glyph := range want {
		m := c.Monsters[id]
		if m == nil {
			t.Errorf("missing monster %q", id)
			continue
		}
		if m.Rune() != glyph {
			t.Errorf("%s glyph = %q, want %q", id, m.Rune(), glyph)
		}
	}
	spawned := false
	for _, l := range c.Levels {
		for _, s := range l.Spawn {
			if _, ok := want[s.Monster]; ok {
				spawned = true
			}
		}
	}
	if !spawned {
		t.Error("expected at least one batch-3 monster placed in a spawn table")
	}
}

func TestEmbeddedRosterBatch4(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	want := map[string]rune{
		"hellhound": 'h', "frostling": 'f', "goatman": 'p',
		"tentacle": 'l', "gloom_lord": 'K', "giant_slimy_toad": 'Y',
	}
	for id, glyph := range want {
		m := c.Monsters[id]
		if m == nil {
			t.Errorf("missing monster %q", id)
			continue
		}
		if m.Rune() != glyph {
			t.Errorf("%s glyph = %q, want %q", id, m.Rune(), glyph)
		}
	}
	// the gloom lord is a second ranged caster — make sure that survived the port
	if g := c.Monsters["gloom_lord"]; g != nil && g.Ranged <= 0 {
		t.Errorf("gloom_lord should be ranged, got %+v", g)
	}
	seen := map[string]bool{}
	for _, l := range c.Levels {
		for _, s := range l.Spawn {
			if _, ok := want[s.Monster]; ok {
				seen[s.Monster] = true
			}
		}
	}
	for id := range want {
		if !seen[id] {
			t.Errorf("expected %q to appear in at least one spawn table", id)
		}
	}
}

func TestEmbeddedRosterBatch5(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	want := map[string]rune{
		"sentinel": 'e', "technician": 't', "burning_skull": 'q',
		"gaoler": 'G', "chrome_angel": 'A', "nameless_horror": 'H',
	}
	for id, glyph := range want {
		m := c.Monsters[id]
		if m == nil {
			t.Errorf("missing monster %q", id)
			continue
		}
		if m.Rune() != glyph {
			t.Errorf("%s glyph = %q, want %q", id, m.Rune(), glyph)
		}
	}
	// the sentinel is a ranged sentry — make sure that survived the port
	if s := c.Monsters["sentinel"]; s != nil && s.Ranged <= 0 {
		t.Errorf("sentinel should be ranged, got %+v", s)
	}
	seen := map[string]bool{}
	for _, l := range c.Levels {
		for _, s := range l.Spawn {
			if _, ok := want[s.Monster]; ok {
				seen[s.Monster] = true
			}
		}
	}
	for id := range want {
		if !seen[id] {
			t.Errorf("expected %q to appear in at least one spawn table", id)
		}
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

// TestEmbeddedAccessories checks the rings and amulet loaded with their bonuses.
func TestEmbeddedAccessories(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if r := c.Items["ring_of_accuracy"]; r == nil || r.Kind != "ring" || r.Attack != 2 {
		t.Errorf("ring_of_accuracy def unexpected: %+v (want kind=ring attack=2)", r)
	}
	if r := c.Items["ring_of_protection"]; r == nil || r.Kind != "ring" || r.Dodge != 2 {
		t.Errorf("ring_of_protection def unexpected: %+v (want kind=ring dodge=2)", r)
	}
	if a := c.Items["amulet_of_warding"]; a == nil || a.Kind != "amulet" || a.Dodge != 3 {
		t.Errorf("amulet_of_warding def unexpected: %+v (want kind=amulet dodge=3)", a)
	}
}

// TestEmbeddedRangedWeapons checks the shortbow and the ranged bosses loaded.
func TestEmbeddedRangedWeapons(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["shortbow"]; w == nil || w.Kind != "weapon" || w.Ranged <= 0 {
		t.Errorf("shortbow def unexpected: %+v", w)
	}
	// Bosses threaten from a distance (fire breath / dark bolts) via the 13c AI.
	if d := c.Monsters["dragon"]; d == nil || d.Ranged <= 0 {
		t.Errorf("dragon should have a ranged attack, got %+v", d)
	}
	if m := c.Monsters["elder_mummylich"]; m == nil || m.Ranged <= 0 {
		t.Errorf("elder_mummylich should have a ranged attack, got %+v", m)
	}
}

// TestEmbeddedPotionEnergy checks the energy potion loaded.
func TestEmbeddedPotionEnergy(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_energy"]; p == nil || p.Kind != "potion" || p.Use != "restore_energy" || p.Power <= 0 {
		t.Errorf("potion_energy def unexpected: %+v", p)
	}
}

// TestEmbeddedFlash checks the flash spellbook loaded.
func TestEmbeddedFlash(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["spellbook_flash"]; b == nil || b.Kind != "spellbook" || b.Use != "flash" || b.Cost <= 0 {
		t.Errorf("spellbook_flash def unexpected: %+v", b)
	}
}

// TestEmbeddedNoxiousCloud checks the noxious cloud spellbook loaded.
func TestEmbeddedNoxiousCloud(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["spellbook_noxious_cloud"]; b == nil || b.Kind != "spellbook" || b.Use != "noxious_cloud" || b.Cost <= 0 {
		t.Errorf("spellbook_noxious_cloud def unexpected: %+v", b)
	}
}

// TestEmbeddedPotionBlindness checks the blindness potion loaded.
func TestEmbeddedPotionBlindness(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_blindness"]; p == nil || p.Kind != "potion" || p.Use != "blindness" || p.Power <= 0 {
		t.Errorf("potion_blindness def unexpected: %+v", p)
	}
}

// TestEmbeddedSpellbook checks the first-aid spellbook loaded.
func TestEmbeddedSpellbook(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	b := c.Items["spellbook_first_aid"]
	if b == nil || b.Kind != "spellbook" || b.Use != "first_aid" || b.Cost <= 0 {
		t.Errorf("spellbook_first_aid def unexpected: %+v", b)
	}
	// The force-bolt spellbook is a targeted attack spell (ranged + damage, no use).
	fb := c.Items["spellbook_force_bolt"]
	if fb == nil || fb.Kind != "spellbook" || fb.Ranged <= 0 || fb.Damage == "" || fb.Cost <= 0 {
		t.Errorf("spellbook_force_bolt def unexpected: %+v", fb)
	}
	// The frost-ray spellbook is a beam attack (strikes a whole line).
	fr := c.Items["spellbook_frost_ray"]
	if fr == nil || fr.Kind != "spellbook" || !fr.Beam || fr.Ranged <= 0 || fr.Damage == "" || fr.Cost <= 0 {
		t.Errorf("spellbook_frost_ray def unexpected: %+v", fr)
	}
}

// TestEmbeddedConsumableBreadth checks the 15h consumables loaded with the right
// use behaviors (instant healing / pain potions, identify / recharge scrolls).
func TestEmbeddedConsumableBreadth(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	want := map[string]string{
		"potion_instant_healing": "instant_healing",
		"potion_pain":            "pain",
		"scroll_identify":        "identify",
		"scroll_recharge":        "recharge",
	}
	for id, use := range want {
		it := c.Items[id]
		if it == nil || it.Use != use {
			t.Errorf("%s def unexpected: %+v (want use %q)", id, it, use)
		}
	}
	if p := c.Items["potion_pain"]; p != nil && p.Power <= 0 {
		t.Errorf("potion_pain needs Power for its damage, got %+v", p)
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

// TestEmbeddedTorch checks the torch light source loaded.
func TestEmbeddedTorch(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["torch"]; w == nil || w.Kind != "light" || w.Light <= 0 {
		t.Errorf("torch def unexpected: %+v", w)
	}
}

// TestEmbeddedDarkLevels checks that some levels are unlit.
func TestEmbeddedDarkLevels(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	dark := 0
	for _, l := range c.Levels {
		if l.Dark {
			dark++
		}
	}
	if dark == 0 {
		t.Error("expected at least one dark level")
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
	if s := c.Items["scroll_scare_monster"]; s == nil || s.Kind != "scroll" || s.Use != "scare" || s.Power <= 0 {
		t.Errorf("scroll_scare_monster def unexpected: %+v", s)
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

func TestEmbeddedConfusionWand(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	w := c.Items["wand_confusion"]
	if w == nil || w.Kind != "wand" || w.Effect != "confuse" || w.EffectTurns <= 0 {
		t.Errorf("wand_confusion def unexpected: %+v", w)
	}
	if w != nil && w.Damage != "" {
		t.Errorf("wand_confusion should be a pure status wand (no damage), got %q", w.Damage)
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

// TestEmbeddedSpeedPotions checks the haste/slow potion pair loaded (16b):
// 20 turns each, the C HASTE_LENGTH/SLOW_LENGTH.
func TestEmbeddedSpeedPotions(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_speed"]; p == nil || p.Kind != "potion" || p.Use != "haste" || p.Power != 20 {
		t.Errorf("potion_speed def unexpected: %+v", p)
	}
	if p := c.Items["potion_slowing"]; p == nil || p.Kind != "potion" || p.Use != "slowness" || p.Power != 20 {
		t.Errorf("potion_slowing def unexpected: %+v", p)
	}
}
