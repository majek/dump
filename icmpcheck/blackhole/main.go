package main

import(
	"fmt"
	"os"
	"net/http"
	"time"
	"strings"
	"io"
	"syscall"
	"net"
)

func main() {
	fmt.Fprintf(os.Stderr, "[ ] Remember to add\n")
	fmt.Fprintf(os.Stderr, "iptables -A INPUT -m string --algo bm --string packettoolongyaithuji6reeNab4XahChaeRah1diej4 -m length --length 901:65535 -j NFQUEUE --queue-num 34\n")
	fmt.Fprintf(os.Stderr, "iptables -A OUTPUT -m mark --mark 34 -j NFQUEUE --queue-num 34\n")
	fmt.Fprintf(os.Stderr, "iptables -t raw -A OUTPUT -m mark --mark 33 -j NOTRACK\n")

	fmt.Fprintf(os.Stderr, "ip6tables -A INPUT -m string --algo bm --string packettoolongyaithuji6reeNab4XahChaeRah1diej4 -m length --length 1286:65535 -j NFQUEUE --queue-num 34\n")
	fmt.Fprintf(os.Stderr, "ip6tables -A OUTPUT -m mark --mark 34 -j NFQUEUE --queue-num 34\n")
	fmt.Fprintf(os.Stderr, "ip6tables -t raw -A OUTPUT -m mark --mark 33 -j NOTRACK\n")
	fmt.Fprintf(os.Stderr,"\n")


//	nl := NewNflog(1)
	nq_frag := NFQueue(34)
	fmt.Fprintf(os.Stderr, "[+] Nfqueue 34 opened\n")


	mtu := 900
	icmpSender := NewIcmpSender(mtu)
	fragSender := NewFragSender(mtu)

	go gohttp()

	for {
		select  {
		case m := <- nq_frag:
			p := &Packet{}
			p.Parse(m, false)
			if p.Tcp.Parsed != true {
				fmt.Printf("not parsed\n")
				continue
			}
			if p.Tcp.DstPort < 1024 || p.Tcp.DstPort == 8080 || p.Tcp.DstPort == 8000 {
				icmpSender.replyIcmp(p)
			} else {
				fragSender.replyFrag(p)
			}
		}
	}
}

var longPayload = strings.Repeat("fragmentmeoopeitii1poth5rah9jooteireZ8eej7", 93) // 3902 bytes
func frag(w http.ResponseWriter, r *http.Request) {
	r.Close = true

	io.WriteString(w, "{\"data\":\"")
	if f, ok := w.(http.Flusher); ok {
		f.Flush()
	}

	// // Send roughly 8KB of data, but stagger it over time to give
	// // a chance for tcp stack to avoid GRO / LRO, and transmit
	// // reasonably small packets. We don't want to test if the end
	// // party can reassemble 64KiB of data packet!
	// io.WriteString(w, "{\"data\":\"")
	// for i := 0; i < 4; i++ {
	// 	io.WriteString(w, longPayload[:2048])
	// 	if f, ok := w.(http.Flusher); ok {
	// 		f.Flush()
	// 	}

	// 	time.Sleep(250*time.Millisecond)
	// }

	// //r.Body.Close()

	hj, ok := w.(http.Hijacker)
	if !ok {
		return
	}

	conn, bufrw, err := hj.Hijack()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		fmt.Printf("BOOO\n")
		return
	}
	tcpConn := conn.(*net.TCPConn)
	file, _ := tcpConn.File()
	fd := file.Fd()

	syscall.SetsockoptInt(int(fd), syscall.SOL_SOCKET, syscall.SO_MARK, 34)

	for i := 0; i < 4; i++ {
		p := longPayload[:2048]
		io.WriteString(bufrw, fmt.Sprintf("%x\r\n%s\r\n",len(p), p))
	}
	bufrw.Flush()

	ti, err := GetTcpinfo(conn)
	if err == nil {
		s := fmt.Sprintf("\", \"mtu\":%d, \"lost_segs\":%d, \"retrans_segs\":%d, \"total_retrans_segs\":%d, \"reord_segs\":%d, \"snd_mss\":%d, \"rcv_mss\":%d}\n", ti.Sys.PathMTU, ti.Sys.LostSegs, ti.Sys.RetransSegs,ti.Sys.TotalRetransSegs, ti.Sys.ReorderedSegs, ti.SenderMSS, ti.ReceiverMSS)
		io.WriteString(bufrw, fmt.Sprintf("%x\r\n%s\r\n0\r\n\r\n", len(s), s))
		bufrw.Flush()
		fmt.Printf(s)
	} else {
		fmt.Printf("BOOO2 %q %q\n", conn, err)
	}

	conn.Close()
}

func icmp(w http.ResponseWriter, r *http.Request) {
	r.Close = true
	data := []byte{}
	for len(data) < 32768 {
		var buf = [4096]byte{}
		n, err := r.Body.Read(buf[:])
		if err != nil {
			if err == io.EOF {
				break
			}
			fmt.Printf("%q\n", err)
			break
		} else {
			data = append(data, buf[:n]...)
		}
	}
	time.Sleep(250*time.Millisecond)
	io.WriteString(w, "{\"msg1\": \"Upload complete\"")
	if f, ok := w.(http.Flusher); ok {
		f.Flush()
	}

	hj, ok := w.(http.Hijacker)
	if !ok {
		return
	}

	conn, bufrw, err := hj.Hijack()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	bufrw.Flush()

	ti, err := GetTcpinfo(conn)
	if err == nil {
		s := fmt.Sprintf(", \"mtu\":%d, \"lost_segs\":%d, \"retrans_segs\":%d, \"total_retrans_segs\":%d, \"reord_segs\":%d, \"snd_mss\":%d, \"rcv_mss\":%d}\n", ti.Sys.PathMTU, ti.Sys.LostSegs, ti.Sys.RetransSegs,ti.Sys.TotalRetransSegs, ti.Sys.ReorderedSegs, ti.SenderMSS, ti.ReceiverMSS)
		io.WriteString(bufrw, fmt.Sprintf("%x\r\n%s\r\n0\r\n\r\n", len(s), s))
		bufrw.Flush()
		fmt.Printf(s)
	}
	conn.Close()
}

func serveStatic (w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "html/" + r.URL.Path[1:])
}

func gohttp() {
	s := &http.Server{Addr: ":80"}
	s.SetKeepAlivesEnabled(false)

	http.HandleFunc("/frag", frag)
	http.HandleFunc("/icmp", icmp)
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		http.ServeFile(w, r, "html/index.html")
	})
	http.HandleFunc("/bootstrap.min.css", serveStatic)
	http.HandleFunc("/ie10-viewport-bug-workaround.css", serveStatic)
	http.HandleFunc("/sticky-footer.css", serveStatic)
	http.HandleFunc("/ie10-viewport-bug-workaround.js", serveStatic)

	s.ListenAndServe()
}

