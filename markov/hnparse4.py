'''
Usage:

$ python hnparse4.py cancer
$ python hnparse4.py steve
$ python hnparse4.py steve jobs
$ python hnparse4.py why
$ python hnparse4.py why your

'''
import cPickle as pickle
import bisect
import sys
import pprint

def retrieve(labels, values, bigram):
    '''
    >>> labels = [(0,0,0), (0,1,0), (0,1,1), (0,2,0)]
    >>> values = [      0,       1,       2,       3]
    >>> retrieve(labels, values, (0,1))
    [((0, 1, 0), 1), ((0, 1, 1), 2)]
    >>> retrieve(labels, values, (0,0))
    [((0, 0, 0), 0)]
    >>> retrieve(labels, values, (0,2))
    [((0, 2, 0), 3)]

    >>> labels = [('a','a','a'), ('a','b','a'), ('a','b','b'), ('a','c','a')]
    >>> values = [            0,             1,             2,             3]
    >>> retrieve(labels, values, ('a', 'b'))
    [(('a', 'b', 'a'), 1), (('a', 'b', 'b'), 2)]
    >>> retrieve(labels, values, ('a', 'a'))
    [(('a', 'a', 'a'), 0)]
    >>> retrieve(labels, values, ('a', 'c'))
    [(('a', 'c', 'a'), 3)]
    '''
    start = tuple(list(bigram) + [0])
    stop  = tuple(list(bigram) + ['zzzzzzzz'])

    a = bisect.bisect_left(labels, start)
    b = bisect.bisect_right(labels, stop)
    assert a <= b
    r = []
    for i in xrange(a,b):
        r.append((
                labels[i],
                values[i],
                ))
    return r


def run(words):
    print "[.] Loading pickle"
    with open('dump2.pickle', 'r') as f:
        trigrams_names  = pickle.load(f)
        trigrams_values = pickle.load(f)

    print "[.] loaded"

    pprint.pprint(retrieve(trigrams_names, trigrams_values, words))


if __name__ == '__main__':
    run(tuple(sys.argv[1:]))
