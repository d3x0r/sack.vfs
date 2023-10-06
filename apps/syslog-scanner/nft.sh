#!/bin/bash
EXT=enp3s0

nft add rule ip filter INPUT iifname "$EXT" ip saddr $1/24 counter drop
