/*
 ***********************************************************************
 *
 * @(#)testmia.c 1.1 94/10/31 21:05:13
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Comment:
 *
 * REVISION HISTORY
 *
 * 03/31/91     Roger Pham      Originated
 *
 ***********************************************************************
 */
#ifndef lint
static char sccsid[] = "@(#)gtprobe.c 1.1 91/03/29 Copyr 1990 Sun Micro";
#endif

#include "gtprobe.h"

u_long mia_pat [] ={0xffffffff, 0xaaaaaaaa, 0x55555555, 0x00000000,
		    0x00000001, 0x00000010, 0x00000100, 0x00001000,
		    0x00010000, 0x00100000, 0x01000000, 0x10000000,
		    0xfffffffe, 0xffffffef, 0xfffffeff, 0xffffefff,
		    0xfffeffff, 0xffefffff, 0xfeffffff, 0xefffffff,
		    0xffff0000, 0x0000ffff, 0x00ffff00, 0x00000000 };

testmia (handleptr)
TA_HANDLE *handleptr;
{
    int i;
    int rd_back;
    u_long address;
    extern char mia_error[];
    extern int verbose;
    extern int timeout;

	printf ("\nStart testing the GT Front End Processor Test Register.\n");

	/* 1st read to determine whether it's going to timeout */
	address = TEST_REG;
	timeout = 2;
	ms_read (handleptr, &rd_back, 4, address);

	/* make sure the FE is halted from whatever */
/*
	rd_back = 0x3;
	address = 0x00840000;
	ms_write (handleptr, &rd_back, 4, address);
*/

	for (i = 0, address = TEST_REG; i < sizeof (mia_pat)/4; i++) {
	    ms_write (handleptr, &mia_pat [i], 4, address);
	    ms_read (handleptr, &rd_back, 4, address);
	    if (verbose){
		printf ("Wrote to the GT FEP Test Reg : 0x%8.8X,", mia_pat [i]);
		printf (" and read back : 0x%8.8X\n", rd_back);
	    }
	    if (rd_back != mia_pat [i]){
		printf ("Wrote to the GT FEP Test Reg : 0x%8.8X,", mia_pat [i]);
		printf (" and read back : 0x%8.8X\n", rd_back);
		program_abort (mia_error);
	    }
	}
    
	printf ("\nDone verifying the WR/RD capability\n");
	printf ("to the GT Front End Processor Test Register.\n");

}

