/*
 ***********************************************************************
 *
 * @(#)testhsa.c 1.1 94/10/31 21:05:12
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

u_long hsa_pat [] ={0x00000001, 0x00000002, 0x00000003, 0x00010000,
                    0x00010001, 0x00010002, 0x00010003, 0x00020000,
                    0x00020001, 0x00020002, 0x00020003, 0x00030000,
                    0x00030001, 0x00030002, 0x00030003, 0x00000000, };

testhsa (handleptr)
TA_HANDLE *handleptr;
{
    int i;
    int rd_back;
    u_long address;
    extern char hsa_error[];
    extern int verbose;
    extern timeout;

	printf ("\nStart testing to the SBus Adapter Mode Register.\n");

	/* 1st read to determine whether it's going to timeout */
	address = HSA_MODE_REG;
	timeout = 1;
	ms_read (handleptr, &rd_back, 4, address);

	for (i = 0; i < sizeof (hsa_pat)/4; i++) {
	    ms_write (handleptr, &hsa_pat [i], 4, address);
	    ms_read (handleptr, &rd_back, 4, address);
	    rd_back &= MODE_MASK;
	    if (verbose){
		printf ("Wrote to the SBus Adapter Mode Reg : 0x%8.8X,", hsa_pat [i]);
		printf (" and read back : 0x%8.8X\n", rd_back);
	    }
	    if (rd_back != hsa_pat [i]){
		printf ("Wrote to the SBus Adapter Mode Reg : 0x%8.8X,", hsa_pat [i]);
		printf (" and read back : 0x%8.8X\n", rd_back);
		program_abort (hsa_error);
	    }
	}
    
	printf ("\nDone verifying WR/RD capability to the SBus Adapter Mode Register.\n");

}

