import sys
import array
import bisect
import itertools

def collide(sa, pa, sb, pb):
    if sb == sa: return None, None
    t1 = (pa + 5. - pb) / (sb - sa)
    t2 = (pb + 5. - pa) / (sa - sb)
    if t1 <= t2:
        return t1, t2
    else:
        return t2, t1

def solve(cars):
    C, S, P = cars.pop()
    ranges = []
    for a_c, a_s, a_p in cars:
        t1, t2 = collide(S, P, a_s, a_p)
        if t1 != None:
            ranges.append( (t1, t2) )
    if len(cars) > 1:
        ranges.extend( solve(cars) )
    return ranges


for test_no in xrange(1, input()+1):
    N = input()
    cars = []
    for n in xrange(N):
        C, S, P = raw_input().split()
        cars.append( (C, int(S), int(P)) )

    ranges = sorted(solve(cars[:]))
    print cars
    print ranges
    dupa = 'Possible'
    for number in sorted(itertools.chain(*ranges)):
        x = 0
        for t1, t2 in ranges:
            if t1 <= number <= t2:
                x += 1
        if x > 2:
            dupa = number
            break

    print 'Case #%i: %s' % (test_no, dupa)
    print >> sys.stderr, 'Case #%i' % (test_no,)
