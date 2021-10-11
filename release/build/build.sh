#!/bin/csh -f
# 
#	@(#)build.sh 1.1 94/10/31
#
echo
echo "*************************************************"
echo "* build started on `date` *"
echo "*************************************************"
echo
#
umask 002
cd /usr/src/sys
make -k clean
make -k all 
