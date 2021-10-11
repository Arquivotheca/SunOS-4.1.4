#!/bin/sh
#
# @(#)extract.sh 1.1 94/10/31
#

mt -f /dev/nrst8 asf 2
dd if=/dev/rst8 bs=20b | uncompress -c | tar xfp -
