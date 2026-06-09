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
	c, err := Load(os.DirFS(dir))
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
	if _, err := Load(os.DirFS(dir)); err == nil {
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
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for multi-rune glyph, got nil")
	}
}

// A file that parses but defines no tiles must be rejected (a level needs tiles).
func TestLoadRejectsEmpty(t *testing.T) {
	dir := writeTiles(t, "# valid TOML, but no [tile.*] tables\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
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
	c, err := Load(os.DirFS(dir))
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
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if len(c.Monsters) != 0 {
		t.Errorf("expected no monsters, got %d", len(c.Monsters))
	}
}

func TestLoadItems(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "items.toml"), []byte("[item.healing_potion]\nname=\"healing potion\"\nglyph=\"!\"\ncolor=\"red\"\nkind=\"potion\"\nuse=\"heal\"\npower=8\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	c, err := Load(os.DirFS(dir))
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
	c, err := Load(os.DirFS(dir))
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
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for invalid item kind")
	}
}

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

func TestLoadRangedMonster(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "monsters.toml"), []byte("[monster.imp]\nname=\"imp\"\nglyph=\"i\"\ncolor=\"red\"\nhp=6\nattack=4\ndodge=3\ndamage=\"1d4\"\nranged=5\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if m := c.Monsters["imp"]; m == nil || m.Ranged != 5 {
		t.Errorf("imp def unexpected: %+v", m)
	}
}

func TestLoadRejectsNegativeRanged(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "monsters.toml"), []byte("[monster.bad]\nname=\"bad\"\nglyph=\"b\"\ncolor=\"normal\"\nhp=3\nattack=1\ndodge=1\ndamage=\"1d2\"\nranged=-1\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for negative ranged")
	}
}

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

