package main

import (
	"bufio"
	"fmt"
	"os"
	"runtime"
	"sort"
)


func stats(i int) {
	var m runtime.MemStats
	runtime.ReadMemStats(&m)

	fmt.Fprintf(os.Stderr, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
		float64(i),               // 1
		float64(m.Mallocs)/1000., // 2
		float64(m.Frees)/1000.,   // 3
		float64(m.HeapObjects),   // 4
		float64(m.HeapSys),       // 5
		float64(m.HeapAlloc),     // 6
		float64(m.HeapIdle),      // 7
		float64(m.NumGC),         // 8
		float64(m.Alloc),         // 9
		float64(m.TotalAlloc),    // 10
		float64(m.Sys),           // 11
		float64(m.Lookups),       // 12
		float64(m.LastGC),       // 13
		float64(m.NextGC),       // 14
	)
}

func main() {
	stdin := bufio.NewReader(os.Stdin)

	cache := iLRUCache{}
	cache.Init(999)

	var i = 0
	for ; true; i++ {
		line, err := stdin.ReadBytes('\n')
		if err != nil {
			break
		}

		key := string(line[:len(line)-1])

		count := 0
		if v, _ := cache.Get(key); v != nil {
			count = v.(int)
		}
		cache.Set(key, count+1)

		//		if i%1000 == 0 {
		stats(i)
		//		}
	}
	stats(i)

	keys := make([]string, 0, len(cache.table))
	for k, _ := range cache.table {
		keys = append(keys, k)
	}

	fmt.Fprintf(os.Stderr, "[*] Results\n")
	sort.Strings(keys)

	for _, k := range keys {
		v, _ := cache.Get(k)
		fmt.Printf("%7d\t%s\n", v.(int), k)
	}
}
