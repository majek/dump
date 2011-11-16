#!/usr/bin/env python
# -*- coding: utf-8 -*-

import asyncore
import collections
import pika
import random
import re
import sys
import time
import threading

sys.setrecursionlimit(10*1000)

B=1
KB=1024
MB=KB*1024

QUEUES = [
    # qname,  qos,  avgsize, devsize,   queue_len,  persistent,
    ("a",       1,    4*KB,   0.5*KB,      100000.0,       False),
    ("b",       1,    4*KB,   0.5*KB,      100000.0,       False),
    ("c",       1,    4*KB,   0.5*KB,      100000.0,       False),
    ("d",       2,    4*KB,   4.0*KB,      100000.0,       False),
    ("e",       2,    5*KB,   1.0*KB,      100000.0,       True),
    ("f",      10,    1*KB,   0.1*KB,      100000.0,       True),
    ("g",       1,   10*KB,   4.0*KB,      100000.0,       True),
]


def send(ch, qname, size_avg, size_dev, persistent):
    r = abs(random.gauss(size_avg, size_dev))
    ch.basic_publish(exchange='',
                 routing_key=qname,
                 body='a' * int(r),
                 properties=pika.BasicProperties(
                                delivery_mode = 2 if  persistent else 1,
                            ),
                 block_on_flow_control = False)


def queue_setup(ch, QUEUES):
    while True:
        qq = []
        for qname, qos, size_avg, size_dev, queue_len, persistent in QUEUES:
            q = ch.queue_declare(queue=qname, durable=True, exclusive=False, auto_delete=False)
            if q.message_count < queue_len:
                qq.append( (qname, q.message_count, queue_len, size_avg, size_dev, persistent) )
        if not qq:
            break
        for qname, message_count, queue_len, size_avg, size_dev, persistent in qq:
            to_send = int(min(queue_len - message_count, 250))
            print "    [?] queue %r queue length: %i items, sending %i" % (qname, message_count, to_send)
            try:
                for _ in xrange(0, to_send):
                    send(ch, qname, size_avg, size_dev, persistent)
            except pika.exceptions.ContentTransmissionForbidden:
                print " [!] can't fill %r due to channel.flow" % (qname,)
                #time.sleep(0.5)


stats=collections.defaultdict(lambda:0)

lag=collections.defaultdict(lambda:0)

DELAY=10.0
def print_stats(QUEUES):
    a = []
    b = []
    for qname, _, _, _, _, _ in QUEUES:
        a.append( '%s:%7.2f' % (qname, stats[qname]/DELAY) )
        b.append( '%s:%7i' % (qname, lag[qname]) )
    print "    [thr] %s" % ('  '.join(a))
    print "    [lag] %s" % ('  '.join(b))
    stats.clear()
    threading.Timer(DELAY, print_stats, [QUEUES]).start()


def queue_consume(conn, QUEUES):
    for qname, qos, size_avg, size_dev, queue_len, persistent in QUEUES:
        ch = conn.channel()
        ch_send = conn.channel()
        ch.basic_qos(prefetch_count=qos)
        q = ch.queue_declare(queue=qname, durable=True, exclusive=False, auto_delete=False)
        def handle_delivery(ch, method, header, body):
            qname = method.routing_key
            stats[qname] += 1
            lag[qname] += 1
            try:
                for i in range(lag[qname]):
                    send(ch_send, qname, size_avg, size_dev, persistent)
                    lag[qname] -= 1
            except pika.exceptions.ContentTransmissionForbidden:
                #print " [!] can't send to %r , channel.flow, lag %s" % (qname, lag[qname])
                #time.sleep(0.5)
                pass
            ch.basic_ack(delivery_tag = method.delivery_tag)
        ch.basic_consume(handle_delivery, queue=qname)



amqp_re = re.compile(r'^amqp://'\
                '((?P<username>[^:]*)[:](?P<password>[^@]*)[@])?' \
                '((?P<host>[^/:]*)([:](?P<port>[^/]*))?)' \
                '(?P<virtual_host>/[^/]*)/?' \
                '([?](?P<query>[^/]*))?$')

def pika_credentials_from_url(amqp_url = None):
    if not amqp_url:
        amqp_url = 'amqp://127.0.0.1/'
    params = amqp_re.match(amqp_url).groupdict()

    credentials = pika.PlainCredentials(
                        params['username'] or 'guest',
                        params['password'] or 'guest',
                    )
    conn_params = pika.ConnectionParameters(
                        params['host'] or '127.0.0.1',
                        port = int(params['port'] or '5672'),
                        virtual_host = params['virtual_host'] or '/',
                        credentials = credentials,
                    )
    return conn_params

def stress_amqp(amqp_url):
    credentials = pika_credentials_from_url(amqp_url)
    conn = pika.AsyncoreConnection(credentials)

    ch = conn.channel()
    queue_setup(ch, QUEUES)
    ch.close()

    threading.Timer(DELAY, print_stats, [QUEUES]).start()
    print " [*] Starting loop"
    queue_consume(conn, QUEUES)
    try:
        asyncore.loop()
        # get some cpu at least every few seconds
    except KeyboardInterrupt:
        pass
    print " [*] Quitting!"
    sys.exit(0)
    conn.close()


if __name__ == '__main__':
    stress_amqp(sys.argv[1])
