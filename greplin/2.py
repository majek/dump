import array
import math
import bisect

def primes(limit):
    p = array.array('L',  [2])
    for n in xrange(3, limit):
        n_root = int(math.sqrt(n))
        i = 0
        while p[i] <= n_root:
            if n % p[i] == 0:
                break
            i+=1
        else:
            p.append( n )
    return p

def fib(limit):
    f = array.array('L', [0, 1])
    c = 0
    while c < limit:
        c = f[-2] + f[-1]
        f.append( c )
    return f

def factorize(n, p):
    f = []
    i = 0
    n_root = int(math.sqrt(n))
    while p[i] <= n_root:
        if n % p[i] == 0:
            f.append( p[i] )
        i+=1
    return f

x = 227000
print "primes"
p = primes(x*3)
print "fib"
f = fib(x*3)

def merge(a, b):
    i = 0
    j = 0
    while i < len(a) and j < len(b):
        if a[i] < b[j]:
            i+=1
        elif a[i] > b[j]:
            j+=1
        else:
            return a[i]
    return "not found"


pi = bisect.bisect_right(p, x)
fi = bisect.bisect_right(f, x)
print "merge"
r =  merge(p[pi:], f[fi:])

f = factorize(r+1, p)
print f
print sum(f)
