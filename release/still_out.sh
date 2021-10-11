#! /bin/sh
#
# @(#)still_out.sh 1.1 94/10/31 SMI
#
# look through the sccs files, find what's still out, and report
#
# exit status is number of files checked out
#

if [ ! -d SCCS/.. ]; then
	echo 'Where are the SCCS_DIRECTORIES?'
	exit -1;
fi

cd SCCS/..
find . -name 'p.*' -exec echo -n {} '' \; -exec cat {} \; | \
    sed -e 's/^\.\///' -e 's/SCCS\/p\.//' | tee /tmp/out$$

out=`cat /tmp/out$$ | wc -l`
rm -f /tmp/out$$
exit $out
