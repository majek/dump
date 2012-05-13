"""
 primes.py  -- Oregon Curriculum Network (OCN)
 Feb  1, 2001  changed global var primes to _primes, added relative primes test
 Dec 17, 2000  appended probable prime generating methods, plus invmod
 Dec 16, 2000  revised to use pow(), removed methods not in text, added sieve()
 Dec 12, 2000  small improvements to erastosthenes()
 Dec 10, 2000  added Euler test
 Oct  3, 2000  modified fermat test
 Jun  5, 2000  rewrite of erastosthenes method
 May 19, 2000  added more documentation
 May 18, 2000  substituted Euclid's algorithm for gcd
 Apr  7, 2000  changed name of union function to 'cover' for consistency
 Apr  6, 2000  added union, intersection -- changed code of lcm, gcd
 Apr  5, 2000  added euler, base, expo functions (renamed old euler to phi)
 Mar 31, 2000  added sieve, renaming/modifying divtrial
 Mar 30, 2000  added LCM, GCD, euler, fermat
 Mar 28, 2000  added lcm
 Feb 18, 2000  improved efficiency of isprime(), made getfactors recursive
"""
import time, random, operator

_primes = [2]    # global list of primes


def iseven(n):
   """Return true if n is even."""
   return n%2==0

def isodd(n):
   """Return true if n is odd."""   
   return not iseven(n)
   
def get2max(maxnb):   
   """Return list of primes up to maxnb."""

   nbprimes = 0
   if maxnb < 2: return []
   
   # start with highest prime so far

   i = _primes[-1]
   # and add...

   i = i + 1 + isodd(i)*1 # next odd number        

   if i <= maxnb:  # if more prime checking needed...

      while i<=maxnb:
         if divtrial(i): _primes.append(i) # append to list if verdict true

         i=i+2     # next odd number

      nbprimes = len(_primes)
   else:
      for i in _primes: # return primes =< maxnb, even if more on file

         if i<=maxnb: nbprimes = nbprimes + 1
         else: break   # quit testing once maxnb exceeded

   
   return _primes[:nbprimes]
       
def get2nb(nbprimes):
   """Return list of primes with nbprimes members."""
   
   if nbprimes>len(_primes):
      # start with highest prime so far

      i = _primes[-1]
      # and add...

      i = i + 1 + isodd(i)*1
      
      while len(_primes)<nbprimes:
         if divtrial(i): _primes.append(i)
         i=i+2

   return _primes[:nbprimes]

def isprime(n):
   """
   Divide by primes until n proves composite or prime.

   Brute force algorithm, will wimp out for humongous n
   return 0 if n is divisible
   return 1 if n is prime"""
   
   nbprimes = 0
   rtnval = 1

   if n == 2: return 1
   if n < 2 or iseven(n): return 0
   
   maxnb = n ** 0.5 # 2nd root of n


   # if n < largest prime on file, check for n in list   

   if n <= _primes[-1]: rtnval = (n in _primes)

   # if primes up to root(n) on file, run divtrial (a prime test)

   elif maxnb <= _primes[-1]: rtnval = divtrial(n)
   
   else:
      rtnval = divtrial(n)   # check divisibility by primes so far


      if rtnval==1:       # now, if still tentatively prime...

         # start with highest prime so far

         i = _primes[-1]
         # and add...

         i = i + 1 + isodd(i)*1     # next odd number

         while i <= maxnb:
            if divtrial(i):         # test of primehood

                _primes.append(i)    # append to list if prime

                if not n%i:         # if n divisible by latest prime

                   rtnval = 0       # then we're done

                   break
            i=i+2                   # next odd number


   return rtnval

def iscomposite(n):
   """
   Return true if n is composite.
   Uses isprime"""
   return not isprime(n)

def divtrial(n):
   """
   Trial by division check whether a number is prime."""
   verdict = 1      # default is "yes, add to list"

   cutoff  = n**0.5 # 2nd root of n

   
   for i in _primes:
        if not n%i:      # if no remainder

           verdict = 0   # then we _don't_ want to add

           break
        if i >= cutoff:  # stop trying to divide by

           break         # lower primes when p**2 > n


   return verdict

