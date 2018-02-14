// Based on Gorilla WebSocket example

package main

import (
	"flag"
	"fmt"
	"net"
	"net/http"
	"os"
	"strings"

	"github.com/gorilla/websocket"
)

func AddrToIP(a net.Addr) (net.IP, int) {
	switch x := a.(type) {
	case *net.TCPAddr:
		return x.IP, x.Port
	case *net.UDPAddr:
		return x.IP, x.Port
	case *net.UnixAddr:
		return nil, 0
	default:
		panic("")
	}
}

// use default options
var upgrader = websocket.Upgrader{}

func echo(w http.ResponseWriter, r *http.Request) {
	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] upgrade: %s\n", err)
		return
	}

	for {
		mt, message, err := c.ReadMessage()
		if err != nil && strings.Contains(err.Error(), "1006") {
			break
		} else if err != nil {
			fmt.Fprintf(os.Stderr, "read: %q %s\n", err, err)
			break
		}
		err = c.WriteMessage(mt, message)
		if err != nil {
			fmt.Fprintf(os.Stderr, "[!] write: %s\n", err)
			break
		}
	}
	c.Close()
}

func index(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("Hello World!\r\n"))
}

var (
	listenAddr string
)

func init() {
	flag.StringVar(&listenAddr, "l", "0.0.0.0:8080", "listen addr to run http server on")
}

func main() {
	flag.Parse()

	http.HandleFunc("/echo", echo)
	http.HandleFunc("/", index)

	ln, err := net.Listen("tcp", listenAddr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[-] listen error: %s\n", err)
		os.Exit(-1)
	}

	_, p := AddrToIP(ln.Addr())
	fmt.Printf("%d\n", p)

	err = http.Serve(ln, nil)
	fmt.Fprintf(os.Stderr, "[-] http error: %s\n", err)
	os.Exit(-1)
}
