package game

import "testing"

func TestUpdateFOVOpenField(t *testing.T) {
	c := testContent()
	l := NewLevel(21, 21, c.Tiles["floor"])
	g := &Game{Content: c, Level: l, Player: Pos{X: 10, Y: 10}}
	g.UpdateFOV()
	if !l.At(Pos{X: 10, Y: 10}).Visible || !l.At(Pos{X: 10, Y: 10}).Seen {
		t.Error("player tile should be visible and seen")
	}
	if !l.At(Pos{X: 12, Y: 10}).Visible {
		t.Error("nearby tile should be visible")
	}
	if l.At(Pos{X: 0, Y: 0}).Visible {
		t.Error("far corner (beyond radius) should not be visible")
	}
}

func TestUpdateFOVWallBlocks(t *testing.T) {
	c := testContent()
	l := NewLevel(11, 3, c.Tiles["floor"])
	for y := 0; y < 3; y++ {
		l.Set(Pos{X: 5, Y: y}, c.Tiles["wall"])
	}
	g := &Game{Content: c, Level: l, Player: Pos{X: 1, Y: 1}}
	g.UpdateFOV()
	if !l.At(Pos{X: 5, Y: 1}).Visible {
		t.Error("the wall itself should be visible")
	}
	if l.At(Pos{X: 9, Y: 1}).Visible {
		t.Error("tile behind the wall should be blocked")
	}
}

func TestUpdateFOVClearsStaleVisible(t *testing.T) {
	c := testContent()
	l := NewLevel(21, 21, c.Tiles["floor"])
	g := &Game{Content: c, Level: l, Player: Pos{X: 1, Y: 1}}
	g.UpdateFOV()
	near := Pos{X: 2, Y: 1}
	if !l.At(near).Visible {
		t.Fatal("precondition: near tile visible from start")
	}
	g.Player = Pos{X: 19, Y: 19}
	g.UpdateFOV()
	if l.At(near).Visible {
		t.Error("stale tile should no longer be Visible after moving away")
	}
	if !l.At(near).Seen {
		t.Error("tile should remain Seen after leaving FOV")
	}
}
