package main

import (
	"fmt"
	"time"
)

type Wrt struct {
}

func (w Wrt) Write(p []byte) (n int, err error) {
	// n, err = fmt.Println("blah! %s", p);
	n, err = 0, nil
	return
}

func main() {
	w := new(Wrt)

	t0 := time.Now()
	fmt.Fprint(w, "Hello world\n")
	fmt.Println(time.Since(t0))

	t0 = time.Now()
	fmt.Fprint(w, "Hello world\n")
	fmt.Println(time.Since(t0))

}
