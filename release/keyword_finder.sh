#!/bin/sh
#@(#)keyword_finder.sh 1.1 94/10/31 SMI
 
LIST="arning ARNING ailed AILED annot denied memory working syntax rror: illegal ERROR such undefined"

for i in $LIST
do
        grep $i $1
done

grep "not defined" $1
grep "an\'t" $1
