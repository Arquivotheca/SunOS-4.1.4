#ifndef lint
static char sccsid[] = "@(#)hk_comm.c  1.1 94/10/31 SMI";
#endif

/*
 ****************************************************************
 *
 * hk_comm.c
 *
 *	@(#)hk_comm.c 13.1 91/07/22 18:29:19
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Communication routines used by the hawk simulators.
 *	This version decides which of 3 modes to run in based
 *	on an ENV variable set prior to calling hk_open.
 *
 *  6-Sep-90 Kevin C. Rushforth   Initial version created.
 *  3-Apr-91 Kevin C. Rushforth	  Added hk_ioctl.  Decommissioned HFE mode.
 * 18-Apr-91 Kevin C. Rushforth	  DLX mode is now the default.
 * 14-Aug-91 Chrsitopher Klein	  Stripped out all non-DLX code
 *
 ****************************************************************
 */

#include <stdio.h>

#include <sbusdev/gtmcb.h>
#include "hk_comm.h"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	hk_open
 *		hk_open_sim
 *		hk_open_dlx
 *	hk_ioctl
 *		hk_ioctl_sim
 *		hk_ioctl_dlx
 *	hk_mmap
 *		hk_mmap_sim
 *		hk_mmap_dlx
 *	hk_munmap
 *		hk_munmap_sim
 *		hk_munmap_dlx
 *	hk_vmalloc
 *		hk_vmalloc_sim
 *		hk_vmalloc_dlx
 *	hk_connect
 *		hk_connect_sim
 *		hk_connect_dlx
 *	hk_disconnect
 *		hk_disconnect_sim
 *		hk_disconnect_dlx
 *	hk_close
 *		hk_close_sim
 *		hk_close_dlx
 *
 *==============================================================
 */

#ifdef DEBUG
#define MSG(str) fprintf(stderr, "%s\n", str)
#else
#define MSG(str)
#endif

typedef enum {
    MODE_UNKNOWN,		/* Mode has not yet been set */
    MODE_SIM,			/* Use the simulator */
    MODE_HFE,			/* OBSOLETE: Use the HFE without GT driver */
    MODE_DLX,			/* Use the GT driver */
} mode_type;

/* Static variables */
static mode_type which = MODE_UNKNOWN;	/* Which mode to run in */

/* External function definitions */
extern char *getenv();

int hk_open_sim() {return;};
extern int hk_open_dlx();

int hk_ioctl_sim() {return;};
extern int hk_ioctl_dlx();

char * hk_mmap_sim() {return;};
extern char * hk_mmap_dlx();

int hk_munmap_sim() {return;};
extern int hk_munmap_dlx();

int hk_vmalloc_sim() {return;};
extern int hk_vmalloc_dlx();

int hk_connect_sim() {return;};
extern int hk_connect_dlx();

int hk_disconnect_sim() {return;};
extern int hk_disconnect_dlx();

void hk_close_sim() {return;};
extern void hk_close_dlx();



/*
 *--------------------------------------------------------------
 *
 * hk_open
 *
 *	Call the appropriate open routine, and remember which
 *	one we called for future reference.
 *
 * RETURN VALUE:
 *
 *	 0 = Success
 *	-1 = Failure
 *
 *--------------------------------------------------------------
 */

int
hk_open()
{
    char *name;

    if ((name = getenv("HK_COMM_MODE")) == NULL)
	name = "DLX";

    if (strcmp(name, "SIM") == 0) {
	which = MODE_SIM;
	MSG("hk_comm: opening connection to simulator");
	return (hk_open_sim());
    }
    else if (strcmp(name, "HFE") == 0) {
	fprintf(stderr, "hk_comm: HFE mode is no longer supported\n");
	return (-1);
    }
    else if (strcmp(name, "DLX") == 0) {
	which = MODE_DLX;
	MSG("hk_comm: opening connection to GT driver");
	return (hk_open_dlx());
    }
    else {
	fprintf(stderr, "hk_comm: I don't understand HK_COMM_MODE=%s\n", name);
	return (-1);
    }
} /* End of hk_open */



