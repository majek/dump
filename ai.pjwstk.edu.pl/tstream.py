#!/usr/bin/env python

import urllib2
import sys
import simplejson
import pprint
import datetime
import select
import time

USER='user'
PASS='pass'
URI="http://stream.twitter.com/spritzer.json"
OPENSTREAMS=4

PUBLIC_URI='http://twitter.com/statuses/public_timeline.json'

password_mgr = urllib2.HTTPPasswordMgrWithDefaultRealm()
password_mgr.add_password(None, URI, USER, PASS)
handler = urllib2.HTTPBasicAuthHandler(password_mgr)
opener = urllib2.build_opener(handler)
urllib2.install_opener(opener)

tweet_ids = set()
new_tweet_ids = set() # only for statistics
#['__doc__', '__init__', '__iter__', '__module__', '__repr__', 'close', 'code', 'fileno', 'fp', 'geturl', 'headers', 'info', 'msg', 'next', 'read', 'readline', 'readlines', 'url']

fd_stream = {}
for i in range(OPENSTREAMS):
    stream = urllib2.urlopen(URI)
    fd = stream.fp._sock.fp._sock.fileno()
    stream.repeated_tweets = 0
    stream.new_tweets = 0
    stream.buf = ''
    fd_stream[fd] = stream
    print >> sys.stderr, 'Channel opened'


public_timestamp = 0
stat_timestamp = time.time()
while True:
    now = time.time()
    if now - public_timestamp > 29.0: # every 29 seconds ping the main site, we can block but for not too long
        stream = urllib2.urlopen(PUBLIC_URI)
        for tweet in simplejson.loads(stream.read()):
            tweet_id = tweet['id']
            if tweet_id not in tweet_ids:
                tweet_ids.add( tweet_id )
                if tweet_id > min(new_tweet_ids or [0]):
                    new_tweet_ids.add( tweet_id )
                print simplejson.dumps(tweet)
        stream.close()
        public_timestamp = now
    if now - stat_timestamp > 30.0:
        tmin = min(new_tweet_ids)
        tmax = max(new_tweet_ids)
        tdiff = tmax-tmin
        print >> sys.stderr, 'Min: #%i  Max: #%i  Diff:%i Tweets:%i Average_new:%.3f Average_receive:%.3f tweets/sec We_get_every:%ith tweet' \
                % (tmin, tmax, tdiff, len(new_tweet_ids), tdiff/(now - stat_timestamp), len(new_tweet_ids)/(now - stat_timestamp),  float(tdiff)/len(new_tweet_ids))
        new_tweet_ids = set()
        stat_timestamp = now

    fds, _, _ = select.select(fd_stream.keys(), [], [], 1.0)
    for fd in fds:
        stream = fd_stream[fd]
        tweet_json = stream.readline() # shouldn't block
        tweet = simplejson.loads( tweet_json )
        stream.buf = ''
        tweet_id = tweet['id']
        if tweet_id in tweet_ids:
            stream.repeated_tweets += 1
        else:
            stream.new_tweets += 1
            tweet_ids.add( tweet_id )
            new_tweet_ids.add( tweet_id )
            print tweet_json
        if stream.repeated_tweets > 10 and stream.repeated_tweets/float(stream.new_tweets or 0.0001) > 10.0:
            print >> sys.stderr, 'Channel has only duplicates - closing with repeated %i new %i, %i channels left' \
                        % (stream.repeated_tweets, stream.new_tweets, len(fd_stream)-1,)
            del fd_stream[fd]
            stream.close()


sys.exit()
