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


def run():
    print "[.] Loading pickle"
    with open('dump2.pickle', 'r') as f:
        list_of_trigrams = pickle.load(f)

    trigrams_names  = [k for k, v in list_of_trigrams]
    trigrams_values = [v for k, v in list_of_trigrams]
    list_of_trigrams = None

    print "[.] loaded"

    pprint.pprint(retrieve(trigrams_names, trigrams_values, ('steve', 'jobs')))



if __name__ == '__main__':
    if False:
        import doctest
        doctest.testmod()
    else:
        run()
