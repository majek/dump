#!/usr/bin/python

WIDTH=34
HEIGHT=58

#WIDTH=4
#HEIGHT=6


'''
unique = 0
def r(x,y):
    global unique
    if x==WIDTH and y==HEIGHT:
        unique += 1
        return
    if x < WIDTH: # prawo
        r(x+1, y)
    if y < HEIGHT: # dol
        r(x, y+1)
    return
    
r(0,0)
print "%r" % unique
'''

arr = [ [None]*HEIGHT for a in range(WIDTH) ]
arr[WIDTH-1][HEIGHT-1] = 1

def z(x,y):
    if y < HEIGHT-1:
        a = arr[x][y+1]
    else:
        a = 0
    
    if x < WIDTH-1:
        b = arr[x+1][y]
    else:
        b = 0
    
    if a != None and b != None:
        arr[x][y] = a + b
    else:
        raise ValueError( "%ix%i ->%r or %r"  % (x,y,a,b))

for x in range(WIDTH-1,-1,-1):
    for y in range(HEIGHT-1,-1,-1):
        if x == WIDTH-1 and y == HEIGHT-1:
            continue
        z(x,y)


#for y in range(HEIGHT):
#    print "%r" % list( arr[x][y] for x in range(WIDTH) )

print "%i" % (arr[0][0])