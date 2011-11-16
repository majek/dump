#!/usr/bin/env python
# -*- coding: utf-8 -*-


import thread
import sys, os
import functools
import traceback
import time
import random

import amqplib.client_0_8 as amqp

QUEUES = [
    # qname, msg/sec,   min/max size,   queue_len  persistent noack
    ("a",       0.1,      1000, 10000,         100000.0,       False,    False),
    ("b",       1.0,      1000, 10000,         100000.0,       True,    False),
    ("c",      10.0,      1000, 10000,         100000.0,       False,    False),
    ("d",      1000.0,    1000, 10000,         100000.0,       True,    True),
    ("e",      1000.0,    1000, 10000,         100000.0,       True,    False),
    ("f",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("g",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("ga",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gb",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gc",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gd",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("ge",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gf",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gg",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gh",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gi",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gj",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gk",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gl",      100.0,    1000, 10000,         100000.0,       False,    False),
    ("gm",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gn",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("go",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gp",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gq",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gr",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gs",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gt",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gu",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gv",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gw",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gx",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gy",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("gz",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("g1",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("g2",      100.0,    1000, 10000,         100000.0,       False,    False),
#    ("g3",      100.0,    1000, 10000,         100000.0,       False,    False),
    ]

FREE_QUEUES = [
    # qname, msg/sec,   min/max size,   queue_len  persistent noack
    (name, 1.0, minsz, maxsz, 10.0,  False, noack) \
        for (name, msgsec, minsz, maxsz, queuelen, persistent, noack) in QUEUES
    ]


def logwrapper(m):
    @functools.wraps(m)
    def wrapper(*args, **kwargs):
        try:
            return m(*args, **kwargs)
        except:
            print "[**] Traceback!\n%s" % \
                                (traceback.format_exc(),)
            os.abort()

    return wrapper

def basic_publish(ch, qname, min_size, max_size, persistent):
    body = 'a' * random.randint(min_size, max_size)
    msg = amqp.Message(body, application_headers={})
    #if persistent:
    if True:
        msg.properties["delivery_mode"] = 2
    ch.basic_publish(msg, '', routing_key=qname)

    #ch.basic_publish(exchange='',
        #routing_key=qname,
        #body=body,
        #properties=rabbitmq.BasicProperties(
                    #delivery_mode = 2, # persistent
                        #))

def basic_get(ch, qname):
    method = ch.basic_get(qname, no_ack=True)

@logwrapper
def queue_setup(conn_args, qname, ratio, min_size, max_size, queue_len, persistent, noack):
    assert queue_len >= ratio
    print " [*] #%s queue:%r ratio:%.3f" % (qname, qname, ratio)
    #conn = rabbitmq.Connection('127.0.0.1')
    conn = amqp.Connection(**conn_args)
    ch = conn.channel()
    message_count = ch.queue_declare(queue=qname, durable=True, exclusive=False, auto_delete=False)[1]
    ch.basic_qos(0, 1, False)
    ch.exchange_declare(qname+"+e", "headers", durable=True)
    ch.queue_bind(qname, qname+"+e",'routingkey', { "x-match":"all", "a":"b"})

    if message_count < queue_len:
        print " [*] #%s filling queue, %i items" % (qname, abs(message_count-queue_len), )
        for i in xrange(message_count, queue_len):
            basic_publish(ch, qname, min_size, max_size, persistent)
    if message_count > queue_len:
        print " [*] #%s emptying queue, %i items" % (qname, abs(message_count-queue_len), )
        for i in xrange(queue_len, message_count):
            basic_get(ch, qname)
    
    received = [0]
    last_received = [0]
    t0 = time.time()
    def handle_delivery(msg):
        received[0] += 1
        tpassed = time.time() - t0
        tshould = received[0] / ratio
        nm = max(1, int(ratio))
        if received[0] % nm == 0:
            for i in xrange(nm):
                basic_publish(ch, qname, min_size, max_size, persistent)
            print " [*] #%s  ratio:%8.3f drift:%8.3f td:%6.3f" % (qname, 
                        ratio, ratio-received[0]/tpassed, tshould-tpassed)
            last_received[0] = received[0]
        if tpassed < tshould:
            time.sleep(tshould - tpassed)
        if noack == False:
            msg.channel.basic_ack(msg.delivery_tag)

    tag = ch.basic_consume(qname, callback=handle_delivery, no_ack=noack)
    while True:
        ch.wait()
   
import sys
import re
amqp_re = re.compile(r'^amqp://(?P<userid>[^:]*):(?P<password>[^@]*)@(?P<host>[^/]*)(?P<virtual_host>/[^/]*)/?$')

def main():
    conn_args = amqp_re.match(sys.argv[1]).groupdict()
    
    queues = QUEUES
    if len(sys.argv)==3 and "-f" == sys.argv[2]:
        print " [**] freeing queue"
        queues = FREE_QUEUES
    
    for args in queues:
        thread.start_new_thread(queue_setup, tuple([conn_args] + list(args)))
    

    print " [*] Starting loop"

    try:
        while True:
            time.sleep(3) # get some cpu at least once few seconds
    except KeyboardInterrupt:
        pass
    print " [*] Quitting!"
    sys.exit(0)


if __name__ == '__main__':
    main()
