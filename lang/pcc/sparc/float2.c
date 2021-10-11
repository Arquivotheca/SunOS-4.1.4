#ifndef lint
static	char sccsid[] = "@(#)float2.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


/*
 *	float2.c
 */

#include "cpass2.h"

static void
ldfcon( p, constval, constreg )
    NODE *p;
    double constval;
    char constreg;
{
    /*
     * load the constant constval of type p->in.type into the register designated by constreg.
     * note the sprintfs.
     */
    char pattern[60]; /* arbitrary number */
    NODE val;
    char szchar; /* 'd' for double, ' ' for float */

    val = *p; /* type &c */
    val.fpn.op = FCON;
    val.fpn.dval = constval;
    switch (p->in.type){
    case DOUBLE:
	szchar = 'd'; break;
    default:
	szchar = ' '; break;
    }
    if (picflag) {
	if (smallpic) {
	    expand( &val, INTAREG,	"	ld	[%l7+CR],A1\n");
	} else {
	    expand( &val, INTAREG,  "	set	CR,A1\n");
	    expand( &val, INTAREG,	"	ld	[%l7+A1],A1\n");
	}
	sprintf( pattern,"	ld%c	[A1],A%c\n", szchar, constreg);
	expand( &val, INTBREG, pattern);
    } else {
	expand( &val, INTAREG,	"	sethi	%hi(CR),A1\n");
	sprintf( pattern,"	ld%c	[A1 + %%lo(CR)],A%c\n", szchar, constreg );
	expand( &val, INTBREG, pattern );
    }
}

floatcode( p, cookie )
    register NODE *p;
{
    /*
     * we mean to emit floating-point code. figure out what we want to do,
     * figure out who we are, and do the right thing.
     */
    register o, ty;
    register NODE * l;
    int labno;

    ty = p->in.type;
    o  = p->in.op;

    switch (o){
    case REG:
    case UNARY MUL:
		/* FORCC */
		/* have if(f)... Rewrite it as if (f != 0.0) */
		ldfcon( p, 0.0, '2');
		expand( p,     FORCC,   "	fcmpZ.	AR,A2\n	nop\n");
		return;
	/* end of case EA */
    case SCONV:
		/* unsigned-to-float/double conversion */
		/* operand already in floating register */
		/* first do naive conversion */
		expand( p, cookie, "	fitoZ.	AL,A2\n");
		/* now compare with zero */
		ldfcon( p, 0.0, '3' );
		expand( p,     FORCC,   "	fcmpZ.	A2,A3\n	nop\n");
		/* conditional branch on sign of result */
		labno = getlab();
		printf("	fbge	L%d\n", labno);
		/* and in the delay slot ... */
		ldfcon( p, 4.29496729600000000e+09, '3');
		expand( p, cookie, "	faddZ.	A2,A3,A2\n");
		deflab(labno);
		return;

    default:

	print_str("HEY! cookie = ");
	prcook( cookie);
	putchar('\n');
	fwalk( p, eprint, 0);
	cerror("floatcode got a tree it didn't expect");
    }

}
