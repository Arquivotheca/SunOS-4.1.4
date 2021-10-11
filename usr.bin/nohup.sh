#! /bin/sh
#
#	@(#)nohup.sh 1.1 94/10/31 SMI; from UCB
#
trap "" 1 15
if test -t 2>&1  ; then
	echo "Sending output to 'nohup.out'"
	exec nice -5 $* >>nohup.out 2>&1
else
	exec nice -5 $* 2>&1
fi
