#ifndef lint
static	char sccsid[] = "@(#)opmatch.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"

/*
 * int
 * opmatch(op,tableop)
 *	Decides if an operator 'tableop' from the code templates
 *	table should be considered in pattern matching when a given
 *	operator 'op' shows up in an expression tree.
 */

extern int mamask[];	/* from match.c */

int
opmatch(op,tableop)
	unsigned op;
	register unsigned tableop;
{
	register opmtemp;

	/*
	 * check for match against a specific operator
	 */
	if ( tableop < OPSIMP )
		return( tableop == op );

	/*
	 * check for match against an operator class (e.g., OPSIMP)
	 */
	if ((opmtemp=mamask[tableop - OPSIMP])&SPFLG){
		switch (op) {
		case NAME:
		case FCON:
		case ICON:
		case OREG:
		case REG:
			return(1);
		case UNARY MUL:
			/*
			 * for autoincrement addressing, recognize
			 * *p++, *--p as LTYPEs, even though they
			 * aren't.  We have to finish this test
			 * by calling shumul() in match() (bletch)
			 */
			return(1);
		}
		return(0);
	}
	/*
	 * same check on ASG operator class
	 */
	return( (dope[op]&(opmtemp|ASGFLG)) == opmtemp );
}
