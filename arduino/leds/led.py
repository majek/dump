#!/usr/bin/env python

import serial
import sys
import time


BLACK='\x00\x00\x00'
RED='\x12\x00\x00'
WHITE='\x09\x09\x09'
GREEN='\x00\x08\x00'

COLORS = [BLACK, RED, WHITE, GREEN]

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

    def write(self):
        if self.sfd:
            x = '\xff\xff\xff' + ''.join(self.led)
            self.sfd.write(x)
            self.sfd.flush()


def main():
    s = Serial()
    colors = COLORS[:]
    while True:
        c, colors = colors[0], colors[1:] + colors[:1]
        for i in xrange(30):
            s.led[i] = c
        s.write()
        time.sleep(1)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
