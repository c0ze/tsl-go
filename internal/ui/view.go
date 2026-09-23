package ui

// The View: a read-only snapshot of the game (map cells, HUD, messages) that
// a front-end draws.

import (
	"fmt"
	"strings"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
)

// Cell is one rendered grid cell.
type Cell struct {
	Glyph rune
	Color content.Color
	Dim   bool    // render dimmed (remembered-but-not-currently-visible)
	Light float64 // 0..1 brightness of a visible tile; renderers scale Colour by it for the torch-lit falloff
	// Entity is the content id of what stands on the cell — an item or
	// monster def id ("item" for one not yet identified), or "player" — and
	// "" for bare terrain. Graphic
	// front-ends pick sprites by it where glyph and colour are ambiguous
	// (boots and body armour share '[', corpses and rations '%').
	Entity string
}

// HUD carries the status line's segments individually so a front-end can
// style each one (the web colours HP by ratio, EP cyan, location amber, …);
// Line renders the plain single-string form the terminal draws.
type HUD struct {
	HP, HPMax int
	EP, EPMax int    // EPMax 0 hides the EP segment
	Location  string // current level's display name
	Wield     string // wielded weapon's display name, "none" when empty-handed
	Wear      string // worn armour's display name, "none" when unarmoured
	Worn      string // comma-joined worn accessories, "" when none
	Effects   string // comma-joined active effect labels, "" when none
}

// View is a read-only snapshot the front-end draws.
type View struct {
	W, H     int
	Cells    []Cell // len W*H, row-major
	Status   string // pre-rendered HUD line (HP, depth, gear) — always HUD.Line()
	HUD      HUD    // the same HUD segment by segment, for front-ends that colour them
	Messages []string
	LevelID  string   // current level id; front-ends that key off it (web music) use this
	Sounds   []string // per-turn sound-effect cues; front-ends that support it (web SFX) play them
	Base     []Cell   // terrain-only layer (entities are composited into Cells); web tiles draw terrain under entities
	Player   game.Pos // the player's map position; the terminal scrolls a short screen around it
}

// At returns a pointer to the cell at (x, y), which must be in bounds
// (0 <= x < W, 0 <= y < H). All callers in this package satisfy that.
func (v *View) At(x, y int) *Cell { return &v.Cells[y*v.W+x] }

// PlayerGlyph is the unpolymorphed player's glyph.
const PlayerGlyph = '@'

// PlayerColor is the player's glyph color.
const PlayerColor = content.ColorNormal

// BuildView produces the View for the current game state: tiles in the player's
// FOV are drawn bright, remembered (Seen) tiles dim, and unseen tiles blank.
func BuildView(g *game.Game) View {
	l := g.Level
	v := View{W: l.W, H: l.H, Cells: make([]Cell, l.W*l.H)}
	radius := g.VisionRadius()
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			t := l.At(game.Pos{X: x, Y: y})
			def := t.Appears() // an unrevealed trap wears its disguise
			switch {
			case t.Visible:
				*v.At(x, y) = Cell{Glyph: displayGlyph(l, x, y, def), Color: def.Color, Light: tileLight(g.Player.X, g.Player.Y, x, y, radius)}
			case t.Seen:
				*v.At(x, y) = Cell{Glyph: displayGlyph(l, x, y, def), Color: def.Color, Dim: true}
			default:
				*v.At(x, y) = Cell{Glyph: ' ', Color: content.ColorNormal}
			}
		}
	}
	v.Base = append([]Cell(nil), v.Cells...) // snapshot the terrain before entities composite on top
	for _, it := range l.Items {
		if l.InBounds(it.Pos) && l.At(it.Pos).Visible {
			paint(v.At(it.Pos.X, it.Pos.Y), it.Def.Rune(), it.Def.Color, entityID(g, it.Def))
		}
	}
	for _, m := range l.Creatures {
		if !l.InBounds(m.Pos) || !l.At(m.Pos).Visible {
			continue
		}
		if m.Disguised && m.DisguiseAs != nil { // a mimic wears its loot glamour
			paint(v.At(m.Pos.X, m.Pos.Y), m.DisguiseAs.Rune(), m.DisguiseAs.Color, entityID(g, m.DisguiseAs))
			continue
		}
		paint(v.At(m.Pos.X, m.Pos.Y), m.Def.Rune(), m.Def.Color, m.Def.ID)
	}
	if l.InBounds(g.Player) {
		if g.Shape != nil { // a polymorphed player wears the form's glyph
			paint(v.At(g.Player.X, g.Player.Y), g.Shape.Rune(), g.Shape.Color, g.Shape.ID)
		} else {
			paint(v.At(g.Player.X, g.Player.Y), PlayerGlyph, PlayerColor, "player")
		}
	}
	v.HUD = buildHUD(g)
	v.Status = v.HUD.Line()
	v.Messages = lastN(g.Messages, 4)
	v.LevelID = l.ID
	v.Sounds = g.Sounds
	v.Player = g.Player
	return v
}

// entityID is the id a front-end may see for an item on the ground: its def
// id once the player knows what it is, else the neutral "item" — an
// unidentified potion must not name its type to the page.
func entityID(g *game.Game, def *content.ItemDef) string {
	if !g.IsIdentified(&game.Item{Def: def}) {
		return "item"
	}
	return def.ID
}

// paint overlays a glyph and colour onto an already-built cell, keeping the
// light the underlying visible tile carries so items, creatures, and the player
// dim with the torchlight like the floor they stand on.
func paint(c *Cell, glyph rune, color content.Color, entity string) {
	c.Glyph, c.Color, c.Entity = glyph, color, entity
}

// buildHUD summarises the player's vitals and gear, segment by segment.
func buildHUD(g *game.Game) HUD {
	h := HUD{
		HP: g.PlayerHP, HPMax: g.PlayerMax,
		EP: g.EP, EPMax: g.EPMax,
		Location: g.LocationName(),
		Wield:    "none", Wear: "none",
		Worn:    wornAccessories(g),
		Effects: g.EffectsSummary(),
	}
	if h.Location == "" {
		h.Location = "the dungeon"
	}
	if g.Weapon != nil && g.Weapon.Def != nil {
		h.Wield = g.DisplayName(g.Weapon)
	}
	if g.Armor != nil && g.Armor.Def != nil {
		h.Wear = g.DisplayName(g.Armor)
	}
	return h
}

// Line renders the HUD as the plain status line the terminal draws (and the
// web falls back to conceptually — its coloured spans carry the same text).
func (h HUD) Line() string {
	s := fmt.Sprintf("HP %d/%d", h.HP, h.HPMax)
	if h.EPMax > 0 {
		s += fmt.Sprintf("   EP %d/%d", h.EP, h.EPMax)
	}
	s += fmt.Sprintf("   %s   Wield: %s   Wear: %s", h.Location, h.Wield, h.Wear)
	if h.Worn != "" {
		s += "   Worn: " + h.Worn
	}
	if h.Effects != "" {
		s += "   [" + h.Effects + "]"
	}
	return s
}

// wornAccessories joins the display names of the worn ring and amulet, returning
// "" when neither slot is filled (so the HUD omits the segment entirely).
func wornAccessories(g *game.Game) string {
	var names []string
	for _, it := range []*game.Item{g.Ring, g.Amulet, g.Boots, g.Head, g.Cloak} {
		if it != nil && it.Def != nil {
			names = append(names, g.DisplayName(it))
		}
	}
	return strings.Join(names, ", ")
}

// lastN returns up to the last n elements of s.
func lastN(s []string, n int) []string {
	if len(s) <= n {
		return s
	}
	return s[len(s)-n:]
}
