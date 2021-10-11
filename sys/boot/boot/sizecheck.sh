#!/bin/sh
#
# @(#)sizecheck.sh 1.1 94/10/31 SMI
#
size=`size $1 | tail -1 | awk ' { print $1 + $2; } '`
if [ $size -ge 7680 ]
then
	echo Boot block `expr $size - 7680` bytes too big!!
	exit 1
fi
echo Boot block is `expr 100 '*' $size '/' 7680`% full.
exit 0
