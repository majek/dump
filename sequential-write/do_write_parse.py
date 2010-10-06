#!/usr/bin/env python
from collections import defaultdict
import math
import sys

def count(i):
    avg = sum(i) / len(i)
    sum_sq = sum( [x*x for x in i] )
    return avg, math.sqrt( (sum_sq / len(i)) - (avg*avg) )


for filename in sys.argv[1:]:
    _write, size, _fsync, _disk = filename.split('-')
    size = int(size)

    dd = defaultdict(list)
    for line in file(filename, 'r').readlines():
        k, v = line.split()
        v = float(v)
        dd[k].append(v)

    for k, v in dd.iteritems():
        avg, dev = count(v)
        v2 = [size/i for i in v]
        avg2, dev2 = count(v2)
        print "%s\t%.3f\t%.3f\t%.6f\t%.6f  #%s %s" % (size, avg, dev, avg2, dev2, filename, k)

        if k in ['msync', 'fdatasync']:
            curr = [v, v2]
            sum_raw = [a+b for (a,b) in zip(v, prev_v)]
            sum_mbs = [size/i for i in sum_raw]
            avg_raw, dev_raw = count(sum_raw)
            avg_mbs, dev_mbs = count(sum_mbs)
            print "%s\t%.3f\t%.3f\t%.6f\t%.6f  #%s %s" % (
                size,
                avg_raw, dev_raw,
                avg_mbs, dev_mbs,
                filename, "%s + %s" % (prev_k, k))

        prev_v = v
        prev_k = k


# spin, write+fdatasync
[
102.795495,
109.264601,
108.251728,
110.460893,
109.937905,
113.187019,
]

# spin, memset + msync
spin = [
96.54356,
101.156289,
102.014740,
103.746471,
106.291871,
107.804159,
]

# ssd, memset + msync
ssd = [
56.99281,
60.124751,
63.475930,
62.529965,
66.32680,
67.03126,
]

print "spin", count(spin)
print "ssd", count(ssd)
