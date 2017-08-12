package main

import (
	"github.com/mikioh/tcp"
	"github.com/mikioh/tcpinfo"
	"net"
)

func GetTcpinfo(c net.Conn) (*tcpinfo.Info, error) {
	tx := c.(*net.TCPConn)
	tc, err := tcp.NewConn(tx)
	if err != nil {
		return nil, err
	}

	var (
		o tcpinfo.Info
		b [4096]byte
	)
	i, err := tc.Option(o.Level(), o.Name(), b[:])
	if err != nil {
		return nil, err
	}
	return i.(*tcpinfo.Info), nil
}
