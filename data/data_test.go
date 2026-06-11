package data

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
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
	if s := c.Items["scroll_blink"]; s == nil || s.Kind != "scroll" || s.Use != "teleport" || s.Name != "blink scroll" {
		t.Errorf("scroll_blink def unexpected (0.40 treasure.c names it \"blink scroll\"): %+v", s)
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

// TestEmbeddedSleepPotion checks the knockout potion loaded (16c): 25 turns,
// the C SLEEP_DURATION.
func TestEmbeddedSleepPotion(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_sleep"]; p == nil || p.Kind != "potion" || p.Use != "tranquilize" || p.Power != 25 {
		t.Errorf("potion_sleep def unexpected: %+v", p)
	}
}

// TestEmbeddedWaterLevels checks the water tile + per-level pool counts loaded
// (18a): the Dungeon dips a toe, the Drowned City earns its name.
func TestEmbeddedWaterLevels(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Tiles["water"]; w == nil || !w.Water || w.Passable || !w.Transparent {
		t.Errorf("water tile def unexpected: %+v", w)
	}
	if l := c.Levels["dungeon"]; l == nil || l.Water != 1 {
		t.Errorf("dungeon should have 1 water pool, got %+v", l)
	}
	if l := c.Levels["underpass"]; l == nil || l.Water != 3 {
		t.Errorf("underpass should have 3 water pools, got %+v", l)
	}
	if l := c.Levels["drowned_city"]; l == nil || l.Water != 4 {
		t.Errorf("drowned_city should have 4 water pools, got %+v", l)
	}
}

// TestEmbeddedLevitationPotion checks the float potion loaded (18b): 20 turns,
// the C LEVITATE_TIME.
func TestEmbeddedLevitationPotion(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_levitation"]; p == nil || p.Kind != "potion" || p.Use != "levitate" || p.Power != 20 {
		t.Errorf("potion_levitation def unexpected: %+v", p)
	}
}

// TestEmbeddedLurker checks the Drowned City's pool boss loaded (18c): the
// Lurker per C unique.c, with its tentacle retinue wiring.
func TestEmbeddedLurker(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	l := c.Monsters["lurker"]
	if l == nil || l.Glyph != "L" || l.HP != 22 || l.Attack != 95 || l.Speed != 120 {
		t.Fatalf("lurker def unexpected: %+v", l)
	}
	if !l.Swim || !l.Permaswim || l.Effect != "poison" {
		t.Errorf("lurker should be a pool-bound venomous swimmer: %+v", l)
	}
	if m := c.Monsters["merman"]; m == nil || !m.Swim || m.Permaswim {
		t.Errorf("merman should free-swim but walk ashore too: %+v", m)
	}
	if tn := c.Monsters["tentacle"]; tn == nil || !tn.Swim || !tn.Permaswim {
		t.Errorf("tentacle should never leave the water: %+v", tn)
	}
	d := c.Levels["drowned_city"]
	if d == nil || d.Boss != "lurker" || d.Retinue != "tentacle" || d.RetinueCount != 8 {
		t.Errorf("drowned_city should host the Lurker with 8 tentacles: %+v", d)
	}
}

// TestEmbeddedRosterBatch6 checks batch 6 loaded with canonical 0.40 glyphs
// (13g): electric snake, sludge dweller, severed hand, floating brain, flame
// spirit (monster.c stats, glyph.c glyphs).
func TestEmbeddedRosterBatch6(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	checks := []struct {
		id     string
		glyph  string
		hp     int
		speed  int
		ranged bool
	}{
		{"electric_snake", "S", 3, 35, false},
		{"sludge_dweller", "s", 2, 60, true},
		{"severed_hand", "p", 4, 80, false},
		{"floating_brain", "b", 15, 70, true},
		{"flame_spirit", "j", 20, 60, false},
	}
	for _, want := range checks {
		m := c.Monsters[want.id]
		if m == nil {
			t.Errorf("%s missing", want.id)
			continue
		}
		if m.Glyph != want.glyph || m.HP != want.hp || m.Speed != want.speed {
			t.Errorf("%s: glyph/hp/speed = %q/%d/%d, want %q/%d/%d", want.id, m.Glyph, m.HP, m.Speed, want.glyph, want.hp, want.speed)
		}
		if (m.Ranged > 0) != want.ranged {
			t.Errorf("%s: ranged = %d, want caster=%v", want.id, m.Ranged, want.ranged)
		}
	}
	// Spot-check the C-faithful spawn homes (places.c std_enemy).
	spawns := map[string]string{"dungeon": "electric_snake", "catacombs": "severed_hand", "frozen_vault": "floating_brain", "dragons_lair": "flame_spirit", "laboratory": "sludge_dweller"}
	for lvl, mon := range spawns {
		found := false
		for _, s := range c.Levels[lvl].Spawn {
			if s.Monster == mon {
				found = true
			}
		}
		if !found {
			t.Errorf("%s should spawn %s (C places.c)", lvl, mon)
		}
	}
}

