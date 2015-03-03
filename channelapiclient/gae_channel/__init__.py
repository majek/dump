"""
Author: Manu, thembrown@gmail.com
License: LGPL, V3

Client API for Google Appengine Channel API.
I reverse-engineered the protocol. It seems to work, though it's not well
tested yet. Be aware that google could change the underlying protocol without any
notice. The best thing you can expect in such a case is to get ChannelError
exception. But it could also just stop workin in some other way.
You've been warned.

It does NOT work with dev_appsserver.py. You need to have your app deployed.

== Usage ==:

channel = Client(token='your channel token')
for msg in chan.messages():
    print msg

Also have a look at /demo

"""
import urllib2, urllib
import json, re, random, string, logging, time
import time, itertools, socket

import talkparser

#TODO: handle urllib2.HTTPError: HTTP Error 401: Token timed out. line 259, _connect


class ChannelError(StandardError):
    """ Raised if something unexpected occurs.
    If google changes the protocol - this is the exception you'll get
    """
    pass


class _TalkMessageCorrupted(StandardError):
    pass


class TalkMessageReader(object):
    """ Helper class to read/parse channel messages"""

    def __init__(self, connection):
        self.conn = connection

    def messages(self):
        """ iterator that yields messages """
        try:
            for submission in self.submissions():
                talk_messages = talkparser.parse(submission)
                # each submission can contain multiple talk messages
                # we yield them seperately
                for talk_msg in talk_messages:
                    yield talk_msg
        except talkparser.ParseError, e:
            # map parse errors for easier handling   
            raise _TalkMessageCorrupted(
                'Talk message could not be parsed: %s' % str(e))

    def submissions(self):
        """
         Generator for all submissions on the connection
         raises TalkMessageCorrupted if a submission is incomplete
        """
        while True:
            # a submission has a preceeding line containing its length
            num_chars = self.readline()
            if not num_chars:
                # end of stream
                return
            try:
                num_chars = int(num_chars)
            except ValueError:
                raise _TalkMessageCorrupted(
                    "length of submission expected, got %s" % num_chars)
            s = self.conn.read(num_chars)
            if num_chars != len(s):
                # submission corrupted
                raise _TalkMessageCorrupted(
                    'Submission: %d chars expected, got %d' % (
                        num_chars, len(s)))
            yield s

    def readline(self):
        """ unbuffered readline"""
        line = ''
        while True:
            # unbuffered read
            c = self.conn.read(1)
            if not c: return None
            if c == '\n':
                return line
            else:
                line += c


