#! /bin/sh
#
# 	@(#)makexcl.sh 1.1 94/10/31 SMI
#
#	makexcl [ file ]
#
#	grovel through $file (mktp_input default) and compose
#	an exclude list of exclude lists of the form "exclude.lists/usr.*"
#	clean off any trash at the end of the line
#	and substitute any "ARCH"'s found
#
file=mktp_input
if [ $# -eq 1 ]; then
	file=$1
fi

grep Command.\*k=\`cat.exclude\.lists\/usr\. $file | \
	sed -e 's/^.*k=`cat exclude/exclude/' -e 's/`.*$//' | \
	sort -u | \
	awk '{ printf("%s\n", $1); }' - | \
	sed -e "s/ARCH/${ARCH}/"
