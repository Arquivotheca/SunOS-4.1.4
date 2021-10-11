#! /bin/sh
# @(#)fastboot.sh 1.1 94/10/31 SMI; from UCB 4.2
PATH=/bin:/usr/bin:/usr/etc:$PATH
export PATH
cp /dev/null /fastboot
reboot "$@"
