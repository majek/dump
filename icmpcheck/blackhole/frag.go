package main

import (
	"fmt"
	"math/rand"
	"os"
	"syscall"
	"time"
)

func checksum(buf []byte) uint16 {
	sum := uint32(0)

	for ; len(buf) >= 2; buf = buf[2:] {
		sum += uint32(buf[0])<<8 | uint32(buf[1])
	}
	if len(buf) > 0 {
		sum += uint32(buf[0]) << 8
	}
	for sum > 0xffff {
		sum = (sum >> 16) + (sum & 0xffff)
	}
	csum := ^uint16(sum)
	/*
	 * From RFC 768:
	 * If the computed checksum is zero, it is transmitted as all ones (the
	 * equivalent in one's complement arithmetic). An all zero transmitted
	 * checksum value means that the transmitter generated no checksum (for
	 * debugging or for higher level protocols that don't care).
	 */
	if csum == 0 {
		csum = 0xffff
	}
	return csum
}

// Calculate the TCP/IP checksum defined in rfc1071.  The passed-in csum is any
// initial checksum data that's already been computed.
func tcpipChecksum(data []byte, csum uint32) uint16 {
	// to handle odd lengths, we loop to length - 1, incrementing by 2, then
	// handle the last byte specifically by checking against the original
	// length.
	length := len(data) - 1
	for i := 0; i < length; i += 2 {
		// For our test packet, doing this manually is about 25% faster
		// (740 ns vs. 1000ns) than doing it by calling binary.BigEndian.Uint16.
		csum += uint32(data[i]) << 8
		csum += uint32(data[i+1])
	}
	if len(data)%2 == 1 {
		csum += uint32(data[length]) << 8
	}
	for csum > 0xffff {
		csum = (csum >> 16) + (csum & 0xffff)
	}
	return ^uint16(csum)
}

type fragSender struct {
	fragFd     int
	fragv6Fd   int
	v4fragsize int
	v6fragsize int
}

var IP_NODEFRAG = 22

func NewFragSender(v4fragsize, v6fragsize int) *fragSender {
	fs := &fragSender{}

	fragFd, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_RAW, syscall.IPPROTO_RAW)
	if err != nil {
		fmt.Fprintf(os.Stderr, "socket(AF_INET, SOCK_RAW, TCP): %s\n", err)
		os.Exit(1)
	}

	syscall.SetsockoptInt(fragFd, syscall.IPPROTO_IP, syscall.IP_HDRINCL, 1)
	syscall.SetsockoptInt(fragFd, syscall.IPPROTO_IP, IP_NODEFRAG, 1)
	syscall.SetsockoptInt(fragFd, syscall.SOL_SOCKET, syscall.SO_MARK, 33)

	fragv6Fd, err := syscall.Socket(syscall.AF_INET6, syscall.SOCK_RAW, syscall.IPPROTO_RAW)
	if err != nil {
		fmt.Fprintf(os.Stderr, "socket(AF_INET6, SOCK_RAW, IPPROTO_TCP): %s\n", err)
		os.Exit(1)
	}

	syscall.SetsockoptInt(fragv6Fd, syscall.IPPROTO_IPV6, syscall.IP_HDRINCL, 1)
	syscall.SetsockoptInt(fragv6Fd, syscall.SOL_SOCKET, syscall.SO_MARK, 33)

	fs.fragFd = fragFd
	fs.fragv6Fd = fragv6Fd
	fs.v4fragsize = v4fragsize
	fs.v6fragsize = v6fragsize
	return fs
}

func chunkify(data []byte, sz int) [][]byte {
	var r [][]byte
	for len(data) > 0 {
		c := data
		if len(c) > sz {
			c = c[:sz]
		}
		data = data[len(c):]
		r = append(r, c)
	}
	return r
}

