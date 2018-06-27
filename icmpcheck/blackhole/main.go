package main

import (
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"strings"
	"syscall"
	"time"
)

func main() {
	v4mtu := 905
	v6mtu := 1285
	fmt.Fprintf(os.Stderr, `[ ] Remember to add
iptables -A INPUT -m string --algo bm --string packettoolongyaithuji6reeNab4XahChaeRah1diej4 -m length --length %d:65535 -j NFQUEUE --queue-num 34
iptables -A OUTPUT -m mark --mark 34 -j NFQUEUE --queue-num 34
iptables -t raw -A OUTPUT -m mark --mark 33 -j NOTRACK
iptables -t raw -I PREROUTING -p tcp --dport 80 -j NOTRACK

ip6tables -A INPUT -m string --algo bm --string packettoolongyaithuji6reeNab4XahChaeRah1diej4 -m length --length %d:65535 -j NFQUEUE --queue-num 34
ip6tables -A OUTPUT -m mark --mark 34 -j NFQUEUE --queue-num 34
ip6tables -t raw -A OUTPUT -m mark --mark 33 -j NOTRACK
ip6tables -t raw -I PREROUTING -p tcp --dport 80 -j NOTRACK
`, v4mtu-20+1, v6mtu-40+1)
	// 886 + 20 bytes of IP header = 906 bytes or longer. We want MTU=905
	// 1246 + 40 bytes of IPv6 header = 1286 bytes or longer. We want MTU=1285.

	// PREROUTING NOTRACK is needed to make sure inbound
	// fragmented POST /icmp would not get dropped on the -m
	// string rule. Yes, I did saw a DF+MF fragmented TCP segments
	// on inbound.

	// OUTPUT NOTRACK is needed to prevent outbound
	// de-fragmentation. By default iptables conntrack might
	// re-fragment packets. The IP_NODEFRAG somehow helps, but I
	// don't know specifically how it interacts with conntrack.

	nq_frag := NFQueue(34, v4mtu, v6mtu)
	fmt.Fprintf(os.Stderr, "[+] Nfqueue 34 opened\n")

	icmpSender := NewIcmpSender(v4mtu, v6mtu)
	fragSender := NewFragSender(512, 512)
	traceSender := NewTraceSender()

	go gohttp()

	for {
		select {
		case p := <-nq_frag:
			if p.doTrace {
				traceSender.send(p)
			} else if p.doIcmp {
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

	fmt.Fprintf(w, "{\"data\":\"")
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
		fmt.Fprintf(bufrw, "%x\r\n%s\r\n", len(p), p)
	}
	bufrw.Flush()

	ti, err := GetTcpinfo(conn)
	if err == nil {
		s := fmt.Sprintf("\", \"mtu\":%d, \"lost_segs\":%d, \"retrans_segs\":%d, \"total_retrans_segs\":%d, \"reord_segs\":%d, \"snd_mss\":%d, \"rcv_mss\":%d}\n", ti.Sys.PathMTU, ti.Sys.LostSegs, ti.Sys.RetransSegs, ti.Sys.TotalRetransSegs, ti.Sys.ReorderedSegs, ti.SenderMSS, ti.ReceiverMSS)
		fmt.Fprintf(bufrw, "%x\r\n%s\r\n0\r\n\r\n", len(s), s)
		bufrw.Flush()
		fmt.Printf(s)
	} else {
		fmt.Printf("BOOO2 %q %q\n", conn, err)
	}

	conn.Close()
}

var TRACESTRING="tracefragmentmeoopeitii1poth5rah9jooteireZ8eej7"
func trace(w http.ResponseWriter, r *http.Request) {
	r.Close = true

	fmt.Fprintf(w, "{\"data\":\"")
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
		fmt.Printf("BOOO\n")
		return
	}

	tcpConn := conn.(*net.TCPConn)
	file, _ := tcpConn.File()
	fd := file.Fd()

	syscall.SetsockoptInt(int(fd), syscall.SOL_SOCKET, syscall.SO_MARK, 34)

	p := TRACESTRING
	fmt.Fprintf(bufrw, "%x\r\n%s\r\n", len(p), p)
	bufrw.Flush()

	ti, err := GetTcpinfo(conn)
	if err == nil {
		s := fmt.Sprintf("\", \"mtu\":%d, \"lost_segs\":%d, \"retrans_segs\":%d, \"total_retrans_segs\":%d, \"reord_segs\":%d, \"snd_mss\":%d, \"rcv_mss\":%d}\n", ti.Sys.PathMTU, ti.Sys.LostSegs, ti.Sys.RetransSegs, ti.Sys.TotalRetransSegs, ti.Sys.ReorderedSegs, ti.SenderMSS, ti.ReceiverMSS)
		fmt.Fprintf(bufrw, "%x\r\n%s\r\n0\r\n\r\n", len(s), s)
		bufrw.Flush()
		fmt.Printf(s)
	} else {
		fmt.Printf("BOOO2 %q %q\n", conn, err)
	}

	conn.Close()
}


func icmp(w http.ResponseWriter, r *http.Request) {
	r.Close = true
	data := make([]byte, 32768)
	_, err := io.ReadFull(r.Body, data)
	if err != nil && err != io.EOF {
		// fmt.Fprintf(os.Stderr, "read() %q\n", err);
	}
	time.Sleep(250 * time.Millisecond)
	fmt.Fprintf(w, "{\"msg1\": \"Upload complete\"")
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
		s := fmt.Sprintf(", \"mtu\":%d, \"lost_segs\":%d, \"retrans_segs\":%d, \"total_retrans_segs\":%d, \"reord_segs\":%d, \"snd_mss\":%d, \"rcv_mss\":%d}\n", ti.Sys.PathMTU, ti.Sys.LostSegs, ti.Sys.RetransSegs, ti.Sys.TotalRetransSegs, ti.Sys.ReorderedSegs, ti.SenderMSS, ti.ReceiverMSS)
		fmt.Fprintf(bufrw, "%x\r\n%s\r\n0\r\n\r\n", len(s), s)
		bufrw.Flush()
		fmt.Printf(s)
	}
	conn.Close()
}

