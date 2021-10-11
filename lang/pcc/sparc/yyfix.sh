#!/bin/sh
#
# @(#)yyfix.sh	1.1 (Sun) 10/31/94
#
>rodata.c
for i in $*
do ed - y.tab.c <<!
/^\(.*\)$i[ 	]*\[]/s//extern \1 $i[];\\
\1 $i []/
.ka
/}/kb
'a-r rodata.c
'a,'bw rodata.c
'a,'bd
w
q
!
done
