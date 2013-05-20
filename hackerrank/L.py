import functools

# How many ways there are to construct a row of length M?
def ways(M):
    if M >= 4:
        s = ways(M-4) + ways(M-3) + ways(M-2) + ways(M-1)
    elif M == 3:
        s = 4 # ways(M-3) + ways(M-2) + ways(M-1)
    elif M == 2:
        s = 2 # ways(M-2) + ways(M-1)
    elif M == 1:
        s = 1 # ways(M-1)
    elif M == 0:
        s = 1
    return s

# How many ways of constructing a non-breakable wall of height N?
def nonbreak(M, N):
    # All possibilities:
    all_ways = ways(M) ** N

    # Some of them are
    for i in xrange(1, M):
        all_ways -= nonbreak(i, N) * ways(M-i) ** N
    return all_ways


for case_no in xrange(input()):
    N, M = map(int, raw_input().split())

    print nonbreak(M, N) % 1000000007


# aaa
# aab
# abb
# abc

# a

# aab
# abc
