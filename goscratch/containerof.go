package main

import (
	"container/list"
	"fmt"
	"unsafe"
)

type Item struct {
	someOffset uint
	el         list.Element
	value      string
}

func main() {
	a := &Item{value: "Alice"}

	el := &a.el

	var tmpItem Item
	recoveredA := (*Item)(unsafe.Pointer(uintptr(unsafe.Pointer(el)) - unsafe.Offsetof(tmpItem.el)))

	fmt.Printf("%#v\n", recoveredA.value)
}
