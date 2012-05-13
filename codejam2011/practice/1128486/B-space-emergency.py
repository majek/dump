import sys

def solve(L, t, dists):
    r = 0
    if L > 0:
        dists.reverse()
        while dists:
            if t >= dists[-1]:
                t -= dists[-1]
                r += dists[-1]
                dists.pop()
            else:
                break
        dists.reverse()
        if t and dists:
            dists[0] -= t
            r += t
            t = 0

    dists = sorted(dists)
    while L and dists:
        x = dists.pop()
        r += x / 2
        L -= 1

    r += sum(dists)

    return r

for test_no in xrange(1, input()+1):
    ints = map(long, raw_input().split())
    L, t, N, C = ints[0:4]
    a = ints[4:]
    assert len(a) == C

    while len(a) < N:
        a = a + a
    a = map(lambda x:x*2, a[:N])
    assert len(a) == N


    print 'Case #%i: %i' % (test_no, solve(L, t, a))
    print >> sys.stderr, 'Case #%i' % (test_no,)
