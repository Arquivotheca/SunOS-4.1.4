#ifndef lint
static char sccsid[] = "@(#)gp2util.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1987-1989 by Sun Microsystems, Inc.
 */

/* GP2 support for gpconfig */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/gp1reg.h>

#define	ITEMSIN(array)	(sizeof (array) / sizeof (array)[0])

/*
 * a counter on each section of the microcode file to control on exactly
 * which section to load into GP2.
 */
static struct section {
    int             xp,
                    rp,
                    pp,
                    shmem;
}               sec_hdr = {

    (int) 'x', (int) 'r', (int) 'p', (int) 's'
};

gp2_usesection(xp, rp, pp, shmem)
    int             xp,
                    rp,
                    pp,
                    shmem;
{
    sec_hdr.xp = xp;
    sec_hdr.rp = rp;
    sec_hdr.pp = pp;
    sec_hdr.shmem = shmem;
}

gp2check(gp)
    caddr_t         gp;
{
    return (((struct gp2reg *) gp)->ident & GP2_ID_MASK) == GP2_ID_VALUE;
}

gp2_reset(addr)
    caddr_t         addr;
{
    register short *p;

    /* halt GP */
    ((struct gp1 *) addr)->reg.reg2.control = 0;

    /* clear out shared memory */
    p = (short *) (addr + GP1_SHMEM_OFFSET);
    PR_LOOP(GP1_SHMEM_SIZE / sizeof *p, *p++ = 0);

    ((struct gp1 *) addr)->shmem.gp_alloc_h = 0x8000;
    ((struct gp1 *) addr)->shmem.gp_alloc_l = 0x00ff;
}


gp2_start(addr)
    caddr_t         addr;
{
    ((struct gp1 *) addr)->reg.reg2.control = 0;
    ((struct gp1 *) addr)->reg.reg2.control =
	GP2_CR_XP_HLT | GP2_CR_RP_HLT;
    ((struct gp1 *) addr)->reg.reg2.control =
	GP2_CR_XP_HLT | GP2_CR_RP_HLT | GP2_CR_PP_HLT |
	GP2_CR_XP_RST | GP2_CR_RP_RST | GP2_CR_PP_RST;
}

/*
 * load microcode file file format: magic # ("GP2") { int type ('x', 'r',
 * 'p', 's') int addr, length char data[length] }
 */

#define	GP2_MAGIC	('G' << 24 | 'P' << 16 | '2' << 8)

