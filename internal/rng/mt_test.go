package rng

import "testing"

// Canonical MT19937 output for init_by_array({0x123,0x234,0x345,0x456})
// (see the commented main() in common/mt19937ar.c / the reference mt19937ar.out).
func TestKnownVector(t *testing.T) {
	g := NewWithKey([]uint32{0x123, 0x234, 0x345, 0x456})
	// Verified against the reference C (common/mt19937ar.c) compiled and run.
	want := []uint32{
		1067595299, 955945823, 477289528, 4107218783, 4228976476,
		3344332714, 3355579695, 227628506,
	}
	for i, w := range want {
		if got := g.Uint32(); got != w {
			t.Fatalf("Uint32 #%d = %d, want %d", i, got, w)
		}
	}
}

func TestSeedDeterministic(t *testing.T) {
	a := NewWithSeed(12345)
	b := NewWithSeed(12345)
	for i := 0; i < 100; i++ {
		if x, y := a.Uint32(), b.Uint32(); x != y {
			t.Fatalf("same seed diverged at %d: %d != %d", i, x, y)
		}
	}
}

func TestIntnRange(t *testing.T) {
	g := NewWithSeed(1)
	for i := 0; i < 10000; i++ {
		v := g.Intn(7)
		if v < 0 || v >= 7 {
			t.Fatalf("Intn(7) = %d, out of [0,7)", v)
		}
	}
}

func TestIntnPanicsOnNonPositive(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Fatal("expected panic for Intn(0)")
		}
	}()
	NewWithSeed(1).Intn(0)
}

func TestNewWithKeyPanicsOnEmptyKey(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Fatal("expected panic for NewWithKey([])")
		}
	}()
	NewWithKey([]uint32{})
}

func TestSnapshotRestoreContinuesSequence(t *testing.T) {
	g := NewWithSeed(42)
	for i := 0; i < 100; i++ {
		g.Uint32() // advance into the stream
	}
	snap := g.Snapshot()
	want := make([]uint32, 50)
	for i := range want {
		want[i] = g.Uint32()
	}
	r := Restore(snap)
	for i := range want {
		if got := r.Uint32(); got != want[i] {
			t.Fatalf("restored stream diverged at %d: got %d, want %d", i, got, want[i])
		}
	}
}

func TestRestoreRejectsBadSnapshot(t *testing.T) {
	if Restore([]uint32{1, 2, 3}) != nil {
		t.Error("a truncated snapshot must not restore")
	}
}