/*
 *--------------------------------------------------------------
 *
 * hk_ioctl
 *
 *	Call the appropriate ioctl routine.
 *
 *--------------------------------------------------------------
 */

int
hk_ioctl(request, arg)
    int request;			/* ioctl request */
    char *arg;				/* Argument to ioctl */
{
    switch (which) {
    case MODE_SIM:
	return (hk_ioctl_sim(request, arg));
    case MODE_DLX:
	return (hk_ioctl_dlx(request, arg));
    default:
	return (-1);
    }
} /* End of hk_ioctl */



/*
 *--------------------------------------------------------------
 *
 * hk_mmap
 *
 *	Call the appropriate mmap routine.
 *
 *--------------------------------------------------------------
 */

char *
hk_mmap(suggested_base_address, size)
    char *suggested_base_address;	/* ...as the name implies */
    unsigned size;			/* Needed size of shared memory */
{
    switch (which) {
    case MODE_SIM:
	return (hk_mmap_sim(suggested_base_address, size));
    case MODE_DLX:
	return (hk_mmap_dlx(suggested_base_address, size));
    default:
	return ((char *) NULL);
    }
} /* End of hk_mmap */



/*
 *--------------------------------------------------------------
 *
 * hk_munmap
 *
 *	Call the appropriate munmap routine.
 *
 *--------------------------------------------------------------
 */

int
hk_munmap(addr, size)
    char *addr;				/* Virtual address to be returned */
    unsigned size;			/* Size of memory */
{
    switch (which) {
    case MODE_SIM:
	return (hk_munmap_sim(addr, size));
    case MODE_DLX:
	return (hk_munmap_dlx(addr, size));
    default:
	return (-1);
    }
} /* End of hk_munmap */



/*
 *--------------------------------------------------------------
 *
 * hk_vmalloc
 *
 *	Call the appropriate vmalloc routine.
 *
 *--------------------------------------------------------------
 */

int
hk_vmalloc(address, size)
    char *address;			/* Starting address to "back up" */
    unsigned size;			/* Number of bytes to back up */
{
    switch (which) {
    case MODE_SIM:
	return (hk_vmalloc_sim(address, size));
    case MODE_DLX:
	return (hk_vmalloc_dlx(address, size));
    default:
	return (-1);
    }
} /* End of hk_vmalloc */



/*
 *--------------------------------------------------------------
 *
 * hk_connect
 *
 *	Call the appropriate connect routine.
 *
 *--------------------------------------------------------------
 */

int
hk_connect(pconnect)
    struct gt_connect *pconnect;	/* Pointer to gt connect info */
{
    switch (which) {
    case MODE_SIM:
	return (hk_connect_sim(pconnect));
    case MODE_DLX:
	return (hk_connect_dlx(pconnect));
    default:
	return (-1);
    }
} /* End of hk_connect */



/*
 *--------------------------------------------------------------
 *
 * hk_disconnect
 *
 *	Call the appropriate disconnect routine.
 *
 *--------------------------------------------------------------
 */

int
hk_disconnect()
{
    switch (which) {
    case MODE_SIM:
	return (hk_disconnect_sim());
    case MODE_DLX:
	return (hk_disconnect_dlx());
    default:
	return (-1);
    }
} /* End of hk_disconnect */



/*
 *--------------------------------------------------------------
 *
 * hk_close
 *
 *	Shut down hawk and return everything.
 *
 *--------------------------------------------------------------
 */

void
hk_close(delete_files)
    int delete_files;			/* Delete all shared memory files */
{
    switch (which) {
    case MODE_SIM:
	hk_close_sim(delete_files);
	return;
    case MODE_DLX:
	hk_close_dlx(delete_files);
	return;
    default:
	return;
    }
} /* End of hk_close */

/* End of hk_comm.c */
