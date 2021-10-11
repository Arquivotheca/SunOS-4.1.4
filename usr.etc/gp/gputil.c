#ifndef lint
static char sccsid[] = "@(#)gputil.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

/*
 * low level gp routines for loading microcode, restarting etc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/gp1reg.h>

#ifndef	MAP_FIXED
#define	MAP_FIXED	0	/* 3.x compatibility */
#endif

#define	ITEMSIN(array)	(sizeof (array) / sizeof (array)[0])


gp1_open(name, addr)
	char *name;
	caddr_t *addr;
{
	int fd;
	caddr_t p;
	char *valloc();
	caddr_t mmap();

	if ((fd = open(name, O_RDWR | O_NDELAY, 0)) < 0)
		error("cannot open device %s", name);

	if ((p = (caddr_t) valloc(GP2_SIZE)) == 0)
		error("out of memory (needed %dK)", GP1_SIZE / 1024);

	if (mmap(p, GP2_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_FIXED, fd, 0) == (caddr_t) -1 &&
		mmap(p, GP1_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_FIXED, fd, 0) == (caddr_t) -1)
		error("mmap failed for %s", name);

	*addr = p;
	return fd;
}

/* 
 * pump priming microcode
 *
 *	;		; 	cjp, go .; | jump to zero
 */
static short prime[] = {
	0x0068, 0x0000, 0x7140, 0x0000
};

gp1_reset(addr)
	caddr_t addr;
{
	register struct gp1reg *gpr = &((struct gp1 *) addr)->reg.reg;
	register short *p;

	/* halt GP */
	gpr->gpr_csr.control =
		GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET;
	gpr->gpr_csr.control = 0;

	/* load pipe-priming microcode */
	gpr->gpr_ucode_addr = 0;
	p = prime;
	PR_LOOP(ITEMSIN(prime), gpr->gpr_ucode_data = *p++);

	/* start the processors */
	gpr->gpr_csr.control = 0;
	gpr->gpr_csr.control = 
		GP1_CR_VP_STRT0 | GP1_CR_VP_CONT | 
		GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;

	/* halt them again */
	gpr->gpr_csr.control =
		GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET;
	gpr->gpr_csr.control = 0;

	/* clear out shared memory */
	p = (short *) (addr + GP1_SHMEM_OFFSET);
	PR_LOOP(GP1_SHMEM_SIZE / sizeof *p, *p++ = 0);

	((struct gp1 *) addr)->shmem.gp_alloc_h = 0x8000;
	((struct gp1 *) addr)->shmem.gp_alloc_l = 0x00ff;
}


gp1_start(addr)
	caddr_t addr;
{
	((struct gp1 *) addr)->reg.reg.gpr_csr.control = 0;
	((struct gp1 *) addr)->reg.reg.gpr_csr.control = 
		GP1_CR_VP_STRT0 | GP1_CR_VP_CONT | 
		GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;
}

/*
 * load microcode file
 * file format:
 *	short address
 *	short # of lines (8 bytes/line)
 *	data
 */

#define	UBUFSIZE	4096

gp1_load(gp, filename)
	caddr_t gp;
	char *filename;
{
	FILE *fp;

	register short *addr = &((struct gp1 *) gp)->reg.reg.gpr_ucode_addr;
	register short *data = &((struct gp1 *) gp)->reg.reg.gpr_ucode_data;
	register short *p;

	u_short hdr[2];
	int lines;

	short ucode[UBUFSIZE][4];

	if ((fp = fopen(filename, "r")) == NULL)
		error("cannot open microcode file %s", filename);

	while (fread((char *) hdr, sizeof hdr, 1, fp) == 1) {
		*addr = hdr[0];

		while (hdr[1] > 0) {
			if ((lines = hdr[1]) > UBUFSIZE)
				lines = UBUFSIZE;
			hdr[1] -= lines;

			if (fread((char *) ucode, sizeof ucode[0],
				lines, fp) != lines)
				break;

			p = ucode[0];
			PR_LOOP(lines, 
				*data = *p++;
				*data = *p++;
				*data = *p++;
				*data = *p++);
		}
	}

	if (ferror(fp))
		error("error reading microcode file %s", filename);

	(void) fclose(fp);
}


#define	TESTVAL		0x5a

gp1_sizemem(gp)
	caddr_t gp;
{
	register struct gp1reg *gpr = & ((struct gp1 *) gp)->reg.reg;
	register int size;

	for (size = 64; size > 0; size >>= 1) {
		gpr->gpr_ucode_addr = size << 10;
		gpr->gpr_ucode_data = size;
	}

	gpr->gpr_ucode_addr = 0;
	size = gpr->gpr_ucode_data & 255;

	gpr->gpr_ucode_addr = size << 9;
	gpr->gpr_ucode_data = TESTVAL;
	gpr->gpr_ucode_addr = size << 9;
	if ((gpr->gpr_ucode_data & 255) != TESTVAL)
		size >>= 1;

	return size;
}

/* 
 * Rip up the start of the microcode and retile with
 * physical addresses of the gp and bound color boards.
 */
/*ARGSUSED*/
gp1_fb_config(addr, gpaddr, fbaddr, nfb, gbunit)
	caddr_t addr;
	short gpaddr, *fbaddr;
	int nfb, gbunit;
{
	register struct gp1reg *gpr = &((struct gp1 *) addr)->reg.reg;
	register short *p;
	short ucode[24];

	/* rip up the microcode */
	gpr->gpr_ucode_addr = 4;
	p = ucode;
	PR_LOOP(24, *p++ = gpr->gpr_ucode_data);

	/* stuff in the addresses */
	ucode[3]  = gpaddr;
	ucode[7]  = fbaddr[0];
	ucode[11] = fbaddr[1];
	ucode[15] = fbaddr[2];
	ucode[19] = fbaddr[3];
	ucode[23] = gbunit;

	/* replace the microcode */
	gpr->gpr_ucode_addr = 4;
	p = ucode;
	PR_LOOP(24, gpr->gpr_ucode_data = *p++);
}
