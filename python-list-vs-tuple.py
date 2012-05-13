import time

def do_tuple(no):
    for i in xrange(no):
        xx = (1,2,3,4,5,6,7,8,9)

def do_list(no):
    for i in xrange(no):
        xx = [1,2,3,4,5,6,7,8,9]


def do_str_add(no):
    for i in xrange(no):
        xx = 'abcdef' + 'ghijlk'

def do_str_join(no):
    for i in xrange(no):
        xx = ''.join(('abcdef', 'ghijlk'))


def do_str_double_join(no):
    for i in xrange(no):
        a = 'abcdef'
        b = ''.join(('ghijlk', 'aaaaaaa'))
        xx = ''.join( (a, b) )

def do_str_join_extend(no):
    for i in xrange(no):
        a = ['abcdef']
        a.extend( ('ghijlk', 'aaaaaaa') )
        xx = ''.join( a )



import cStringIO as StringIO

def do_plus(no):
    a = "a"*4096
    for i in xrange(no):
        b = ""
        for j in range(40):
            b += a

def do_join(no):
    a = "a"*4096
    for i in xrange(no):
        b = []
        for j in range(40):
            b.append( a )
        ''.join(b)

import sys
sys.path.append('puka')
from puka import simplebuffer

def do_simplebuffer(no):
    a = "a"*4096
    for i in xrange(no):
        b = simplebuffer.SimpleBuffer()
        for j in range(40):
            b.write( a )
        b.read()
        b.flush()

#tests = [do_tuple, do_list,
#tests = [do_str_add, do_str_join]
#tests = [do_str_join_extend, do_str_double_join]
tests = [do_simplebuffer, do_plus, do_join]
for test in tests:
    print '%r' % test
    a=time.time()
    test(10000)
    b=time.time()
    print '%6.0fms' % ((b-a)*1000.0,)

