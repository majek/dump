import string
import random
import sys

CHARS = list(set(string.printable) -
             set(string.whitespace) -
             set(r'"\\`\',!0O|/;l'))

DELIM=","

sys.stdout.write(" " + DELIM)
for b in string.ascii_lowercase:
    sys.stdout.write("%c%s" % (b, DELIM))

sys.stdout.write("\n")

for a in string.ascii_lowercase:
    sys.stdout.write("%c%s" % (a, DELIM))
    for b in string.ascii_lowercase:
        sys.stdout.write("%c%s" % (random.choice(CHARS), DELIM))
    sys.stdout.write("\n")