func writeLevelsFixture(t *testing.T, levelsBody string) string {
	t.Helper()
	dir := t.TempDir()
	w := func(name, body string) {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	w("tiles.toml", "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n")
	w("monsters.toml", "[monster.ratman]\nname=\"ratman\"\nglyph=\"r\"\ncolor=\"brown\"\nhp=3\nattack=2\ndodge=1\ndamage=\"1d2\"\n")
	w("levels.toml", levelsBody)
	return dir
}

const twoLevelGraph = `
[level.dungeon]
name = "the Dungeon"
width = 60
height = 24
start = true
links = ["cave"]
monsters = 5
[[level.dungeon.spawn]]
monster = "ratman"
weight = 1

[level.cave]
name = "the Cave"
width = 40
height = 20
links = ["dungeon"]
monsters = 3
[[level.cave.spawn]]
monster = "ratman"
weight = 2
`

func TestLoadLevels(t *testing.T) {
	c, err := Load(os.DirFS(writeLevelsFixture(t, twoLevelGraph)))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	d, ok := c.Levels["dungeon"]
	if !ok || d.Name != "the Dungeon" || !d.Start || d.W != 60 || d.H != 24 {
		t.Fatalf("unexpected dungeon level: %+v", d)
	}
	if len(d.Links) != 1 || d.Links[0] != "cave" {
		t.Errorf("dungeon links = %v", d.Links)
	}
	if len(d.Spawn) != 1 || d.Spawn[0].Monster != "ratman" || d.Spawn[0].Weight != 1 {
		t.Errorf("dungeon spawn = %+v", d.Spawn)
	}
}

func TestLoadRejectsNoStart(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nlinks=[\"a\"]\n"
	if _, err := Load(os.DirFS(writeLevelsFixture(t, body))); err == nil {
		t.Fatal("expected error: no start level")
	}
}

func TestLoadRejectsTwoStarts(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"b\"]\n[level.b]\nname=\"B\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"a\"]\n"
	if _, err := Load(os.DirFS(writeLevelsFixture(t, body))); err == nil {
		t.Fatal("expected error: two start levels")
	}
}

func TestLoadRejectsUnknownLink(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"nowhere\"]\n"
	if _, err := Load(os.DirFS(writeLevelsFixture(t, body))); err == nil {
		t.Fatal("expected error: link to unknown level")
	}
}

func TestLoadRejectsUnknownSpawnMonster(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"a\"]\nmonsters=1\n[[level.a.spawn]]\nmonster=\"dragon\"\nweight=1\n"
	if _, err := Load(os.DirFS(writeLevelsFixture(t, body))); err == nil {
		t.Fatal("expected error: spawn references unknown monster")
	}
}

func TestLoadRejectsZeroSpawnWeight(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"a\"]\nmonsters=1\n[[level.a.spawn]]\nmonster=\"ratman\"\nweight=0\n"
	if _, err := Load(os.DirFS(writeLevelsFixture(t, body))); err == nil {
		t.Fatal("expected error: spawn weight must be >= 1")
	}
}

func writeEndgameFixture(t *testing.T, tilesExtra, levelsBody string) string {
	t.Helper()
	dir := t.TempDir()
	w := func(name, body string) {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	w("tiles.toml", "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"+tilesExtra)
	w("monsters.toml", "[monster.ratman]\nname=\"ratman\"\nglyph=\"r\"\ncolor=\"brown\"\nhp=3\nattack=2\ndodge=1\ndamage=\"1d2\"\n")
	w("levels.toml", levelsBody)
	return dir
}

const altarTileTOML = "[tile.altar]\nglyph=\"_\"\ncolor=\"cyan\"\npassable=true\ntransparent=true\nwin=true\n"

func TestLoadAltarBossLevel(t *testing.T) {
	body := "[level.chapel]\nname=\"Chapel\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"chapel\"]\naltar=true\nboss=\"ratman\"\n"
	c, err := Load(os.DirFS(writeEndgameFixture(t, altarTileTOML, body)))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if ch := c.Levels["chapel"]; ch == nil || !ch.Altar || ch.Boss != "ratman" {
		t.Errorf("unexpected chapel: %+v", ch)
	}
	if a := c.Tiles["altar"]; a == nil || !a.Win {
		t.Error("altar tile should have win=true")
	}
}

func TestLoadRejectsUnknownBoss(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"a\"]\nboss=\"dragon\"\n"
	if _, err := Load(os.DirFS(writeEndgameFixture(t, altarTileTOML, body))); err == nil {
		t.Fatal("expected error: boss references unknown monster")
	}
}

func TestLoadRejectsAltarWithoutWinTile(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"a\"]\naltar=true\n"
	if _, err := Load(os.DirFS(writeEndgameFixture(t, "", body))); err == nil {
		t.Fatal("expected error: altar level needs a defined win tile")
	}
}

func TestLoadTileEffect(t *testing.T) {
	dir := writeTiles(t, "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n\n[tile.dart_trap]\nglyph=\"^\"\ncolor=\"red\"\npassable=true\ntransparent=true\neffect=\"poison\"\neffect_turns=5\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if tr := c.Tiles["dart_trap"]; tr == nil || tr.Effect != "poison" || tr.EffectTurns != 5 {
		t.Errorf("dart_trap def unexpected: %+v", tr)
	}
}

func TestLoadRejectsEffectTileWithoutTurns(t *testing.T) {
	dir := writeTiles(t, "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n\n[tile.bad]\nglyph=\"^\"\ncolor=\"red\"\npassable=true\ntransparent=true\neffect=\"poison\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: effect tile needs effect_turns > 0")
	}
}

func TestLoadRejectsTrapsWithoutTrapTile(t *testing.T) {
	body := "[level.a]\nname=\"A\"\nwidth=60\nheight=24\nstart=true\nlinks=[\"a\"]\ntraps=3\n"
	if _, err := Load(os.DirFS(writeLevelsFixture(t, body))); err == nil {
		t.Fatal("expected error: traps>0 needs a dart_trap tile")
	}
}

func writeItemsFixture(t *testing.T, itemsBody string) string {
	t.Helper()
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte("[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "items.toml"), []byte(itemsBody), 0o644); err != nil {
		t.Fatal(err)
	}
	return dir
}

func TestLoadWandItem(t *testing.T) {
	dir := writeItemsFixture(t, "[item.zap]\nname=\"zap wand\"\nglyph=\"/\"\ncolor=\"blue\"\nkind=\"wand\"\ndamage=\"2d6\"\npower=5\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["zap"]; w == nil || w.Kind != "wand" || w.Damage != "2d6" {
		t.Errorf("wand def unexpected: %+v", w)
	}
}

func TestLoadRejectsWandBadDamage(t *testing.T) {
	dir := writeItemsFixture(t, "[item.zap]\nname=\"zap\"\nglyph=\"/\"\ncolor=\"blue\"\nkind=\"wand\"\ndamage=\"oops\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: wand needs a valid damage spec")
	}
}

func TestLoadEffectWand(t *testing.T) {
	dir := writeItemsFixture(t, "[item.venom]\nname=\"venom wand\"\nglyph=\"/\"\ncolor=\"green\"\nkind=\"wand\"\neffect=\"poison\"\neffect_turns=8\npower=5\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	w := c.Items["venom"]
	if w == nil || w.Kind != "wand" || w.Effect != "poison" || w.EffectTurns != 8 {
		t.Errorf("effect wand def unexpected: %+v", w)
	}
	if w != nil && w.Damage != "" {
		t.Errorf("effect-only wand should carry no damage spec, got %q", w.Damage)
	}
}

func TestLoadRejectsWandWithoutDamageOrEffect(t *testing.T) {
	dir := writeItemsFixture(t, "[item.dud]\nname=\"dud\"\nglyph=\"/\"\ncolor=\"blue\"\nkind=\"wand\"\npower=5\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: a wand needs a damage spec or an effect")
	}
}

func TestLoadRejectsItemEffectWithoutTurns(t *testing.T) {
	dir := writeItemsFixture(t, "[item.venom]\nname=\"venom\"\nglyph=\"/\"\ncolor=\"green\"\nkind=\"wand\"\neffect=\"poison\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: item effect needs effect_turns > 0")
	}
}

