import math

for t in xrange(1, input()+1):
    L, P, C = map(int, raw_input().split())

    l = L
    i = 0
    while l < P:
        i += 1
        l *= C

    print 'Case #%i: %i' % (t, math.ceil(math.log(i, 2)))

