from pprint import pprint as pp

n = [3, 4, 9, 14, 15, 19, 28, 37, 47, 50, 54, 56, 59, 61, 70, 73, 78, 81, 92, 95, 97, 99]


def allsubs(l):
    if len(l) == 0:
        return [ [] ]
    else:
        s = allsubs( l[1:] )
        return s + [[l[0]] + x for x in s]

print len(
    filter(
        lambda l: sum(l[:-1]) == l[-1] if l else False,
        allsubs( n )))
