/*
 ***********************************************************************
 *
 * @(#)gtprobe.c 1.1 94/10/31 21:05:08
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Comment:
 *	This program enables user to probe for a gt-device whenever
 *	there is doubt of the communication between the device and 
 *	the host machine.
 *
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

int verbose;
int signal_handler();
TA_HANDLE *handleptr;
int timeout;
char *gt = "gt";

main(argc, argv)
int argc;
char *argv[];
{
    int stat;
    int i;
    int rd_back;
    char *v;
    SBus *sbus;

    signal (SIGSEGV, signal_handler);
    signal (SIGINT, signal_handler);
    signal (SIGBUS, signal_handler);

    /* check for the [-v] option */
    if (argc > 1){
	v = argv [1];
	if (*++v != 'v'){
	    printf ("Usage: gtprobe [-v]\n");
	    program_abort ("wrong option was entered.");
	}
	verbose = 1;
    }
    else
	verbose = 0;

    printf ("\nThis program must run on SunOS Release 4.1 or ");
    printf ("higher as superuser.\n");
    printf ("Moreover, it cannot have another program/process ");
    printf ("concurrently uses the GT device.\n\n");

    /* use the system command "devinfo" to find out what devices are
     * on the 3 SBus slots.  This command is only avail on 4.1 or greater.
     */

    /* get space for the new TA_HANDLE */
    handleptr = (TA_HANDLE *) malloc(sizeof(TA_HANDLE));
    if (handleptr  == 0) {
        fprintf(stderr, "Error allocating space for handleptr\n");
        program_abort ("it cannot malloc() system' memory.");
    }

    sbus = (SBus *) malloc (sizeof (SBus));

    device_info (sbus);

    /* check the GT_SLOT env variable and prompt user for any change */

    stat = gt_info(sbus);

    /* open the device and map in the address */

    device_open (stat, handleptr, verbose);

    testhsa (handleptr);	/* probe the SBus Adapter or HAC */

    sleep (2);			/* wait for the IO buffer to print */

    testmia (handleptr);	/* probe the MIA's Test Register */

    rp_xsum (handleptr);
 
    ta_close (handleptr);	/* done */
}

signal_handler ( sig, code, scp, addr )
int sig, code;
struct sigcontext *scp;
char *addr;

  {

    char ch[8];

        switch (sig) {
            case SIGINT :
                program_abort ("an interrupt was encountered.");
                break;

            case SIGSEGV :      /* this could be a bus error */
                printf ("\ngtprobe: ***** Seg Error Occurred ***** \n");
                /* no break, continue to the next case */

            case SIGBUS :       /* this should be a bus error */
                printf("\ngtprobe: Bus Error Occurred, LB addr = 0x%x",
                        handleptr->erraddr);
                printf(", SBus = 0x%x\n", handleptr->erraddr<<2);
                
		switch (timeout) {
		    case 1 :	/* SBus Adapter probing */
			program_abort ("the SBus Adapter is in the wrong SBus slot");
		    case 2 :	/* MIA Test Register probing */
			program_abort ("the GT is not connected, not powered, or a bad cable");
		    case 3 :	/* OpenBoot PROM probing */
			program_abort ("the RP is not connected, not powered, or a bad cable");
		    default :	/* Unknown cause */
			break;
		}

		break;
            default:
                break;
        }

	return (0);

/*      printf ("Enter 'q' to exit or <cr> to continue: ");
        gets (ch);
        if (ch [0] == 'q')
            exit (sig);
        else
            return(0);
*/

}
