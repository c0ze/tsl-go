package game

import (
	"encoding/json"
	"fmt"
	"io"
	"sort"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

// Save/load (#20, C saveload.c). The C dumps raw structs through a pointer
// table; the format here is JSON DTOs — the faithful part is the behavior
// (an exact resume), not the bytes. Def pointers travel as content IDs and
// the equipped slots as inventory indices.

type savedEffect struct {
	Kind  string `json:"k"`
	Turns int    `json:"t"`
}

type savedItem struct {
	ID      string `json:"id"`
	Pos     Pos    `json:"pos"`
	Charges int    `json:"ch,omitempty"`
}

type savedCreature struct {
	ID         string        `json:"id"`
	Pos        Pos           `json:"pos"`
	HP         int           `json:"hp"`
	Faction    Faction       `json:"f,omitempty"`
	Energy     int           `json:"e,omitempty"`
	Effects    []savedEffect `json:"fx,omitempty"`
	Ally       bool          `json:"ally,omitempty"`
	Lifetime   int           `json:"life,omitempty"`
	Disguised  bool          `json:"dis,omitempty"`
	DisguiseAs string        `json:"das,omitempty"`
}

type savedTile struct {
	ID             string `json:"id"`
	Visible        bool   `json:"v,omitempty"`
	Seen           bool   `json:"s,omitempty"`
	Disguise       string `json:"d,omitempty"`
	Revealed       bool   `json:"r,omitempty"`
	TrapDifficulty int    `json:"td,omitempty"`
}

type savedLevel struct {
	ID        string          `json:"id"`
	W         int             `json:"w"`
	H         int             `json:"h"`
	Tiles     []savedTile     `json:"tiles"`
	Creatures []savedCreature `json:"creatures,omitempty"`
	Items     []savedItem     `json:"items,omitempty"`
	Start     Pos             `json:"start"`
	Return    Pos             `json:"return"`
	Entered   bool            `json:"entered,omitempty"`
	Portals   []Portal        `json:"portals,omitempty"`
	Dark      bool            `json:"dark,omitempty"`
}

type savedGame struct {
	Player       Pos               `json:"player"`
	PlayerHP     int               `json:"hp"`
	PlayerMax    int               `json:"hpmax"`
	EP           int               `json:"ep"`
	EPMax        int               `json:"epmax"`
	EPTurn       int               `json:"epturn,omitempty"`
	PlayerEnergy int               `json:"penergy,omitempty"`
	SwimFatigue  int               `json:"swim,omitempty"`
	RecallLevel  string            `json:"rlevel,omitempty"`
	RecallPos    Pos               `json:"rpos,omitempty"`
	RecallSet    bool              `json:"rset,omitempty"`
	Dead         bool              `json:"dead,omitempty"`
	Won          bool              `json:"won,omitempty"`
	DeathCause   string            `json:"cause,omitempty"`
	Messages     []string          `json:"msgs,omitempty"`
	Effects      []savedEffect     `json:"fx,omitempty"`
	Identified   map[string]bool   `json:"identified,omitempty"`
	Known        map[string]bool   `json:"known,omitempty"`
	Shape        string            `json:"shape,omitempty"`
	Appearances  map[string]string `json:"appearances,omitempty"`
	Inventory    []savedItem       `json:"inventory,omitempty"`
	Weapon       int               `json:"weapon"` // inventory indices; -1 = empty slot
	Armor        int               `json:"armor"`
	Ring         int               `json:"ring"`
	Amulet       int               `json:"amulet"`
	Boots        int               `json:"boots"`
	Head         int               `json:"head"`
	Cloak        int               `json:"cloak"`
	RNG          []uint32          `json:"rng,omitempty"`
	Current      string            `json:"current"` // the level the player is on
	Levels       []savedLevel      `json:"levels"`
}

func saveEffects(fx []Effect) []savedEffect {
	out := make([]savedEffect, 0, len(fx))
	for _, e := range fx {
		out = append(out, savedEffect{Kind: e.Kind, Turns: e.Turns})
	}
	return out
}

func loadEffects(fx []savedEffect) []Effect {
	out := make([]Effect, 0, len(fx))
	for _, e := range fx {
		out = append(out, Effect{Kind: e.Kind, Turns: e.Turns})
	}
	return out
}

// slotIndex locates an equipped item inside the inventory (-1 when empty).
func slotIndex(inv []*Item, slot *Item) int {
	for i, it := range inv {
		if it == slot {
			return i
		}
	}
	return -1
}

// Save serializes the whole game — player, clocks, identify state, and the
// Dungeon's entire level cache — as JSON. The Behaviors registry, content
// defs, and the level builder are runtime wiring, re-injected on load.
func (g *Game) Save(w io.Writer) error {
	sg := savedGame{
		Player: g.Player, PlayerHP: g.PlayerHP, PlayerMax: g.PlayerMax,
		EP: g.EP, EPMax: g.EPMax, EPTurn: g.epTurn,
		PlayerEnergy: g.playerEnergy, SwimFatigue: g.swimFatigue,
		RecallLevel: g.recallLevel, RecallPos: g.recallPos, RecallSet: g.recallSet,
		Dead: g.Dead, Won: g.Won, DeathCause: g.DeathCause,
		Messages: g.Messages, Effects: saveEffects(g.Effects),
		Identified: g.Identified, Known: g.Known, Appearances: g.appearances,
		Weapon: slotIndex(g.Inventory, g.Weapon), Armor: slotIndex(g.Inventory, g.Armor),
		Ring: slotIndex(g.Inventory, g.Ring), Amulet: slotIndex(g.Inventory, g.Amulet),
		Boots: slotIndex(g.Inventory, g.Boots), Head: slotIndex(g.Inventory, g.Head),
		Cloak: slotIndex(g.Inventory, g.Cloak),
	}
	if g.Shape != nil {
		sg.Shape = g.Shape.ID
	}
	if g.RNG != nil {
		sg.RNG = g.RNG.Snapshot()
	}
	for _, it := range g.Inventory {
		sg.Inventory = append(sg.Inventory, savedItem{ID: it.Def.ID, Pos: it.Pos, Charges: it.Charges})
	}
	if g.Dungeon != nil {
		sg.Current = g.Dungeon.current
		ids := make([]string, 0, len(g.Dungeon.cache))
		for id := range g.Dungeon.cache {
			ids = append(ids, id)
		}
		sort.Strings(ids) // deterministic save bytes
		for _, id := range ids {
			sg.Levels = append(sg.Levels, saveLevel(id, g.Dungeon.cache[id]))
		}
	} else if g.Level != nil { // bare unit-test games: a single anonymous level
		sg.Levels = append(sg.Levels, saveLevel(g.Level.ID, g.Level))
		sg.Current = g.Level.ID
	}
	enc := json.NewEncoder(w)
	return enc.Encode(&sg)
}

func saveLevel(id string, l *Level) savedLevel {
	sl := savedLevel{
		ID: id, W: l.W, H: l.H,
		Start: l.Start, Return: l.Return, Entered: l.entered,
		Portals: l.Portals, Dark: l.Dark,
	}
	sl.Tiles = make([]savedTile, len(l.tiles))
	for i, t := range l.tiles {
		st := savedTile{ID: t.Def.ID, Visible: t.Visible, Seen: t.Seen,
			Revealed: t.Revealed, TrapDifficulty: t.TrapDifficulty}
		if t.Disguise != nil {
			st.Disguise = t.Disguise.ID
		}
		sl.Tiles[i] = st
	}
	for _, c := range l.Creatures {
		sc := savedCreature{ID: c.Def.ID, Pos: c.Pos, HP: c.HP, Faction: c.Faction,
			Energy: c.Energy, Effects: saveEffects(c.Effects),
			Ally: c.Ally, Lifetime: c.Lifetime, Disguised: c.Disguised}
		if c.DisguiseAs != nil {
			sc.DisguiseAs = c.DisguiseAs.ID
		}
		sl.Creatures = append(sl.Creatures, sc)
	}
	for _, it := range l.Items {
		sl.Items = append(sl.Items, savedItem{ID: it.Def.ID, Pos: it.Pos, Charges: it.Charges})
	}
	return sl
}

// LoadGame reconstructs a saved game. Content defs, the behaviors registry,
// and the level builder are the same runtime wiring cmd injects at boot; an
// ID the content no longer knows fails the load (the fail-fast convention).
func LoadGame(r io.Reader, c *content.Content, behaviors map[string]Behavior,
	build func(*content.LevelDef) (*Level, error)) (*Game, error) {
	var sg savedGame
	if err := json.NewDecoder(r).Decode(&sg); err != nil {
		return nil, fmt.Errorf("load: %w", err)
	}
	g := &Game{
		Content: c, Behaviors: behaviors,
		Player: sg.Player, PlayerHP: sg.PlayerHP, PlayerMax: sg.PlayerMax,
		EP: sg.EP, EPMax: sg.EPMax, epTurn: sg.EPTurn,
		playerEnergy: sg.PlayerEnergy, swimFatigue: sg.SwimFatigue,
		recallLevel: sg.RecallLevel, recallPos: sg.RecallPos, recallSet: sg.RecallSet,
		Dead: sg.Dead, Won: sg.Won, DeathCause: sg.DeathCause,
		Messages: sg.Messages, Effects: loadEffects(sg.Effects),
		Identified: sg.Identified, Known: sg.Known, appearances: sg.Appearances,
	}
	if sg.Shape != "" {
		if g.Shape = c.Monsters[sg.Shape]; g.Shape == nil {
			return nil, fmt.Errorf("load: unknown shape %q", sg.Shape)
		}
	}
	if sg.RNG != nil {
		if g.RNG = rng.Restore(sg.RNG); g.RNG == nil {
			return nil, fmt.Errorf("load: corrupt RNG snapshot")
		}
	}
	for _, si := range sg.Inventory {
		it, err := loadItem(c, si)
		if err != nil {
			return nil, err
		}
		g.Inventory = append(g.Inventory, it)
	}
	for slot, idx := range map[**Item]int{&g.Weapon: sg.Weapon, &g.Armor: sg.Armor, &g.Ring: sg.Ring, &g.Amulet: sg.Amulet, &g.Boots: sg.Boots, &g.Head: sg.Head, &g.Cloak: sg.Cloak} {
		if idx >= 0 {
			if idx >= len(g.Inventory) {
				return nil, fmt.Errorf("load: equipped slot index %d out of range", idx)
			}
			*slot = g.Inventory[idx]
		}
	}
	cache := map[string]*Level{}
	for _, sl := range sg.Levels {
		lvl, err := loadLevel(c, sl)
		if err != nil {
			return nil, err
		}
		cache[sl.ID] = lvl
	}
	if cache[sg.Current] == nil {
		return nil, fmt.Errorf("load: current level %q missing from save", sg.Current)
	}
	g.Level = cache[sg.Current]
	if len(c.Levels) > 0 { // a real dungeon graph (bare test games have none)
		g.Dungeon = &Dungeon{defs: c.Levels, cache: cache, current: sg.Current, build: build}
	}
	return g, nil
}

func loadItem(c *content.Content, si savedItem) (*Item, error) {
	def := c.Items[si.ID]
	if def == nil {
		return nil, fmt.Errorf("load: unknown item %q", si.ID)
	}
	return &Item{Def: def, Pos: si.Pos, Charges: si.Charges}, nil
}

func loadLevel(c *content.Content, sl savedLevel) (*Level, error) {
	if len(sl.Tiles) != sl.W*sl.H {
		return nil, fmt.Errorf("load: level %q has %d tiles for %dx%d", sl.ID, len(sl.Tiles), sl.W, sl.H)
	}
	lvl := &Level{
		W: sl.W, H: sl.H, ID: sl.ID,
		Start: sl.Start, Return: sl.Return, entered: sl.Entered,
		Portals: sl.Portals, Dark: sl.Dark,
		tiles: make([]Tile, len(sl.Tiles)),
	}
	for i, st := range sl.Tiles {
		def := c.Tiles[st.ID]
		if def == nil {
			return nil, fmt.Errorf("load: unknown tile %q", st.ID)
		}
		t := Tile{Def: def, Visible: st.Visible, Seen: st.Seen,
			Revealed: st.Revealed, TrapDifficulty: st.TrapDifficulty}
		if st.Disguise != "" {
			if t.Disguise = c.Tiles[st.Disguise]; t.Disguise == nil {
				return nil, fmt.Errorf("load: unknown disguise tile %q", st.Disguise)
			}
		}
		lvl.tiles[i] = t
	}
	for _, sc := range sl.Creatures {
		def := c.Monsters[sc.ID]
		if def == nil {
			return nil, fmt.Errorf("load: unknown monster %q", sc.ID)
		}
		cr := &Creature{Def: def, Pos: sc.Pos, HP: sc.HP, Faction: sc.Faction,
			Energy: sc.Energy, Effects: loadEffects(sc.Effects),
			Ally: sc.Ally, Lifetime: sc.Lifetime, Disguised: sc.Disguised}
		if sc.DisguiseAs != "" {
			if cr.DisguiseAs = c.Items[sc.DisguiseAs]; cr.DisguiseAs == nil {
				return nil, fmt.Errorf("load: unknown disguise item %q", sc.DisguiseAs)
			}
		}
		lvl.Creatures = append(lvl.Creatures, cr)
	}
	for _, si := range sl.Items {
		it, err := loadItem(c, si)
		if err != nil {
			return nil, err
		}
		lvl.Items = append(lvl.Items, it)
	}
	return lvl, nil
}
