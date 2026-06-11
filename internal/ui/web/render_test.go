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
