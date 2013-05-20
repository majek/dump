import sys

W = [1, 1, 2, 4]

def modexp ( g, u, p ):
    # via: http://stackoverflow.com/questions/5486204/fast-modulo-calculations-in-python-and-ruby
    """computes s = (g ^ u) mod p
    args are base, exponent, modulus
    (see Bruce Schneier's book, _Applied Cryptography_ p. 244)"""
    s = 1
    while u != 0:
        if u & 1:
            s = (s * g) % p
        u >>= 1
        g = (g * g) % p
    return s


for case_no in xrange(input()):
    print >> sys.stderr, 'Case %r' % (case_no,)
    N, M = map(int, raw_input().split())

    while len(W) <= M:
        W.append(W[-1] + W[-2] + W[-3] + W[-4])

    # This is the core. This is too slow:
    #    W_n = [(w ** N) % 1000000007 for w in W]
    # This is okay:
    W_n = [modexp(w, N, 1000000007) for w in W]

    nonbreak = [None] * (M + 1)

    for m in xrange(M + 1):
        all_ways = W_n[m]

        breakable = sum(nonbreak[i] * W_n[m-i] for i in xrange(1, m))
        nonbreak[m] = (all_ways - breakable) % 1000000007

    print nonbreak[M] % 1000000007
