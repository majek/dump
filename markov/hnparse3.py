import re
import itertools
import collections
import cPickle as pickle

split_re = re.compile('[!\t \-\(\)?:/\r\n]+')

FILENAME='hnfull.txt'

def load_file(filename):
    with open(filename, 'rb') as fd:
        for line in fd:
            # last element is always empty due to \n
            yield [intern(w) for w in split_re.split(line.lower().replace('&amp;', '&'))][:-1]


print "[.] Loading trigrams"
trigrams = collections.defaultdict(lambda:0)

for line in load_file(FILENAME):
    for ngram in itertools.izip(line[:-2],  line[1:-1], line[2:]):
        trigrams[ngram] += 1


print "[.] Moving from dict to list"
list_of_trigrams = list(trigrams.iteritems())
trigrams = None
list_of_trigrams.sort()

trigrams_names  = [k for k, v in list_of_trigrams]
trigrams_values = [v for k, v in list_of_trigrams]
list_of_trigrams = None

print "[.] Saving"
with open('dump2.pickle', 'wb') as f:
    pickle.dump(trigrams_names, f)
    pickle.dump(trigrams_values, f)

print "[!] All done"
