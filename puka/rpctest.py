#!/usr/bin/env python

import sys
sys.path.append("..")

import puka

client = puka.Client("amqp://localhost/")
promise = client.connect()
client.wait(promise)

promise = client.queue_declare(exclusive=True)
queue = client.wait(promise)['queue']

promise = client.close()
client.wait(promise)




client = puka.Client("amqp://localhost/")
promise = client.connect()
client.wait(promise)

promise = client.basic_publish(exchange='', routing_key=queue, body="blah")
print client.wait(promise)

