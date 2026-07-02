package web

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/ui"
)

func miniView() ui.View {
	v := ui.View{W: 3, H: 1, Cells: make([]ui.Cell, 3)}
	*v.At(0, 0) = ui.Cell{Glyph: '@', Color: content.ColorNormal}
	*v.At(1, 0) = ui.Cell{Glyph: '<', Color: content.ColorNormal} // must escape
	*v.At(2, 0) = ui.Cell{Glyph: '#', Color: content.ColorNormal, Dim: true}
	return v
}

func TestRenderHTMLEscapesAndRuns(t *testing.T) {
	out := RenderHTML(miniView(), nil)
	if !strings.Contains(out, "&lt;") {
		t.Error("glyphs must be HTML-escaped")
	}
	if strings.Count(out, "<span") != 2 {
		t.Errorf("identical styles should share a span run, got %d spans in %q", strings.Count(out, "<span"), out)
	}
	if !strings.Contains(out, `class="c-normal dim"`) {
		t.Errorf("dim cells carry the dim class, got %q", out)
	}
}

func TestLightClassBuckets(t *testing.T) {
	cases := []struct {
		light float64
		want  string
	}{
		{1.0, ""}, // at the player's feet: full torchlight, no class
		{0.9, ""}, // rounds into the top bucket
		{0.7, " lt3"},
		{0.5, " lt2"},
		{0.2, " lt1"}, // tileLight's floor at the edge of sight
		{0.0, " lt0"},
		{-1, " lt0"}, // clamped
		{2, ""},      // clamped
	}
	for _, c := range cases {
		if got := lightClass(c.light); got != c.want {
			t.Errorf("lightClass(%v) = %q, want %q", c.light, got, c.want)
		}
	}
}

func TestRenderHTMLTorchLight(t *testing.T) {
	v := ui.View{W: 4, H: 1, Cells: make([]ui.Cell, 4)}
	*v.At(0, 0) = ui.Cell{Glyph: '@', Color: content.ColorNormal, Light: 1.0}
	*v.At(1, 0) = ui.Cell{Glyph: '·', Color: content.ColorNormal, Light: 0.5}
	*v.At(2, 0) = ui.Cell{Glyph: '·', Color: content.ColorNormal, Light: 0.5}
	*v.At(3, 0) = ui.Cell{Glyph: '#', Color: content.ColorNormal, Dim: true}
	out := RenderHTML(v, nil)
	if !strings.Contains(out, `class="c-normal">@`) {
		t.Errorf("a fully-lit cell keeps the bare colour class, got %q", out)
	}
	if !strings.Contains(out, `class="c-normal lt2">··`) {
		t.Errorf("equally-lit cells share one bucketed span run, got %q", out)
	}
	if strings.Contains(out, "dim lt") || strings.Contains(out, `lt2 dim`) {
		t.Errorf("remembered tiles carry dim, never a light bucket, got %q", out)
	}
	if !strings.Contains(out, `class="c-normal dim"`) {
		t.Errorf("remembered tiles keep the dim class, got %q", out)
	}
}

func TestRenderHTMLCursorStaysBright(t *testing.T) {
	v := ui.View{W: 2, H: 1, Cells: make([]ui.Cell, 2)}
	*v.At(0, 0) = ui.Cell{Glyph: '·', Color: content.ColorNormal, Light: 0.4}
	*v.At(1, 0) = ui.Cell{Glyph: '·', Color: content.ColorNormal, Light: 0.4}
	cur := game.Pos{X: 1, Y: 0}
	out := RenderHTML(v, &cur)
	if !strings.Contains(out, `class="c-normal cursor"`) {
		t.Errorf("the crosshair cell skips the light bucket to stay full-bright, got %q", out)
	}
}

func TestRenderHTMLCursor(t *testing.T) {
	cur := game.Pos{X: 1, Y: 0}
	out := RenderHTML(miniView(), &cur)
	if !strings.Contains(out, "cursor") {
		t.Errorf("the target crosshair should mark its cell, got %q", out)
	}
}

func TestStatusHTMLSegments(t *testing.T) {
	v := ui.View{HUD: ui.HUD{HP: 15, HPMax: 20, EP: 3, EPMax: 10,
		Location: "Dungeon <1>", Wield: "dagger", Wear: "none",
		Worn: "ring of protection", Effects: "Poisoned"}}
	out := StatusHTML(v)
	for _, want := range []string{
		`<span class="s-hp-good">HP 15/20</span>`,
		`<span class="s-ep">EP 3/10</span>`,
		`<span class="s-loc">Dungeon &lt;1&gt;</span>`, // and escaped
		`<span class="s-label">Wield:</span> dagger`,
		`<span class="s-label">Worn:</span> ring of protection`,
		`<span class="s-eff">[Poisoned]</span>`,
	} {
		if !strings.Contains(out, want) {
			t.Errorf("status HTML missing %q, got %q", want, out)
		}
	}
}

