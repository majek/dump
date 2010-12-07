
export DEBIAN_FRONTEND=noninteractive
apt-get -qy update
apt-get -qy install openswan xl2tpd

#IPADDRESS="1.1.1.1"
#SHAREDSECRET="password"

[ ! -e /etc/ppp/chap-secrets.orig ] || mv /etc/ppp/chap-secrets /etc/ppp/chap-secrets.orig
cat << EOF > /etc/ppp/chap-secrets
# Secrets for authentication using CHAP
# client	server	secret		IP addresses
#user		*	"pass"		10.0.0.0/24
a		*	"a"		10.0.0.0/24
EOF
chmod o-r /etc/ppp/chap-secrets


[ ! -e /etc/ipsec.conf.orig ] || mv /etc/ipsec.conf /etc/ipsec.conf.orig
cat << EOF > /etc/ipsec.conf
version 2.0

config setup
	virtual_private=%v4:10.0.0.0/8,%v4:192.168.0.0/16,%v4:172.16.0.0/12,%v4:!10.0.0.0/24
        nat_traversal=yes
	oe=no
        protostack=netkey
	force_keepalive=yes
	keep_alive=29

conn L2TP
	aggrmode = no
        authby=secret
        pfs=no
        rekey=no
	type = transport # encrypting only payload
	keyingtries = 1
	failureshunt = reject

	# type = tunnel # encrypting full ip packet - MTU smaller by 32 bytes
	forceencaps=yes # required

        left=$IPADDRESS
        leftnexthop=%defaultroute
        leftprotoport=17/1701
        right=%any
        rightprotoport=17/%any
        rightsubnetwithin=0.0.0.0/0

        auto=add

	## Fixup for "The server does not respond. Please verify your server address and try again.".
	## Only OSX 10.5.
	dpddelay=10
	dpdtimeout=11
	dpdaction=clear

	compress=yes		# Don't compress again on ppp layer.
	keyexchange=ike
	# ikelifetime=240m

	#esp=aes128-sha1
	#ike=aes128-sha-modp1024
EOF

[ ! -e /etc/ipsec.secrets.orig ] || mv /etc/ipsec.secrets /etc/ipsec.secrets.orig
cat << EOF > /etc/ipsec.secrets
# RCSID $Id: ipsec.secrets.proto,v 1.3.6.1 2005/09/28 13:59:14 paul Exp $
# This file holds shared secrets or RSA private keys for inter-Pluto
# authentication.  See ipsec_pluto(8) manpage, and HTML documentation.

# RSA private key for this host, authenticating it to any other host
# which knows the public part.  Suitable public keys, for ipsec.conf, DNS,
# or configuration of other implementations, can be extracted conveniently
# with "ipsec showhostkey".

%any %any: PSK "$SHAREDSECRET"
EOF
chmod 600 /etc/ipsec.secrets


cat << EOF > /etc/ppp/options.xl2tpdnew
# Google dns
ms-dns 8.8.8.8
name HygenicNetwork

auth
-pap
+chap

ktune

#crtscts
lock
#-vj
proxyarp
noipx
#maxfail 32
idle 3600

# no compression
noccp
nobsdcomp
debug
hide-password
#mtu 1400
#mru 1400

connect-delay 5000

logfd 2
logfile /var/log/l2tpd.log

lcp-echo-interval 300 # lcp delay 5 minutes
lcp-echo-failure 4

ipv6 ::1,::2
EOF

[ ! -e /etc/xl2tpd/xl2tpd.conf.orig ] || mv /etc/xl2tpd/xl2tpd.conf /etc/xl2tpd/xl2tpd.conf.orig
cat << EOF > /etc/xl2tpd/xl2tpd.conf
[global]

[lns default]
exclusive = no
ip range = 10.0.0.200-10.0.0.254
local ip = 10.0.0.1
require chap = yes
refuse pap = yes
require authentication = yes
name = HygenicNetwork
ppp debug = yes
pppoptfile = /etc/ppp/options.xl2tpdnew
length bit = yes
EOF


cat << EOF > /etc/sysctl.d/99-vpn.conf
net.ipv4.ip_forward = 1
net.ipv6.conf.all.forwarding = 1
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.default.send_redirects = 0
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
EOF
service procps start


cat << EOF > /etc/iptables.up.rules
*nat
-A POSTROUTING -o eth0 -j MASQUERADE
COMMIT


*filter

# Allows all loopback (lo0) traffic and drop all traffic to 127/8 that doesn't use lo0
-A INPUT -i lo -j ACCEPT
-A INPUT ! -i lo -d 127.0.0.0/8 -j REJECT

# Accepts all established inbound connections
-A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

-A INPUT -m policy --dir in --pol ipsec -p udp --dport 1701 -j ACCEPT
-A INPUT -p udp --dport 1701 -j REJECT

-A OUTPUT -m policy --dir out --pol ipsec -p udp --sport 1701 -j ACCEPT
-A OUTPUT -p udp --sport 1701 -j REJECT


# Allows all outbound traffic
# You could modify this to only allow certain traffic
-A OUTPUT -j ACCEPT

# Allows HTTP and HTTPS connections from anywhere (the normal ports for websites)
-A INPUT -p tcp -m state --state NEW --dport 22 -j ACCEPT
-A INPUT -p tcp -m state --state NEW --dport 44 -j ACCEPT
-A INPUT -p tcp --dport 80 -j ACCEPT
-A INPUT -p tcp --dport 443 -j ACCEPT

-A INPUT -p udp --dport 500 -j ACCEPT
-A INPUT -p udp --dport 4500 -j ACCEPT


# Allows SSH connections for script kiddies
# THE -dport NUMBER IS THE SAME ONE YOU SET UP IN THE SSHD_CONFIG FILE
# -A INPUT -p tcp -m state --state NEW --dport 30000 -j ACCEPT

# Now you should read up on iptables rules and consider whether ssh access 
# for everyone is really desired. Most likely you will only allow access from certain IPs.

# Allow ping
-A INPUT -p icmp -m icmp --icmp-type 8 -j ACCEPT

# log iptables denied calls (access via 'dmesg' command)
-A INPUT -m limit --limit 5/min -j LOG --log-prefix "iptables denied: " --log-level 7

# Reject all other inbound - default deny unless explicitly allowed policy:
-A INPUT -j DROP

-A FORWARD -p tcp -m multiport --dports 25,137:139,445 -j REJECT --reject-with icmp-admin-prohibited
-A FORWARD -p udp --dport    137:139 -j REJECT --reject-with icmp-admin-prohibited

-A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT
-A FORWARD --src 10.0.0.0/24 -o eth0 -m state --state NEW -j ACCEPT
-A FORWARD -j DROP


COMMIT
EOF

cat << EOF > /etc/iptables.up.rules.del
*nat
COMMIT

*filter
COMMIT
EOF


cat << EOF > /etc/network/if-pre-up.d/iptablesload
#!/bin/sh
iptables-restore < /etc/iptables.up.rules
exit 0
EOF
chmod +x /etc/network/if-pre-up.d/iptablesload

iptables-restore < /etc/iptables.up.rules

/etc/init.d/xl2tpd restart
/etc/init.d/ipsec restart



