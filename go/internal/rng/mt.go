// Package rng is a faithful Go port of the Mersenne Twister MT19937 PRNG
// (mt19937ar.c by Matsumoto & Nishimura), used so dungeon generation is
// reproducible from a seed. Ported from common/mt19937ar.c; Uint32 reproduces
// genrand_int32 bit-for-bit. Not safe for concurrent use.
package rng

const (
	n         = 624
	m         = 397
	matrixA   = 0x9908b0df
	upperMask = 0x80000000
	lowerMask = 0x7fffffff
)

// MT is a Mersenne Twister generator.
type MT struct {
	mt  [n]uint32
	mti int
}

// NewWithSeed seeds with a single 32-bit value (init_genrand).
func NewWithSeed(s uint32) *MT {
	g := &MT{}
	g.initGenrand(s)
	return g
}

// NewWithKey seeds with a key array (init_by_array). It panics on an empty key.
func NewWithKey(key []uint32) *MT {
	if len(key) == 0 {
		panic("rng: NewWithKey requires a non-empty key")
	}
	g := &MT{}
	g.initByArray(key)
	return g
}

func (g *MT) initGenrand(s uint32) {
	g.mt[0] = s
	for i := 1; i < n; i++ {
		g.mt[i] = 1812433253*(g.mt[i-1]^(g.mt[i-1]>>30)) + uint32(i)
	}
	g.mti = n
}

func (g *MT) initByArray(key []uint32) {
	g.initGenrand(19650218)
	i, j := 1, 0
	k := n
	if len(key) > k {
		k = len(key)
	}
	for ; k > 0; k-- {
		g.mt[i] = (g.mt[i] ^ ((g.mt[i-1] ^ (g.mt[i-1] >> 30)) * 1664525)) + key[j] + uint32(j)
		i++
		j++
		if i >= n {
			g.mt[0] = g.mt[n-1]
			i = 1
		}
		if j >= len(key) {
			j = 0
		}
	}
	for k = n - 1; k > 0; k-- {
		g.mt[i] = (g.mt[i] ^ ((g.mt[i-1] ^ (g.mt[i-1] >> 30)) * 1566083941)) - uint32(i)
		i++
		if i >= n {
			g.mt[0] = g.mt[n-1]
			i = 1
		}
	}
	g.mt[0] = 0x80000000
}

// Uint32 returns the next value on [0, 2^32) (genrand_int32).
func (g *MT) Uint32() uint32 {
	var y uint32
	mag01 := [2]uint32{0, matrixA}
	if g.mti >= n {
		if g.mti == n+1 {
			g.initGenrand(5489)
		}
		var kk int
		for kk = 0; kk < n-m; kk++ {
			y = (g.mt[kk] & upperMask) | (g.mt[kk+1] & lowerMask)
			g.mt[kk] = g.mt[kk+m] ^ (y >> 1) ^ mag01[y&1]
		}
		for ; kk < n-1; kk++ {
			y = (g.mt[kk] & upperMask) | (g.mt[kk+1] & lowerMask)
			g.mt[kk] = g.mt[kk+(m-n)] ^ (y >> 1) ^ mag01[y&1]
		}
		y = (g.mt[n-1] & upperMask) | (g.mt[0] & lowerMask)
		g.mt[n-1] = g.mt[m-1] ^ (y >> 1) ^ mag01[y&1]
		g.mti = 0
	}
	y = g.mt[g.mti]
	g.mti++
	y ^= y >> 11
	y ^= (y << 7) & 0x9d2c5680
	y ^= (y << 15) & 0xefc60000
	y ^= y >> 18
	return y
}

// Intn returns a uniformly distributed int in [0, nn). It panics if nn <= 0.
// Rejection sampling removes modulo bias.
func (g *MT) Intn(nn int) int {
	if nn <= 0 {
		panic("rng: Intn requires n > 0")
	}
	if uint64(nn) > uint64(^uint32(0)) {
		panic("rng: Intn requires n <= 2^32-1")
	}
	u := uint32(nn)
	thresh := (-u) % u // == 2^32 mod u: reject the low biased range
	for {
		v := g.Uint32()
		if v >= thresh {
			return int(v % u)
		}
	}
}

// Snapshot captures the generator's full internal state (the MT word array
// plus the index) so a saved game's dice continue exactly where they stopped.
func (g *MT) Snapshot() []uint32 {
	out := make([]uint32, n+1)
	copy(out, g.mt[:])
	out[n] = uint32(g.mti)
	return out
}

// Restore rebuilds a generator from a Snapshot, or returns nil if the
// snapshot is malformed.
func Restore(snapshot []uint32) *MT {
	if len(snapshot) != n+1 {
		return nil
	}
	g := &MT{}
	copy(g.mt[:], snapshot[:n])
	g.mti = int(snapshot[n])
	if g.mti < 0 || g.mti > n {
		return nil
	}
	return g
}