func TestStatusHTMLOmitsEmptySegments(t *testing.T) {
	v := ui.View{HUD: ui.HUD{HP: 2, HPMax: 20, Location: "the dungeon", Wield: "none", Wear: "none"}}
	out := StatusHTML(v)
	if strings.Contains(out, "EP") || strings.Contains(out, "Worn") || strings.Contains(out, "s-eff") {
		t.Errorf("EP/Worn/effects segments should be omitted when empty, got %q", out)
	}
	if !strings.Contains(out, `<span class="s-hp-crit">HP 2/20</span>`) {
		t.Errorf("low HP should class as critical, got %q", out)
	}
}

func TestHPClassRatios(t *testing.T) {
	cases := []struct {
		hp, max int
		want    string
	}{
		{20, 20, "s-hp-good"},
		{14, 20, "s-hp-good"}, // just over 2/3
		{13, 20, "s-hp-warn"},
		{7, 20, "s-hp-warn"}, // just over 1/3
		{6, 20, "s-hp-crit"},
		{0, 20, "s-hp-crit"},
		{5, 0, "s-hp-good"}, // degenerate max: never divide by zero
	}
	for _, c := range cases {
		if got := hpClass(c.hp, c.max); got != c.want {
			t.Errorf("hpClass(%d, %d) = %q, want %q", c.hp, c.max, got, c.want)
		}
	}
}

func TestMessageClassSeverity(t *testing.T) {
	cases := []struct {
		msg, want string
	}{
		{"The gnoblin hits you for 3.", "m-bad"},
		{"The imp blasts you for 5.", "m-bad"},
		{"You die...", "m-bad"},
		{"The poison overcomes you. You die.", "m-bad"},
		{"You get burned by lava!", "m-bad"},
		{"You step on a polymorph trap!", "m-bad"},
		// monsterAttacks' effect-application follow-ups are harm too
		{"The pit viper poisons you.", "m-bad"},
		{"The sludge dweller slows you.", "m-bad"},
		{"The floating brain confuses you.", "m-bad"},
		{"The wraith frightens you.", "m-bad"},
		{"The severed hand afflicts you.", "m-bad"},
		{"You pick up the crowbar.", "m-good"},
		{"You quaff the potion of healing and recover 6 HP.", "m-good"},
		{"You learn force bolt.", "m-good"},
		{"You ascend to demigodhood. You win!", "m-good"},
		{"You hit the gnoblin for 4.", ""}, // your own blows stay neutral
		{"The gnoblin misses you.", ""},
		{"You open the door.", ""},
		// killCreature names no killer — "The imp dies." could be the player's
		// ally felled by a hostile, so deaths stay neutral
		{"The gnoblin dies.", ""},
		{"The imp dies.", ""},
		// the player's own wand applying an effect phrases "...the <monster>."
		{"The wand of slowness slows the gnoblin.", ""},
		{"The wand of misery afflicts the gnoblin.", ""},
	}
	for _, c := range cases {
		if got := messageClass(c.msg); got != c.want {
			t.Errorf("messageClass(%q) = %q, want %q", c.msg, got, c.want)
		}
	}
}

func TestMessagesHTML(t *testing.T) {
	out := MessagesHTML([]string{"The rat hits you for 2.", "You pick up the <sword>.", "You open the door."})
	if !strings.Contains(out, `<span class="m-bad">The rat hits you for 2.</span>`) {
		t.Errorf("damage lines carry m-bad, got %q", out)
	}
	if !strings.Contains(out, `<span class="m-good">You pick up the &lt;sword&gt;.</span>`) {
		t.Errorf("pickup lines carry m-good and escape HTML, got %q", out)
	}
	if !strings.Contains(out, "You open the door.\n") || strings.Contains(out, `<span>You open`) {
		t.Errorf("neutral lines render bare, got %q", out)
	}
}

func TestMenuHTMLMarksSelection(t *testing.T) {
	out := MenuHTML(ui.MenuSpec{Title: "Eat what?", Items: []string{"ration", "corpse"}}, 1)
	if !strings.Contains(out, "&gt; b) corpse") {
		t.Errorf("the selection marker should sit on entry b, got %q", out)
	}
	if !strings.Contains(out, "  a) ration") {
		t.Errorf("unselected entries keep the plain prefix, got %q", out)
	}
}
