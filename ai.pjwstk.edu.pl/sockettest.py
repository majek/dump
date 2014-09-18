import socket, time, os, sys
'''
Linux version 2.6.17-11-generic


tcp        0      0 127.0.0.1:60885         127.0.0.1:8081          ESTABLISHED
tcp        0      0 127.0.0.1:8081          127.0.0.1:60885         ESTABLISHED

tcp        0      0 127.0.0.1:60886         127.0.0.1:8081          ESTABLISHED
tcp        0      0 127.0.0.1:8081          127.0.0.1:60886         ESTABLISHED

tcp        0      0 127.0.0.1:60887         127.0.0.1:8081          ESTABLISHED
tcp        0      0 127.0.0.1:8081          127.0.0.1:60887         SYN_RECV   


127.0.0.1.60885 > 127.0.0.1.8081: S win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.8081 > 127.0.0.1.60885: S ack 2644248117 win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.60885 > 127.0.0.1.8081: . ack 1 win 8198

127.0.0.1.60886 > 127.0.0.1.8081: S win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.8081 > 127.0.0.1.60886: S ack 2646928857 win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.60886 > 127.0.0.1.8081: . ack 1 win 8198

127.0.0.1.60887 > 127.0.0.1.8081: S 2644799914:2644799914(0) win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198

127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198
127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198

FULL
11:13:37.931073 IP 127.0.0.1.60885 > 127.0.0.1.8081: S 2644248116:2644248116(0) win 32792 <mss 16396,nop,wscale 2>
11:13:37.931104 IP 127.0.0.1.8081 > 127.0.0.1.60885: S 2644043371:2644043371(0) ack 2644248117 win 32792 <mss 16396,nop,wscale 2>
11:13:37.931122 IP 127.0.0.1.60885 > 127.0.0.1.8081: . ack 1 win 8198

11:13:38.931087 IP 127.0.0.1.60886 > 127.0.0.1.8081: S 2646928856:2646928856(0) win 32792 <mss 16396,nop,wscale 2>
11:13:38.931116 IP 127.0.0.1.8081 > 127.0.0.1.60886: S 2644794659:2644794659(0) ack 2646928857 win 32792 <mss 16396,nop,wscale 2>
11:13:38.931133 IP 127.0.0.1.60886 > 127.0.0.1.8081: . ack 1 win 8198

11:13:39.937110 IP 127.0.0.1.60887 > 127.0.0.1.8081: S 2644799914:2644799914(0) win 32792 <mss 16396,nop,wscale 2>
11:13:39.937138 IP 127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
11:13:39.937156 IP 127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198

11:13:44.337341 IP 127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
11:13:44.338025 IP 127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198
11:13:50.345240 IP 127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
11:13:50.345432 IP 127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198
11:14:02.345041 IP 127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
11:14:02.345262 IP 127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198
11:14:26.544622 IP 127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
11:14:26.559779 IP 127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198
11:15:14.747782 IP 127.0.0.1.8081 > 127.0.0.1.60887: S 2644235487:2644235487(0) ack 2644799915 win 32792 <mss 16396,nop,wscale 2>
11:15:14.747973 IP 127.0.0.1.60887 > 127.0.0.1.8081: . ack 1 win 8198


'''

# def nasluchuj(n):
PORT = 8081
HOST = 'localhost'
LISTENNO = 0
CONNECTS = 3
    
    
def server():
    s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,
                    s.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR) | 1 )
    s.bind((HOST, PORT))
    s.listen(LISTENNO)
    while True:
        #conn,addr = s.accept()
        #print "accepted %r:%r" % (addr)
        print "going to ifinite loop"
        while True:
            time.sleep(300)


def client():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST,PORT))
    print "connected %s:%s" % (HOST, PORT)
    # s.send('test_'*1)
    a = s.recv(1024)
    print "sent"
    while True:
        time.sleep(300)
    # s.close()
    #print "close"
    
    



if __name__ == '__main__':
    if os.fork()>0:
        server()
        sys.exit()
    else:
        #sys.exit()
        time.sleep(1)
        for i in range(CONNECTS):
            time.sleep(1)
            if os.fork()>0:
                client()
                sys.exit()
        sys.exit()
