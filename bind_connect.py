import socket
import random
import time


HOSTS = ["google.com", "onet.pl", "wp.pl", "cloudflare.com"]


port = random.randint(5000,6000)

print "port: %s" % (port,)
sds = []
for host in HOSTS:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('0.0.0.0', port))
    s.connect((host, 80))

    sds.append( s )

for s in sds:
    print s.getsockname(), s.getpeername()
    s.send("GET /")

print "waiting"
time.sleep(130)
