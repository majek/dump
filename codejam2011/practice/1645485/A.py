import sys
import array

def Cases(B):
    if B == 1: return [[True], [False]]
    acc = []
    print >> sys.stderr, B
    for case in Cases(B-1):
        acc.append([True] + case)
        acc.append([False] + case)
    print >> sys.stderr, B, '1'
    return acc

def Prob(a, case):
    p = 1
    for i, c in enumerate(case):
        if c: p *= a[i]
        else: p *= 1. - a[i]
    return p

def solve(B, a):
    remaining = B - len(a)

    cases = Cases(len(a))
    probs = [Prob(a, case) for case in cases]

    expected = B + 1 + 1
    for backspaces in range(len(a)+1):
        row = []
        for case in cases:
            left = case[:len(a)-backspaces]
            if 0 not in left:
                # okay
                row.append(backspaces +backspaces + remaining + 1)
            else:
                row.append(backspaces +backspaces + remaining + 1 + B + 1 )
        expected = min(expected, sum(map(lambda (a,b):a*b, zip(row, probs))) )

    #print probs, expected
    return expected

for test_no in xrange(1, input()+1):
    A, B = map(int, raw_input().split())
    a = map(float, raw_input().split())

    print 'Case #%i: %.6f' % (test_no, solve(B, a))
    print >> sys.stderr, 'Case #%i' % (test_no,)