// TestEmbeddedNecromancer checks the Catacombs boss loaded (13h): the
// Necromancer per C unique.c — glacial, ranged, with a chilling touch.
func TestEmbeddedNecromancer(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	n := c.Monsters["necromancer"]
	if n == nil || n.Glyph != "N" || n.HP != 30 || n.Speed != 10 || n.Ranged <= 0 {
		t.Fatalf("necromancer def unexpected: %+v", n)
	}
	if n.Effect != "slow" {
		t.Errorf("the Necromancer's cold touch should chill (slow on hit): %+v", n)
	}
	if l := c.Levels["catacombs"]; l == nil || l.Boss != "necromancer" {
		t.Errorf("catacombs should host the Necromancer (C places.c), got %+v", l)
	}
}

// TestEmbeddedYuckAndElixir checks the last two portable potion-table entries
// loaded (15r). Note the elixir's 0.40 name has no "potion of" prefix.
func TestEmbeddedYuckAndElixir(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_yuck"]; p == nil || p.Kind != "potion" || p.Use != "yuck" {
		t.Errorf("potion_yuck def unexpected: %+v", p)
	}
	if p := c.Items["elixir"]; p == nil || p.Kind != "potion" || p.Use != "elixir" || p.Name != "elixir" {
		t.Errorf("elixir def unexpected (0.40 name is just \"elixir\"): %+v", p)
	}
}

// TestEmbeddedWeights checks the C weight table loaded (16d): kind defaults
// fill unset items, explicit gear weights override, and a corpse outweighs
// the whole 400 allowance (rules.h WEIGHT_CORPSE 599 — the anti-hoarding rule).
func TestEmbeddedWeights(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["healing_potion"].Weight; w != 7 {
		t.Errorf("potions default to WEIGHT_POTION 7, got %d", w)
	}
	if w := c.Items["scroll_identify"].Weight; w != 4 {
		t.Errorf("scrolls default to WEIGHT_SCROLL 4, got %d", w)
	}
	if w := c.Items["corpse"].Weight; w != 599 {
		t.Errorf("a corpse weighs 599 (more than the allowance), got %d", w)
	}
	if w := c.Items["rune_armor"].Weight; w != 80 {
		t.Errorf("rune armor is WEIGHT_HEAVY_ARMOR 80, got %d", w)
	}
	if w := c.Items["dagger"].Weight; w != 18 {
		t.Errorf("dagger is WEIGHT_DAGGER 18, got %d", w)
	}
}

// TestEmbeddedAmnesiaScroll checks the map-eraser loaded (15t).
func TestEmbeddedAmnesiaScroll(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["scroll_amnesia"]; s == nil || s.Kind != "scroll" || s.Use != "amnesia" {
		t.Errorf("scroll_amnesia def unexpected: %+v", s)
	}
}

// TestEmbeddedMarkRecall checks the teleport-home pair loaded (15u) — like the
// blink scroll, 0.40 names them without the "scroll of" prefix.
func TestEmbeddedMarkRecall(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["scroll_mark"]; s == nil || s.Kind != "scroll" || s.Use != "mark" || s.Name != "mark scroll" {
		t.Errorf("scroll_mark def unexpected: %+v", s)
	}
	if s := c.Items["scroll_recall"]; s == nil || s.Kind != "scroll" || s.Use != "recall" || s.Name != "recall scroll" {
		t.Errorf("scroll_recall def unexpected: %+v", s)
	}
}

// TestEmbeddedTrapDetection checks scroll 8 of 10 loaded (18d) — prefix-less
// 0.40 name, like blink/mark/recall.
func TestEmbeddedTrapDetection(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["scroll_trap_detection"]; s == nil || s.Kind != "scroll" || s.Use != "detect_traps" || s.Name != "trap detection scroll" {
		t.Errorf("scroll_trap_detection def unexpected: %+v", s)
	}
}

// TestEmbeddedLava checks the molten half of the hazard pair loaded (18e):
// only the Dragons Lair runs with lava (C places.c:352).
func TestEmbeddedLava(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if l := c.Tiles["lava"]; l == nil || !l.Lava || l.Passable || !l.Transparent {
		t.Errorf("lava tile def unexpected: %+v", l)
	}
	if l := c.Levels["dragons_lair"]; l == nil || l.Lava != 12 {
		t.Errorf("dragons_lair should run with 12 lava pools, got %+v", l)
	}
}

// TestEmbeddedMagicWeapon checks scroll 9 of 10 loaded (15v) — this one keeps
// its "scroll of" prefix in 0.40 (treasure.c:1135).
func TestEmbeddedMagicWeapon(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["scroll_magic_weapon"]; s == nil || s.Kind != "scroll" || s.Use != "magic_weapon" || s.Power != 22 {
		t.Errorf("scroll_magic_weapon def unexpected: %+v", s)
	}
}

// TestEmbeddedSummonFamiliar checks the final scroll loaded (15w): all ten
// 0.40 scrolls are now in.
func TestEmbeddedSummonFamiliar(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["scroll_familiar"]; s == nil || s.Kind != "scroll" || s.Use != "summon_familiar" || s.Power != 500 || s.Name != "scroll of Summon Familiar" {
		t.Errorf("scroll_familiar def unexpected: %+v", s)
	}
}

