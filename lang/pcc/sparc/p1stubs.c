#ifndef lint
static	char sccsid[] = "@(#)p1stubs.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * p1stubs.c -- only used in 2-phase mode
 */

#include "cpass1.h"
#ifndef ONEPASS
char *
rnames[] = {  /* keyed to register number tokens */
	"*",   "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7",
	"%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
	"%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
	"%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
	"%f0", "%f1", "%f2", "%f3", "%f4", "%f5", "%f6", "%f7",
	"%f8",  "%f9",  "%f10", "%f11", "%f12", "%f13", "%f14", "%f15",
	"%f16", "%f17", "%f18", "%f19", "%f20", "%f21", "%f22", "%f23",
	"%f24", "%f25", "%f26", "%f27", "%f28", "%f29", "%f30", "%f31",
};

int floatmath;
#endif not ONEPASS

/* NODE * */
addroreg(l)
/*
 * OREG was built in clocal()
 * for an auto or formal parameter
 * now its address is being taken
 * local code must unwind it
 * back to PLUS/MINUS REG ICON
 * according to local conventions
 */
{
	cerror("address of OREG taken");
}

p2bend( functype ) {
#ifndef VCOUNT
	printf("%c%d	\n", BEND, DECREF(functype));
#endif
}

p2bbeg() 
{ 
	extern int regvar, ftnno; 
#ifndef VCOUNT
	printf("%c%d	%d	%d	\n", 
		BBEG, ftnno, autooff, regvar );
#endif
}

p2compile() {;}

p2tree(p){ 
#ifndef VCOUNT
    printf("%c%d	\n", EXPR, lineno); prtree(p);
#endif
}
