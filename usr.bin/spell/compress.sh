#! /bin/sh
#
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	@(#)compress.sh 1.1 94/10/31 SMI; from S5R3 1.2
#
#	compress - compress the spell program log

trap 'rm -f /usr/tmp/spellhist;exit' 1 2 3 15
echo "COMPRESSED `date`" > /usr/tmp/spellhist
grep -v ' ' /usr/lib/spell/spellhist | sort -fud >> /usr/tmp/spellhist
cp /usr/tmp/spellhist /usr/lib/spell
rm -f /usr/tmp/spellhist