def erastosthenes(n):
   """
   Suggestions from Ka-Ping Yee, John Posner and Tim Peters"""
   sieve = [0, 0, 1] + [1, 0] * (n/2) # [0 0 1 1 0 1 0...]

   prime = 3                          # initial odd prime

   while prime**2 <= n:
       for i in range(prime**2, n+1, prime*2): 
            sieve[i] = 0      # step through sieve by prime*2

       prime += 1 + sieve[prime+1:].index(1) # get next prime

   # filter includes corresponding integers where sieve = 1

   return filter(lambda i, sieve=sieve: sieve[i], range(n+1))

def sieve(n):
   """
   In-place sieving of odd numbers, adapted from code
   by Mike Fletcher
   """
   candidates = range(3, n+1, 2)  # start with odds

   for p in candidates:                
       if p:                   # skip zeros

	   if p*p>n: break     # done 

	   for q in xrange(p*p, n+1, 2*p):  # sieving

		candidates[(q-3)/2] = 0 
   return [2] + filter(None, candidates)  # [2] + remaining nonzeros


def base(n,b):
   """
   Accepts n in base 10, returns list corresponding to n base b."""
   output = []
   while n>=1:
      n,r = divmod(n,b) # returns quotient, remainder

      output.append(r)
   output.reverse()
   return output

def fermat(n,b=2):
   """Test for primality based on Fermat's Little Theorem.
   
   returns 0 (condition false) if n is composite, -1 if
   base is not relatively prime
   """
   if gcd(n,b)>1: return -1
   else:          return pow(b,n-1,n)==1

def jacobi(a,n):
  """Return jacobi number.
  
  source: http://www.utm.edu/research/primes/glossary/JacobiSymbol.html"""
  j = 1
  while not a == 0:
    while iseven(a):
      a = a/2
      if (n%8 == 3 or n%8 == 5): j = -j

    x=a; a=n; n=x  # exchange places

    if (a%4 == 3 and n%4 == 3): j = -j
    a = a%n  

  if n == 1: return j
  else: return 0

def euler(n,b=2):
   """Euler probable prime if (b**(n-1)/2)%n = jacobi(a,n).

   (stronger than simple fermat test)"""
   term = pow(b,(n-1)/2.0,n)
   jac  = jacobi(b,n)
   if jac == -1: return term == n-1
   else: return  term == jac
   
def getfactors(n):
   """Return list containing prime factors of a number."""
   if isprime(n) or n==1: return [n]
   else: 
       for i in _primes:
           if not n%i: # if goes evenly

               n = n/i
               return [i] + getfactors(n)

def gcd(a,b):
   """Return greatest common divisor using Euclid's Algorithm."""
   while b:      
	a, b = b, a % b
   return a

def lcm(a,b):
   """
   Return lowest common multiple."""
   return (a*b)/gcd(a,b)

def GCD(terms):
   "Return gcd of a list of numbers."
   return reduce(lambda a,b: gcd(a,b), terms)

def LCM(terms):
   "Return lcm of a list of numbers."   
   return reduce(lambda a,b: lcm(a,b), terms)

def phi(n):
   """Return number of integers < n relatively prime to n."""
   product = n
   used = []
   for i in getfactors(n):
      if i not in used:    # use only unique prime factors

         used.append(i)
         product = product * (1 - 1.0/i)
   return int(product)

def relprimes(n,b=1):
   """
   List the remainders after dividing n by each
   n-relative prime * some relative prime b
   """
   relprimes = []
   for i in range(1,n):
      if gcd(i,n)==1:  relprimes.append(i)
   print "          n-rp's: %s" % (relprimes)
   relprimes = map(operator.mul,[b]*len(relprimes),relprimes)
   newremainders = map(operator.mod,relprimes,[n]*len(relprimes))
   print "b * n-rp's mod n: %s" % newremainders
      
