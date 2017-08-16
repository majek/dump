package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"sync"
)

func main() {
	fmt.Printf("[+] Listening on 127.0.0.1:8080\n")

	lAddr, err := net.ResolveTCPAddr("tcp", "127.0.0.1:8080")
	if err != nil {
		panic(err)
	}

	oAddr, err := net.ResolveTCPAddr("tcp", "127.0.0.1:80")
	if err != nil {
		panic(err)
	}

	ln, err := net.ListenTCP("tcp", lAddr)
	if err != nil {
		fmt.Printf("Listen(): %s\n", err)
		os.Exit(1)
	}
	for {
		conn, err := ln.AcceptTCP()
		if err != nil {
			fmt.Printf("Accept(): %s\n", err)
			os.Exit(1)
		}
		go proxyTcp(conn, oAddr)
	}
}

func proxyTcp(conn *net.TCPConn, oAddr *net.TCPAddr) {
	fmt.Printf("[+] %s opened\n", conn.RemoteAddr())

	orig, err := net.DialTCP("tcp", nil, oAddr)
	if err != nil {
		fmt.Printf("[!] origin Connect(): %s\n", err)
		conn.Close()
		return
	}

	// Nagle is disabled by default in Golang, but better be
	// explicit.
	conn.SetNoDealay(true)
	orig.SetNoDealay(true)

	// SetKeepAlive , SetLinger

	var wg sync.WaitGroup
	wg.Add(2)

	go proxyHalfDuplex(conn, orig, &wg)
	go proxyHalfDuplex(orig, conn, &wg)
	wg.Wait()

	fmt.Printf("[-] %s closed\n", conn.RemoteAddr())
	conn.Close()
	orig.Close()
}

func proxyHalfDuplex(a, b *net.TCPConn, wg *sync.WaitGroup) {
	var buf [4096]byte
	for {
		n, err := a.Read(buf[:])
		if err != nil {
			if err != io.EOF {
				fmt.Printf("[!] %s error Read(): %s\n", a.RemoteAddr(), err)
			}
			goto exit
		}
		if n == 0 {
			fmt.Printf("[!] %s error Read(): EOF?\n", a.RemoteAddr())
			goto exit
		}
		s := buf[:n]
		for len(s) > 0 {
			x, err := b.Write(s)
			if err != nil {
				fmt.Printf("[!] %s error Read(): %s\n", a, err)
				goto exit
			}
			s = s[x:]
		}
	}
exit:

	a.CloseRead()
	b.CloseWrite()
	wg.Done()
}
