/*
 *********************************************************************** *
 *
 * @(#)gtlib.c 1.1 94/10/31 21:05:05
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
static char sccsid[] = "@(#)gtlib.c 1.1 94/10/31 Copyr 1990 Sun Micro";
#endif

#include "gtprobe.h"

/*******************************/
/* close the GT hardware    */
/* returns -1 error, 0 success */
/*******************************/

int 
ta_close(handleptr)
    TA_HANDLE *handleptr;
{
    if (handleptr->status != FE_OPEN)
	return (-1);

/* mark the handle as closed */
    handleptr->status = 0;
    if (munmap((char *) handleptr->mapped_mem, HK_DEVICE_SIZE)) {
	return (-1);
    }
    if (close(handleptr->jetfefilehandle)) {
	return (-1);
    }

    free((char *) handleptr);
    return (0);
} /* End of ta_close */


/*********************************/
/* initialize the GT hardware */
/* returns -1 error, 0 success   */
/*********************************/

int 
ta_init(tahandle)
    TA_HANDLE *tahandle;
{
    unsigned smr;

/* for GT, the thing to do is set G0 = 0, FE_RESET = 1, & HK_RESET = 1.
   Hold both reset for one millisecond, then set both to zero. This 
   routine should only be called if you really want hawk-2 reset. We may need
   to reset the Hsa, but for now, do nothing */

    if (tahandle->status!=FE_OPEN) return(-1);
    smr = 2;
    ta_write(tahandle, &smr, sizeof(smr), MIA_ATU_SYNC_ADDR);
    usleep(100);	/* wait for any outstanding MIA cache flushes */
    smr = FE_RESET;
    ta_write(tahandle, &smr, sizeof(smr), MIA_HCR_ADDR);
    usleep(1000);
    smr = 0;	/* need to clear reset bit before anything else */
    ta_write(tahandle, &smr, sizeof(smr), MIA_HCR_ADDR);
    smr = FE_GO;  /* the GO bit should normally be ON, the i860 s/b in hold */
    ta_write(tahandle, &smr, sizeof(smr), MIA_HCR_ADDR);
    usleep(30000);
    return (0);
} /* End of ta_init */



/*
 *----------------------------------------------------------------------
 *
 * ta_read
 *
 *	Reads one or more words from the GT front end
 *
 * RETURN VALUE:
 *
 *	number of bytes read.
 *
 *----------------------------------------------------------------------
 */

int
ta_read(tahandle, buffptr, length, address)
    TA_HANDLE *tahandle;
    register int *buffptr;
    int length;
    int address;
{
    register int *srcptr;
    register int count;

    if (tahandle->status != FE_OPEN)
	return (FE_FAILURE);

#ifdef DEBUG
    fprintf(stderr, "ta_read: reading address 0x%08x, length (%x)\n", address, length);
#endif
    address -= FE_LBBASEADDR;
    if ((address >= (HK_DEVICE_SIZE >> 2)) || (address < 0)) {
        fprintf(stderr, "ta_read: Address out of range 0x%08X\n", address + FE_LBBASEADDR);
        return (FE_FAILURE);
    }
    if (length > 0) {
	length = length >> 2;
	srcptr = (int *) &tahandle->mapped_mem[address];
	count = length - 1;
	do {
	    tahandle->erraddr = address++;
	    *buffptr++ = *srcptr++;
	} while (--count != -1);
    }
    return (length << 2);
} /* End of ta_read */



/*
 *----------------------------------------------------------------------
 *
 * ta_write
 *
 *	Writes one or more words to the GT front end
 *
 * RETURN VALUE:
 *
 *	number of bytes written.
 *
 *----------------------------------------------------------------------
 */

int
ta_write(tahandle, buffptr, length, address)
    TA_HANDLE *tahandle;
    register int *buffptr;
    int length;
    int address;
{
    register int *destptr;
    register count;
    if (tahandle->status != FE_OPEN)
	return (FE_FAILURE);
#ifdef DEBUG
    fprintf(stderr, "ta_write: writing address 0x%08x, length (%x)\n", address, length);
#endif
    address -= FE_LBBASEADDR;
    if ((address >= (HK_DEVICE_SIZE >> 2)) || (address < 0)) {
        fprintf(stderr, "ta_read: Address out of range 0x%08X\n", address + FE_LBBASEADDR);
        return (FE_FAILURE);
    }
    if (length > 0) {
	length = length >> 2;
	destptr = (int *) &tahandle->mapped_mem[address];
	count = length - 1;
	do {
	    tahandle->erraddr = address++;
	    *destptr++ = *buffptr++;
	} while (--count != -1);
    }
    return (length << 2);
} /* End of ta_write */



