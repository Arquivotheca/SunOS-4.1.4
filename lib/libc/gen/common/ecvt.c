#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)ecvt.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

#include <stdio.h>
extern char    *
econvert(), *fconvert();

static char     *efcvtbuffer;

char           *
ecvt(arg, ndigits, decpt, sign)
	double          arg;
	int             ndigits, *decpt, *sign;
{
	if (efcvtbuffer == NULL)
		efcvtbuffer = (char *)calloc(1,1024);
	return econvert(arg, ndigits, decpt, sign, efcvtbuffer);
}

char           *
fcvt(arg, ndigits, decpt, sign)
	double          arg;
	int             ndigits, *decpt, *sign;
{
	if (efcvtbuffer == NULL)
		efcvtbuffer = (char *)calloc(1,1024);
	return fconvert(arg, ndigits, decpt, sign, efcvtbuffer);
}
