#!/usr/bin/env python

COUNT=26000

import time
import sys
sys.path.append("..")

import puka

client = puka.Client("amqp:///")

promise = client.connect()
client.wait(promise)

promise = client.queue_declare()
queue_name = client.wait(promise)['queue']

def fill_queue():
    t0 = time.time()
    for i in xrange(COUNT):
        client.basic_publish(exchange='', routing_key=queue_name, body='')
    t1 = time.time()

    print "Raw publish %i messages: %.1fs (%i msgs/s)" % (
        COUNT, t1-t0, COUNT/(t1-t0))

    promise = client.basic_publish(exchange='', routing_key='', body='')
    client.wait(promise)

    t2 = time.time()
    print "Publish %i messages and waited for network to be flushed:" \
        " %.1fs (%i msgs/s)" % (
        COUNT, t2-t0, COUNT/(t2-t0))


t0 = time.time()
fill_queue()
consume_promise = client.basic_consume(queue=queue_name)
for i in xrange(COUNT):
    result = client.wait(consume_promise)
    client.basic_ack(result)

t1 = time.time()
print "Consume %i messages no_ack=False: %.1fs (%i msgs/s)" % (
    COUNT, t1-t0, COUNT/(t1-t0))

promise = client.basic_cancel(consume_promise)
client.wait(promise)


t0 = time.time()
fill_queue()
consume_promise = client.basic_consume(queue=queue_name, no_ack=True)
for i in xrange(COUNT):
    result = client.wait(consume_promise)
t1 = time.time()
print "Consume %i messages no_ack=True: %.1fs (%i msgs/s)" % (
    COUNT, t1-t0, COUNT/(t1-t0))