/*
 *----------------------------------------------------------------------
 *
 * ms_read
 *
 *	Reads one or more words from host memory (or from GT).
 *
 * RETURN VALUE:
 *
 *	number of bytes read.
 *
 *----------------------------------------------------------------------
 */

int 
ms_read(tahandle, buffptr, size, msaddr)
    TA_HANDLE *tahandle;
    int *buffptr;
    int size;
    int msaddr;
{
    unsigned jetaddr;
    int nwords;
    unsigned offset;
    unsigned *srcptr;
    u_char *cptr;
    u_short *sptr;
 
    if ((msaddr >= (unsigned) tahandle->vaddr) &&
        (msaddr < ((unsigned) tahandle->vaddr + tahandle->vmsize)))
    {
#ifdef DEBUG
        fprintf(stderr, "ms_read: reading from locked down physical memory: 0x%.8X\n",
            msaddr);
#endif DEBUG
        nwords = (size + 3) / 4;
        offset = (msaddr - (unsigned) tahandle->vaddr) / 4;
        srcptr = &tahandle->vaddr[offset];
        while (nwords-- > 0)
            *buffptr++ = *srcptr++;
    }
    else if ((msaddr >= HK_HOSTBASEADDR) &&
        (msaddr < (HK_HOSTBASEADDR + HK_DEVICE_SIZE)))
    {
        switch(size) {
        case 0:
        case 3:
            fprintf(stderr, stderr, "ms_write: illegal size (%d)\n", size);
            return (0);
 
        case 1:
            cptr = (u_char *) ((int) tahandle->mapped_mem + msaddr -
                HK_HOSTBASEADDR);
#ifdef DEBUG
            fprintf(stderr,
                "ms_read (1 byte), addr = 0x%.8X\n", msaddr);
#endif DEBUG
            *buffptr = (unsigned) *cptr;
            return(size);
 
        case 2:
            msaddr &= ~0x1;
            sptr = (u_short *) ((int) tahandle->mapped_mem + msaddr -
                HK_HOSTBASEADDR);
#ifdef DEBUG
            fprintf(stderr,
                "ms_read (2 byte), addr = 0x%.8X\n", msaddr);
#endif DEBUG
            *buffptr = (unsigned) *sptr;
            return(size);
 
        default:
            jetaddr = (msaddr - HK_HOSTBASEADDR) >> 2;
#ifdef DEBUG
            fprintf(stderr, "ms_read: MS+ addr = 0x%.8X, LB addr = 0x%.8X\n",
            msaddr, jetaddr);
#endif DEBUG
            return (ta_read(tahandle, buffptr, size, jetaddr));
	}
    }
    else {
        fprintf(stderr, "ms_read: illegal address (0x%.8X)\n", msaddr);
        return (0);
    }
} /* End of ms_read */



/*
 *----------------------------------------------------------------------
 *
 * ms_write
 *
 *	Writes one or more words to Moonshine+ memory (or to GT).
 *
 * RETURN VALUE:
 *
 *	number of bytes written.
 *
 *----------------------------------------------------------------------
 */

