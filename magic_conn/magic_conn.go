package main

import (
	"net"
	"os"
	"syscall"
	"time"
)

const (
	TCP_CLIENT_IPV4 = 100
	TCP_CLIENT_IPV6 = 101
)

func MagicTCPDialTimeout(dst net.Addr, clientIp net.IP, timeout time.Duration) (net.Conn, error) {
	a, err := net.ResolveTCPAddr(dst.Network(), dst.String())
	if err != nil {
		return nil, err
	}

	var domain int
	if a.IP.To4() != nil {
		domain = syscall.AF_INET
	} else {
		domain = syscall.AF_INET6
	}

	fd, err := syscall.Socket(domain, syscall.SOCK_STREAM, 0)
	if err != nil {
		return nil, err
	}

	var sa syscall.Sockaddr
	if domain == syscall.AF_INET {
		var x [4]byte
		copy(x[:], a.IP.To4())
		sa = &syscall.SockaddrInet4{Port: a.Port, Addr: x}
	} else {
		var x [16]byte
		copy(x[:], a.IP.To16())
		sa = &syscall.SockaddrInet6{Port: a.Port, Addr: x}
	}

	if clientIp.To4() != nil {
		var x [4]byte
		copy(x[:], []byte(clientIp.To4()))
		syscall.SetsockoptInet4Addr(fd, syscall.IPPROTO_TCP, TCP_CLIENT_IPV4, x)
	} else {
		x := clientIp.To16()[:8]
		xs := string(x)

		// Sadly, the SetsockoptString gets _string_ as
		// argument. We have _byte slice_. As far as I (MM)
		// understand golang - this conversion is lossless. We
		// can just pass the composed string, no matter of
		// UTF-8 sequences or surrogates.
		syscall.SetsockoptString(fd, syscall.IPPROTO_TCP, TCP_CLIENT_IPV6, xs)
	}

	if timeout.Nanoseconds() > 0 {
		// Set SO_SNDTIMEO
		var (
			tv syscall.Timeval
			ns = timeout.Nanoseconds()
		)
		tv.Sec = ns / 1000000000
		tv.Usec = (ns - tv.Sec*1000000000) / 1000

		if err := syscall.SetsockoptTimeval(fd, syscall.SOL_SOCKET, syscall.SO_SNDTIMEO, &tv); err != nil {
			syscall.Close(fd)
			return nil, err
		}
	}

	// This is blocking.
	if err := syscall.Connect(fd, sa); err != nil {
		syscall.Close(fd)
		return nil, err
	}

	if timeout.Nanoseconds() > 0 {
		// Clear SO_SNDTIMEO
		var tv syscall.Timeval
		syscall.SetsockoptTimeval(fd, syscall.SOL_SOCKET, syscall.SO_SNDTIMEO, &tv)
	}

	f := os.NewFile(uintptr(fd), "socket")
	c, err := net.FileConn(f)
	f.Close()
	if err != nil {
		return nil, err
	}
	return c, nil
}

func MagicTCPDialTimeoutTTL(dst *net.TCPAddr, timeout time.Duration) (net.Conn, int, error) {
	var domain int
	if dst.IP.To4() != nil {
		domain = syscall.AF_INET
	} else {
		domain = syscall.AF_INET6
	}

	fd, err := syscall.Socket(domain, syscall.SOCK_STREAM, 0)
	if err != nil {
		return nil, 255, err
	}

	mapFd, bpfFd, _ := AttachTTLBPF(fd)

	var sa syscall.Sockaddr
	if domain == syscall.AF_INET {
		var x [4]byte
		copy(x[:], dst.IP.To4())
		sa = &syscall.SockaddrInet4{Port: dst.Port, Addr: x}
	} else {
		var x [16]byte
		copy(x[:], dst.IP.To16())
		sa = &syscall.SockaddrInet6{Port: dst.Port, Addr: x}
	}

	if timeout.Nanoseconds() > 0 {
		// Set SO_SNDTIMEO
		var (
			tv syscall.Timeval
			ns = timeout.Nanoseconds()
		)
		tv.Sec = ns / 1000000000
		tv.Usec = (ns - tv.Sec*1000000000) / 1000

		if err := syscall.SetsockoptTimeval(fd, syscall.SOL_SOCKET, syscall.SO_SNDTIMEO, &tv); err != nil {
			syscall.Close(fd)
			return nil, 255, err
		}
	}

	// This is blocking.
	if err := syscall.Connect(fd, sa); err != nil {
		syscall.Close(fd)
		return nil, 255, err
	}

	if timeout.Nanoseconds() > 0 {
		// Clear SO_SNDTIMEO
		var tv syscall.Timeval
		syscall.SetsockoptTimeval(fd, syscall.SOL_SOCKET, syscall.SO_SNDTIMEO, &tv)
	}

	minDist := ReadTTLBPF(mapFd)
	DetachTTLBPF(fd, mapFd, bpfFd)

	f := os.NewFile(uintptr(fd), "socket")
	c, err := net.FileConn(f)
	f.Close()
	if err != nil {
		return nil, 255, err
	}
	return c, minDist, nil
}
