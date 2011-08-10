#!/usr/bin/env python

import sys
sys.path.append("..")

import puka


client = puka.Client("amqp://localhost/")
promise = client.connect()
client.wait(promise)

promise = client.queue_declare(exclusive=True)
q = client.wait(promise)

promise = client.queue_declare(exclusive=True)
qq = client.wait(promise)

promise = client.queue_declare(exclusive=True)
q1 = client.wait(promise)

promise = client.queue_declare(exclusive=True)
q2 = client.wait(promise)

promise = client.exchange_declare(exchange="a", type="x-presence")
client.wait(promise)

promise = client.queue_bind(queue=q['queue'], exchange="a", routing_key='listen')
client.wait(promise)

promise = client.queue_bind(queue=qq['queue'], exchange="a", routing_key='listen')
client.wait(promise)

promise = client.queue_bind(queue=q1['queue'], exchange="a", routing_key="_")
client.wait(promise)

promise = client.queue_unbind(queue=q1['queue'], exchange="a", routing_key="_")
client.wait(promise)

promise = client.queue_bind(queue=q2['queue'], exchange="a", routing_key="q2")
client.wait(promise)

for i in range(10):
    promise = client.basic_get(queue=q['queue'], no_ack=True)
    print client.wait(promise)

for i in range(10):
    promise = client.basic_get(queue=qq['queue'], no_ack=True)
    print client.wait(promise)


