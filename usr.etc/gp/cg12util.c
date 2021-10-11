#ifndef lint
static char sccsid[] = "@(#)cg12util.c 1.1 94/10/31 SMI";
#endif

/*
 * cg12util.c
 */

#include <stdio.h>
#include "coff.h"
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sun/fbio.h>
#include <sbusdev/cg12reg.h>
#include <pixrect/gp1reg.h>

#define INIT_DELAY	600000
#define CG12DEBUG

#ifdef CG12DEBUG
int cg12_debug = 1;
#define DEBUGF(level, args)	_STMT(if (cg12_debug >= (level)) printf args;)
#else CG12DEBUG
#define DEBUGF(level, args) 
#endif CG12DEBUG
 
int dev_id;

cg12check(fd)
    int fd;
{
    DEBUGF(3, ("cg12check: fd:%d\n",fd));

    if(ioctl(fd, FBIO_DEVID, &dev_id) < 0) 
    {
    	DEBUGF(2, ("cg12check: FBIO_DEVID ioctl failed\n"));
	return 0;
    }

    DEBUGF(3, ("cg12check: dev_id:%d\n",dev_id));

    return dev_id;
}

cg12_reset(fd, dsphalt)
    int fd;
    int dsphalt;
{
    int err;

    DEBUGF(3, ("cg12_reset: fd:%d,dsphalt:%d\n",fd,dsphalt));

    if((err=ioctl(fd, FBIO_U_RST, &dsphalt)) < 0 )
	DEBUGF(1, ("could not reset DSP\n"));

    return err;
}

