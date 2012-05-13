
T = input()

def solve(wires):
    wires = sorted(wires)
    s = 0
    for e, [A, B] in enumerate(wires):
        for a, b in wires[e+1:]:
            if a > A and b < B:
                s += 1
    return s


for t in xrange(1, T+1):
    N = input()
    wires = [map(int, raw_input().split())
             for n in xrange(N)]

    print 'Case #%i: %i' % (t, solve(wires))
