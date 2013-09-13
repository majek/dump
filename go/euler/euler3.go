package main

import (
	"fmt"
	"math"
)

func primes(limit uint64) chan uint64 {
	arr := make([]bool, limit)
	c := make(chan uint64)
	go func() {
		for i := uint64(2); i < limit; i++ {
			if arr[i] == false {
				c <- i
				for j := i * 2; j < limit; j += i {
					arr[j] = true
				}
			}
		}
		close(c)
	}()
	return c
}

func main() {
	number := uint64(600851475143)
	pr := primes(uint64(math.Sqrt(float64(number))) + 1)

	last := uint64(1)
	for p := range pr {
		if number%p == 0 {
			last = p
		}
	}
	fmt.Printf("%d\n", last)
}
