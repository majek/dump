#!/usr/bin/env python

class ATree(object):
    def __init__(self):
        self._a = {}

    def insert(self, key):
        n = 0
        print '__',
        while n in self._a:
            if self._a[n] == key:
                raise ValueError
            print ('L' if key < self._a[n] else 'R'),
            n = n*2 + (1 if key < self._a[n] else 2)
        self._a[n] = key
        print

    def delete(self, key):
        n = 0
        while n in self._a:
            if self._a[n] == key:
                break
            n = n*2 + (1 if key < self._a[n] else 2)
        else:
            raise ValueError
        self._delete(n, key)

    def _delete(self, n, prev):
        left = n*2 + 2
        right = n*2 + 2
        if left in self._a and right not in self._a:
            self._a[n] = self._a[left]
            self._delete(left, self._a[left])
        elif left not in self._a and right in self._a:
            self._a[n] = self._a[right]
            self._delete(right, self._a[right])
        elif left in self._a and right in self._a:
            self._a[n] = self._a[right]
            self._delete(right, self._a[right])
        else:
            del self._a[n]

    # def find(self, key):
    #     raise NotImplementedError

    def p(self):
        l = 1
        p = []
        ns = [0]
        empty = False
        while not empty:
            empty = True
            new_ns = []
            p += ('l=%2i' % l,)
            b = 0.0
            for n in ns:
                b += 160.0/(l+1)
                if n in self._a:
                    p += ("%*s" % (int(b), ' %s' % self._a.get(n, '')),)
                    b = 0
                    empty = False
                new_ns += (n*2 + 1, n*2 + 2)
            p += ("\n",)
            ns = new_ns
            l *= 2
        print ''.join(p)

    def __str__(self):
        return "%r" % (self._a,)



def main():
    t = ATree()
    t.insert(5)
    t.insert(3)
    t.insert(4)
    t.insert(1)
    t.insert(7)
    t.insert(9)
    t.insert(13)
    t.insert(12)
    t.insert(15)
    t.insert(115)
    t.insert(11)
    t.insert(6)
    t.insert(8)
    t.insert(7.5)
    t.p()
    print t

    t.delete(5)
    t.p()
    print t


if __name__ == '__main__':
    main()
