package rng

import "testing"

func TestRollInRange(t *testing.T) {
	g := NewWithSeed(1)
	for i := 0; i < 1000; i++ {
		v := g.Roll(2, 6) // 2..12
		if v < 2 || v > 12 {
			t.Fatalf("Roll(2,6) = %d out of [2,12]", v)
		}
	}
}

func TestChanceBounds(t *testing.T) {
	g := NewWithSeed(1)
	// x huge vs n=1 → almost always true; n huge vs x=1 → almost always false.
	hi := 0
	for i := 0; i < 1000; i++ {
		if g.Chance(100, 1) {
			hi++
		}
	}
	if hi < 900 {
		t.Errorf("Chance(100,1) true only %d/1000, expected most", hi)
	}
	lo := 0
	for i := 0; i < 1000; i++ {
		if g.Chance(1, 100) {
			lo++
		}
	}
	if lo > 100 {
		t.Errorf("Chance(1,100) true %d/1000, expected few", lo)
	}
}

func TestRollSpec(t *testing.T) {
	g := NewWithSeed(1)
	for i := 0; i < 1000; i++ {
		if v := g.RollSpec("1d4"); v < 1 || v > 4 {
			t.Fatalf("RollSpec(1d4) = %d out of [1,4]", v)
		}
		if v := g.RollSpec("2d3+1"); v < 3 || v > 7 {
			t.Fatalf("RollSpec(2d3+1) = %d out of [3,7]", v)
		}
	}
	if g.RollSpec("garbage") != 0 {
		t.Error("RollSpec on malformed spec should return 0")
	}
}
