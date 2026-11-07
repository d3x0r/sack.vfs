#!/bin/bash
# usage: ufw.sh <network/prefix> [timeout]
# ufw has no timeouts, so [timeout] is ignored and the ban stays until
# ufw-unban.sh removes it.  prepend (ufw 0.36+) puts it ahead of allow rules
# and, unlike 'insert 1', works for IPv6 too.

ufw prepend deny from "$1"
