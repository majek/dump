#!/usr/bin/env python

import math

class StdDev(object):
    def __init__(self):
        self.sum, self.sum_sq, self.count = 0.0, 0.0, 0
    def add(self, value):
        self.count += 1
        self.sum += value
        self.sum_sq += value ** 2
    def avg(self):
        if self.count > 0:
            return self.sum / self.count
        else:
            return 0.0
    def dev(self):
        variance = (self.sum_sq / self.count) - (self.avg() ** 2)
        return math.sqrt(variance)


sd = StdDev()

ignore = 5
while 1:
    try:
        line = raw_input()
    except EOFError:
        break
    if not line or line.startswith("total"):
        continue
    if ignore != 0:
        ignore -= 1
        continue
    lps, lpt_avg, lpt_dev, r_avg, r_dev = map(float, line.split(','))
    sd.add(1000000000.0 / lps)


print 'ns per loop avg, deviation'
print '%.3f, %.3f' % (sd.avg(), sd.dev())
