package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestItemAtAndRemove(t *testing.T) {
	l := NewLevel(5, 5, testContent().Tiles["floor"])
	it := &Item{Def: &content.ItemDef{ID: "potion", Glyph: "!"}, Pos: Pos{2, 2}}
	l.Items = append(l.Items, it)
	if l.ItemAt(Pos{2, 2}) != it {
		t.Fatal("ItemAt(2,2) should find the item")
	}
	if l.ItemAt(Pos{0, 0}) != nil {
		t.Error("ItemAt(0,0) should be nil")
	}
	l.RemoveItem(it)
	if l.ItemAt(Pos{2, 2}) != nil {
		t.Error("item should be gone after RemoveItem")
	}
}
