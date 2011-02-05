#!/usr/bin/env python
'''
Example of simple producer, creates one message and exits.
'''
import pika
import sys
import time


if __name__ == '__main__':
    connection = pika.BlockingConnection(pika.ConnectionParameters(
            '127.0.0.1',
            credentials=pika.PlainCredentials('guest', 'guest')))

    channel = connection.channel()

    r = channel.queue_declare(arguments={'x-expires':10000L})

    h = {
            'a': 'a',
            'b': 1,
            'c': -1,
            'd': 4611686018427387904L,
            'e': -4611686018427387904L,
            'f': [1,2,3],
            'g': [1,2,3,[1,2,3]],
            }

    channel.basic_publish(exchange='',
                          routing_key=r.queue,
                          body='test',
                          properties=pika.BasicProperties(headers=h))
    channel.close()
    channel = connection.channel()

    a = channel.basic_get(queue=r.queue, no_ack=True)
    print repr(h)
    print repr(a._properties.headers)

    
