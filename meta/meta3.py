def init_attributes(name, bases, dict):
    if 'attributes' in dict:
        for attr in dict['attributes']:
            dict[attr] = None

    return type(name, bases, dict)

class Initialised(object):
    __metaclass__ = init_attributes
    attributes = ['foo', 'bar', 'baz']

a = Initialised()
print 'foo =>', a
