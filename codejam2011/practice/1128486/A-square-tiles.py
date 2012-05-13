

def solve(data):
    for row in xrange(len(data)):
        for col in xrange(len(data[0])):
            if data[row][col] == '#':
                if (data[row][col+1] == '#' and
                    data[row+1][col] == '#' and
                    data[row+1][col+1] == '#'):

                    data[row][col] = '/'
                    data[row][col+1] = '\\'
                    data[row+1][col] = '\\'
                    data[row+1][col+1] = '/'
                else:
                    return False
    return data

for t in xrange(1, input()+1):
    R, C = map(int, raw_input().split())
    lines = [list(raw_input()) for r in xrange(R)]

    try:
        s = solve(lines)
    except IndexError:
        s = False

    print 'Case #%i:' % (t, )
    if not s:
        print 'Impossible'
    else:
        for line in s:
            print ''.join(line)


