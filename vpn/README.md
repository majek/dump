VPN server configuration
========================

... that works with Mac OS X, iPhone, iPad vpn clients (l2tp over ipsec).


On Ubuntu server
----------------

 * Dependencies

    apt-get install openswan xl2dp radvd

 * Remember about setting ip in ipsec.conf.


Firewall
--------

 * Open udp ports 500 and 4500:

    -A INPUT -p udp --dport 500 -j ACCEPT
    -A INPUT -p udp --dport 4500 -j ACCEPT

 * Masquerade/Snat outgoing traffic:

    -A POSTROUTING -o eth0 -j MASQUERADE

 * Reject stuff to port 1701/udp that doesn't go over ipsec:

    -A INPUT -m policy --dir in --pol ipsec -p udp --dport 1701 -j ACCEPT
    -A INPUT -p udp --dport 1701 -j REJECT

    -A OUTPUT -m policy --dir out --pol ipsec -p udp --sport 1701 -j ACCEPT
    -A OUTPUT -p udp --sport 1701 -j REJECT

 * Sysctl:

    net.ipv4.ip_forward=1
    net.ipv6.conf.all.forwarding=1
    net.ipv4.conf.all.send_redirects = 0
    net.ipv4.conf.default.send_redirects = 0
    net.ipv4.conf.all.accept_redirects = 0
    net.ipv4.conf.default.accept_redirects = 0

