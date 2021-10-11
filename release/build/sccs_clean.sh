#!/bin/csh -f
#	@(#)sccs_clean.sh 1.1 94/10/31
cd $1
if (-d SCCS) then
	sccs clean
endif
