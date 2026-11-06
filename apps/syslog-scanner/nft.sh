#!/bin/bash
# usage: nft.sh <network/prefix> [timeout]     e.g.  nft.sh 1.2.3.0/24 86400s
# Adds the network to the sets made by nft-setup.sh for [timeout] (default 24h).

case "$1" in *:*) SET=v6;; *) SET=v4;; esac
TIMEOUT=${2:-24h}

# One transaction: the first add makes sure the element exists so the delete
# can't fail; delete + add then restarts the timeout for repeat offenders.
nft -f - <<EOF
add element inet banlist $SET { $1 }
delete element inet banlist $SET { $1 }
add element inet banlist $SET { $1 timeout $TIMEOUT }
EOF
