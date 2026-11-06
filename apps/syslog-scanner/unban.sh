#!/bin/bash
# usage: unban.sh <network/prefix>     removes it from the nft-setup.sh sets

case "$1" in *:*) SET=v6;; *) SET=v4;; esac

nft delete element inet banlist $SET "{ $1 }"
