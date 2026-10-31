package main

import (
	"math"
	"fmt"
)

func Round1(n uint) uint {
	return uint(math.Ceil(math.Log2(float64(n))))
}



var multiplyDeBruijnBitPosition = [...]uint{0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31}
func log2Floor(v uint32) uint {
	// via: http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
	// Equivalent of: uint(math.Floor(math.Log2(float64(n))))

	v |= v >> 1
	v |= v >> 2
	v |= v >> 4
	v |= v >> 8
	v |= v >> 16
	return multiplyDeBruijnBitPosition[uint32(v * 0x07C4ACDD) >> 27]
}

func log2Ceil(v uint32) uint {
	var isNotPowerOfTwo uint = 1
	if (v & (v - 1)) == 0 {
		isNotPowerOfTwo = 0
	}
	return log2Floor(v) + isNotPowerOfTwo
}

// func roundToNextPowerOf2(v uint) uint {
// 	// via: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
// 	// Equivalent of: 1 << uint(math.Ceil(math.Log2(float64(n))))
// 	v--;
// 	v |= v >> 1;
// 	v |= v >> 2;
// 	v |= v >> 4;
// 	v |= v >> 8;
// 	v |= v >> 16;
// 	v++;
// 	return v
// }

func main() {
	for n := uint32(0); n < 65536; n++ {
		fmt.Printf("%d %d==%d %v\n", n, Round1(uint(n)), log2Ceil(n), (n & (n - 1)) == 0)
		if Round1(uint(n)) != log2Ceil(n) {
			fmt.Printf("bad\n")
			//panic("A")
		}
	}
}
