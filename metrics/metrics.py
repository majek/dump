#!/usr/bin/env python

import collections
import datetime
import serial
import simplejson as json
import sys
import time
import urllib
import urllib2

sfd = serial.Serial("/dev/ttyUSB0", 115200,
                    parity=serial.PARITY_EVEN,
                    stopbits=serial.STOPBITS_TWO)


refresh = 5
key = 'proc.loadavg.1m/node-type=m,node=*'
url = "http://metrics.cfops.it:2210/query/%s" % urllib.quote(key)

MAX=58

def main():
    counter = 0
    err = False
    while True:
        if err:
            x = ['\x00\x00\x00'] * 60
            sfd.write('\x00\x01\x02' + ''.join(x))
            err = False
            time.sleep(1)

        counter += 1

        now = time.time()
        dtnow = datetime.datetime.fromtimestamp(now)

        cur_url = url + '?' + urllib.urlencode({
                'start': str(long(now) - 3*60,),
    #            'end': str(long(time.time()) - 60,),
    #            'rate': 'false',
    #            'aggregator': 'max',
    #            'downsample': '10s-max',
                }.items())

        try:
            r = urllib2.urlopen(cur_url)
            data = r.read()
        except urllib2.URLError, e:
            print >> sys.stderr, "HTTP error: %s" % (e,)
            err = True
            continue
        if r.code != 200:
            print >> sys.stderr, "HTTP error: %r" % (r.code,)
            err = True
            continue

        delta = datetime.datetime.now() - dtnow

        hg = collections.Counter(min(MAX, int(timeseries[-1][1]))
                                 for _node, timeseries \
                                     in json.loads(data)[key].iteritems())
        if not hg:
            print >> sys.stderr, "Received empty data"
            err = True
            continue

        print "\033[2J\033[;H"
        print "%s (query took %s second)s" % (dtnow, delta.total_seconds())

        for k in xrange(MAX):
            print '%4s\t%s' % (k, '*' * hg[k])
        print

        items = sum(hg.values())
        s = 0
        q = []
        for k in xrange(MAX):
            s += hg[k]
            if s >= (len(q) * items / 4):
                q.append( k )
        print "quartiles=%s " % (q)


        RED='\x20\x00\x00'
        WHITE='\x20\x20\x20'
        GREEN='\x00\x20\x00'

        x = ['\x00\x00\x00'] * 60
        for i in xrange(q[4]):
            x[i] = RED
        for i in xrange(q[3]):
            x[i] = WHITE
        for i in xrange(q[2]):
            x[i] = GREEN

        x[-1] = '\x20\x20\x20' if counter % 2 else '\x00\x00\x00'

        sfd.write('\x00\x01\x02' + ''.join(x))

        delta = datetime.datetime.now() - dtnow
        wait = max(1, refresh - delta.total_seconds())
        time.sleep(wait)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt, e:
        pass
