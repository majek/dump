#!/usr/bin/python

tofind=[ (5, 'ghi', '.js'),
         (2, 'pqr', '.js')]

dir = '.'
import os
import os.path

def getfiles(dir):
    for dirname, dirnames, filenames in os.walk(dir):
        for filename in filenames:
            fn = os.path.join(dirname, filename)
            if not os.path.isfile(fn):
                continue
            
            yield fn, fn[len(dir)+1:]

sums=[]
for lineno, infix, postfix in tofind:
    sum = 0
    for fullfn, fn in getfiles(dir):
        if fn.find(infix) > -1 and fn.endswith(postfix):
            #print "%s/%s %s" % (infix, postfix, fn)
            fd = open(fullfn)
            n = 0
            for line in fd:
                n = n + 1
                if n == lineno:
                    sum += int(line)
                    print "%s/%s/%s  %s %s" % (infix,postfix,lineno, fn, line)
                    break
            fd.close()
    if sum != 0:
        sums.append(sum)

print "%r" % sums
print "%i" % ( reduce(lambda a,b:a*b, sums) )

