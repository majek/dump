#!/usr/bin/env python

import sys
sys.path.append("..")


import puka
import time

clients = []
while True:
    client = puka.Client("amqp:///")
    promise = client.connect()
    try:
        client.wait(promise)
        clients.append(client)
    except Exception, e:
        print "e", e
        time.sleep(1)
    if len(clients) > 900:
        break

print "waiting"
puka.loop(clients)
