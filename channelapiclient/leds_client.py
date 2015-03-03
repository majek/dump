#!/usr/bin/env python

import serial
import sys
import time
import thread

last_updated = [0]

sfd = serial.Serial("/dev/ttyUSB0", 115200,
                    parity=serial.PARITY_EVEN,
                    stopbits=serial.STOPBITS_TWO)

def main():
    while True:
        data = raw_input()
        data = data[:180]
        if len(data) != 180:
            print >> sys.stderr, "[!] invalid data on stdin"
            continue
        sfd.write('\x00\x01\x02' + data)
        last_updated[0] = time.time()

def check_freshness():
    while True:
        if time.time() - last_updated[0] > 60:
            print "[!] stale data. clearing"
            sfd.write('\x00\x01\x02' + ('\x00\x00\x00' * 60))
        time.sleep(20)

if __name__ == '__main__':
    thread.start_new_thread(check_freshness, ())
    main()
