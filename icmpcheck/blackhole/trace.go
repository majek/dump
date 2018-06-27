package main

import (
	"fmt"
	"os"
	"syscall"
	"time"
)



type traceSender struct {
	fragFd     int
	fragv6Fd   int
}

func NewTraceSender() *traceSender {
	fs := &traceSender{}

	fragFd, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_RAW, syscall.IPPROTO_RAW)
	if err != nil {
		fmt.Fprintf(os.Stderr, "socket(AF_INET, SOCK_RAW, TCP): %s\n", err)
		os.Exit(1)
	}

	syscall.SetsockoptInt(fragFd, syscall.IPPROTO_IP, syscall.IP_HDRINCL, 1)
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
	return fs
}

func (fs *traceSender) send(p *Packet) {
	data := p.L4Data

	fmt.Printf("tracesender\n")
	if p.IP.Ipver == 4 {
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

		for ttl := uint8(1); ttl < 35; ttl++ {
			ip_hdr := []uint8{
				0x45 + 0, 0, 0, 0, // 9*4 = 36 bytes of options
				0, 0, 0, 0,
				ttl, 6, 0, 0,
				0, 0, 0, 0,
				dstr[0], dstr[1], dstr[2], dstr[3],

				// 0x44, 0x24, 0x05, 0x00,

				// 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
				// 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
				// 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
				// 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
			}
			ip_hdr = append(ip_hdr, data...)
			err := syscall.Sendto(fs.fragFd, ip_hdr, 0, &addr)
			if err != nil {
				fmt.Fprintf(os.Stderr, "sendto %s\n", err)
			}
			time.Sleep(10 * time.Millisecond)
		}
	}

	if p.IP.Ipver == 6 {
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

		for ttl := uint8(1); ttl < 35; ttl++ {
			l := len(data)
			ip_hdr := []uint8{
				0x60, 0x00, 0, 0,
				uint8(l >> 8), uint8(l), 6, ttl,
			}
			ip_hdr = append(ip_hdr, srcr[:]...)
			ip_hdr = append(ip_hdr, dstr[:]...)
			ip_hdr = append(ip_hdr, data...)

			err := syscall.Sendto(fs.fragv6Fd, ip_hdr, 0, &addr)
			if err != nil {
				fmt.Fprintf(os.Stderr, "sendto %s\n", err)
			}
			time.Sleep(10*time.Millisecond)
		}
	}
}
