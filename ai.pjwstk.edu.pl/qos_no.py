import amqplib.client_0_8 as amqp
import itertools
import os
import thread
import time

QNAME='myqueue'

def get_ch():
    conn = amqp.Connection('127.0.0.1')
    ch = conn.channel()
    qname, _, _ = ch.queue_declare(QNAME)
    return conn, ch

def sender():
    conn, ch = get_ch()
    for i in itertools.count():
        msg = amqp.Message("%i" % (i) )
        ch.basic_publish(msg, '', routing_key=QNAME)
        time.sleep(0.5)

def receiver(name):
    conn, ch = get_ch()

    def callback(msg):
        delay = 1 + (int(msg.body) % 2)
        print "%s: got %s, waiting %s" % (name, msg.body, delay)
        time.sleep(delay)
        msg.channel.basic_ack(msg.delivery_tag)

    ch.basic_consume(QNAME, callback=callback)

    while True:
        ch.wait()

if __name__ == '__main__':
    thread.start_new_thread(receiver, ("1st",))
    thread.start_new_thread(receiver, ("2nd",))

    sender()



