#!/bin/sh
curl -s https://github.com/majek/dump/raw/master/vpn/vpn.sh > /tmp/vpn.sh
IPADDRESS=`curl -s http://169.254.169.254/latest/meta-data/local-ipv4`
SHAREDSECRET="password"

. /tmp/vpn.sh


cat << EOF > /etc/cron.hourly/upgrade
#!/bin/sh
export DEBIAN_FRONTEND=noninteractive
apt-get -qy update
apt-get -qy upgrade
EOF
chmod 750 /etc/cron.hourly/upgrade

. /etc/cron.hourly/upgrade

apt-get -q install openntpd