def testeuler(a,n):
   """Test Euler's Theorem"""
   a = long(a)
   if gcd(a,n)>1:
      print "(a,n) not relative primes"
   else:
      print "Result: %s" % pow(a,phi(n),n)

def goldbach(n):
   """Return pair of primes such that p1 + p2 = n."""
   rtnval = []

   _primes = get2max(n)
   
   if isodd(n) and n >= 5:
      rtnval = [3] # 3 is a term

      n = n-3      # 3 + goldbach(lower even)

      if n==2: rtnval.append(2) # catch 5

   else:
      if n<=3: rtnval = [0,0] # quit if n too small       

         
   for i in range(len(_primes)):
      # quit if we've found the answer

      if len(rtnval) >= 2: break 
      # work back from highest prime < n

      testprime = _primes[-(i+1)]
      for j in _primes:
         # j works from start of list

         if j + testprime == n:
              rtnval.append(j)
              rtnval.append(testprime) # answer!

              break
         if j + testprime > n:
              break  # ready for next testprime


   return rtnval

"""
The functions below have to do with encryption, and RSA in
particular, which uses large probable _primes.  More discussion
at the Oregon Curriculum Network website is at
http://www.inetarena.com/~pdx4d/ocn/clubhouse.html
"""

def bighex(n):
    hexdigits = list('0123456789ABCDEF')
    hexstring = random.choice(hexdigits[1:])
    for i in range(n):
        hexstring += random.choice(hexdigits)
    return eval('0x'+hexstring+'L')

def bigdec(n):
    decdigits = list('0123456789')
    decstring = random.choice(decdigits[1:])
    for i in range(n):
        decstring += random.choice(decdigits)
    return long(decstring)

def bigppr(digits=100):
    """
    Randomly generate a probable prime with a given
    number of decimal digits
    """
    start = time.clock()
    print "Working..."
    candidate = bigdec(digits) # or use bighex

    if candidate&1==0:
        candidate += 1
    prob = 0
    while 1:
        prob=pptest(candidate)
        if prob>0: break
        else: candidate += 2
    print "Percent chance of being prime: %r" % (prob*100)
    print "Elapsed time: %s seconds" % (time.clock()-start)        
    return candidate
        
def pptest(n):
    """
    Simple implementation of Miller-Rabin test for
    determining probable primehood.
    """
    bases  = [random.randrange(2,50000) for x in range(90)]

    # if any of the primes is a factor, we're done

    if n<=1: return 0
    
    for b in bases:
        if n%b==0: return 0
        
    tests,s  = 0L,0
    m        = n-1

    # turning (n-1) into (2**s) * m

    while not m&1:  # while m is even

        m >>= 1
        s += 1
    for b in bases:
        tests += 1
        isprob = algP(m,s,b,n)
        if not isprob: break
            
    if isprob: return (1-(1./(4**tests)))
    else:      return 0

def algP(m,s,b,n):
    """
    based on Algorithm P in Donald Knuth's 'Art of
    Computer Programming' v.2 pg. 395 
    """
    result = 0
    y = pow(b,m,n)    
    for j in range(s):
       if (y==1 and j==0) or (y==n-1):
          result = 1
          break
       y = pow(y,2,n)       
    return result

def invmod(a,b):
    """
    Return modular inverse using a version Euclid's Algorithm
    Code by Andrew Kuchling in Python Journal:
    http://www.pythonjournal.com/volume1/issue1/art-algorithms/
    -- in turn also based on Knuth, vol 2.
    """
    a1, a2, a3 = 1L,0L,a
    b1, b2, b3 = 0L,1L,b
    while b3 != 0:
        # The following division will drop decimals.

        q = a3 / b3  
        t = a1 - b1*q, a2 - b2*q, a3 - b3*q
        a1, a2, a3 = b1, b2, b3
        b1, b2, b3 = t
    while a2<0: a2 = a2 + a
    return a2


# code highlighted using py2html.py version 0.8