cg12_load(fd, filename, debug, noclr)
    int fd;
    char *filename;
    int debug;
    int noclr;
{
    FILE *coffin, *coffout;
    COFF_HEADER coffhead;
    OPT_HEADER opthead;
    SECTION_HEADER secthead[MAX_SECTIONS];
    unsigned int *buffer, *ptr, *dest;
    int i, j, k;
    int length;
    int mmapsize = CG12_DRAM_SIZE;
    int regs; 
    caddr_t va;
    u_int *va_dram;
    u_int *va_vram;
    int cur_psn;
    u_int tmp;
    struct cg12_ctl_sp *cg12_ctl;
    u_int fbreg[5];

    /* open the input file */
    if ((coffin = fopen (filename, "rb")) == NULL) 
    {
        DEBUGF(1, ("cannot open '%s' for read\n", filename));
        return (-1);
    }
 
    /* read COFF file header */
    fread (&coffhead, COFF_HDR_SIZE, 1, coffin);

    if (coffhead.magic != C30_MAGIC) 
    {
        DEBUGF(1, ("%s is not a valid COFF format\n", filename));
	return (-1);
    }

    /* Read optional header. */
    if (coffhead.optheadlen == OPT_HDR_SIZE) 
        fread (&opthead, coffhead.optheadlen, 1, coffin);

    /* validate header info */
    if ((opthead.magic != C30_OPT_MAGIC) ||
        (coffhead.optheadlen != OPT_HDR_SIZE)) 
    {

	DEBUGF(1, ("%s is not a valid COFF file, bad optional header\n",
	filename));

	return (-1);
    }

    /* read no of sections */
    if( coffhead.nheaders > MAX_SECTIONS) 
    {
	DEBUGF(1, ("too many sections\n"));

	return(-1);
    }

    for (i = 0; i < coffhead.nheaders; i++) 
    {
	fread (&(secthead[i]), SECTION_HDR_SIZE, 1, coffin);

        DEBUGF(4, ("section:  %8s\n", secthead[i].name));
    }

    cur_psn = ftell(coffin);

    if (dev_id > 2)
        regs = CG12_VOFF_DRAM_HR;
    else
        regs = CG12_VOFF_DRAM;

    if ((va = mmap((caddr_t) 0, mmapsize, PROT_READ|PROT_WRITE, 
	MAP_SHARED|_MAP_NEW, fd, regs)) == (caddr_t) -1) 
    {

	DEBUGF(1, ("mmap failed\n"));

	return (-1);

    } else
	DEBUGF(3, ("mmap returned 0x%x\n", va));

    va_dram = (u_int *)va;

    /* clear out all of dram first */
    if (!noclr)
    {
	register unsigned int *mp = va_dram;
	register unsigned int sz;
	
	for (sz = 0; sz < CG12_DRAM_SIZE; mp += 8, sz += (4*8)) {
	    mp[0] = 0; mp[1] = 0; mp[2] = 0; mp[3] = 0;
	    mp[4] = 0; mp[5] = 0; mp[6] = 0; mp[7] = 0;
	}
	for (; sz < CG12_DRAM_SIZE; sz += 4)
	    *mp++ = 0;
    }

    if (dev_id > 2)
    {
        regs = CG12_VOFF_COLOR24_HR;
        mmapsize = CG12_COLOR24_SIZE_HR;
    } else
    {
        regs = CG12_VOFF_COLOR24;
        mmapsize = CG12_COLOR24_SIZE;
    }

    if ((va = mmap((caddr_t) 0, mmapsize, PROT_READ|PROT_WRITE, 
	MAP_PRIVATE|_MAP_NEW, fd, regs)) == (caddr_t) -1) 
    {

	DEBUGF(1, ("mmap failed\n"));

	return (-1);

    } else
	DEBUGF(3, ("mmap returned 0x%x\n", va));

    va_vram = (u_int *)va;

    if (dev_id > 2)
	regs = CG12_VOFF_CTL_HR;
    else
	regs = CG12_VOFF_CTL;

    mmapsize = 0x2000;

    if ((va = mmap((caddr_t) 0, mmapsize, PROT_READ|PROT_WRITE, 
	MAP_PRIVATE|_MAP_NEW, fd, regs)) == (caddr_t) -1) 
    {

	DEBUGF(1, ("mmap failed\n"));

	return (-1);

    } else
	DEBUGF(3, ("mmap returned 0x%x\n", va));

    cg12_ctl = (struct cg12_ctl_sp *)va;

    fseek (coffin, cur_psn, SEEK_SET);

    fbreg[0] = cg12_ctl->apu.hpage;
    fbreg[1] = cg12_ctl->apu.haccess;
    fbreg[2] = cg12_ctl->dpu.pln_sl_host;
    fbreg[3] = cg12_ctl->dpu.pln_wr_msk_host;
    fbreg[4] = cg12_ctl->dpu.pln_rd_msk_host;

    /* read and process the data for each section. */
    for (i = 0; i < coffhead.nheaders; i++) 
    {

	if (secthead[i].data_off && !(secthead[i].flags & F_DSECT) &&
	    !(secthead[i].flags & F_NOLOAD)) 
	{

	    length = secthead[i].sizelongs;

	    DEBUGF(2, ("section %x:%8s, words:%x, offset:%x, paddr:%x,",
		i,secthead[i].name, secthead[i].sizelongs, secthead[i].data_off,
		secthead[i].physaddr));

	    cur_psn = ftell(coffin);

	    /* take care of c30 complier bug */
	    if (!strncmp(secthead[i].name, "ram3",4))
	    	dest = va_dram + 0x3e000;
	    else if (!strncmp(secthead[i].name, "ram2",4))
	    	dest = va_dram + 0x3c000;
	    else if (!strncmp(secthead[i].name, "ram1",4))
	    	dest = va_dram + 0x3a000;
	    else if (!strncmp(secthead[i].name, "ram0",4))
	    	dest = va_dram + 0x38000;
	    else if (secthead[i].physaddr >= 0x809800)
	    	dest = va_dram + 0x37800 + (secthead[i].physaddr & 0x7ff);
	    else if (secthead[i].physaddr > CG12_DRAM_SIZE) {
		if (dev_id > 2)
		    cg12_ctl->apu.hpage = CG12_HPAGE_24BIT_HR;
		else
		    cg12_ctl->apu.hpage = CG12_HPAGE_24BIT;

		cg12_ctl->apu.haccess = CG12_HACCESS_24BIT;
		cg12_ctl->dpu.pln_sl_host = CG12_PLN_SL_24BIT;
		cg12_ctl->dpu.pln_wr_msk_host = CG12_PLN_WR_24BIT;
		cg12_ctl->dpu.pln_rd_msk_host = CG12_PLN_RD_24BIT;

		dest = va_vram + (secthead[i].physaddr & 0xfffff)  ;
	    }
	    else
	    	dest = va_dram + secthead[i].physaddr;

	    DEBUGF(2, (" loaded at: %x\n",dest));

	    fseek (coffin, secthead[i].data_off, SEEK_SET);
	    buffer = (unsigned int *) malloc(sizeof(int)*length);
	    j = fread (buffer, 4, length, coffin);
	    ptr = buffer;

	    for(k =0; k <j; k++) 
	    {
	    	tmp = *ptr;
		*dest = ((tmp & 0x000000ff) << 24) | ((tmp & 0x0000ff00) << 8) |
		    ((tmp & 0x00ff0000) >> 8) | ((tmp & 0xff000000) >> 24);
		dest++;
		ptr++;
	    }

	    free(buffer);
	    fseek(coffin,cur_psn,SEEK_SET);

	}
	else
	{
	    DEBUGF(2, ("section %x:%8s, words:%x, offset:%x, paddr:%x\n",
		i,secthead[i].name, secthead[i].sizelongs, secthead[i].data_off,
		secthead[i].physaddr));
	}
    }

    *va_dram = opthead.addrexec;
    fclose (coffin);

    cg12_ctl->apu.hpage = fbreg[0];
    cg12_ctl->apu.haccess = fbreg[1];
    cg12_ctl->dpu.pln_sl_host = fbreg[2];
    cg12_ctl->dpu.pln_wr_msk_host = fbreg[3];
    cg12_ctl->dpu.pln_rd_msk_host = fbreg[4];

    return (0);
}

cg12_ucode_ver(fd)
    int fd;
{
    int mmapsize = CG12_SHMEM_SIZE;
    int regs;
    caddr_t va;
    struct gp1_shmem *shmem;

    if (dev_id > 2)
	regs = CG12_VOFF_SHMEM_HR;
    else
	regs = CG12_VOFF_SHMEM;

    if ((va = mmap((caddr_t) 0, mmapsize, PROT_READ|PROT_WRITE,
	MAP_SHARED|_MAP_NEW, fd, regs)) == (caddr_t) -1)
    {
	DEBUGF(1, ("mmap failed\n"));
 
	return (-1);
    } else
	DEBUGF(3, ("mmap returned 0x%x\n", va));
 
    usleep(INIT_DELAY);

    shmem = (struct gp1_shmem *)va;
 
    DEBUGF(1, ("GS Microcode %d.%d.%d\n",shmem->ver_release >> 8,
	shmem->ver_release & 0xff, shmem->ver_serial >> 8));

    DEBUGF(1, ("Copyright 1990, Sun Microsystems Inc.\n"));
}
