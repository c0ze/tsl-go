package behaviors

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/rng"
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

func TestRegenerateAddsEffect(t *testing.T) {
	regen, ok := Registry()["regenerate"]
	if !ok {
		t.Fatal("regenerate behavior not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 20}
	regen(g, &game.Item{Def: &content.ItemDef{Name: "potion of regeneration", Power: 8}})
	if len(g.Effects) != 1 || g.Effects[0].Kind != "regen" || g.Effects[0].Turns != 8 {
		t.Errorf("expected a regen effect for 8 turns, got %v", g.Effects)
	}
}

func TestTeleportBehaviorMovesPlayer(t *testing.T) {
	tele, ok := Registry()["teleport"]
	if !ok {
		t.Fatal("teleport behavior not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(8, 3, floor), Player: game.Pos{X: 1, Y: 1}, RNG: rng.NewWithSeed(1)}
	start := g.Player
	msgs := tele(g, &game.Item{Def: &content.ItemDef{Name: "scroll of teleportation"}})
	if g.Player == start {
		t.Error("teleport scroll should move the player")
	}
	if len(msgs) == 0 {
		t.Error("teleport should return a message")
	}
}

func TestRevealBehaviorMarksSeen(t *testing.T) {
	reveal, ok := Registry()["reveal"]
	if !ok {
		t.Fatal("reveal behavior not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	lvl := game.NewLevel(6, 3, floor)
	g := &game.Game{Level: lvl, Player: game.Pos{X: 1, Y: 1}}
	reveal(g, &game.Item{Def: &content.ItemDef{Name: "scroll of magic mapping"}})
	if !lvl.At(game.Pos{X: 4, Y: 2}).Seen {
		t.Error("magic mapping should mark distant tiles seen")
	}
}

func TestInstantHealingRestores(t *testing.T) {
	heal, ok := Registry()["instant_healing"]
	if !ok {
		t.Fatal("instant_healing not registered")
	}
	g := &game.Game{PlayerHP: 5, PlayerMax: 40, RNG: rng.NewWithSeed(1)}
	heal(g, &game.Item{Def: &content.ItemDef{Name: "potion of instant healing"}})
	if g.PlayerHP <= 5 {
		t.Errorf("instant healing should restore HP, got %d", g.PlayerHP)
	}
}

func TestPainDamages(t *testing.T) {
	pain, ok := Registry()["pain"]
	if !ok {
		t.Fatal("pain not registered")
	}
	g := &game.Game{PlayerHP: 20, PlayerMax: 20}
	pain(g, &game.Item{Def: &content.ItemDef{Name: "potion of pain", Power: 6}})
	if g.PlayerHP != 14 {
		t.Errorf("pain should cost Power HP, got %d", g.PlayerHP)
	}
}

func TestIdentifyScrollReveals(t *testing.T) {
	idScroll, ok := Registry()["identify"]
	if !ok {
		t.Fatal("identify not registered")
	}
	c := &content.Content{Items: map[string]*content.ItemDef{
		"healing":  {ID: "healing", Name: "potion of healing", Kind: "potion", Use: "heal"},
		"idscroll": {ID: "idscroll", Name: "scroll of identify", Kind: "scroll", Use: "identify"},
	}}
	g := &game.Game{Content: c, RNG: rng.NewWithSeed(1)}
	g.AssignAppearances()
	pot := &game.Item{Def: c.Items["healing"]}
	g.Inventory = append(g.Inventory, pot)
	idScroll(g, &game.Item{Def: c.Items["idscroll"]})
	if !g.IsIdentified(pot) {
		t.Error("scroll of identify should reveal an unidentified carried item")
	}
}

func TestRechargeAddsCharges(t *testing.T) {
	recharge, ok := Registry()["recharge"]
	if !ok {
		t.Fatal("recharge not registered")
	}
	g := &game.Game{RNG: rng.NewWithSeed(1)}
	wand := &game.Item{Def: &content.ItemDef{Name: "wand", Kind: "wand", Damage: "1d4"}, Charges: 1}
	g.Inventory = append(g.Inventory, wand)
	recharge(g, &game.Item{Def: &content.ItemDef{Name: "scroll of recharge"}})
	if wand.Charges <= 1 {
		t.Errorf("recharge should add charges, got %d", wand.Charges)
	}
}

func TestRestoreEnergyRefillsEP(t *testing.T) {
	re, ok := Registry()["restore_energy"]
	if !ok {
		t.Fatal("restore_energy not registered")
	}
	g := &game.Game{EP: 2, EPMax: 10}
	re(g, &game.Item{Def: &content.ItemDef{Name: "potion of energy", Power: 5}})
	if g.EP != 7 {
		t.Errorf("EP = %d, want 7 after restoring 5", g.EP)
	}
	g.EP = 8
	re(g, &game.Item{Def: &content.ItemDef{Name: "potion of energy", Power: 5}})
	if g.EP != 10 {
		t.Errorf("EP should clamp at max, got %d", g.EP)
	}
}

func TestFlashBlindsNearby(t *testing.T) {
	flash, ok := Registry()["flash"]
	if !ok {
		t.Fatal("flash not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(10, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	rat := &game.Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	flash(g, &game.Item{Def: &content.ItemDef{Name: "spellbook of flash", Power: 6}})
	if !rat.HasEffect("blind") {
		t.Error("flash should blind a nearby creature")
	}
}

func TestNoxiousCloudPoisonsNearby(t *testing.T) {
	noxious, ok := Registry()["noxious_cloud"]
	if !ok {
		t.Fatal("noxious_cloud not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(10, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	rat := &game.Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	noxious(g, &game.Item{Def: &content.ItemDef{Name: "spellbook of noxious cloud", Power: 6}})
	if !rat.HasEffect("poison") {
		t.Error("noxious cloud should poison a nearby creature")
	}
}

func TestScareFrightensNearby(t *testing.T) {
	scare, ok := Registry()["scare"]
	if !ok {
		t.Fatal("scare not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(10, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	rat := &game.Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	scare(g, &game.Item{Def: &content.ItemDef{Name: "scroll of scare monster", Power: 6}})
	if !rat.HasEffect("fear") {
		t.Error("scare should frighten a nearby creature")
	}
}

func TestBlindnessAddsEffect(t *testing.T) {
	blind, ok := Registry()["blindness"]
	if !ok {
		t.Fatal("blindness not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	blind(g, &game.Item{Def: &content.ItemDef{Name: "potion of blindness", Power: 8}})
	if len(g.Effects) != 1 || g.Effects[0].Kind != "blind" || g.Effects[0].Turns != 8 {
		t.Errorf("drinking blindness should blind the player for Power turns, got %v", g.Effects)
	}
}

func TestFirstAidGrantsRegen(t *testing.T) {
	firstAid, ok := Registry()["first_aid"]
	if !ok {
		t.Fatal("first_aid not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 20}
	firstAid(g, &game.Item{Def: &content.ItemDef{Name: "spellbook of first aid", Power: 6}})
	if len(g.Effects) != 1 || g.Effects[0].Kind != "regen" || g.Effects[0].Turns != 6 {
		t.Errorf("first aid should grant regen for Power (6) turns, got %v", g.Effects)
	}
}

func TestEatMushroomHealsAndPoisons(t *testing.T) {
	eat, ok := Registry()["eat_mushroom"]
	if !ok {
		t.Fatal("eat_mushroom behavior not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 20}
	eat(g, &game.Item{Def: &content.ItemDef{Name: "red mushroom", Power: 4}})
	if g.PlayerHP != 14 {
		t.Errorf("HP = %d, want 14 (healed 4)", g.PlayerHP)
	}
	poisoned := false
	for _, e := range g.Effects {
		if e.Kind == "poison" {
			poisoned = true
		}
	}
	if !poisoned {
		t.Error("eating the red mushroom should poison the player")
	}
}

func TestHasteCancelsSlowAndHastens(t *testing.T) {
	haste, ok := Registry()["haste"]
	if !ok {
		t.Fatal("haste not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	g.AddEffect("slow", 10)
	msgs := haste(g, &game.Item{Def: &content.ItemDef{Name: "potion of speed", Power: 20}})
	if g.HasEffect("slow") {
		t.Error("a potion of speed cancels slow (C potions.c)")
	}
	if !g.HasEffect("haste") {
		t.Error("expected a haste effect from the potion of speed")
	}
	if len(msgs) == 0 || msgs[0] != "You move faster!" {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestSlownessCancelsHasteAndSlows(t *testing.T) {
	slowness, ok := Registry()["slowness"]
	if !ok {
		t.Fatal("slowness not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	g.AddEffect("haste", 10)
	msgs := slowness(g, &game.Item{Def: &content.ItemDef{Name: "potion of slowing", Power: 20}})
	if g.HasEffect("haste") {
		t.Error("a potion of slowing cancels haste (C potions.c)")
	}
	if !g.HasEffect("slow") {
		t.Error("expected a slow effect from the potion of slowing")
	}
	if len(msgs) == 0 || msgs[0] != "You feel very sluggish." {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestTranquilizePutsPlayerToSleep(t *testing.T) {
	tranq, ok := Registry()["tranquilize"]
	if !ok {
		t.Fatal("tranquilize not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	msgs := tranq(g, &game.Item{Def: &content.ItemDef{Name: "potion of sleep", Power: 25}})
	if !g.HasEffect("sleep") {
		t.Error("expected a sleep effect from the potion of sleep")
	}
	if len(msgs) == 0 || msgs[0] != "You fall asleep!" {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestLevitatePotionLiftsPlayer(t *testing.T) {
	lev, ok := Registry()["levitate"]
	if !ok {
		t.Fatal("levitate not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	msgs := lev(g, &game.Item{Def: &content.ItemDef{Name: "potion of levitation", Power: 20}})
	if !g.HasEffect("levitate") {
		t.Error("expected a levitate effect from the potion")
	}
	if len(msgs) == 0 || msgs[0] != "You soar into the air!" {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestElixirStripsAllListedEffects(t *testing.T) {
	elixir, ok := Registry()["elixir"]
	if !ok {
		t.Fatal("elixir not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	for _, kind := range []string{"poison", "haste", "blind", "fear"} {
		g.AddEffect(kind, 10)
	}
	msgs := elixir(g, &game.Item{Def: &content.ItemDef{Name: "elixir"}})
	for _, gone := range []string{"poison", "haste", "blind"} {
		if g.HasEffect(gone) {
			t.Errorf("elixir should strip %s (C treasure_elixir)", gone)
		}
	}
	if !g.HasEffect("fear") {
		t.Error("fear is not in the C's expiry list and should survive")
	}
	if len(msgs) == 0 || msgs[0] != "You feel perfectly normal." {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestYuckIsAllFlavor(t *testing.T) {
	yuck, ok := Registry()["yuck"]
	if !ok {
		t.Fatal("yuck not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10, RNG: rng.NewWithSeed(7)}
	msgs := yuck(g, &game.Item{Def: &content.ItemDef{Name: "potion of yuck"}})
	if len(msgs) == 0 || !strings.Contains(msgs[0], "This tastes like") {
		t.Errorf("expected a taste line, got %v", msgs)
	}
	if len(g.Effects) != 0 || g.PlayerHP != 10 {
		t.Error("yuck is pure flavor: no effects, no damage")
	}
}

func TestAmnesiaWipesTheMap(t *testing.T) {
	amn, ok := Registry()["amnesia"]
	if !ok {
		t.Fatal("amnesia not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(6, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	g.RevealMap()
	msgs := amn(g, &game.Item{Def: &content.ItemDef{Name: "scroll of amnesia"}})
	if g.Level.At(game.Pos{X: 4, Y: 1}).Seen {
		t.Error("amnesia should wipe the automap (C magic.c)")
	}
	if len(msgs) == 0 || msgs[0] != "You suddenly feel very forgetful." {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestMarkAndRecallBehaviors(t *testing.T) {
	mark, ok := Registry()["mark"]
	if !ok {
		t.Fatal("mark not registered")
	}
	recall, ok := Registry()["recall"]
	if !ok {
		t.Fatal("recall not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(8, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	msgs := mark(g, &game.Item{Def: &content.ItemDef{Name: "mark scroll"}})
	if len(msgs) == 0 || msgs[0] != "Destination marked." {
		t.Errorf("expected the C mark message, got %v", msgs)
	}
	home := g.Player
	g.Player = game.Pos{X: 5, Y: 1}
	msgs = recall(g, &game.Item{Def: &content.ItemDef{Name: "recall scroll"}})
	if g.Player != home {
		t.Errorf("recall should return to the mark, at %v", g.Player)
	}
	if len(msgs) == 0 || !strings.Contains(msgs[0], "You find yourself back") {
		t.Errorf("expected the C recall message, got %v", msgs)
	}
}

func TestUnmarkedRecallBehaviorFizzles(t *testing.T) {
	recall, ok := Registry()["recall"]
	if !ok {
		t.Fatal("recall not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(8, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	msgs := recall(g, &game.Item{Def: &content.ItemDef{Name: "recall scroll"}})
	if len(msgs) == 0 || !strings.Contains(msgs[0], "nothing happens") {
		t.Errorf("unmarked recall should fizzle, got %v", msgs)
	}
}

func TestDetectTrapsBehaviorRevealsAll(t *testing.T) {
	detect, ok := Registry()["detect_traps"]
	if !ok {
		t.Fatal("detect_traps not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	trap := &content.TileDef{ID: "dart_trap", Glyph: "^", Color: content.ColorRed, Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	g := &game.Game{Level: game.NewLevel(6, 3, floor), Player: game.Pos{X: 1, Y: 1}}
	g.Level.Set(game.Pos{X: 4, Y: 1}, trap)
	tile := g.Level.At(game.Pos{X: 4, Y: 1})
	tile.Disguise = floor
	tile.TrapDifficulty = 12
	msgs := detect(g, &game.Item{Def: &content.ItemDef{Name: "trap detection scroll"}})
	if !tile.Revealed {
		t.Error("the scroll exposes every trap on the level (C reveal_traps)")
	}
	if len(msgs) == 0 || msgs[0] != "You sense the presence of traps." {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestMagicWeaponIgnitesHands(t *testing.T) {
	mw, ok := Registry()["magic_weapon"]
	if !ok {
		t.Fatal("magic_weapon not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 10}
	msgs := mw(g, &game.Item{Def: &content.ItemDef{Name: "scroll of magic weapon", Power: 22}})
	if !g.HasEffect("flame_hands") {
		t.Error("expected flaming hands from the scroll")
	}
	if len(msgs) == 0 || msgs[0] != "Your hands seem to be on fire." {
		t.Errorf("expected the C message, got %v", msgs)
	}
}

func TestSummonFamiliarBringsAnImp(t *testing.T) {
	sf, ok := Registry()["summon_familiar"]
	if !ok {
		t.Fatal("summon_familiar not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{
		Level:   game.NewLevel(8, 3, floor),
		Player:  game.Pos{X: 1, Y: 1},
		Content: &content.Content{Monsters: map[string]*content.MonsterDef{"imp": {ID: "imp", Name: "imp", Glyph: "i", HP: 6, Attack: 8, Dodge: 2, Damage: "1d4"}}},
	}
	msgs := sf(g, &game.Item{Def: &content.ItemDef{Name: "scroll of Summon Familiar", Power: 500}})
	if len(g.Level.Creatures) != 1 || !g.Level.Creatures[0].Ally {
		t.Error("the scroll should summon a charmed imp")
	}
	if len(msgs) == 0 || msgs[0] != "The imp has arrived." {
		t.Errorf("expected the C arrival line, got %v", msgs)
	}
}

func TestPharmacyIdentifiesAllPotions(t *testing.T) {
	ph, ok := Registry()["pharmacy"]
	if !ok {
		t.Fatal("pharmacy not registered")
	}
	g := &game.Game{
		PlayerHP: 10, PlayerMax: 10,
		Content:    &content.Content{Items: map[string]*content.ItemDef{"potion_pain": {ID: "potion_pain", Name: "potion of pain", Kind: "potion"}}},
		Identified: map[string]bool{},
	}
	msgs := ph(g, &game.Item{Def: &content.ItemDef{Name: "manual of pharmacy"}})
	if !g.Identified["potion_pain"] {
		t.Error("the manual identifies every potion (C reading.c)")
	}
	if len(msgs) == 0 || msgs[0] != "You learn how to identify all potions." {
		t.Errorf("expected the C line, got %v", msgs)
	}
}