int 
ms_write(tahandle, buffptr, size, msaddr)
    TA_HANDLE *tahandle;
    int *buffptr;
    int size;
    int msaddr;
{
    unsigned jetaddr;
    int nwords;
    unsigned offset;
    unsigned *dstptr;
    u_char *cptr;
    u_char cdata;
    u_short *sptr;
    u_short sdata;
 
    if ((msaddr >= (unsigned) tahandle->vaddr) &&
        (msaddr < ((unsigned) tahandle->vaddr + tahandle->vmsize)))
    {
        nwords = (size + 3) / 4;
        offset = (msaddr - (unsigned) tahandle->vaddr) / 4;
        dstptr = &tahandle->vaddr[offset];
        while (nwords-- > 0)
            *dstptr++ = *buffptr++;
    }
    else if ((msaddr >= HK_HOSTBASEADDR) &&
        (msaddr < (HK_HOSTBASEADDR + HK_DEVICE_SIZE)))
    {
        switch(size) {
        case 0:
        case 3:
            fprintf(stderr, stderr, "ms_write: illegal size (%d)\n", size);
            return (0);
 
        case 1:
            cptr = (u_char *) ((int) tahandle->mapped_mem + msaddr -
                HK_HOSTBASEADDR);
            cdata = (u_char) *buffptr;
#ifdef DEBUG
            fprintf(stderr,
                "ms_write (1 byte), addr = 0x%.8X, data = 0x%.2X\n",
                msaddr, cdata);
#endif DEBUG
            *cptr = cdata;
            return(size);
 
        case 2:
            msaddr &= ~0x1;
            sptr = (u_short *) ((int) tahandle->mapped_mem + msaddr -
                HK_HOSTBASEADDR);
            sdata = (u_short) *buffptr;
#ifdef DEBUG
            fprintf(stderr,
                "ms_write (2 byte), addr = 0x%.8X, data = 0x%.4X\n",
                msaddr, sdata);
#endif DEBUG
            *sptr = sdata;
            return(size);
 
        default:
            jetaddr = (msaddr - HK_HOSTBASEADDR) >> 2;
            return (ta_write(tahandle, buffptr, size, jetaddr));
	}
    }
    else {
        fprintf(stderr, "ms_write: illegal address (0x%.8X)\n", msaddr);
        return (0);
    }
} /* End of ms_write */

/*
 *----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------
 */
device_open (stat, handleptr, verbose)
int stat;
TA_HANDLE *handleptr;
int verbose;
{

#ifdef HFE_HARDWARE
    switch (stat) {
        case 1:         /* SBus Adapter is in SBus slot 1 */
                handleptr->jetfefilehandle = open(HK_DEVICE1_NAME, O_RDWR);
                if (handleptr->jetfefilehandle == -1) {
                    fprintf(stderr, "gtprobe opens: ");
                    perror(HK_DEVICE1_NAME);
                    free((char *) handleptr);
                    program_abort ("it cannot open the device requested.");
                }
                fprintf(stderr, "gtprobe opens: GT is at SBus slot 1\n");
                break;
        case 2:         /* SBus Adapter is in SBus slot 2 */
                handleptr->jetfefilehandle = open(HK_DEVICE2_NAME, O_RDWR);
                if (handleptr->jetfefilehandle == -1) {
                    fprintf(stderr, "gtprobe opens: ");
                    perror(HK_DEVICE2_NAME);
                    free((char *) handleptr);
                    program_abort ("it cannot open the device requested.");
                }
                fprintf(stderr, "gtprobe opens: GT is at SBus slot 2\n");
                break;
        case 3:         /* SBus Adapter is in SBus slot 3 */
                handleptr->jetfefilehandle = open(HK_DEVICE3_NAME, O_RDWR);
                if (handleptr->jetfefilehandle == -1) {
                    fprintf(stderr, "gtprobe opens: ");
                    perror(HK_DEVICE3_NAME);
                    free((char *) handleptr);
                    program_abort ("it cannot open the device requested.");
                }
                fprintf(stderr, "gtprobe opens: GT is at SBus slot 3\n");
                break;
        default:
                program_abort("there is no such SBus slot.");
    }
#else
    if ((handleptr->jetfefilehandle = open(HK_DEVICE_NAME, O_RDWR)) == -1) {
        fprintf(stderr, "gtprobe opens: ");
        perror(HK_DEVICE_NAME);
        free((char *) handleptr);
        program_abort ("it cannot open the device requested.");
    }
#endif

    if (verbose) fprintf(stderr, "mmap(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
                 0, HK_DEVICE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                 handleptr->jetfefilehandle, HK_DEVICE_BASE);

    if ((handleptr->mapped_mem =
        (int *) mmap(0, HK_DEVICE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
            handleptr->jetfefilehandle, HK_DEVICE_BASE)) == (int *) -1) {
        perror("gtprobe opens: GT device space");
        close(handleptr->jetfefilehandle);
        return (0);
    }

/* Initialize the physical memory parameters */
    handleptr->vmsize = 0;
    handleptr->paddr = (unsigned *) -1;
    handleptr->vaddr = (unsigned *) -1;
    handleptr->status = FE_OPEN;

}
