import sys
import itertools
import primes

mul = lambda l:reduce(lambda a,b:a*b, l)

def solve(Freq, val, nos):
    wrong = False
    for f in Freqs:
        if (f % val) and (val % f):
            wrong = True
            break
    if not wrong:
        return val
    else:
        if not nos:
            return Ellipsis
        c = nos.pop(0)
        return min([
                solve(Freq, val*c, nos),
                solve(Freq, val, nos),
                ])



for test_no in xrange(1, input()+1):
    N, L, H = map(long, raw_input().split())
    Freqs = map(long, raw_input().split())

    nos = sorted(primes.getfactors(primes.LCM(Freqs)))
    uniq = set(nos)
    for u in uniq:
        nos.remove(u)

    r = solve(Freqs, mul(uniq), nos)
    if L <= r <= H:
        print 'Case #%i: %i' % (test_no, r)
    else:
        print 'Case #%i: NO' % (test_no,)
    print >> sys.stderr, 'Case #%i' % (test_no,)
