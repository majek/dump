import itertools
import array

T = input()

arr = lambda init: array.array('b', init)

unpack = {}
for c in 'ABCDEF0123456789':
    v = int(c, 16)
    unpack[c] = [int(bool(i)) for i in  [(v & 8), (v & 4) , (v & 2), (v & 1)]]

stuff = [{}, {}]
for size in range(513):
    stuff[0][size] = arr([i % 2 for i in range(size)])
    stuff[1][size] = arr([i % 2 for i in range(1, size+1)])

def mark_is_square(x, y, data, size):
    p = data[y][x]
    row_a = stuff[p][size]
    row_b = stuff[(p+1) % 2][size]
    for row in (data[row][x : x+size] for row in xrange(y, y + size, 2)):
        if row != row_a: return 0
    for row in (data[row][x : x+size] for row in xrange(y + 1, y + size, 2)):
        if row != row_b: return 0

    for row in xrange(y, y+size):
        for col in xrange(x, x+size):
            data[row][col] = -1
    return 1


def occupy(data, size):
    if size == 0:
        return {}
    X, Y = len(data[0]), len(data)
    count = 0
    for y in xrange(Y - size+1):
        for x in xrange(X - size+1):
            count += mark_is_square(x, y, data, size)
    ret = occupy(data, size-1)
    if count:
        ret[size] = count
    return ret

for t in xrange(1, T+1):
    [M, N] = map(int, raw_input().split())
    data = [arr(itertools.chain(
                *(unpack[c]
                  for c in raw_input().strip())))
            for n in xrange(M)]
    scores = occupy(data, min(N, M))
    print 'Case #%i: %i' % (t, len(scores))
    for size in sorted(scores.keys(), reverse=1):
        print '%i %i' % (size, scores[size])
