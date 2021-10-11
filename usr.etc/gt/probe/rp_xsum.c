/*
 ***********************************************************************
 *
 * @(#)rp_xsum.c 1.1 94/10/31 21:05:10
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

#define PROM_LAST_ADDR	0x10000 - 4		/* 64K - 4 */

rp_xsum (handleptr)
TA_HANDLE *handleptr;
{
    u_long rd_back;
    u_long address;
    u_long sum;
    u_long xsum;
    unsigned char byte;
    extern char mia_error[];
    extern int verbose;
    extern int timeout;
    Slot_name version;
    int i;

	printf ("\nStart testing to the GT OpenBoot PROM on the Rendering Pipeline.\n");

	/* 1st read to determine whether it's going to timeout */
	address = 0;
	timeout = 3;
	ms_read (handleptr, &rd_back, 4, address);

	/* read the version and filter out non-ascii characters */
	ms_read (handleptr, &version[0], 100, 0x64);
	for (i = 0; i < 0x10; i++){
	    if ((version [i] < 0x20) | (version [i] > 0x7f))
	    	version [i] = 0x20;
	}
	version [10] = NULL;	/* be sure to terminate the string */

	printf ("\The GT OpenBoot PROM's version is: %s\n", version);

	for (address = 0, sum = 0; address < PROM_LAST_ADDR; address += 4) {
	    ms_read (handleptr, &rd_back, 4, address);	/* read all 4 */
	    byte = (rd_back >> 24) & 0xff;		/* slice 'm */
	    sum += byte;
	    byte = (rd_back >> 16) & 0xff;
	    sum += byte;
	    byte = (rd_back >> 8) & 0xff;
	    sum += byte;
	    byte = (rd_back >> 0) & 0xff;
	    sum += byte;
	}

	ms_read (handleptr, &xsum, 4, PROM_LAST_ADDR);	/* hard xsum */

	if (verbose){
	    printf ("The xsum from adding up all the bytes is : 0x%8x\n", sum);
	    printf ("The xsum read at the end of the PROM is : 0x%8x\n", xsum);
	}

	if (sum != xsum){
	    PRINT_LINE;
	    PRINT_LINE;
	    printf ("ERROR: GT Openboot PROM's checksum did not verify.\n");
	    PRINT_LINE;
	    exit (1);
	}
    
	printf ("\nDone verifying the GT Openboot PROM's checksum.\n");

}

