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
