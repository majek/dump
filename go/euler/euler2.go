package main

import (
	"fmt"
)

func fibonacci(c chan<- int) {
	x, y := 0, 1
	for {
		c <- x
		x, y = y, x+y
	}
}

func main() {
	c := make(chan int)
	go fibonacci(c)

	sum := 0
	for v := range c {
		if v > 4000000 {
			break
		}
		if v%2 != 0 {
			sum += v
		}
	}

	fmt.Printf("%d\n", sum)
}
