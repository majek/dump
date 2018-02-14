// Based on Gorilla WebSocket example

package main

import (
	"bytes"
	"flag"
	"fmt"
	"net"
	"net/url"
	"os"
	"time"

	"github.com/gorilla/websocket"
)

var (
	wsUrl   string
	resolve string
	count   int
)

func init() {
	flag.StringVar(&wsUrl, "t", "ws://localhost:8080/echo", "websocket url")
	flag.StringVar(&resolve, "r", "", "force resolve to IP")
	flag.IntVar(&count, "n", 0, "count of probes")
}

func main() {
	flag.Parse()

	u, err := url.Parse(wsUrl)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] Can't parse url %q: %s\n", wsUrl, err)
		os.Exit(-1)
	}

	fmt.Fprintf(os.Stderr, "[+] Connecting to %q\n", u.String())

	dialer := &websocket.Dialer{}
	dialer.NetDial = func(network, addr string) (net.Conn, error) {
		if resolve != "" {
			u, _ := url.Parse(fmt.Sprintf("http://%s", addr))
			addr = fmt.Sprintf("%s:%s", resolve, u.Port())
		}
		return net.Dial(network, addr)
	}

	c, _, err := dialer.Dial(u.String(), nil)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] dial failed: %s\n", err)
		os.Exit(-1)
	}

	ticker := time.NewTicker(time.Second)

	i := 0
	for _ = range ticker.C {
		if count > 0 && i >= count {
			break
		}
		t1 := time.Now()
		msg := []byte(fmt.Sprintf("%08x\n", i))
		err := c.WriteMessage(websocket.TextMessage, msg)
		if err != nil {
			fmt.Fprintf(os.Stderr, "[!] write failed: %s\n", err)
			return
		}

		_, rMsg, err := c.ReadMessage()
		if err != nil {
			fmt.Fprintf(os.Stderr, "[!] read failed: %s\n", err)
			return
		}
		if bytes.Equal(msg, rMsg) == false {
			fmt.Fprintf(os.Stderr, "[!] messages differ: %q vs %q\n", msg, rMsg)
		}
		td := time.Now().Sub(t1)
		tdMs := td.Nanoseconds() / 1000000
		fmt.Printf("%d\n", tdMs)
		i = i + 1
	}
	c.Close()
}