func (fs *fragSender) replyFrag(p *Packet) {
	data := p.L4Data

	if p.IP.Ipver == 4 {
		fragsize := fs.v4fragsize
		var dstr [4]byte
		copy(dstr[:], p.IP.DstIP.To4())
		addr := syscall.SockaddrInet4{
			Port: 0,
			Addr: dstr,
		}

		//		cs_old := (uint16(data[16]) << 8) | uint16(data[17])
		data[16] = 0
		data[17] = 0
		ph := []uint8{}
		ph = append(ph, p.IP.SrcIP.To4()...)
		ph = append(ph, p.IP.DstIP.To4()...)
		ph = append(ph, 0)
		ph = append(ph, 6)
		ph = append(ph, uint8((len(data) >> 8)))
		ph = append(ph, uint8((len(data))))
		ph = append(ph, data...)
		cs := tcpipChecksum(ph, 0)
		// fmt.Printf("old=%04x new=%04x diff=%02x byte=%02x%02x%02x\n", cs_old, cs, cs-cs_old, data[len(data)-3], data[len(data)-2], data[len(data)-1])
		data[16] = uint8(cs >> 8)
		data[17] = uint8(cs)

		ipid := uint16(rand.Uint32())

		chunks := chunkify(data, fragsize)
		// fmt.Printf("data=%d chunks=%d\n", len(data), len(chunks))

		for i, chunk := range chunks {
			//fmt.Printf("i=%d chunks=%d\n", i, len(chunks))
			offset := (i * fragsize) / 8
			ip_hdr := []uint8{
				0x45, 0, 0, 0,
				uint8(ipid >> 8), uint8(ipid), uint8(offset >> 8), uint8(offset),
				64, 6, 0, 0,
				0, 0, 0, 0,
				dstr[0], dstr[1], dstr[2], dstr[3],
			}
			if i != len(chunks)-1 {
				ip_hdr[6] |= (1 << 5) // MORE FRAGMENTS / MF
			}
			ip_hdr = append(ip_hdr, chunk...)
			err := syscall.Sendto(fs.fragFd, ip_hdr, 0, &addr)
			if err != nil {
				fmt.Fprintf(os.Stderr, "sendto %s\n", err)
			}
			time.Sleep(10 * time.Millisecond)
		}
	}

	if p.IP.Ipver == 6 {
		fragsize := fs.v6fragsize
		var dstr [16]byte
		copy(dstr[:], p.IP.DstIP.To16())

		var srcr [16]byte
		copy(srcr[:], p.IP.SrcIP.To16())
		addr := syscall.SockaddrInet6{
			Port: 0,
			Addr: dstr,
		}

		data[16] = 0
		data[17] = 0
		ph := []uint8{}
		ph = append(ph, p.IP.SrcIP.To16()...)
		ph = append(ph, p.IP.DstIP.To16()...)
		ph = append(ph, uint8((len(data) >> 24)))
		ph = append(ph, uint8((len(data) >> 16)))
		ph = append(ph, uint8((len(data) >> 8)))
		ph = append(ph, uint8((len(data))))
		ph = append(ph, 0)
		ph = append(ph, 0)
		ph = append(ph, 0)
		ph = append(ph, 6)
		ph = append(ph, data...)
		cs := tcpipChecksum(ph, 0)
		data[16] = uint8(cs >> 8)
		data[17] = uint8(cs)

		if len(data) > fragsize {
			chunks := chunkify(data, fragsize)

			ipid := rand.Uint32()

			for i, chunk := range chunks {
				l := 8 + len(chunk)
				offset := (i * fragsize) / 8
				ip_hdr := []uint8{
					0x60, 0x00, 0, 0,
					uint8(l >> 8), uint8(l), 44, 64,
				}
				ip_hdr = append(ip_hdr, srcr[:]...)
				ip_hdr = append(ip_hdr, dstr[:]...)
				ip_hdr = append(ip_hdr, []byte{
					6, 0, uint8(offset >> 5), uint8(offset << 3),
					uint8(ipid >> 24), uint8(ipid >> 16), uint8(ipid >> 8), uint8(ipid),
				}...)

				if i != len(chunks)-1 {
					ip_hdr[40+3] |= 1 // MORE FRAGMENTS / MF
				}
				ip_hdr = append(ip_hdr, chunk...)
				err := syscall.Sendto(fs.fragv6Fd, ip_hdr, 0, &addr)
				if err != nil {
					fmt.Fprintf(os.Stderr, "sendto %s\n", err)
				}
				time.Sleep(10 * time.Millisecond)
			}
		} else {
			l := len(data)
			ip_hdr := []uint8{
				0x60, 0x00, 0, 0,
				uint8(l >> 8), uint8(l), 6, 64,
			}
			ip_hdr = append(ip_hdr, srcr[:]...)
			ip_hdr = append(ip_hdr, dstr[:]...)
			ip_hdr = append(ip_hdr, data...)

			err := syscall.Sendto(fs.fragv6Fd, ip_hdr, 0, &addr)
			if err != nil {
				fmt.Fprintf(os.Stderr, "sendto %s\n", err)
			}
		}
	}
}
