#!/bin/bash
# usage: nft-setup.sh [external-interface]
# Run once per boot (with firewall=nft), before the scanners or reload-bans.mjs.
#
# Makes its own table, so it works the same on Arch (plain nftables) and on
# Ubuntu next to ufw: ufw keeps its tables, and this one drops banned
# networks earlier, in prerouting at raw priority, before conntrack spends
# anything on them.
#
# Each set entry carries its own timeout and the kernel removes it when it
# runs out.  Lookup cost stays flat however many entries there are, unlike
# one rule per banned address.
#
# Pass the external interface (e.g. enp3s0) to only drop traffic arriving
# there, so a LAN address that trips a pattern can't lock you out.

nft list table inet banlist >/dev/null 2>&1 && exit 0

IIF=""
[ -n "$1" ] && IIF="iifname \"$1\""

nft -f - <<EOF
table inet banlist {
	set v4 { type ipv4_addr; flags interval, timeout; }
	set v6 { type ipv6_addr; flags interval, timeout; }
	chain pre {
		type filter hook prerouting priority raw; policy accept;
		$IIF ip saddr @v4 counter drop
		$IIF ip6 saddr @v6 counter drop
	}
}
EOF
