#!/bin/bash
set -e
set -u
set -o pipefail

TORDIR=$HOME/tor/tor-dev
export PATH=$PATH:$TORDIR/src/or:$TORDIR/src/tools

# Check dependencies
dpkg -l git proxychains > /dev/null

# Install chutney locally
[ -e chutney ] || git clone https://git.torproject.org/chutney.git

# Remove previous setup
rm -rf chutney/net

# Configure new network
(cd chutney && ./chutney configure networks/basic)

# Amend generated torrc
cat <<EOF > torrc_patch
DisableDebuggerAttachment 0
TokenBucketRefillInterval 1 second
ServerDNSDetectHijacking 0
SocksTimeout 15 minutes
EOF
find chutney/net -name torrc -exec bash -c "cat torrc_patch >> {}" \;


# Most important: start the network
(cd chutney && ./chutney start networks/basic)


cat <<EOF > proxychains.conf
strict_chain
proxy_dns
tcp_read_time_out 180000
#quiet_mode

[ProxyList]
socks5 127.0.0.1 9008
EOF

# Wait until localhost responds via tor:
while [ 1 ]; do
    set +e
    proxychains curl http://127.0.0.1:22
    R=$?
    set -e
    if [ "$R" == "0" ]; then
        break
    fi
done

# Clean up
(cd chutney && ./chutney stop networks/basic)
