#ifndef lint
static  char sccsid[] = "@(#)genassym.c 1.1 94/10/31";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <mon/sunromvec.h>

main()
{
	printf("#define\tINITGETKEY 0x%x\n", &romp->v_initgetkey);
	exit(0);
}