func TestLoadDoorTile(t *testing.T) {
	dir := writeTiles(t, "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"+
		"[tile.door_open]\nglyph=\"'\"\ncolor=\"brown\"\npassable=true\ntransparent=true\n"+
		"[tile.door_closed]\nglyph=\"+\"\ncolor=\"brown\"\nopens_to=\"door_open\"\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	d := c.Tiles["door_closed"]
	if d == nil || d.OpensTo != "door_open" || d.Passable || d.Transparent {
		t.Errorf("door_closed def unexpected: %+v", d)
	}
}

func TestLoadRejectsBadOpensTo(t *testing.T) {
	dir := writeTiles(t, "[tile.floor]\nglyph=\".\"\ncolor=\"normal\"\npassable=true\ntransparent=true\n"+
		"[tile.door_closed]\nglyph=\"+\"\ncolor=\"brown\"\nopens_to=\"nope\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: opens_to references an unknown tile")
	}
}

func TestLoadScrollItem(t *testing.T) {
	dir := writeItemsFixture(t, "[item.tele]\nname=\"scroll of teleportation\"\nglyph=\"?\"\ncolor=\"normal\"\nkind=\"scroll\"\nuse=\"teleport\"\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if s := c.Items["tele"]; s == nil || s.Kind != "scroll" || s.Use != "teleport" {
		t.Errorf("scroll def unexpected: %+v", s)
	}
}

func TestLoadRejectsScrollWithoutUse(t *testing.T) {
	dir := writeItemsFixture(t, "[item.blank]\nname=\"blank scroll\"\nglyph=\"?\"\ncolor=\"normal\"\nkind=\"scroll\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: scroll needs a non-empty use")
	}
}

