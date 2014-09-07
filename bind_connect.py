import socket
import random
import time
import errno


a = []
try:
    for i in range(1000):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('192.168.97.139', 0))
        a.append(s)
except Exception, e:
    print e

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("www.google.com", 80))


'''
addr = ('192.168.97.139', 1091)

print "port: %s" % (addr,)

if True:
    x = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    x.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    x.bind(addr)
    x.connect(("idea.popcount.org", 80))


for i in range(5):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(addr)
    try:
        s.connect(("idea.popcount.org", 80))
        break
    except socket.error, e:
        if e.errno != errno.EADDRNOTAVAIL:
            raise
else:
    raise Exception("Unable to allocate connection")

print "waiting"
time.sleep(130)
'''


'''
HOSTS = ["idea.popcount.org", "www.wp.pl", "idea.popcount.org"]


addr = ('192.168.97.139', 1099)  #random.randint(5000,6000)

print "port: %s" % (addr,)
sds = []
for host in HOSTS:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    #s.bind(('0.0.0.0', port))
    s.bind(addr)
    #s.setblocking(False)
    s.connect((host, 80))
    sds.append( s )

for s in sds:
    print s.getsockname(), s.getpeername()
    s.send("GET /")

print "waiting"
while True:
    time.sleep(130)
'''
