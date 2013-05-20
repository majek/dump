import sys



for case_no in xrange(input()):
    print >> sys.stderr, case_no
    N, M = map(int, raw_input().split())

    W = [1, 1, 2, 4]
    while len(W) <= M:
        W.append(W[-1] + W[-2] + W[-3] + W[-4])

    W_n = [w ** N for w in W]

    nonbreak = [None] * (M + 1)
    for m in xrange(M + 1):
        all_ways = W_n[m]

        for i in xrange(1, m):
            all_ways -= nonbreak[i] * W_n[m-i]
        nonbreak[m] = all_ways

    print nonbreak[M] % 1000000007
