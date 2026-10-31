package main

import (
	"math"
	"math/rand"
	"fmt"
)

func Round1(n uint32) uint {
	if n == 0 {
		return 0
	}
	return uint(math.Floor(math.Log2(float64(n))))
}

var multiplyDeBruijnBitPosition = [...]uint{0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31}
func Round2(v uint32) uint {
	// via: http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
	// Equivalent of: uint(math.Floor(math.Log2(float64(n))))

	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return multiplyDeBruijnBitPosition[uint32(v * 0x07C4ACDD) >> 27]
}

func main() {
	for n := uint(0); n < 1000000; n++ {
		xx := uint32(rand.Int63())
		///fmt.Printf("%d: %d == %d\n", n, Round1(xx), Round2(xx))
		if Round1(xx) != Round2(xx) {
			fmt.Printf("%d: %d != %d\n", n, Round1(xx), Round2(xx))
			panic("error")
		}
	}
}
