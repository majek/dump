def test_metaclass(name, bases, dict):
    print 'The Class Name is', name
    print 'The Class Bases are', bases
    print 'The dict has', len(dict), 'elems, the keys are', dict.keys()

    return "yellow"

class TestName(object, None, int, 1):
    __metaclass__ = test_metaclass
    foo = 1
    def baz(self, arr):
        pass

print 'TestName = ', repr(TestName)
