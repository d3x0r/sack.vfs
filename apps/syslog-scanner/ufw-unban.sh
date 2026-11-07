#!/bin/bash
# usage: ufw-unban.sh <network/prefix>

ufw delete deny from "$1"
