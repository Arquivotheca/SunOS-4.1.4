#ifndef lint
static	char sccsid[] = "@(#)myflags2.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"

#ifdef FORT
int picflag = 0;
int smallpic = 0;
int malign_flag = 0;
int dalign_flag = 0;
#endif

int proflg = 0;
int bnzeroflg = 0;
int chk_ovfl = 0;

myflags( c, cpp )
    char c; 
    register char **cpp;
{
	char *cp = *cpp;
	switch( c ) {

	case 'f':	
		/*
		 * in the f1 and cg backends, FLOATMATH is
		 * a defined constant; in ccom, it's a flag set
		 * by compiler options.
		 */
#ifndef  FORT
#ifdef FLOATMATH
		if (!strcmp("fsingle", cp) || !strcmp("f", cp)) {
			if (FLOATMATH < 1)
				FLOATMATH = 1;
		} else if (!strcmp("fsingle2", cp) || !strcmp("f2", cp)) {
			if (FLOATMATH < 2)
				FLOATMATH = 2;
		}
#endif
#endif
		/* recognized an -fxxx option string; skip over it */
		while (cp[1]) cp++;
		*cpp = cp;
		return;

	case 'b':
		if (!strcmp("bnzero", cp)) {
			bnzeroflg = 1;
		}
		while (cp[1]) cp++;
		*cpp = cp;
		return;

	case 'p':
		proflg = 1;
		return;

	case 'V':
		chk_ovfl++;
		return;

	case 'k':
	case 'K':
		picflag = 1;
		if (c == 'k')
			smallpic = 1;
		return;

	case 'm':
		malign_flag++;
		return;

	case 'd':
		dalign_flag++;
		return;

	default: 
		cerror( "Bad flag %c", c );
	}
}

minit() {/* do nothing for sparc */}

struct dopest {
    int dopeop;         /* operator # */
    char opst[8];       /* operator name */
};
 

static struct dopest indope[] = {
        FABS,   "FABS",   
        FCOS,   "FCOS",
        FSIN,   "FSIN",    
        FTAN,   "FTAN",
        FACOS,  "FACOS",
        FASIN,  "FASIN",
        FATAN,  "FATAN",
        FCOSH,  "FCOSH",
        FSINH,  "FSINH",
        FTANH,  "FTANH",
        FEXP,   "FEXP",
        F10TOX, "F10TOX",
        F2TOX,  "F2TOX",
        FLOGN,  "FLOGN",
        FLOG10, "FLOG10",
        FLOG2,  "FLOG2",
        FSQR,   "FSQR",
        FSQRT,  "FSQRT",
        FAINT,  "FAINT",
        FANINT, "FANINT",
        FNINT,  "FNINT",
        -1,     "",
};

moredope()
{
    register struct dopest *q;
    register int o;

    for (q = indope; q->dopeop >= 0; ++q) {
        o = q->dopeop;
        opst[o] = q->opst;
	switch (q->dopeop){
	default:
	    dope[o] = UTYPE|LIBRFLG;
	    break;
#ifdef FSQRT_OP
	case FSQRT:
	    /* done in hardware, starting with GNUFPC */
	    dope[o] = UTYPE|INTRFLG;
	    break;
#endif
	case FABS:
	case FSQR:
	    dope[o] = UTYPE;
	    break;
	}
    }    
}
