#!/usr/bin/env python

import sys
sys.path.append("..")


import puka


print " [*] setting up"

client = puka.Client("amqp://localhost/")

promise = client.connect()
client.wait(promise)

exchange = "test_"
promise = client.exchange_declare(exchange=exchange, type="topic")
client.wait(promise)

promise = client.queue_declare(exclusive=True)
queue = client.wait(promise)['queue']

for i in xrange(10000):
    promise = client.queue_bind(exchange=exchange,
                                queue=queue,
                                routing_key="blah.blah%i.*" % i)
    client.wait(promise)

print " [*] done. deleting queue"

import time
t0 = time.time()

promise = client.queue_delete(queue)
client.wait(promise)
td = time.time() - t0

print " [*] queue_delete took %f" % td
