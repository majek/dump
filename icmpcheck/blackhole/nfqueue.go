package main

import (
	"github.com/chifflier/nfqueue-go/nfqueue"
	"syscall"
)

var global_ch  chan []byte

func real_callback(payload *nfqueue.Payload) int {
	data := make([]byte, len(payload.Data))
	copy(data, payload.Data)
	payload.SetVerdict(nfqueue.NF_DROP)
	global_ch  <- data
	return 0
}

func NFQueue(queueNo int) chan []byte{
	q := new(nfqueue.Queue)

	ch := make(chan []byte, 32)
	global_ch = ch

	q.SetCallback(real_callback)

	q.Init()

	q.Unbind(syscall.AF_INET)
	q.Unbind(syscall.AF_INET6)
	q.Bind(syscall.AF_INET)
	q.Bind(syscall.AF_INET6)

	q.CreateQueue(queueNo)
	go func (){
		q.Loop()
		q.DestroyQueue()
		q.Close()
	}()
	return ch
}
