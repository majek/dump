import time
from google.appengine.ext import webapp
from google.appengine.api import datastore

ACCUMULATE = False

def monkey_patch(namespace, name, wrapper):
    nname = '_orig_' + name
    if not getattr(namespace, nname, None):
        setattr(namespace, nname, getattr(namespace, name))
        def w(*args, **kwargs):
            return wrapper(getattr(namespace, nname), *args, **kwargs)
        setattr(namespace, name, w)

gqls_debugging = []
'''
from google.appengine.ext import gql

def wr(orig_func, self, *args, **kwargs):
    t0 = time.time()
    r  = orig_func(self, *args, **kwargs)
    t1 = time.time()
    gqls_debugging.append( "%5.0fms %r %r %r" % ((t1-t0)*1000, self._query, args, kwargs) )
    return r
monkey_patch(gql.GQL, 'Run', wr)
    
def wr(orig_func, self, q, *args, **kwargs):
    self._query = q
    return orig_func(self, q, *args, **kwargs)
monkey_patch(gql.GQL, '__init__', wr)
'''

def wr(orig_func, self, *args, **kwargs):
    t0 = time.time()
    r =  orig_func(self, *args, **kwargs)
    t1 = time.time()
    gqls_debugging.append( "%5.0fms GQL %s\n            args: %r %r " % ((t1-t0)*1000, ("%s" % self._ToPb()).replace("\n", "\n            ").strip(), args, kwargs) )
    return r
monkey_patch(datastore.Query, '_Run', wr)

def monkey_patch_simple_timer(namespace, name, comment=None):
    if not comment:
        comment = name.upper()
    def wr(orig_func, *args, **kwargs):
        t0 = time.time()
        r =  orig_func(*args, **kwargs)
        t1 = time.time()
        gqls_debugging.append( "%5.0fms %s %r %r" % ((t1-t0)*1000, comment, args, kwargs) )
        return r
    monkey_patch(datastore, name, wr)

monkey_patch_simple_timer(datastore, 'Put')
monkey_patch_simple_timer(datastore, 'Get')
monkey_patch_simple_timer(datastore, 'Delete')


webapp._RequestHandler = webapp.RequestHandler

class DebugMiddleware(webapp.RequestHandler):
    def __init__(self, *args, **kwargs):
        self._get = self.get
        self.get = self._new_get
        webapp._RequestHandler.__init__(self, *args, **kwargs)
    
    def _new_get(self, *args, **kwargs):
        global gqls_debugging
        if not ACCUMULATE:
            gqls_debugging = []
        t0 = time.time()
        c0 = time.clock()
        r = self._get(*args, **kwargs)
        c1 = time.clock()
        t1 = time.time()
        self.response.out.write("""
<pre>
**** Request took: %5.0fms/%.0fms (real time/cpu time)
**** GQLs, datastore accessed %i times.
%s
</pre>""" % ( (t1-t0) * 1000, (c1-c0) * 1000 , len(gqls_debugging), '\n'.join(gqls_debugging) ) )
        gqls_debugging = []
        return r



