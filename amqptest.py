#!/usr/bin/env python

import sys
sys.path.append("..")


import time
import puka
import random
import uuid

what = 2

if what == 1:
    BINDING_KEY = "app1.channel%s.event"
    EXCHANGE_TYPE = "topic"
if what == 2:
    BINDING_KEY = "app1.channel%s.*"
    EXCHANGE_TYPE = "topic"
if what == 3:
    BINDING_KEY = "app1.channel%s.event"
    EXCHANGE_TYPE = "direct"

ROUTING_KEY = "app1.channel%s.event"


uuid_generate = lambda: str(uuid.uuid4())


def main(m, n):
    print "Testing sending %i messages to %i bindings (%r, %r)" % (
        n, m, BINDING_KEY, EXCHANGE_TYPE)

    client = puka.Client("amqp://localhost/")

    promise = client.connect()
    client.wait(promise)

    exchange = "test_%s" % (EXCHANGE_TYPE,)
    promise = client.exchange_declare(exchange=exchange,
                                      type=EXCHANGE_TYPE)
    client.wait(promise)

    queue = "%s" % (uuid_generate(),)
    promise = client.queue_declare(queue=queue,
                                   auto_delete=True,
                                   durable=False)
    client.wait(promise)

    for i in xrange(m):
        promise = client.queue_bind(exchange=exchange,
                                    queue=queue,
                                    binding_key=BINDING_KEY % (i,))
        client.wait(promise)

    print "Setup finished"
    delivered = 0

    t0 = time.time()

    for i in xrange(n):
        client.basic_publish(exchange=exchange,
                             routing_key=ROUTING_KEY % (random.randint(0,m-1),),
                             body="hello world")

    counter = [0]
    def callback(promise, result):
        counter[0] += 1
        #if counter[0] % 100 == 0:
        #    print "Delivered %s" % (counter[0],)
        if counter[0] == n:
            client.loop_break()

    client.basic_consume(queue=queue, no_ack=True, callback=callback)
    client.loop()

    td = time.time() - t0
    print "%.3fsec  --> %i mps" % (td, n/td)

if __name__ == '__main__':
    argd = dict(enumerate(sys.argv))
    main(int(argd.get(1, '100')), # bindings
         int(argd.get(2, '5000'))) # messages
