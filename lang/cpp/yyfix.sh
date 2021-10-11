#! /bin/sh

#
#       @(#)yyfix.sh	1.1     10/31/94
#

rm -f rodata.c
>rodata.c
for i in $*
do
rm -f rodata-temp.c
>rodata-temp.c
/bin/ed - y.tab.c <<!
/^\(.*\)$i[ 	]*\[][	 ]*=[	 ]*{/ka
/}/kb
'a,'bw rodata-temp.c
'a s/^\(.*\)$i[ 	]*\[][	 ]*=[	 ]*{/extern \1 $i[];/
'a+1,'bd
w y.tab.c
q
!
cat rodata-temp.c >> rodata.c
rm -f rodata-temp.c
done
