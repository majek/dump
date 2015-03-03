#!/usr/bin/env python

import gae_channel
import urllib2
import sys
import json
import time


def fetch_token(url):
    return urllib2.urlopen(url).read()


if __name__ == '__main__':
    if len(sys.argv) == 2:
        url = sys.argv[1]
    else:
        channel_name = 'leds60'
        url = 'http://curious-drive-87310.appspot.com/%s/' % (channel_name,)
    print >> sys.stderr, '[*] getting token from %r' % (url,)
    token = fetch_token(url)
    token = token.strip()
    while True:
        print >> sys.stderr, '[*] Token: %r' % (token,)
        for i in xrange(100):
            try:
                channel = gae_channel.Client(token=token)
                for msg in channel.messages():
                    payload = json.loads(msg)
                    print '%r' % (payload,)
            except Exception, e:
                print >> sys.stderr, '[!] Retrying: %r' % (e,)
                time.sleep(1)
