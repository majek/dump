#!/usr/bin/python

import sys
FILE = 'primes1.txt-o'
W=[3,51,237,1185]
#W=[3,6]
fd=open(FILE)
PRIMES = map(int, fd)
fd.close()

PRIMED = dict( ((p, True) for p in PRIMES ) )



def test(n, i, k):
    s=0
    for i in range(i):
        s += PRIMES[i] - (PRIMES[i-n] if i-n>=0 else 0)
        if s == k:
            l = PRIMES[max(i-n+1,0):i+1]
            if len(l) != n:
                continue
            print "  [got] n=%i i=%i sum %i=%i %.80r" % (n, i, s,sum(l), l)
            break

p=6226663
for n in W:
    test(n, PRIMES.index(p), p)
sys.exit(0)

def ttt(n):
    s=0
    for i in range(len(PRIMES)):
        s += PRIMES[i] - (PRIMES[i-n] if i-n>=0 else 0)
        #l = PRIMES[max(i-n+1,0):i+1]
        #print s, sum(l),l
        if s in PRIMED:
            l = PRIMES[max(i-n+1,0):i+1]
            if len(l) != n:
                continue
            #print "#%i  [got] i=%i sum %i=%i %.80r" % (n, i, s,sum(l), l)
            yield s


xxx = []
for n in W:
    xxx.append( set(ttt(n)) )

a = reduce(lambda a,b:a.intersection(b), xxx)
print "%r" % a
