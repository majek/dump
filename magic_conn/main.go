package main

import (
	"fmt"
	"net"
	"os"
	"time"
)

func main() {
	target := "google.com:80"
	addr, err := net.ResolveTCPAddr("tcp4", target)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] resolv failed %s: %s\n", target, err)
		os.Exit(1)
	}

	conn, ttl, err := MagicTCPDialTimeoutTTL(addr, 0*time.Second)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] conn failed %s: %s\n", target, err)
		os.Exit(1)
	}
	fmt.Printf("[+] All good. Measured TTL distance to %s: %d\n", target, ttl)
	conn.Close()
	return
}