gp2_load(gp, filename, debug)
    caddr_t         gp;
    char           *filename;
    int 	debug;
{
    FILE           *fp;
    int             load_shmem = 0;

    register struct gp2reg *gpr = &((struct gp1 *) gp)->reg.reg2;

    struct {
	int             type,
	                addr,
	                len;
    }               hdr;

    union {
	char            c[8];
	u_int           w[2];
    }               buf;
    struct section  small,
                    big;
    struct section  sec_count;

    register int    addr,
                    len;
    if ((fp = fopen(filename, "r")) == NULL)
	error("cannot open microcode file %s", filename);

    if (fread(buf.c, 4, 1, fp) != 1 ||
	buf.w[0] != GP2_MAGIC)
	error("invalid microcode file %s", filename);

    sec_count.xp = sec_count.pp = sec_count.rp = sec_count.shmem = 0;

    while (fread((char *) &hdr, sizeof hdr, 1, fp) == 1) {
	len = hdr.len;
	switch (hdr.type) {
	case 'X':
	    sec_count.xp++;
	    break;
	case 'P':
	    sec_count.pp++;
	    break;
	case 'S':
	    sec_count.shmem++;
	    break;
	case 'R':
	    sec_count.rp++;
	    break;
	}
	if (fseek(fp, hdr.len, L_INCR) < 0)
	    error("error seeking microcode file %s", filename);
    }

    if (fseek(fp, 4, L_SET) < 0)       /* rewind and skip magic */
	error("error seeking microcode file %s", filename);

    /* if the section we'd like to load is not there, use the generic ones */
    if (isupper((char) sec_hdr.xp) && !sec_count.xp)
	sec_hdr.xp = (int) tolower((char) sec_hdr.xp);
    if (isupper((char) sec_hdr.rp) && !sec_count.rp)
	sec_hdr.rp = (int) tolower((char) sec_hdr.rp);
    if (isupper((char) sec_hdr.pp) && !sec_count.pp)
	sec_hdr.pp = (int) tolower((char) sec_hdr.pp);
    if (isupper((char) sec_hdr.shmem) && !sec_count.shmem)
	sec_hdr.shmem = (int) tolower((char) sec_hdr.shmem);

    if (debug) {
	printf("gp2ucode_debug, use sections %c %c %c %c\n",
	       sec_hdr.xp, sec_hdr.rp, sec_hdr.pp, sec_hdr.shmem);
    }
    sec_count.xp = sec_count.pp = sec_count.rp = sec_count.shmem = 0;

    /*
     * Load sectioned microcode.  Shmem is loaded only preceeded by RP or XP
     * sections
     */
    while (fread((char *) &hdr, sizeof hdr, 1, fp) == 1) {
	addr = hdr.addr;
	len = hdr.len;
	if (debug) {
	    printf("gp2ucode_dubug: section %c addr %x length %d\n",
		   isalnum(hdr.type) ? hdr.type : '?', hdr.addr, hdr.len);
	}

	if (hdr.type == sec_hdr.xp) {  /* XP microcode */
	    len >>= 3;
	    while (--len >= 0) {
		if (fread(buf.c, 8, 1, fp) != 1)
		    break;
		gpr->xp_addr = addr++;
		gpr->xp_data_h = buf.w[0];
		gpr->xp_data_l = buf.w[1];
	    }
	    sec_count.xp++;
	    load_shmem = 1;
	}
	else if (hdr.type == sec_hdr.rp) {	/* RP microcode */
	    len >>= 2;
	    while (--len >= 0) {
		if (fread(buf.c, 4, 1, fp) != 1)
		    break;
		gpr->rp_addr = addr++;
		gpr->rp_data = buf.w[0];
	    }
	    sec_count.rp++;
	    load_shmem = 1;
	}
	else if (hdr.type == sec_hdr.pp) {	/* PP microcode */
	    len >>= 3;
	    while (--len >= 0) {
		if (fread(buf.c, 8, 1, fp) != 1)
		    break;
		gpr->pp_addr = addr++;
		gpr->pp_data_h = buf.w[0];
		gpr->pp_data_l = buf.w[1];
	    }
	    sec_count.pp++;
	    load_shmem = 0;
	}
	else if (load_shmem &&	       /* shared memory init */
		 (hdr.type == sec_hdr.shmem ||
		  (hdr.type & sec_hdr.shmem) == ('s' & 'S'))) {
	    if (fread((char *) gp + addr, 1, len, fp) == len)
		sec_count.shmem++;
	    load_shmem = 0;
	}
	else {
	    /* print out message area */
	    char           *read_in,
	                   *malloc();
	    if (debug)
		printf("\t\tsection not loaded\n");
	    if (hdr.type == 0 &&
		(read_in = malloc(len + 1)) &&
		fread(read_in, len, 1, fp) == 1) {
		read_in[len] = 0;
		printf("%s", read_in);
		(void) free(read_in);
	    }
	    else
		while (--len >= 0)
		    (void) getc(fp);
	    load_shmem = 0;
	}
    }


    if (ferror(fp))
	error("error reading microcode file %s", filename);

    /* Have we loaded all sections? */
    {
	static char    *fmt = "%s section microcode not loaded";
	int             if_exit = 0;
	if (sec_count.xp <= 0)
	    warning(fmt, "XP"), if_exit++;

	if (sec_count.rp <= 0)
	    warning(fmt, "RP"), if_exit++;

	if (sec_count.pp <= 0)
	    warning(fmt, "PP"), if_exit++;

	if (sec_count.shmem <= 0)
	    warning(fmt, "SHMEM"), if_exit++;

	if (if_exit)
	    error("error reading microcode file %s", filename);
    }

    (void) fclose(fp);
}
