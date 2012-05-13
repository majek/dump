


def number_of_recycled(N, M):
    xsum = 0
    Ms = str(M)
    for i in xrange(N, M):
        ns = str(i)

        ems = [ns[j:] + ns[:j] for j in xrange(1, len(ns))]
        ems2 = set(em for em in ems if ns < em <= Ms and em[0] != '0' )
        xsum += len(ems2)
    return xsum

#print number_of_recycled(1000000, 2000000)
#print number_of_recycled(100, 500)

for test_no in range(1, input()+1):
    N, M = map(int, raw_input().split())
    print "Case #%s: %s" % (test_no, number_of_recycled(N, M))
