import b

def init_attributes(name, bases, cls):
    for k in [b.a,b.b,b.c]:
        cls[k.__name__] = k

    return type(name, bases, cls)


class T(object):
    __metaclass__ = init_attributes


t = T()
t.name = 't'

print dir(t)
print t.a
t.a()
t.b()
t.c()


x = T()
x.name = 'x'

t.a(b=1,a=2,c=3)
x.a()

aa = t.a
bb = x.a

print aa()
print bb()



