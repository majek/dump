import webapp2
from google.appengine.api import memcache


class ChannelHandler(webapp2.RequestHandler):
    def get(self, channel_name):
        self.response.headers['Content-Type'] = 'appplication/binary'
        self.response.write('%s\n' % (memcache.get(channel_name),))

    def post(self, channel_name):
        memcache.set(channel_name, self.request.body)
        self.response.out.write('ok\n')


application = webapp2.WSGIApplication([
    ('/(\w+)/', ChannelHandler),
], debug=True)
