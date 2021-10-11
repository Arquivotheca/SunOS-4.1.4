/*
 ***********************************************************************
 *
 * @(#)device_info.c 1.1 94/10/31 21:05:03
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Comment:
 *	This program module execute a UNIX system command that is only
 *	available in system 4.1 or greater namely "devinfo".  It then
 *	pipes the output of the command into a file called "devinfo.log".
 *	A search for what devices if there is any and which SBus slot
 *	they occupy.  The log file is in a format that looks like this:
 *
 *       Node 'sbus', unit #0
 *                 Register specifications:
 *                   Bus Type=0x1, Address=0xf8000000, Size=2000000
 *               Node 'dma', unit #0
 *                         Register specifications:
 *                           Bus Type=0x1, Address=0xf8400000, Size=4
 *               Node 'esp', unit #0
 *                         Register specifications:
 *                           Bus Type=0x1, Address=0xf8800000, Size=2c
 *                         Interrupt specifications:
 *                           Interrupt Priority=3
 *                       Node 'sd', unit #1
 *                       Node 'sd', unit #0
 *               Node 'le', unit #0
 *                         Register specifications:
 *                           Bus Type=0x1, Address=0xf8c00000, Size=4
 *                         Interrupt specifications:
 *                           Interrupt Priority=5
 *               Node 'cgsix', unit #0
 *                         Register specifications:
 *                           Bus Type=0x1, Address=0xfa000000, Size=1000000
 *                         Interrupt specifications:
 *                           Interrupt Priority=7
 *               Node 'bwtwo', unit #0
 *                         Register specifications:
 *                           Bus Type=0x1, Address=0xfe000000, Size=1000000
 *                         Interrupt specifications:
 *                           Interrupt Priority=7
 *
 *
 * REVISION HISTORY
 *
 * 03/31/91	Roger Pham	Originated
 *
 ***********************************************************************
 */
#include "gtprobe.h"

#define LOG_FILE	"/tmp/devinfo.log"
#define FOR_WRITE	"w+"
#define SYS_CALL	"/usr/etc/devinfo -v > /tmp/devinfo.log"
#define LINE_LENGTH	100

device_info (sbus)
SBus *sbus;
{
    FILE *fp, *fopen();
    Slot_name line1, line2, line3;
    char *l1_ptr, *l2_ptr, *l3_ptr;

    extern char log_error[];
    extern char quote_error[];

	fp = fopen (LOG_FILE, FOR_WRITE);
	if (fp == NULL) program_abort (log_error);

	system (SYS_CALL);

	rewind (fp);

	l1_ptr = line1;
	l2_ptr = line2;
	l3_ptr = line3;

	sbus->slot1 = 0;
	sbus->slot2 = 0;
	sbus->slot3 = 0;

	do {
	    strcpy (l1_ptr, l2_ptr);	/* cascading & save previous 2 lines */
	    strcpy (l2_ptr, l3_ptr);
	    fgets (l3_ptr, sizeof (line1), fp);
	    /* explicitly searching for the 3 SBus addresses */
	    if (string_match (l3_ptr, "0xfa000000")) {
		node_print (l1_ptr, 1, &(sbus->slot1));
		strcpy (sbus->device1, l1_ptr);	/* copy device name over*/
	    }
	    if (string_match (l3_ptr, "0xfc000000")) {
		node_print (l1_ptr, 2, &(sbus->slot2));
		strcpy (sbus->device2, l1_ptr);
	    }
	    if (string_match (l3_ptr, "0xfe000000")) {
		node_print (l1_ptr, 3, &(sbus->slot3));
		strcpy (sbus->device3, l1_ptr);
	    }
	} while (fp->_cnt != 0); 

}

/* return a match status if there is a string s2 in string s1 */
string_match (s1, s2)
char *s1, *s2;
{
    unsigned char i, j;
    unsigned char ls1, ls2;
    char *ss2;

	ls1 = strlen (s1);			/* determine the lengths */
	ls2 = strlen (s2);

	for (i = 0; i < ls1; i++) {
	    ss2  = s2;
	    if (*s1 == *ss2) {			/* if 1 char matches */
		for (j = 1; j < ls2; j++) {	/* proceed w/ the rest */
		    ++s1;
		    ++ss2;
		    if (*s1 == *ss2) {}		/* continue if matches */
		    else break;			/* break if mismatched */
		}
		if (j == ls2) return (1);	/* all s2 was matched */
	    }
	    else s1++;				/* proceed w/ the next char */
	}

	return (0);				/* the search failed */
}

/*
 *	This module prints out the node name which is quoted in the string.
 *	There is 1 requirement, that is the input string has to be in this
 *	format: 	Node 'cgsix', unit #0
 */
node_print (s1, slot, sbus)
char *s1;
int slot;
int *sbus;		/* notice it is not a structure here */
{
    char *s2 = "'";
    char *s3;

	s3 = s1;
	if (string_match (s3, "'gt'"))
	    *sbus = 0;			/* mark available for opening */
	else
	    *sbus = 1;			/* the slot is locked */

	/* search for that first single quote */
	s3 = (char *) strpbrk (s1, s2);
	if (s3 == NULL) program_abort (quote_error);
	s3++;			/* advance pass the single quote */
	strcpy (s1, s3);	/* discard the first quote */
	s3 = (char *) strpbrk (s1, s2);
	if (s3 == NULL) program_abort (quote_error);
	/* discard the second quote, leaving behind only the name */
	strcpy (s3, "\0");
	printf ("A '%s' device is found in SBus slot %d \n", s1, slot);
}

/* allow reasonable program aborting */
program_abort (s1)
char *s1;
{
	printf ("\nThe program exited because %s\n", s1);
	exit (1);
}
