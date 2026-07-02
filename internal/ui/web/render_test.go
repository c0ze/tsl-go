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

func TestMenuHTMLMarksSelection(t *testing.T) {
	out := MenuHTML(ui.MenuSpec{Title: "Eat what?", Items: []string{"ration", "corpse"}}, 1)
	if !strings.Contains(out, "&gt; b) corpse") {
		t.Errorf("the selection marker should sit on entry b, got %q", out)
	}
	if !strings.Contains(out, "  a) ration") {
		t.Errorf("unselected entries keep the plain prefix, got %q", out)
	}
}
