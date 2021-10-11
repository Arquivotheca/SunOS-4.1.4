/*
 ***********************************************************************
 *
 * @(#)gt_info.c 1.1 94/10/31 21:05:04
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Comment:
 *	This program module search the environment and determines
 *	whether the GT_SLOT environment variable was defined.  In
 *	either case, it allows the user to change within the realm
 *	of this program.
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

#define	MAX_PICK_COUNT	3	/* since there are 3 bus slots, the maximum
				# of picks allows should not be more than 3 */

gt_info (sbus)
SBus *sbus;
{
    register int    gt_slot;
    char user;
    int pick, count;
    int found;
    extern char *gt;

    /* search if a 'gt' device was found */
    if (strcmp (sbus->device1, gt) == 0)
	found = 1;
    else if (strcmp (sbus->device2, gt) == 0)
	found = 2;
    else if (strcmp (sbus->device3, gt) == 0)
	found = 3;
    else
	found = 0;

    if (found){		/* if found then default to this */
    	gt_slot = found;
    }
    else{		/* search for the empty slot to default */

	if (!sbus->slot1)
	    gt_slot = 1;
	else if (!sbus->slot2)
	    gt_slot = 2;
	else if (!sbus->slot3)
	    gt_slot = 3;
	else
	    program_abort ("all 3 SBus slots are occupied");

        printf ("\nNo 'gt' device was found anywhere.  ");
	printf ("Therefore, it's defaulted to slot #%d.\n", gt_slot);
    }

    PRINT_LINE;
	
    for (count = 0; count < MAX_PICK_COUNT; count++) {

	printf ("Enter the slot number you wish to change or <cr> to default\n");

	user = getchar ();

	if (user == '\n')		/* check if that was a return */
		pick = gt_slot;
	else
		pick = atoi (&user);

	switch (pick) {
	    case 1 :
		if (sbus->slot1) {
		    printf ("This SBus slot is already occupied by the device");
		    printf (" '%s'\n", sbus->device1);
		    break;
		}
		else
		    return (pick);
	    case 2 :
		if (sbus->slot2) {
		    printf ("This SBus slot is already occupied by the device");
		    printf (" '%s'\n", sbus->device2);
		    break;
		}
		else
		    return (pick);
	    case 3 :
		if (sbus->slot3) {
		    printf ("This SBus slot is already occupied by the device");
		    printf (" '%s'\n", sbus->device3);
		    break;
		}
		else
		    return (pick);
	    default :
		printf ("No such SBus slot\n");
	}

	getchar ();			/* go pass the EOF */

	PRINT_LINE;
    }

    program_abort ("user keeps entering the wrong SBus slot");
}

