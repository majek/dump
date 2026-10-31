
package main

var b = 23;

func Alloc() int {
	var c []byte
	if b <= 8192 {
		c = make([]byte, 8192)
	} else {
		c = make([]byte, b)
	}
	return len(c)
}

func main() {
	print(Alloc(), "\n")
}