func TestLoadTorchItem(t *testing.T) {
	dir := writeItemsFixture(t, "[item.torch]\nname=\"torch\"\nglyph=\"~\"\ncolor=\"red\"\nkind=\"light\"\nlight=6\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["torch"]; w == nil || w.Kind != "light" || w.Light != 6 {
		t.Errorf("torch def unexpected: %+v", w)
	}
}

func TestLoadRejectsNegativeLight(t *testing.T) {
	dir := writeItemsFixture(t, "[item.bad]\nname=\"bad\"\nglyph=\"~\"\ncolor=\"red\"\nkind=\"weapon\"\ndamage=\"1d4\"\nlight=-1\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for negative light")
	}
}

func TestLoadRejectsLightlessLight(t *testing.T) {
	dir := writeItemsFixture(t, "[item.dud]\nname=\"dud\"\nglyph=\"~\"\ncolor=\"red\"\nkind=\"light\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: a light item needs light > 0")
	}
}

func TestLoadSpellbook(t *testing.T) {
	dir := writeItemsFixture(t, "[item.book]\nname=\"spellbook of first aid\"\nglyph=\"+\"\ncolor=\"blue\"\nkind=\"spellbook\"\nuse=\"first_aid\"\ncost=4\npower=6\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["book"]; b == nil || b.Kind != "spellbook" || b.Use != "first_aid" || b.Cost != 4 {
		t.Errorf("spellbook def unexpected: %+v", b)
	}
}

func TestLoadRejectsCostlessSpellbook(t *testing.T) {
	dir := writeItemsFixture(t, "[item.book]\nname=\"book\"\nglyph=\"+\"\ncolor=\"blue\"\nkind=\"spellbook\"\nuse=\"first_aid\"\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: a spellbook needs cost > 0")
	}
}

func TestLoadRejectsUselessSpellbook(t *testing.T) {
	dir := writeItemsFixture(t, "[item.book]\nname=\"book\"\nglyph=\"+\"\ncolor=\"blue\"\nkind=\"spellbook\"\ncost=4\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error: a spellbook needs a use or a ranged damage spec")
	}
}

func TestLoadBeamSpellbook(t *testing.T) {
	dir := writeItemsFixture(t, "[item.ray]\nname=\"spellbook of frost ray\"\nglyph=\"+\"\ncolor=\"cyan\"\nkind=\"spellbook\"\nbeam=true\nranged=6\ndamage=\"2d4\"\ncost=6\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["ray"]; b == nil || !b.Beam || b.Ranged != 6 {
		t.Errorf("frost-ray spellbook unexpected: %+v", b)
	}
}

func TestLoadDamageSpellbook(t *testing.T) {
	dir := writeItemsFixture(t, "[item.bolt]\nname=\"spellbook of force bolt\"\nglyph=\"+\"\ncolor=\"red\"\nkind=\"spellbook\"\nranged=6\ndamage=\"2d6\"\ncost=5\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if b := c.Items["bolt"]; b == nil || b.Ranged != 6 || b.Damage != "2d6" || b.Use != "" {
		t.Errorf("force-bolt spellbook unexpected: %+v", b)
	}
}

func TestLoadRangedWeapon(t *testing.T) {
	dir := writeItemsFixture(t, "[item.shortbow]\nname=\"shortbow\"\nglyph=\"}\"\ncolor=\"brown\"\nkind=\"weapon\"\nattack=1\ndamage=\"1d6\"\nranged=6\n")
	c, err := Load(os.DirFS(dir))
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if w := c.Items["shortbow"]; w == nil || w.Ranged != 6 {
		t.Errorf("shortbow def unexpected: %+v", w)
	}
}

func TestLoadRejectsNegativeItemRanged(t *testing.T) {
	dir := writeItemsFixture(t, "[item.bow]\nname=\"bow\"\nglyph=\"}\"\ncolor=\"brown\"\nkind=\"weapon\"\ndamage=\"1d6\"\nranged=-1\n")
	if _, err := Load(os.DirFS(dir)); err == nil {
		t.Fatal("expected error for negative item ranged")
	}
}
