#ifndef lint
static  char sccsid[] = "@(#)fpasciisr.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <machine/reg.h>
#include "adb.h"
#include "fpascii.h"

extern char *ecvt(), *index();

/*
 * Prints single precision number d.
 */
void
singletos(f, s)
	float f;	/* may be widened due to C rules */
	char *s;
{
   int decpt, sign;
   char *ecp;

	ecp = ecvt( f, 16, &decpt, &sign );
	if( sign ) *s++ = '-';
	*s++ = *ecp++;		/* copy first digit */
	*s++ = '.';		/* decimal point */
	strcpy( s, ecp );	/* copy the rest of the number */
	s = index( s, 0 );	/* ptr to end of s */

	sprintf( s, "E%03d", decpt );
}


/*
 * Prints double precision number d.
 */
void
doubletos(d, s)
	double d;
	char *s;
{
   int decpt, sign;
   char *ecp;

	ecp = ecvt( d, 16, &decpt, &sign );
	if( sign ) *s++ = '-';
	*s++ = *ecp++;		/* copy first digit */
	*s++ = '.';		/* decimal point */
	strcpy( s, ecp );	/* copy the rest of the number */
	s = index( s, 0 );	/* ptr to end of s */

	sprintf( s, "E%03d", decpt );
}


/*
 * print 16 fpu registers in ascending order. The 'firstreg'
 * parameter is the starting # of the sequence.
 */
printfpuregs(firstreg)
	int firstreg;
{
	int i,r,pval,val;
	char fbuf[64];

	union {
	    struct { int i1, i2; } ip;	/* "int_pair" */
	    float		   float_overlay;
	    double		   dbl_overlay;
	} u_ipfd;	/* union of int-pair, float, and double */

	printf("%-5s %8X\n", regnames[REG_FSR], readreg(REG_FSR)) ;
	for (i = 0; i <= 15 ; ++i , pval = val ) {
		r = firstreg + i;

		val = readreg(r) ;
		printf("%-5s %8X", regnames[r], val ) ;

		u_ipfd.ip.i1 = val;
		/* singletos( u_ipfd.float_overlay, fbuf );
		printf("   %s", fbuf);
		*/
		printf("   %f", u_ipfd.float_overlay );

		if( i&1 ) {
		    u_ipfd.ip.i1 = pval;   u_ipfd.ip.i2 = val;
		    /*
		    doubletos( u_ipfd.dbl_overlay, fbuf );
		    printf("   %s", fbuf);
		    */
		    printf("   %F", u_ipfd.dbl_overlay );
		}
		printf("\n", fbuf);
	}
}
