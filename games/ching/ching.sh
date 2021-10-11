#! /bin/sh
#
#	@(#)ching.sh	1.1	(Berkeley)	94/10/31
#
cd /usr/games/lib/ching.d
PATH=:$PATH
case $1 in
	[6-9]*)	H=$1;shift;;
esac
if	test $H
then	phx $H | nroff $* macros - | more -s
else	cno > "/tmp/#$$" 
	echo "  "
	phx `cat "/tmp/#$$"` | nroff $* macros - | more -s
	rm "/tmp/#$$"
fi
