import sys, time, collections
sys.setrecursionlimit(10096)

def findtopbit(vo):
    i = 0
    v = vo
    while v:
        i += 1
        v = v >> 1
    i -= 1
    assert vo >= (1 << i)
    assert vo < (1 << (i+1))

xor = lambda l: reduce(lambda a,b:a^b, l)

def solve(p, px, su, sx, mem, result, mask):
    for i in xrange(len(p)):
        p2 = p[:i] + p[i+1:]
        e = p[i]
        su2 = su + e

        if p2 and max(p2) < mask: continue

        k = p2
        if k not in mem:
            solve(p2, px ^ e, su2, sx ^ e, mem, result, mask)
            mem[k] = True
            if p2 and (px ^ e) == (sx ^ e):
                result.append(su2)



for case in xrange(input()):
    N = input()
    candies = map(int, raw_input().split())

    topbit = findtopbit(max(candies))

    mem = {}
    r = []
    solve(tuple(sorted(candies)), xor(candies), 0, 0, mem, r, topbit)
    if not r:
        print "Case #%i: NO" % (case+1,)
    else:
        print "Case #%i: %i" % (case+1, max(r))