var mtuLongPayload = strings.Repeat("x", 4096)

func mtupage(w http.ResponseWriter, r *http.Request) {
	r.Close = true
	fmt.Fprintf(w, "{\"msg1\": \"")
	data := make([]byte, 32768)
	_, err := io.ReadFull(r.Body, data)
	if err != nil && err != io.EOF {
		// fmt.Fprintf(os.Stderr, "read() %q\n", err);
	}
	time.Sleep(250 * time.Millisecond)

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
	fmt.Fprintf(bufrw, "%x\r\n%s\r\n", len(mtuLongPayload), mtuLongPayload)
	bufrw.Flush()

	ti, err := GetTcpinfo(conn)
	if err == nil {
		s := fmt.Sprintf("\", \"mtu\":%d, \"lost_segs\":%d, \"retrans_segs\":%d, \"total_retrans_segs\":%d, \"reord_segs\":%d, \"snd_mss\":%d, \"rcv_mss\":%d}\n", ti.Sys.PathMTU, ti.Sys.LostSegs, ti.Sys.RetransSegs, ti.Sys.TotalRetransSegs, ti.Sys.ReorderedSegs, ti.SenderMSS, ti.ReceiverMSS)
		fmt.Fprintf(bufrw, "%x\r\n%s\r\n0\r\n\r\n", len(s), s)
		bufrw.Flush()
		fmt.Printf(s)
	}
	conn.Close()
}

func serveStatic(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "html/"+r.URL.Path[1:])
}

func gohttp() {
	s := &http.Server{Addr: ":80"}
	s.SetKeepAlivesEnabled(false)

	http.HandleFunc("/frag", frag)
	http.HandleFunc("/mtu", mtupage)
	http.HandleFunc("/icmp", icmp)
	http.HandleFunc("/trace", trace)
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		http.ServeFile(w, r, "html/index.html")
	})
	http.HandleFunc("/bootstrap.min.css", serveStatic)
	http.HandleFunc("/ie10-viewport-bug-workaround.css", serveStatic)
	http.HandleFunc("/sticky-footer.css", serveStatic)
	http.HandleFunc("/ie10-viewport-bug-workaround.js", serveStatic)

	s.ListenAndServe()
}
