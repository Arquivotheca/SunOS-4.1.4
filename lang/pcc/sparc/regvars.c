#ifndef lint
static	char sccsid[] = "@(#)regvars.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


# include "cpass1.h"

/*
 * Register variable allocation:
 *
 * regvar_init()	Initialize register variable allocation
 * regvar_alloc(type)	Allocate register for variable of given type
 * regvar_avail(type)	Is register available for var of given type
 */

extern int twopass;

		/* initial allocations */
#define INITIAL_D_ALLOCATION  0x3fff003c /* l0 - l7, i0 - i5, g2 - g5 */
#define INITIAL_F_ALLOCATION  0xffffff00 /* f8 - f31 */


long regvar = INITIAL_D_ALLOCATION, floatvar = INITIAL_F_ALLOCATION;

void
regvar_init()
{
	regvar = INITIAL_D_ALLOCATION;
	floatvar = INITIAL_F_ALLOCATION;
}

int
regvar_alloc(type )
	TWORD type;
{
	register rvar;
	int size;
	if ( blevel == 1 ){
		/*
		 * parameter register: make something up later.
		 */
		rvar = NOOFFSET;
	} else if ( ISFLOATING(type) ) {
		/*
		 * allocate floating point register
		 */
		NEXTF(rvar, floatvar);
		if ( type == DOUBLE ){
		    /* doubles take two */
		    rvar -= 1;
		    /* and must be even aligned */
		    rvar -= rvar&1;
		    while (rvar >= MIN_FVAR){
		        /* this loop cannot fail, since regvar_avail returned true */
		    	if (F_AVAIL(rvar,floatvar) && F_AVAIL(rvar+1,floatvar)){
		    	    SETF(floatvar, rvar);
		    	    SETF(floatvar, rvar+1);
		    	    break;
		    	}
		    	rvar -= 2;
		    }
		}else{
		    SETF(floatvar, rvar);
		}
	} else {
		/*
		 * allocate data register
		 */
		NEXTD(rvar, regvar);
		SETD(regvar, rvar);
	}
	return(rvar);
}

int
regvar_avail(type )
{
	register rvar;
	int size;

	if ( twopass ) {
		/* leave register allocation up to iropt */
		return 0;
	}
	if ( ISFLOATING(type) ) {
		NEXTF(rvar, floatvar);
		if ( type == DOUBLE ){
		    /* doubles take two */
		    rvar -= 1;
		    /* and must be even aligned */
		    rvar -= rvar&1;
		    while (rvar >= MIN_FVAR){
		    	if (F_AVAIL(rvar,floatvar) && F_AVAIL(rvar+1,floatvar)){
		    	    break;
		    	}
		    	rvar -= 2;
		    }
		}
		return( rvar >= MIN_FVAR );
	}
	if (cisreg(type)){
		NEXTD(rvar, regvar );
		return( rvar >= MIN_DVAR );
	} else
		return 0;
}
