import sys
import array
import bisect


def solve(ratings, stars, acc):
    if not ratings: return 0

    if ratings[0][0] > stars:
        return Ellipsis

    k = (stars, tuple(ratings))
    if k in acc: return acc[k]

    rating = ratings.pop(0)
    if rating[1] <= stars:
        # straigt to two stars
        ret= solve(ratings, stars + rating[2], acc)
        if ret is not Ellipsis: ret += 1
    else:
        # can't do two star
        aaa = ratings[:]
        bbb = ratings
        bisect.insort(aaa, (rating[1], rating[1], 2))
        a = solve(aaa, stars, acc)

        bisect.insort(bbb, (rating[1], rating[1], 1))
        b = solve(bbb, stars + 1, acc)
        if b is not Ellipsis: b += 1

        ret= min(a,b)
    #acc[k] = ret
    return ret


for test_no in xrange(1, input()+1):
    N = input()
    ratings = []
    for n in range(N):
        a, b = map(int, raw_input().split())
        ratings.append( (a,b,2) )
    ratings.sort()

    r  = solve(ratings, 0, {})
    if r is Ellipsis:
        print 'Case #%i: %s' % (test_no, 'Too Bad')
    else:
        print 'Case #%i: %i' % (test_no, r)

    print >> sys.stderr, 'Case #%i' % (test_no,)
