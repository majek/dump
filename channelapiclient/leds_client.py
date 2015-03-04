#!/usr/bin/env python

import serial
import sys
import time
import thread

last_updated = [0]
last_data = ''

sfd = serial.Serial("/dev/ttyUSB0", 115200,
                    parity=serial.PARITY_EVEN,
                    stopbits=serial.STOPBITS_TWO)

def main():
    while True:
        data = raw_input()
        print "[!] got data! len=%d" % (len(data),)
        if last_data != data:
            continue
        sfd.write('\xff\xff\xff' + data)
        last_updated[0] = time.time()
        last_data = data

def check_freshness():
    while True:
        if time.time() - last_updated[0] > 60:
            print "[!] stale data. clearing"
            sfd.write('\xff\xff\xff' + ('\x00\x00\x00' * 90))
        time.sleep(20)

if __name__ == '__main__':
    thread.start_new_thread(check_freshness, ())
    main()
