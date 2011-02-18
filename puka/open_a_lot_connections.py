#!/usr/bin/env python

import sys
sys.path.append("..")


import puka


clients = []
for i in range(128):
    client = puka.Client("amqp:///")
    promise = client.connect()
    clients.append(client)


while True:
    try:
        puka.loop(clients)
    except Exception, e:
        print e

