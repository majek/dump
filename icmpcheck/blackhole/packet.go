package main

import "net"

type MacAddress [6]byte

func NewMacAddress(slice []byte) MacAddress {
	if len(slice) != 6 {
		panic("")
	}
	var x [6]byte
	copy(x[:], slice[:6])
	return x
}

const (
	PROTOCOL_TCP = 0x06
	PROTOCOL_UDP = 0x11

	TCP_FIN = uint8(1 << 0)
	TCP_SYN = uint8(1 << 1)
	TCP_RST = uint8(1 << 2)
	TCP_PSH = uint8(1 << 3)
	TCP_ACK = uint8(1 << 4)
	TCP_URG = uint8(1 << 5)
	TCP_ECE = uint8(1 << 6)
	TCP_CWR = uint8(1 << 7)

	TCP_NS  = uint8(1 << 0)
	TCP_RES = uint8(1<<1 | 1<<2 | 1<<3)
)

func Bswapuint16(a uint16) uint16 {
	return (a >> 8) | (a << 8)
}

type Packet struct {
	doIcmp          bool
	doTrace bool
	AgentAddress    net.IP
	InputIfaceValue uint32
	SrcAS           uint32
	DstAS           uint32

	Ethernet struct {
		Parsed     bool
		SrcMac     MacAddress
		DstMac     MacAddress
		EthType    uint16
		VlanParsed bool
		Vlan       uint16
	}

	L3Data []byte

	IP struct {
		Parsed bool
		Ipver  int

		// ipv4
		HdrLength    uint8 // expressed in number of 32-bit words
		DSCP         uint8
		ECN          uint8
		TotalLength  uint32
		Id           uint16
		Flags        uint8
		FragOff      uint16
		TTL          uint8
		Protocol     uint8
		SrcIP, DstIP net.IP
		Options      []byte

		// ipv6
		Flow uint32

		Data []byte
	}

	L4Data []byte
	Tcp    struct {
		Parsed           bool
		SrcPort, DstPort uint16
		Seq, Ack         uint32
		DataOff          uint8 // expressed in number of 32-bit words
		Res              uint8
		ECNNS            uint8 // LSB of 12th byte of TCP header
		Flags            uint8 // 13th byte of TCP header
		Win, Urg         uint16
		Options          []byte
	}

	Udp struct {
		Parsed           bool
		SrcPort, DstPort uint16
	}

	L5Data []byte
}

