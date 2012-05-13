from collections import defaultdict

for case in xrange(input()):
    line = raw_input().split()
    line.reverse()

    combines = {}
    for i in range(int(line.pop())):
        prod = line.pop()
        combines[prod[0] + prod[1]] = prod[2]
        combines[prod[1] + prod[0]] = prod[2]

    opposed = defaultdict(set)
    for i in range(int(line.pop())):
        d = line.pop()
        opposed[d[0]].add(d[1])
        opposed[d[1]].add(d[0])

    _N = line.pop()

    s = []
    uniq = defaultdict(lambda:0)
    for e in line.pop():
        if len(s) > 0:
            k = s[-1] + e
            if k in combines:
                uniq[s[-1]] -= 1
                s.pop()
                s.append(combines[k])
                continue
        s.append(e)
        uniq[e] += 1
        for c in opposed[e]:
            if uniq[c] > 0:
                s = []
                uniq.clear()
                continue

    print "Case #%i: [%s]" % (case+1, ', '.join(s))
