#! /bin/sh
#
#	@(#)ccat.sh 1.1 94/10/31 SMI; from UCB 4.1 83/02/11
#
for file in $*
do
	/usr/old/uncompact < $file
done
