package main

import (
	"fmt"
)

func multiplies(n int, c chan int) {
	for s := n; true; s += n {
		c <- s
	}
}

func merge(a, b, c chan int) {
	var v1, v2 int
	var f1, f2 = false, false
	for {
		if f1 == false {
			v1 = <-a
			f1 = true
		}
		if f2 == false {
			v2 = <-b
			f2 = true
		}
		if v1 < v2 {
			f1 = false
			c <- v1
		} else if v1 > v2 {
			f2 = false
			c <- v2
		} else {
			f1 = false
			f2 = false
			c <- v1
		}
	}
}

func main() {
	var three = make(chan int)
	go multiplies(3, three)

	var five = make(chan int)
	go multiplies(5, five)

	var three_or_five = make(chan int)
	go merge(three, five, three_or_five)

	sum := 0
	for v := range three_or_five {
		if v >= 1000 {
			break
		}
		sum += v
	}

	fmt.Printf("%d\n", sum)
}
