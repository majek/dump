import wsgiref.handlers
import cgi

from google.appengine.ext import webapp
#from google.appengine.api import users

from google.appengine.api import urlfetch
import random
import re, time, datetime, yaml
import logging

TIMEURL="http://www.time.gov/timezone.cgi?UTC/s/0"
TIMERE=re.compile(">([0-2]?[0-9]):([0-5]?[0-9]):([0-6]?[0-9])<")




def get_time():
    t0 = time.time()
    try:
        now = datetime.datetime.utcnow()
        resp = urlfetch.fetch("%s&r=%f" %  (TIMEURL, random.random()), payload=None, method=urlfetch.GET, headers={}, allow_truncated=False)
    except urlfetch.DownloadError:
        return None, None

    t1 = time.time()
    
    try:
        h, m, s = map(int, TIMERE.search(resp.content).groups())
    except AttributeError:
        return None, None
    
    utc = now.replace(hour=h, minute=m, second=s)
    a = (now-utc)
    b = (utc-now)
    if a.seconds < b.seconds:
        tdiff = a
    else:
        tdiff = b
    return (tdiff.seconds * 1000000.0 + tdiff.microseconds), (t1-t0) * 1000000
    
    
def get_server_id():
    try:
        fd   = open('/base/python_dist/search.config')
        data = fd.read()
        fd.close()
    except IOError:
        return 'unknown'
    
    return '%s' % data.__hash__()
    
    
class MainPage(webapp.RequestHandler):
    def get(self):
        # receive 10 reads
        remote_timers = [get_time() for i in range(10)]
        
        # filter out broken reads
        remote_timers = filter(lambda (a,b): True if a is not None else False, remote_timers)
        
        # mean square error
        mse = sum( [ (r*r) for r, t in remote_timers  ] ) / float(len(remote_timers))
        
        # mean square error
        wget_mse = sum( [ (t*t) for r, t in remote_timers  ] ) / float(len(remote_timers))

        o={
            'utc_mean_square_error_seconds': mse / (1000000.0**2),
            'wgettime_mean_square_error_seconds': wget_mse / (1000000.0**2),
            'utc_diffs_seconds': map( lambda a:a/1000000.0, [ r for r, t in remote_timers  ]),
            'wgettimes_seconds': map( lambda a:a/1000000.0, [ t for r, t in remote_timers  ]),
            'server_id':get_server_id(),
        }
        
        self.response.headers["Content-Type"] = "text/plain"
        self.response.out.write(
            yaml.dump(o, default_flow_style=False)
            )
    


application = webapp.WSGIApplication(
                                       [('/', MainPage),
                                        
                                        ],
                                       debug=True)

def main():
  wsgiref.handlers.CGIHandler().run(application)

if __name__ == "__main__":
  main()
