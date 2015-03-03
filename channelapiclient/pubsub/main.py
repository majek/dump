import webapp2
from google.appengine.api import channel
import json


class ChannelHandler(webapp2.RequestHandler):
    def get(self, channel_name):
        self.response.headers['Content-Type'] = 'text/plain'
        #self.response.write('Hello, World! %r\n' % (channel_name,))

        token = channel.create_channel(channel_name)
        self.response.write('%s\n' % (token,))

    def post(self, channel_name):
        channel.send_message(channel_name,)

    def post(self, channel_name):
        msg = self.request.body
        if msg is None:
            self.response.error(400)
            self.response.out.write('payload data parameter missing')
        else:
            msg = json.dumps(msg)
            try:
                channel.send_message(channel_name, msg)
            except Exception, e:
                self.response.out.write('exception %r\n' % (e,))
            self.response.out.write('written %r\n' % (msg,))


application = webapp2.WSGIApplication([
    ('/(\w+)/', ChannelHandler),
], debug=True)
