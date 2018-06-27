package main

import (
	"github.com/chifflier/nfqueue-go/nfqueue"
	"syscall"
	"bytes"
	"fmt"
)

var global_ch chan *Packet
var global_v4mtu, global_v6mtu int



func real_callback(payload *nfqueue.Payload) int {
	data := make([]byte, len(payload.Data))
	copy(data, payload.Data)

	p := &Packet{}
	p.Parse(data, false)
	if p.Tcp.Parsed != true {
		payload.SetVerdict(nfqueue.NF_ACCEPT)
	} else {
		if p.Tcp.DstPort < 1024 || p.Tcp.DstPort == 8080 || p.Tcp.DstPort == 8000 {
			if (p.IP.Ipver == 4 && (p.IP.Flags == 0x1 || p.IP.FragOff != 0)) { // MF
				fmt.Printf("Fragment on inbound flags=0x%x offset=0x%x\n", p.IP.Flags, p.IP.FragOff)
				payload.SetVerdict(nfqueue.NF_ACCEPT)
			} else if (p.IP.Ipver == 4 && int(p.IP.TotalLength) <= global_v4mtu) || (p.IP.Ipver == 6 && int(p.IP.TotalLength) <= global_v6mtu) {
				payload.SetVerdict(nfqueue.NF_ACCEPT)
			} else {
				p.doIcmp = true
				payload.SetVerdict(nfqueue.NF_DROP)
				global_ch <- p
			}
		} else {
			if (bytes.Index(p.L4Data, []byte(TRACESTRING)) > -1) {
				p.doTrace = true
				payload.SetVerdict(nfqueue.NF_DROP)
				global_ch <- p
			} else {
				payload.SetVerdict(nfqueue.NF_DROP)
				global_ch <- p
			}
		}
	}
	return 0
}

func NFQueue(queueNo int, v4mtu, v6mtu int) chan *Packet {

	ch := make(chan *Packet, 32)
	global_ch = ch
	global_v4mtu = v4mtu
	global_v6mtu = v6mtu

	go func() {
		for {
			q := new(nfqueue.Queue)
			q.SetCallback(real_callback)

			q.Init()

			q.Unbind(syscall.AF_INET)
			q.Unbind(syscall.AF_INET6)
			q.Bind(syscall.AF_INET)
			q.Bind(syscall.AF_INET6)
			q.CreateQueue(queueNo)
			fmt.Printf("[+] NEW QUEUE\n")
			q.Loop()
			q.DestroyQueue()
			q.Close()
			fmt.Printf("[-] QUEUE DESTROYED\n")
		}
	}()
	return ch
}
