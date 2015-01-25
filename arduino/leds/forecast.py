#!/usr/bin/env python

#
# curl --silent "https://api.forecast.io/forecast/TOKEN/lat,long" > forecast.json
#

import json
import serial
import sys
import time


BLACK='\x00\x00\x00'
RED='\x08\x00\x00'
WHITE='\x03\x03\x03'
GREEN='\x00\x04\x00'
BLUE='\x00\x00\x04'

COLORS = {
    ' ': BLACK,
    'r': RED,
    'w': WHITE,
    'g': GREEN,
    'b': BLUE,
}


class Serial(object):
    def __init__(self):
        try:
            self.sfd = serial.Serial("/dev/ttyUSB0", 115200,
                                parity=serial.PARITY_NONE,
                                stopbits=serial.STOPBITS_ONE)
            print >> sys.stderr, "[x] Serial opened %s" % (self.sfd,)
        except serial.serialutil.SerialException:
            self.sfd = None
            print >> sys.stderr, "[x] Nope, serial disabled"

        self.led = [BLACK] * 30
        self.write()

    def write(self):
        if self.sfd:
            self.sfd.write('\xff\xff\xff' + ''.join(self.led))
            self.sfd.flush()


def f_to_c(f):
    return (f - 32) / 1.8000

def i_to_mm(i):
    return i / 0.039370

def bands(b, a=0, c=10):
    b = int(round(b))
    return max(a, min(b,c))

def main(json_file):
    s = Serial()

    with open(json_file, 'rb') as fd:
        data = json.loads(fd.read())

    daily = data['daily']['data'][0]


    t_max = f_to_c(daily['apparentTemperatureMax'])
    t_min = f_to_c(daily['apparentTemperatureMin'])
    print 'temp: min=%.1f max=%.1f' % (t_min, t_max)

    C = [' '] * 30
    if abs(t_min -t_max) < 2.5:
        t_min -= 2.5
    for i in xrange(bands(t_min/2.5, a=-10), bands(t_max/2.5, a=-10)):
        if i < 0:
            C[10+i] = 'g'
        else:
            C[i] = 'r'





    cloud = daily['cloudCover']
    print 'cloud: %.2f' % (cloud,)

    for i in xrange(bands(cloud * 10)):
        C[19-i] = 'w'


    mmrain = i_to_mm(daily['precipIntensityMax'])
    print 'rain: %.2fmm' % (mmrain,)
    # daily['precipProbability']

    for i in xrange(bands(mmrain / 0.2)):
        C[20+i] = 'b'

    print C[:10]
    print C[10:20]
    print C[20:30]


    s.led[:30] = [COLORS[ c ] for c in C]

    for i in xrange(20):
        s.write()
        time.sleep(0.01)
    if s.sfd:
        s.sfd.close()

if __name__ == "__main__":
    try:
        main(sys.argv[1])
    except KeyboardInterrupt:
        pass
