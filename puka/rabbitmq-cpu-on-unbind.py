#!/usr/bin/env python

import sys
sys.path.append("..")


import puka
import time



client = puka.Client("amqp://localhost/")

promise = client.connect()
client.wait(promise)

exchange = "test_"
promise = client.exchange_declare(exchange=exchange, type="topic")
client.wait(promise)


def create_queue(exchange, bindings):
    promise = client.queue_declare(exclusive=True)
    queue = client.wait(promise)['queue']

    for i in xrange(bindings):
        promise = client.queue_bind(exchange=exchange,
                                    queue=queue,
                                    routing_key="blah.blah%i.*" % i)
        client.wait(promise)
    return queue

def queue_unbind(exchange, queue, bindings):
    for i in xrange(bindings):
        promise = client.queue_unbind(exchange=exchange,
                                      queue=queue,
                                      routing_key="blah.blah%i.*" % i)
        client.wait(promise)
    promise = client.queue_delete(queue)
    client.wait(promise)

def queue_delete(exchange, queue, bindings):
    promise = client.queue_delete(queue)
    client.wait(promise)

for bindings in [100, 200, 1000, 1200]:
    print "%i bindings -> " % bindings,
    t0 = time.time()
    queue = create_queue(exchange, bindings)
    td = time.time() - t0
    print " create=%.3fsec" % td,

    t0 = time.time()
    queue = queue_delete(exchange, queue, bindings)
    td = time.time() - t0
    print " delete=%.3fsec" % td,

    t0 = time.time()
    queue = create_queue(exchange, bindings)
    td = time.time() - t0
    print " create=%.3fsec" % td,

    t0 = time.time()
    queue = queue_unbind(exchange, queue, bindings)
    td = time.time() - t0
    print " unbind=%.3fsec" % td,
    print ""