// TestEmbeddedBreathers checks the six 0.40 breathers loaded (19a) — note the
// hellhound breathes POISON in the C (monster.c:588), not fire.
func TestEmbeddedBreathers(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	breaths := map[string]string{
		"dragon": "fire", "burning_skull": "fire", "flame_spirit": "fire",
		"lurker": "poison", "hellhound": "poison", "giant_slimy_toad": "poison",
	}
	for id, want := range breaths {
		if m := c.Monsters[id]; m == nil || m.Breath != want {
			t.Errorf("%s should breathe %s, got %+v", id, want, m)
		}
	}
}

// TestEmbeddedBreathBooks checks 0.40's first two spellbooks loaded (19b),
// with the C's "book of" capitalized names.
func TestEmbeddedBreathBooks(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["book_breathe_fire"]; b == nil || b.Kind != "spellbook" || b.Breath != "fire" || b.Cost != 5 || b.Name != "book of Breathe Fire" {
		t.Errorf("book_breathe_fire def unexpected: %+v", b)
	}
	if b := c.Items["book_noxious_breath"]; b == nil || b.Kind != "spellbook" || b.Breath != "poison" || b.Cost != 5 || b.Name != "book of Noxious Breath" {
		t.Errorf("book_noxious_breath def unexpected: %+v", b)
	}
}

// TestEmbeddedDeathspellAndPharmacy checks the 15x books loaded; the manual of
// camouflage is faithfully absent (commented out in 0.40's treasure.c).
func TestEmbeddedDeathspellAndPharmacy(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["book_deathspell"]; b == nil || !b.Deathspell || b.Cost != 1 || b.Name != "book of Deathspell" {
		t.Errorf("book_deathspell def unexpected: %+v", b)
	}
	if b := c.Items["manual_pharmacy"]; b == nil || b.Use != "pharmacy" || b.Name != "manual of pharmacy" {
		t.Errorf("manual_pharmacy def unexpected: %+v", b)
	}
	if _, exists := c.Items["manual_camouflage"]; exists {
		t.Error("camouflage is commented out in 0.40 and should stay absent")
	}
}

// TestEmbeddedArrows checks the quiver loaded (14a): C crude arrows, glyph ':',
// weight 1 (WEIGHT_MISSILE) via the ammo kind default.
func TestEmbeddedArrows(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if a := c.Items["arrow"]; a == nil || a.Kind != "ammo" || a.Glyph != ":" || a.Power != 8 || a.Weight != 1 {
		t.Errorf("arrow def unexpected: %+v", a)
	}
}

// TestEmbeddedMimic checks the last bestiary regular loaded (13i): rooted,
// mimic-flagged, and lurking on every level at weight 1 (C places.c).
func TestEmbeddedMimic(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	m := c.Monsters["mimic"]
	if m == nil || !m.Mimic || m.Glyph != "m" || m.HP != 5 || m.Speed != 70 {
		t.Fatalf("mimic def unexpected: %+v", m)
	}
	for id, l := range c.Levels {
		if len(l.Spawn) == 0 {
			continue
		}
		found := false
		for _, s := range l.Spawn {
			if s.Monster != "mimic" {
				continue
			}
			found = true
			if s.Weight != 1 {
				t.Errorf("level %s mimic weight = %d, want 1 (C places.c)", id, s.Weight)
			}
		}
		if !found {
			t.Errorf("level %s should carry the mimic at weight 1 (C places.c)", id)
		}
	}
}

// TestEmbeddedKingOfWorms checks the talk-verb unique loaded (13j): the
// Ominous Cave's boss with his C lore line, plus the four interact flavor
// lines from actions.c.
func TestEmbeddedKingOfWorms(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	k := c.Monsters["king_of_worms"]
	if k == nil || k.Glyph != "W" || k.HP != 30 || k.Speed != 10 {
		t.Fatalf("king_of_worms def unexpected: %+v", k)
	}
	if k.Chat != "He keeps the wisdom that you need, the password that you want." {
		t.Errorf("the King's line must be exact (C actions.c:877), got %q", k.Chat)
	}
	if l := c.Levels["ominous_cave"]; l == nil || l.Boss != "king_of_worms" {
		t.Errorf("the Ominous Cave should host the King (C places.c), got %+v", l)
	}
	for _, id := range []string{"gnoblin", "ghoul", "imp", "slime"} {
		if m := c.Monsters[id]; m == nil || m.Chat == "" {
			t.Errorf("%s should carry its C interact line", id)
		}
	}
}

// TestEmbeddedPolymorph checks the final potion loaded (22a): with it the
// 0.40 potion table is complete.
func TestEmbeddedPolymorph(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if p := c.Items["potion_polymorph"]; p == nil || p.Kind != "potion" || p.Use != "polymorph" || p.Power != 85 {
		t.Errorf("potion_polymorph def unexpected: %+v", p)
	}
}