class Client(object):
    """ GAE Channel client
    """

    def __init__(self, token, logger=None):
        """ token has to be the token as returned by
        channel.create_channel on GEA
        """
        self.base_url = "https://talkgadget.google.com/talkgadget"
        self.token = token
        self._request_id = 1
        self.sid = None
        self.gsessionid = None
        self.clid = None
        if not logger:
            logger = logging.getLogger('Channel')
            logger.addHandler(logging.StreamHandler())
            #logger.setLevel(logging.DEBUG)
        self.logger = logger
        self._trial = 1
        # this is what's gonna be the AID parameter for requests
        self._msg_id = 1
        # reconnect if long polls time out
        self.auto_reconnect = True
        self.max_reconnects_trials = 5
        self.reconnect_trial_ = 0
        self.backoff_ = 1


    def messages(self):
        """
        Generator that yields channel messages as sent with GAE channel API
        It takes care of doing multiple long poll requests
        If network erros occur, 
        """
        self.initialize()
        self._fetch_sid()
        self._connect()
        while True:
            try:
                for message in self.long_poll_messages():
                    yield message
                self.backoff_ = 1
                reconnect_trial_ = 0
            except _TalkMessageCorrupted, e:
                self.logger.warn('TalkMessageCorrupted: %s' % str(e))
                self.logger.warn('reconnecting')
            # how to handle this?
            #except urllib2.HTTPError, e:
            #    if e.code == 400 and e.msg == 'Unknown SID':
            #        self.logger.warn('SID unknown')
            #        self._fetch_sid()
            #    else:
            #       self.logger.error('unknown error "%s"' % e.msg)
            #       raise
            except socket.error, e:
                # timeout (connection loss)
                self.logger.warn('Long poll timed out: %s' % e)
                if self.auto_reconnect and\
                   self.reconnect_trial_ < self.max_reconnects_trials:
                    self.logger.warn(
                        'reconnecting in %d seconds' % self.backoff_)
                    self.reconnect_trial_ += 1
                    time.sleep(self.backoff_)
                    if self.backoff_ < 32:
                        self.backoff_ *= 2
                else:
                    raise
                    #self._trial += 1

    def long_poll_messages(self):
        """
        generator for channel messages
        initiates a long GET pool and
        yields messages as sent with GAE channel api
        """
        # send connect signal to talk servers
        # needed to receive messages
        url = self._get_bind_url(CI=0,
                                 AID=self._msg_id,
                                 TYPE='xmlhttp',
                                 RID='rpc')
        self.logger.debug("Staring long poll from URL: %s" % url)
        # open a long-poll connection
        connection = urllib2.urlopen(url, timeout=30)
        reader = TalkMessageReader(connection)
        for talk_msg in reader.messages():
            # we are only interessted in channel messages
            # bet we keep track of message ids, which can also occur in other
            # talk messages
            # try
            msg_id, talk_body = talk_msg
            self._msg_id = msg_id
            self.logger.debug("talk msg %s: %s" % (msg_id, talk_body))
            chan_body = self._parse_channel_message(talk_body)
            if chan_body:
                self.logger.debug("channel msg: %s" % repr(chan_body))
                yield chan_body
                #if isinstance(talk_msg, list) and :

    def _parse_channel_message(self, talk_body):
        """ unpacks talk message bodys
        returns channel message body if it contains a channel message
        """
        try:
            (c, (clid, (ae, chan_body))) = talk_body
            if ae:
                self.sid = ae
            if clid == self.clid and ae == 'ae' and c == 'c':
                return chan_body
            else:
                return None
        except ValueError:
            return None


    def _get_bind_url(self, **kwargs):
        params = {'token': self.token,
                  'gsessionid': self.gsessionid,
                  'clid': self.clid,
                  'prop': 'data',
                  'zx': random_string(12),
                  't': self._trial}
        if self.sid:
            params['SID'] = self.sid
        params.update(kwargs)
        url = self.base_url + "/dch/bind?VER=8&" + urllib.urlencode(params)
        return url

    def _get_init_url(self):
        """ build url to fetch gsession_id and clid from"""
        # cn seems to just be a random 10 digit code


        xpc = {"cn": random_string(10),
               "tp": 'null',
               "ppu": "http://hangoverapi.appspot.com/_ah/channel/xpc_blank",
               "lpu": "http://talkgadget.google.com/talkgadget/xpc_blank"}
        params = {'token': self.token,
                  'xpc': json.dumps(xpc)}
        url = self.base_url + '/d?' + urllib.urlencode(params)
        return url

    def _fetch_sid(self):
        """
        fetch a sid which is probably a session id
        the session id is contained in a talk message that looks like this:
        [0,["c","SESSION_ID",,8]]
        """

        def parse_sid(talk_msg):
            """ look for sid in talk message,
             returns sid or None if no sid is found
            """
            try:
                msg_id, body = talk_msg
                if body[0] == 'c' and len(body) > 1:
                    return body[1]
            except ValueError:
                pass
            return None

        self.logger.debug("Fetching sid")
        url = self._get_bind_url(RID=self._request_id, CVER=1)
        self._request_id += 1
        # I don't know what count is good for yet
        post_data = {'count': 0}
        conn = urllib2.urlopen(url, data=urllib.urlencode(post_data))
        reader = TalkMessageReader(conn)

        sid_gen = (parse_sid(talk_msg) for talk_msg in reader.messages())
        sids = itertools.ifilter(None, sid_gen)
        try:
            self.sid = sids.next()
        except StopIteration:
            raise ChannelError("fetch_sid: no sid found")

        self.logger.debug("sid: %s" % self.sid)


    def _connect(self):
        """ _connect is needed in order to receive channel messages"""
        if not self.is_initialized():
            self.initialize()
        url = self._get_bind_url(RID=self._request_id, AID=self._msg_id,
                                 CVER=1)
        data = {'count': '1',
                'ofs': '0',
                'req0_m': '["connect-add-client"]',
                'req0_c': self.clid,
                'req0__sc': 'c'}
        formdata = urllib.urlencode(data)
        self.logger.debug("connecting, URL: %s\nData: %s" % (url, formdata))
        # we dont care about the response
        urllib2.urlopen(url, data=formdata)
        self._request_id += 1

    def is_initialized(self):
        return self.clid is not None


    def initialize(self):
        """ fetches gsession_id and clid
        these are needed in order to perform any calls to /bind
        """
        self.logger.debug("initializing")
        req = urllib2.urlopen(self._get_init_url())
        self._parse_init_response(req.read())


    def _parse_init_response(self, html):
        """
        relevant part of /d response looks like this:
        we extract clid and gsid from it
        var a = new chat.WcsDataClient("http://talkgadget.google.com/talkgadget/",
                "",
        "3496699hh4591E19", # clid
        "-p__ZFNDJmm-VozEjdST0A", #gsid
        "unknown string",
        "WCX",
        token
        );"""
        chat_call = re.search('new chat\.WcsDataClient\(([^\)]+)', html,
                              re.MULTILINE)
        if not chat_call:
            raise ChannelError("Talk Server returned unexpected response")
        params = chat_call.group(1)
        self.logger.debug("chat client parameters: %s" % params)
        chat_params = [p.strip('" \n') for p in params.split(',')]
        self.clid = chat_params[2]
        self.gsessionid = chat_params[3]
        self.logger.debug("clid: %s" % self.clid)
        self.logger.debug("gsessionid: %s" % self.gsessionid)


def random_string(len):
    return ''.join(random.choice(string.ascii_letters) for i in range(len))
