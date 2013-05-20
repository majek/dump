import sys

W = [1, 1, 2, 4]

for case_no in xrange(input()):
    print >> sys.stderr, 'Case %r' % (case_no,)
    N, M = map(int, raw_input().split())

    while len(W) <= M:
        W.append(W[-1] + W[-2] + W[-3] + W[-4])

    W_n = [pow(w, N, 1000000007) for w in W]

    nonbreak = [None] * (M + 1)

    for m in xrange(M + 1):
        all_ways = W_n[m]

        breakable = sum(nonbreak[i] * W_n[m-i] for i in xrange(1, m))
        nonbreak[m] = (all_ways - breakable) % 1000000007

    print nonbreak[M] % 1000000007
