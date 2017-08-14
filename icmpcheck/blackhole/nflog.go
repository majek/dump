package main

import (
	"fmt"
	"github.com/Alkorin/nflog"
	"os"
)

func NewNflog(l int) chan *nflog.Msg {
	conf := nflog.NewConfig()
	conf.Groups = []uint16{uint16(l)}
	conf.CopyRange = 65535
	conf.Return.Errors = true

	n, err := nflog.New(conf)
	if err != nil {
		fmt.Fprintf(os.Stderr, "[!] Can't open nflog: %s\n", err)
		os.Exit(1)
	}

	ch := make(chan *nflog.Msg, 64)

	go func() {
		for {
			m := <-n.Messages()
			ch <- &m
		}
	}()

	return ch
}
