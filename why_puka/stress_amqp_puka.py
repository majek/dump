#!/usr/bin/env python

import sys
sys.path.append("..")

import puka
import time
import collections

AMQP_URL = "amqp://macrabbit1/"
QUEUE_CNT = 32
BURST_SIZE = 120
QUEUE_SIZE = 1000
BODY_SIZE = 1
PREFETCH_CNT = 1
MSG_HEADERS = {'persistent': False}
PUBACKS = True

counter = 0
counter_t0 = time.time()

class AsyncGeneratorState(object):
    def __init__(self, client, gen):
        self.gen = gen
        self.client = client
        self.promises = collections.defaultdict(list)

        self.waiting_for = self.gen.next()
        self.client.set_callback(self.waiting_for, self.callback_wrapper)

    def callback_wrapper(self, t, result):
        self.promises[t].append(result)
        while self.waiting_for in self.promises:
            result = self.promises[self.waiting_for].pop(0)
            if not self.promises[self.waiting_for]:
                del self.promises[self.waiting_for]
            self.waiting_for = self.gen.send(result)
        self.client.set_callback(self.waiting_for, self.callback_wrapper)

def puka_async_generator(method):
    def wrapper(client, *args, **kwargs):
        AsyncGeneratorState(client, method(client, *args, **kwargs))
        return None
    return wrapper


@puka_async_generator
def worker(client, q, msg_cnt, body, prefetch_cnt, inc, avg):
    result = (yield client.queue_declare(queue=q, durable=True))
    fill = max(msg_cnt - result['message_count'], 0)

    while fill > 0:
        fill -= BURST_SIZE
        for i in xrange(BURST_SIZE):
            promise = client.basic_publish(exchange='', routing_key=q,
                                          body=body, headers=MSG_HEADERS)
        yield promise # Wait only for one in burst (the last one).
        inc(BURST_SIZE)

    consume_promise = client.basic_consume(queue=q, prefetch_count=prefetch_cnt)
    while True:
        msg = (yield consume_promise)
        t0 = time.time()
        yield client.basic_publish(exchange='', routing_key=q,
                             body=body, headers=MSG_HEADERS)
        td = time.time() - t0
        avg(td)
        client.basic_ack(msg)
        inc()


average = average_count = 0.0
def main():
    client = puka.Client(AMQP_URL, pubacks=PUBACKS)
    promise = client.connect()
    client.wait(promise)

    def inc(value=1):
        global counter
        counter += value

    def avg(td):
        global average, average_count
        average += td
        average_count += 1

    for q in ['q%04i' % i for i in range(QUEUE_CNT)]:
        worker(client, q, QUEUE_SIZE, 'a' * BODY_SIZE, PREFETCH_CNT, inc, avg)


    global counter, average, average_count

    print ' [*] loop'
    while True:
        t0 = time.time()
        client.loop(timeout=1.0)
        td = time.time() - t0
        average_count = max(average_count, 1.0)
        print "send: %i  avg: %.3fms " % (counter/td,
                                          (average/average_count)*1000.0)
        counter = average = average_count = 0

if __name__ == '__main__':
    main()