func (f *Packet) Parse(l2Payload []byte, hasEthernet bool) {
	var empty Packet
	*f = empty

	l3Offset := 0

	if hasEthernet && len(l2Payload) >= 14 {
		d := l2Payload
		l := &f.Ethernet
		l.Parsed = true
		l.EthType = (uint16(d[12]) << 8) | uint16(d[13])
		l3Offset = 14

		l.DstMac = NewMacAddress(l2Payload[:6])
		l.SrcMac = NewMacAddress(l2Payload[6:12])

		if l.EthType == 0x8100 {
			if len(d) < 18 {
				return
			}
			l.VlanParsed = true
			l.Vlan = (uint16(d[14]) << 8) | uint16(d[15])
			l.EthType = (uint16(d[16]) << 8) | uint16(d[17])
			l3Offset = 18
		}
	}

	l3Payload := l2Payload[l3Offset:]
	if len(l3Payload) < 1 {
		return
	}
	f.L3Data = l3Payload

	switch {
	case f.Ethernet.Parsed && f.Ethernet.EthType == 0x0800 && (l3Payload[0]&0xF0) == 0x40:
		f.IP.Ipver = 4
	case f.Ethernet.Parsed && f.Ethernet.EthType == 0x86DD && (l3Payload[0]&0xF0) == 0x60:
		f.IP.Ipver = 6
	case f.Ethernet.Parsed == false && (l3Payload[0]&0xF0) == 0x40:
		f.IP.Ipver = 4
	case f.Ethernet.Parsed == false && (l3Payload[0]&0xF0) == 0x60:
		f.IP.Ipver = 6
	}

	var l4Payload []byte

	switch {
	case f.IP.Ipver == 4 && len(l3Payload) >= 20:
		d := l3Payload
		l := &f.IP
		l.Parsed = true

		l.HdrLength = d[0] & 0xf
		l.DSCP = d[1] >> 2
		l.ECN = d[1] & 0x03
		l.TotalLength = (uint32(d[2]) << 8) | uint32(d[3])
		l.Id = (uint16(d[4]) << 8) | uint16(d[5])
		l.Flags = uint8(d[6]) >> 5
		l.FragOff = (uint16((d[6] & 0x1f)) << 8) | uint16(d[7])
		l.TTL = d[8]
		l.Protocol = d[9]
		// 10 and 11 are checksum
		l.SrcIP = net.IP(bcopy(d[12:16]))
		l.DstIP = net.IP(bcopy(d[16:20]))

		hdr_len := int(l.HdrLength) * 4
		if hdr_len < 20 || len(d) < hdr_len {
			return
		}
		if uint32(len(d)) > l.TotalLength {
			d = d[:l.TotalLength]
		}
		l.Data = d

		if len(d) >= hdr_len {
			l4Payload = d[hdr_len:]
		}

		if len(d) >= 20 {
			l.Options = d[20:hdr_len]
		}

	case f.IP.Ipver == 6 && len(l3Payload) >= 40:
		d := l3Payload
		l := &f.IP
		l.Parsed = true
		ver_tos := (uint32(d[0]) << 24) | (uint32(d[1]) << 16) | (uint32(d[2]) << 8) | uint32(d[3])
		l.DSCP = uint8(ver_tos>>22) & 0x3f
		l.ECN = uint8(ver_tos>>20) & 0x03
		l.Flow = ver_tos & 0x0fffff
		l.TotalLength = (uint32(d[4]) << 8) | uint32(d[5]) + 40
		l.Protocol = d[6]
		l.TTL = d[7]
		l.SrcIP = net.IP(bcopy(d[8:24]))
		l.DstIP = net.IP(bcopy(d[24:40]))
		// TODO: follow next header if it ain't UDP or TCP

		if uint32(len(d)) > l.TotalLength {
			d = d[:l.TotalLength]
		}
		l.Data = d
		l4Payload = d[40:]

	}
	f.L4Data = l4Payload

	if len(l4Payload) < 1 {
		return
	}

	var l5Data []byte

	switch {
	case f.IP.Protocol == PROTOCOL_TCP && len(l4Payload) >= 20:
		d := l4Payload
		l := &f.Tcp
		l.Parsed = true
		l.SrcPort = (uint16(d[0]) << 8) | uint16(d[1])
		l.DstPort = (uint16(d[2]) << 8) | uint16(d[3])

		l.Seq = (uint32(d[4]) << 24) | (uint32(d[5]) << 16) | (uint32(d[6]) << 8) | uint32(d[7])
		l.Ack = (uint32(d[8]) << 24) | (uint32(d[9]) << 16) | (uint32(d[10]) << 8) | uint32(d[11])
		l.DataOff = uint8(d[12]>>4) & 0xF
		l.Res = (d[12] & TCP_RES) >> 1
		l.ECNNS = d[12] & TCP_NS
		l.Flags = l4Payload[13]
		l.Win = (uint16(d[14]) << 8) | uint16(d[15])
		// 16, 17 - checksum
		l.Urg = (uint16(d[18]) << 8) | uint16(d[19])

		data_off := int((d[12]>>4)&0xF) * 4
		if data_off < 20 || len(d) < data_off {
			return
		}
		l5Data = l4Payload[data_off:]
		l.Options = l4Payload[20:data_off]

	case f.IP.Protocol == PROTOCOL_UDP && len(l4Payload) >= 8:
		d := l4Payload
		l := &f.Udp
		l.Parsed = true

		l.SrcPort = (uint16(d[0]) << 8) | uint16(d[1])
		l.DstPort = (uint16(d[2]) << 8) | uint16(d[3])

		l5Data = l4Payload[8:]
	}

	f.L5Data = l5Data
}

func bcopy(a []byte) []byte {
	var b = make([]byte, len(a))
	copy(b, a)
	return b
}
