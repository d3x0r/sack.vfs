#!/bin/bash
echo y | ufw delete $(ufw status numbered |grep $1 | sed  's/\[[ ]*\([0-9]*\)\].*$/\1/')

