#ifndef lint
static char sccsid[] = "@(#)hk_comm_dlx.c  1.1 94/10/31 SMI";
#endif

/*
 ****************************************************************
 *
 * hk_comm_dlx.c
 *
 *	@(#)hk_comm_dlx.c 13.1 91/07/22 18:29:23
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Communication routines used by the hawk programs.
 *
 *  6-Sep-90 Kevin C. Rushforth	  Initial version created.
 * 12-Oct-90 Kevin C. Rushforth	  Verify that the wrong kernel isn't used.
 * 15-Oct-90 Kevin C. Rushforth	  Fix to kernel verification routine.
 *  8-Feb-91 Kevin C. Rushforth	  Added hk_munmap() and hk_disconnect()
 * 15-Mar-91 Kevin C. Rushforth	  Removed check for _hawktbl symbol.
 * 17-Mar-91 Kevin C. Rushforth	  Print messages only when "HAWK_VERBOSE"
 *  3-Apr-91 Kevin C. Rushforth	  Added hk_ioctl.
 * 14-Aug-91 Christopher Klein    Changed gt{mcb,reg}.h to look in the system
 * 14-Aug-91 Christopher Klein    Changed #ifdef _sun4_ to #ifdef sparc
 *
 ****************************************************************
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <sbusdev/gtmcb.h>
#include <sbusdev/gtreg.h>

#ifdef sparc
#define GT_DEVICE_NAME	"/dev/gta0"



/*
 *==============================================================
 *
 * Procedure Hierarchy:
 *
 *	hk_open_dlx
 *		open
 *	hk_ioctl_dlx
 *		hk_connect_dlx
 *		hk_disconnect_dlx
 *		ioctl
 *	hk_mmap_dlx
 *		mmap
 *	hk_munmap_dlx
 *		munmap
 *	hk_vmalloc_dlx
 *		hk_ioctl_dlx
 *	hk_connect_dlx
 *		ioctl
 *	hk_disconnect_dlx
 *		ioctl
 *	hk_close_dlx
 *		close
 *
 *==============================================================
 */

/* Static variables */
static int gt_fd = -1;			/* File descriptor returned by GT */
static int connected = 0;		/* True if we are connected to Hawk */
static int hawk_verbose = 0;		/* True if we are verbose */

/* Forward and external function definitions */
extern char *getenv();



/*
 *--------------------------------------------------------------
 *
 * hk_open_dlx
 *
 *	Open the GT device driver.
 *
 * RETURN VALUE:
 *
 *	 0 = Success
 *	-1 = Failure
 *
 *--------------------------------------------------------------
 */

int
hk_open_dlx()
{
    if (getenv("HAWK_VERBOSE"))
	hawk_verbose = 1;

    if ((gt_fd = open(GT_DEVICE_NAME, O_RDWR)) < 0)
	return (-1);
    else
	return (0);
} /* End of hk_open_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_ioctl_dlx
 *
 *	Unmap in DLX memory of the specified size.
 *
 *--------------------------------------------------------------
 */

int
hk_ioctl_dlx(request, arg)
    int request;			/* ioctl request */
    char *arg;				/* Argument to ioctl */
{
    if (gt_fd == -1) {
	errno = EBADF;
	return (-1);
    }

    switch (request) {
    case FB_CONNECT:
	return (hk_connect_dlx((struct gt_connect *) arg));
    case FB_DISCONNECT:
	return (hk_disconnect_dlx());
    default:
	return (ioctl(gt_fd, request, arg));
    }
} /* End of hk_ioctl_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_mmap_dlx
 *
 *	Map in DLX memory of the specified size.
 *
 *--------------------------------------------------------------
 */

char *
hk_mmap_dlx(suggested_base_address, size)
    char *suggested_base_address;	/* ...as the name implies */
    unsigned size;			/* Needed size of shared memory */
{
    int flags;				/* Mapping flags */
    char *vmptr;			/* Pointer returned by mmap */

    if (gt_fd == -1) {
	errno = EBADF;
	return (NULL);
    }

    flags = MAP_SHARED;
    vmptr = (char *) mmap(suggested_base_address, size,
	PROT_READ | PROT_WRITE, flags, gt_fd, 0);

    if ((int) vmptr == -1) {
	if (hawk_verbose)
	    perror("hk_mmap: mmap");

	return (NULL);
    }

    return (vmptr);
} /* End of hk_mmap_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_munmap_dlx
 *
 *	Unmap in DLX memory of the specified size.
 *
 *--------------------------------------------------------------
 */

int
hk_munmap_dlx(addr, size)
    char *addr;				/* Virtual address to be returned */
    unsigned size;			/* Size of memory */
{
    if (gt_fd == -1) {
	errno = EBADF;
	return (-1);
    }

    return (munmap(addr, size));
} /* End of hk_munmap_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_vmalloc_dlx
 *
 *	Force the memory assosiated with the specified address
 *	range to be "backed up" by space in the file system.
 *
 *	This routine is only include for backward compatibility.
 *	Users should *really* use hk_ioctl() for this functionality.
 *
 *--------------------------------------------------------------
 */

int
hk_vmalloc_dlx(address, size)
    char *address;			/* Starting address to "back up" */
    unsigned size;			/* Number of bytes to back up */
{
    struct fb_vmback back;

    back.vm_addr = (caddr_t) address;
    back.vm_len = size;
    return (hk_ioctl_dlx(FB_VMBACK, (char *) &back));
} /* End of hk_vmalloc_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_connect_dlx
 *
 *	Get pointer to user MCB and connect to the kernel MCB.
 *	The user may now use Hawk.  The user had better make sure
 *	all pointers and command bits are set properly.
 *
 *--------------------------------------------------------------
 */

int
hk_connect_dlx(pconnect)
    struct gt_connect *pconnect;	/* Pointer to dlx connect info */
{
    int status;

    if (gt_fd == -1) {
	errno = EBADF;
	return (-1);
    }

    if (connected) {
	if (hawk_verbose)
	    fprintf(stderr, "hk_connect cannot be called more than once\n");

	errno = EINVAL;
	return (-1);
    }

    if ((status = ioctl(gt_fd, FB_CONNECT, pconnect)) == 0)
	connected = 1;

    return (status);
} /* End of hk_connect_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_disconnect_dlx
 *
 *	Stop using the Hawk.
 *
 *--------------------------------------------------------------
 */

int
hk_disconnect_dlx()
{
    if (connected) {
	connected = 0;
	return (ioctl(gt_fd, FB_DISCONNECT, 0));
    }
    else
	return (0);
} /* End of hk_connect_dlx */



/*
 *--------------------------------------------------------------
 *
 * hk_close_dlx
 *
 *	Close connection to Hawk.
 *
 *--------------------------------------------------------------
 */

void
hk_close_dlx(delete_files)
    int delete_files;			/* Delete all shared memory files */
{
    (void) hk_disconnect_dlx();
    (void) close(gt_fd);
} /* End of hk_close */

#else !sun4
/* This set of routines is not available on a sun3 */
int
hk_open_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return (-1);
}

int
hk_ioctl_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return (-1);
}

char *
hk_mmap_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return ((char *) NULL);
}

int
hk_munmap_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return (-1);
}

int
hk_vmalloc_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return (-1);
}

int
hk_connect_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return (-1);
}

int
hk_disconnect_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
    return (-1);
}

void
hk_close_dlx()
{
    fprintf(stderr,
	"The DLX version of libhcom.a is not available on a sun3\n");
}
#endif !sun4

/* End of hk_comm_dlx.c */
