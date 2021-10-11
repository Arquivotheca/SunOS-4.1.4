#ifndef lint
static	char sccsid[] = "@(#)ip.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Driver for Interphase SMD-2180 controller.
 */
#include "ip.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>

#include <machine/psl.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/mbvar.h>
#include <sundev/ipreg.h>


int ipprobe(), ipslave(), ipattach(), ipgo(), ipdone(), ippoll();

struct	mb_ctlr *ipcinfo[NIPC];
struct	mb_device *ipdinfo[NIP];
struct mb_driver ipcdriver = {
	ipprobe, ipslave, ipattach, ipgo, ipdone, ippoll, 
	sizeof (struct ipdevice), "ip", ipdinfo, "ipc", ipcinfo, MDR_BIODMA,
};

#define	NUNIT		8		/* max # of units per controller */
#define	NLPART		NDKMAP		/* # of logical partitions (8) */
#define	UNIT(dev)	((dev>>3) % NUNIT)
#define	LPART(dev)	(dev % NLPART)

#define	SECSIZE	512

/*
 * Definition of a unit
 * (A unit is a physical partition of a drive,
 * either the whole drive or the fixed or cartridge
 * part of a Lark or the fixed head vs. moving head
 * areas of a Fujitsu).
 * Units with the 4 bit on are the funny ones.
 */
struct	unit {
	struct	dk_map un_map[NLPART];	/* logical partitions */
	char	un_present;		/* physical partition is present */
	struct	dk_geom un_g;		/* disk geometry */
	struct	buf un_utab;		/* for queuing per unit */
	struct	buf un_rtab;		/* for raw I/O */
	struct	buf un_sbuf;		/* for special commands */
	u_char	un_scmd;		/* the special command */
	u_char	un_serr;		/* the error from special command */
	u_char	un_sasync;		/* noone is waiting for scmd */
	int	un_ltick;		/* last time active */ 
	struct	mb_device *un_md;	/* generic unit */
	struct	mb_ctlr *un_mc;		/* generic controller */
} ipunits[NIP];

#define	FUNNYBIT	4
#define	FUNNY(u)	((u)&FUNNYBIT)
#define	NORMAL(u)	((u)&~FUNNYBIT)

#define	MAXHEAD		4	/* max heads to search for a label */

/*
 * Data per controller
 */
struct	ctlr {
	struct unit	*c_units[NUNIT];/* units on controller */
	struct ipdevice	*c_io;		/* ptr to I/O space data */
	caddr_t		c_iopb;		/* ptr to IOPB */
	short		c_type;		/* controller type */
	char		c_present;	/* controller is present */
	/* current transfer: */
	int		c_baddr;	/* physical buffer address */
	daddr_t		c_blkno;	/* current block */
	short		c_nsect;	/* block(sector) count */
	short		c_cmd;		/* current command */
	short		c_retries;	/* retry count */
	short		c_restores;	/* restore count */
	char		c_wantint;	/* expecting interrupt */
} ipctlrs[NIPC];

#define	IP_R0(c)	c->c_io->ip_r0
#define	IP_R1(c)	c->c_io->ip_r1
#define	IP_R2(c)	c->c_io->ip_r2
#define	IP_R3(c)	c->c_io->ip_r3

/*
 * Error message control
 */
#define	EL_RETRY	3
#define	EL_REST		2
#define	EL_FAIL		1

int iperrlvl = EL_RETRY;

#define	IPERROR(err)	iperrors[err-0x10]

struct	error {
	char	e_retry;	/* # of times to retry */
	char 	e_restore;	/* # of times to restore after retries */
	char	*e_name;	/* error message */
} iperrors[] = {
	0,	1,	"disk not ready",			/* 0x10 */
#define	E_NOTREADY 	0x10
	0,	0,	"invalid disk address",			/* 0x11 */
	1,	8,	"seek error",				/* 0x12 */
	3,	1,	"checksum error -- data field",		/* 0x13 */
	0,	0,	"invalid command code",			/* 0x14 */
#define	E_INVCMD	0x14
	0,	1,	"invalid track in IOPB",		/* 0x15 */
	1,	1,	"invalid sector in command",		/* 0x16 */
	1,	1,	"?",					/* 0x17 */
	5,	1,	"bus timeout",				/* 0x18 */
	3,	1,	"write error",				/* 0x19 */
	0,	1,	"disk write protected",			/* 0x1a */
#define	E_NOTSEL 0x1B
	0,	3,	"unit not selected",			/* 0x1b */
	3,	1,	"no address mark -- header field",	/* 0x1c */
	3,	1,	"no data mark -- data field",		/* 0x1d */
#define	E_UFAULT 0x1E
	0,	3,	"unit fault",				/* 0x1e */
	3,	1,	"data overrun timeout",			/* 0x1f */
	0,	1,	"surface overrun",			/* 0x20 */
	3,	1,	"id field error -- wrong sector read",	/* 0x21 */
	3,	1,	"id field or ECC error",		/* 0x22 */
	1,	1,	"?",					/* 0x23 */
	1,	1,	"?",					/* 0x24 */
	1,	1,	"?",					/* 0x25 */
	3,	8,	"no sector pulse",			/* 0x26 */
	3,	8,	"data overrun",				/* 0x27 */
	3,	8,	"no index pulse on write format",	/* 0x28 */
	3,	1,	"sector not found",			/* 0x29 */
	3,	8,	"id field error -- wrong head",		/* 0x2a */
	3,	8,	"invalid sync in data field",		/* 0x2b */
	3,	8,	"invalid sync in header field",		/* 0x2c */
	1,	8,	"seek timeout error",			/* 0x2d */
	1,	8,	"busy timeout",				/* 0x2e */
	1,	8,	"not on-cylinder at beginning of a seek",/* 0x2f */
	0,	3,	"rtz timeout",				/* 0x30 */
	3,	8,	"format overrun on data",		/* 0x31 */
#define	E_UNKNOWN	0x32
	1,	1,	"unknown error",			/* 0x32 */
};

#define	b_cylin b_resid

#ifdef INTRLVE
daddr_t dkblock();
#endif

int	ipwstart, ipwatch();		/* Have started guardian */
int	ipticks;			/* timer for guardian */
#define	IPTIMO	20			/* time until disk check */

/*
 * Determine existence and type of controller
 */
ipprobe(reg, ctlr)
	caddr_t reg;
{
	register struct ctlr *c = &ipctlrs[ctlr];
	register struct iopb0 *ip0;
	register int nms, piopb;

	c->c_io = (struct ipdevice *)reg;
	if (pokec((char *)&IP_R0(c), (char)0))
		return (0);

	c->c_iopb = (caddr_t)rmalloc(iopbmap, (long)IPIOPBSZ);
	if (c->c_iopb == NULL) {
		printf("ipprobe: no iopb space\n");
		return (0);
	}

	c->c_present = 1;

	/* determine controller type */
	clrint(c);
	DELAY(1000);
	bzero(c->c_iopb, IPIOPBSZ);
	ip0 = (struct iopb0 *)c->c_iopb;
	ip0->i0_cmd = IP_RESET;
	ip0->i0_ioaddr = ((int)c->c_io - (int)mbio) & 0xFF;
	piopb = (char *)c->c_iopb - DVMA;	/* cntlr gets phys addr */
	IP_R1(c) = piopb >> 16;
	IP_R2(c) = (piopb >> 8) & 0xFF;
	IP_R3(c) = piopb & 0xFF;
	IP_R0(c) = IP_GO;
	c->c_type = DKC_UNKNOWN;
	for (nms = 0; nms < 100; nms++) {
		DELAY(1000);
		if (ip0->i0_status == IP_OK) {
			c->c_type = DKC_SMD2180; 
			break;
		}
	}
	clrint(c);
	if (c->c_type == DKC_UNKNOWN) {
		printf("ipc%d: Unknown controller type at mbio 0x%x\n",
		    ctlr, reg - mbio);
		rmfree(iopbmap, (long)IPIOPBSZ, (long)c->c_iopb);
		c->c_present = 0;
		return (0);
	}
	return (sizeof (struct ipdevice));
}

/*
 * See if a slave unit exists
 * Since the Interphase controller won't 
 * tell us we always say yes.
 */
/*ARGSUSED*/
ipslave(md, reg)
	struct mb_device *md;
	caddr_t reg;
{

	return (1);
}

ipattach(md)
	register struct mb_device *md;
{
	register struct ctlr *c = &ipctlrs[md->md_ctlr];
	register struct unit *un = &ipunits[md->md_unit];
	struct dk_label *l;
	int head, i;
	int unit = md->md_slave;

	if (ipwstart == 0) {
		timeout(ipwatch, (caddr_t)0, hz);
		ipwstart++;
	}
	c->c_units[md->md_slave] = un;
	un->un_md = md;
	un->un_mc = md->md_mc;

	/* Allocate space for label in Mainbus memory */
	l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
	if (l == NULL) {
		printf("ipattach: no space for disk label\n");
		un->un_present = 0;
		return;
	}

	/* 
 	 * Initialize unit (physical drive)
	 */
	un->un_g.dkg_ncyl = 1;
	un->un_g.dkg_bcyl = 0;
	un->un_g.dkg_nhead = MAXHEAD;
	un->un_g.dkg_bhead = 0;
	un->un_g.dkg_nsect = 2;
	un->un_g.dkg_intrlv = 0;
	un->un_g.dkg_gap1 = 20;
	un->un_g.dkg_gap2 = 22;
	if (simple(c, unit, IP_RESTORE) != 0) {
		un->un_present = 0;
		return;
	}

	/* Search for a label */
	for (head = 0; head < MAXHEAD; head++) {
		l->dkl_magic = 0;	/* clear from previous try */
		if (getlabel(c, unit, head, l) == 0)
			continue;
		if (islabel(un - ipunits, unit, head, l) == 0)
			continue;
		uselabel(un, l);
		break;
	}
	rmfree(iopbmap, (long)SECSIZE, (long)l);

	if (!un->un_present) {
		for (i=0; i<NLPART; i++) {
			un->un_map[i].dkl_cylno = 0;
			un->un_map[i].dkl_nblk = 0;
		}
	}
}

static
uselabel(un, l)
	register struct unit *un;
	register struct dk_label *l;
{
	int i, intrlv;

	printf("ip%d: <%s>\n", un - ipunits, l->dkl_asciilabel);
	un->un_present = 1;
	un->un_g.dkg_ncyl = l->dkl_ncyl;
	un->un_g.dkg_bcyl = 0;	/* for now */
	un->un_g.dkg_nhead = l->dkl_nhead;
	un->un_g.dkg_bhead = l->dkl_bhead;
	un->un_g.dkg_nsect = l->dkl_nsect;
	un->un_g.dkg_gap1 = l->dkl_gap1;
	un->un_g.dkg_gap2 = l->dkl_gap2;
	un->un_g.dkg_intrlv = l->dkl_intrlv;
	for (i = 0; i < NLPART; i++)
		un->un_map[i] = l->dkl_map[i]; /* struct assignment */
	if (un->un_md->md_dk >= 0) {
		intrlv = un->un_g.dkg_intrlv;
		if (intrlv <= 0 || intrlv >= un->un_g.dkg_nsect)
			intrlv = 7;
		dk_bps[un->un_md->md_dk] = SECSIZE*60*un->un_g.dkg_nsect/intrlv;
	}
}

static
getlabel(c, unit, head, l)
	register struct ctlr *c;
	register struct dk_label *l;
{
	int error, retries, restores;
	register int ipn = (c->c_units[unit] - ipunits);

	l->dkl_magic = 0;
	clrint(c);
	restores = 0;
	do {
		retries = 0;
		do {
			ipcmd(c, IP_READ, (char *)l-DVMA, unit, 0, head, 0, 1);
			error = ipwait(c);
		} while (error && retries++ < IPERROR(error).e_retry);
	} while (error && restores++ < IPERROR(error).e_restore);
	if (error) {
		printf("ip%d: error %x reading label on head %d: %s\n", 
			ipn, error, head, IPERROR(error).e_name);
		return (0);
	}
	return (1);
}

ipwait(c)
	register struct ctlr *c;
{
	register struct iopb0 *ip0;

	while (IP_R0(c) & IP_BUSY)
		DELAY(30);
	ip0 = (struct iopb0 *)c->c_iopb;
	while (ip0->i0_status==IP_DBUSY || ip0->i0_status==0)
		DELAY(30);
	clrint(c);
	if (ip0->i0_status == IP_ERROR)
		return (ip0->i0_error);
	return (0);
}

static
islabel(ipn, unit, head, l)
	register struct dk_label *l;
{

	if (l->dkl_magic != DKL_MAGIC)
		return (0);
	if (!ck_cksum(l)) {
		printf("ip%d: Corrupt label on head %d\n", ipn, head);
		return (0);
	}
	if (head != l->dkl_bhead) {
		printf("ip%d: Misplaced label on head %d\n", ipn, head);
		return (0);
	}
	if (l->dkl_ppart != 0 && l->dkl_ppart != 1) {
		printf("ip%d: Unsupported phys partition # %d\n", 
			ipn, l->dkl_ppart);
		return (0);
	}
	if (FUNNY(unit) && !l->dkl_ppart)
		return (0);
	if (l->dkl_ppart && !FUNNY(unit))
		return (0);
	return (1);
}

/*
 * Check the checksum of the label
 */
static
ck_cksum(l)
	struct dk_label *l;
{
	short *sp, sum = 0;
	short count = sizeof(struct dk_label)/sizeof(short);

	sp = (short *)l;
	while (count--) 
		sum ^= *sp++;
	return (sum ? 0 : 1);
}

/*
 * Give a simple command to a controller
 * and spin until done.
 * Returns the error number or zero
 */
static
simple(c, unit, cmd)
	struct ctlr *c;
{

	clrint(c);
	ipcmd(c, cmd, 0, unit, 0, 0, 0, 0);
	return (ipwait(c));
}

/*
 * Get rid of interrupt condition
 * Sets clear bit and waits a while for it to take effect
 */
static
clrint(c)
	struct ctlr *c;
{

	while (IP_R0(c) & IP_BUSY)
		DELAY(30);
	IP_R0(c) = IP_CLRINT;
	DELAY(30);
}

ipcmd(c, cmd, dmaddr, unit, cylinder, head, sector, secnt) 
	struct ctlr *c;
{
	register int piopb;
	register struct iopb0 *ip0;

	unit = 1 << (unit&3);
	piopb = (char *)c->c_iopb - DVMA;	/* cntlr gets phys addr */

	while (IP_R0(c) & IP_BUSY)
		DELAY(30);
	ip0 = (struct iopb0 *)c->c_iopb;
	ip0->i0_cmd = cmd;
	ip0->i0_status = 0;
	ip0->i0_error = 0;
	ip0->i0_unit_cylhi = (unit << 4) | ((cylinder>>8) & 0x0F);
	ip0->i0_cylinder = cylinder & 0xFF;
	ip0->i0_sector = sector;
	ip0->i0_secnt = secnt;
	ip0->i0_buf_xmb = ((dmaddr >> 16) & 0xF) | IP_BUS | IP_REL;
	ip0->i0_buf_lsb = dmaddr & 0xFF;
	ip0->i0_buf_msb = (dmaddr>>8) & 0xFF;
	ip0->i0_head = head;
	ip0->i0_ioaddr = ((int)c->c_io - (int)mbio) & 0xFF;
	ip0->i0_burstlen = IP0_BURSTLEN;
	ip0->i0_nxt_xmb = IP_BUS | IP_REL;
	ip0->i0_nxt_lsb = 0;
	ip0->i0_nxt_msb = 0;
	ip0->i0_seg_lsb = 0;
	ip0->i0_seg_msb = 0;
	/* point controller at iopb and start it up */
	IP_R1(c) = (piopb >> 16) | IP_BUS;
	IP_R2(c) = (piopb >> 8) & 0xFF;
	IP_R3(c) = piopb & 0xFF;
	IP_R0(c) = IP_GO;
}

/*ARGSUSED*/
ipopen(dev, flag)
	dev_t dev;
{
	struct unit *un;
	struct dk_label *l;
	int i, head, unit;
	
	unit = UNIT(dev);
	un = &ipunits[unit];
	if (un->un_mc == 0)	/* never attached */
		return (ENXIO);
	if (!un->un_present) {
		for (i = 0; i < NLPART; i++) {
			un->un_map[i].dkl_cylno = 0;
			un->un_map[i].dkl_nblk = 1;
		}
		un->un_g.dkg_ncyl = 1;
		un->un_g.dkg_bcyl = 0;
		un->un_g.dkg_nhead = MAXHEAD;
		un->un_g.dkg_bhead = 0;
		un->un_g.dkg_nsect = 2;
		if (ipcommand(dev, IP_RESTORE, (daddr_t)0, (caddr_t)0, 0, 0)) {
			return (EIO);
		}
		un->un_present = 1;
	
		/* Allocate space for label in Multibus memory */
		/* XXX wrong -- iopb addresses are not usable */
		l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
		if (l == NULL) {
			printf("ipopen: no buffer for disk label\n");
			return (EIO);
		}
	
		/* Search for a label */
		for (head = 0; head < MAXHEAD; head++) {
			l->dkl_magic = 0;	/* clear from previous try */
			un->un_g.dkg_bhead = head;
			if (ipcommand(dev, IP_READ, (daddr_t)0, (caddr_t)l,
			    SECSIZE, 0) != 0)
				continue;
			if (!islabel(un-ipunits, un->un_md->md_slave, head, l))
				continue;
			uselabel(un, l);
			break;
		}
		if (head >= MAXHEAD)
			un->un_g.dkg_bhead = 0;
		rmfree(iopbmap, (long)SECSIZE, (long)l);
	}
	return (0);
}
 
ipsize(dev)
	dev_t dev;
{
	struct unit *un = &ipunits[UNIT(dev)];
	struct dk_map *lp = &un->un_map[LPART(dev)];

	if (!un->un_present)
		return (-1);
	return (lp->dkl_nblk);
}
 
ipstrategy(bp)
	register struct buf *bp;
{
	register struct unit *un;
	register struct dk_map *lp;
	register struct buf *dp;
	register int unit, s;
	daddr_t bn;

	unit = dkunit(bp);
	if (unit >= NIP)
		goto bad;

	un = &ipunits[unit];
	lp = &un->un_map[LPART(bp->b_dev)];

	if (!un->un_present && bp != &un->un_sbuf)
		goto bad;

	bn = dkblock(bp);
	if (bn > lp->dkl_nblk || lp->dkl_nblk == 0)
		goto bad;

	if (bn == lp->dkl_nblk) {	/* EOF */
		bp->b_resid = bp->b_bcount;
		iodone(bp);
		return;
	}

	bp->b_cylin = bn / (un->un_g.dkg_nsect * un->un_g.dkg_nhead);
	bp->b_cylin += lp->dkl_cylno + un->un_g.dkg_bcyl;
	dp = &un->un_utab;
	s = splx(pritospl(un->un_mc->mc_intpri));
	disksort((struct diskhd *)dp, bp);
	if (dp->b_active == 0) {
		(void) ipustart(un);
		bp = &un->un_mc->mc_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) ipstart(un->un_mc);
	}
	(void) splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

ipcommand(dev, cmd, daddr, b_addr, blen, nowait)
	dev_t dev;
	daddr_t daddr;
	caddr_t b_addr;
{
	register struct unit *un = &ipunits[UNIT(dev)];
	register struct buf *bp;
	int s;

	bp = &un->un_sbuf;
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags&B_BUSY) {
		if (nowait) {
			(void) splx(s);
			return (0);
		}
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);

	bp->b_dev = dev;
	un->un_scmd = cmd;
	un->un_sasync = nowait;
	bp->b_bcount = blen;
	bp->b_un.b_addr = b_addr;
	bp->b_blkno = daddr;

	ipstrategy(bp);
	if (nowait)
		return (0);
	iowait(bp);
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	return (bp->b_flags & B_ERROR);
}

/*
 * Unit start routine.
 */
ipustart(un)
	register struct unit *un;
{
	register struct buf *dp;
	register struct mb_device *md;
	register struct mb_ctlr *mc;

	if (un == 0)
		return;
	md = un->un_md;
	mc = un->un_mc;
	dk_busy &= ~(1<<md->md_dk);

	dp = &un->un_utab;
	if (dp->b_actf == NULL)
		return;

	if (!dp->b_active) {
		dp->b_active = 1;
		/*
	 	 * Mark unit busy for iostat.
	 	 */
		if (md->md_dk >= 0) {
			dk_busy |= 1<<md->md_dk;
			dk_seek[md->md_dk]++;
		}
	}

	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller
	 * (unless its already there.)
	 */
	if (dp->b_active != 2) {
		dp->b_forw = NULL;
		if (mc->mc_tab.b_actf == NULL)
			mc->mc_tab.b_actf = dp;
		else
			mc->mc_tab.b_actl->b_forw = dp;
		mc->mc_tab.b_actl = dp;
		dp->b_active = 2;
	}
}

/*
 * Set up a transfer for the controller
 */
ipstart(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct unit *un;
	register struct ctlr *c;
	register struct dk_map *lp;
	daddr_t bn;

loop:
	/*
	 * Pull a request off the controller queue
	 */
	if ((dp = mc->mc_tab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		mc->mc_tab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy, and
	 * determine destination of this request.
	 */
	mc->mc_tab.b_active++;

	un = &ipunits[dkunit(bp)];
	c = &ipctlrs[mc->mc_ctlr];

	bn = dkblock(bp);
	lp = &un->un_map[LPART(bp->b_dev)];
	c->c_nsect = howmany(bp->b_bcount, SECSIZE);
	c->c_nsect = min((u_int)c->c_nsect, (u_int)(lp->dkl_nblk-bp->b_blkno));
	bn += (lp->dkl_cylno * un->un_g.dkg_nhead * un->un_g.dkg_nsect);
	c->c_blkno = bn;
	bp->b_resid = bp->b_bcount;
	if (bp == &un->un_sbuf &&
	    un->un_scmd != IP_READ && un->un_scmd != IP_WRITE)
		ipgo(mc);
	else
		(void) mbgo(mc);
}

/*
 * Start up a transfer.
 * Called via mbgo after buffer is
 * mapped into Mainbus memory space.
 */
ipgo(mc)
	register struct mb_ctlr *mc;
{
	register struct unit *un;
	register struct ctlr *c;
	register struct buf *bp, *dp;
	int unit;

	c = &ipctlrs[mc->mc_ctlr];
	dp = mc->mc_tab.b_actf;
	if (dp == NULL || dp->b_actf == NULL)
		panic("ipgo queueing error 1");
	bp = dp->b_actf;
	unit = UNIT(bp->b_dev);
	un = &ipunits[unit];
	if (dp != &un->un_utab)
		panic("ipgo queueing error 2");
	
	if (bp == &un->un_sbuf) {
		c->c_nsect = howmany(bp->b_bcount, SECSIZE);
		c->c_cmd = un->un_scmd;
		if (c->c_cmd == IP_READ || c->c_cmd == IP_WRITE)
			c->c_baddr = MBI_ADDR(mc->mc_mbinfo);
		else
			c->c_baddr = (int)bp->b_un.b_addr; /* not an address */
	} else {
		if (bp->b_flags & B_READ)
			c->c_cmd = IP_READ;
		else
			c->c_cmd = IP_WRITE;
		c->c_baddr = MBI_ADDR(mc->mc_mbinfo);
	}

	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<unit;
		dk_xfer[unit]++;
		dk_wds[unit] += bp->b_bcount>>6;
	}
	ipchunk(c, un);
}

ipchunk(c, un)
	register struct ctlr *c;
	register struct unit *un;
{
	int cyl, head, sect, nsect;

	sect = c->c_blkno % un->un_g.dkg_nsect;
	head = c->c_blkno / un->un_g.dkg_nsect;
	cyl = head / un->un_g.dkg_nhead;
	head = head % un->un_g.dkg_nhead;
	head += un->un_g.dkg_bhead;
	cyl += un->un_g.dkg_bcyl;
	nsect = MIN(c->c_nsect, un->un_g.dkg_nsect - sect);

	c->c_wantint = 1;
	ipcmd(c, c->c_cmd, c->c_baddr, un->un_md->md_slave,
	    cyl, head, sect, nsect);

}

/*
 * Handle a disk interrupt.
 */
ipint(ctlr) 
	int ctlr;
{
	register struct ctlr *c = &ipctlrs[ctlr];
	register struct unit *un;
	register struct buf *bp;
	register struct iopb0 *ip0;
	register struct mb_ctlr *mc;
	register struct mb_device *md;
	struct error *e;
	register int status, cmd, error, secnt, unit;

	if (!c->c_wantint) {
		clrint(c);
		return;
	}
	c->c_wantint = 0;
	while (IP_R0(c) & IP_BUSY)
		;
	ip0 = (struct iopb0 *)c->c_iopb;
	while (ip0->i0_status == IP_DBUSY)
		;
	IP_R0(c) = IP_CLRINT;
	status = ip0->i0_status;
	cmd = ip0->i0_cmd;
	secnt = ip0->i0_secnt;
	error = ip0->i0_error;
	ip0->i0_status = 0;

	/*
	 * Get device and block structures, and a pointer
	 * to the mb_device for the drive.
	 */
	mc = ipcinfo[ctlr];
	bp = mc->mc_tab.b_actf->b_actf;
	md = ipdinfo[dkunit(bp)];
	un = &ipunits[dkunit(bp)];
	un->un_ltick = ipticks;
	dk_busy &= ~(1 << md->md_dk);

	if (bp == NULL) {
		printf("ipint: bad bp\n");
		return;
	}

	if (status == IP_ERROR) {		/* error handling! */
		e = &IPERROR(error);
		if (c->c_retries++ >= e->e_retry) {
			/* retries exhausted, try restore */
			c->c_retries = 0;
			if (c->c_restores++ >= e->e_restore) {
				/* complete failure */
				if (iperrlvl >= EL_FAIL)
					errmsg(cmd, bp, e, "failed");
				bp->b_flags |= B_ERROR;
				if (bp == &un->un_sbuf &&
				    un->un_scmd == IP_RESTORE) {
					un->un_present = 0;
					if (un->un_md->md_dk >= 0)
						dk_bps[un->un_md->md_dk] = 0;
					printf("ip%d: offline\n", dkunit(bp));
				}
				if (bp == &un->un_sbuf &&
				    un->un_scmd != IP_READ &&
				    un->un_scmd != IP_WRITE)
					ipdone(mc);
				else
					mbdone(mc);
			} else {
				/* do a restore */
				if (iperrlvl >= EL_REST)
					errmsg(cmd, bp, e, "restore");
				unit = UNIT(bp->b_dev);
				unit = ipunits[unit].un_md->md_slave;
				c->c_wantint = 1;
				ipcmd(c, IP_RESTORE, 0, unit,0,0,0,0);
			}
		} else {
			/* retry */
			if (iperrlvl >= EL_RETRY)
				errmsg(cmd, bp, e, "retry");
			ipchunk(c, un);
		}
		return;
	}

	/* status == OK */
	if (cmd == IP_RESTORE && 
	    !(bp == &un->un_sbuf && un->un_scmd == IP_RESTORE)) {
		/* error recovery */
		ipchunk(c, un);
		return;
	}

	/* transfer worked */
	if (cmd == IP_READ || cmd == IP_WRITE)
		c->c_nsect -= secnt;
	else
		c->c_nsect = 0;
	c->c_blkno += secnt;
	c->c_baddr += (secnt * SECSIZE);
	bp->b_resid -= (secnt * SECSIZE);
	if (bp->b_resid < 0)
		bp->b_resid = 0;
	if (c->c_nsect != 0)		/* more to do */
		ipchunk(c, un);
	else {
		c->c_retries = c->c_restores = 0;
		if (bp == &un->un_sbuf && un->un_scmd != IP_READ &&
		    un->un_scmd != IP_WRITE)
			ipdone(mc);
		else
			mbdone(mc);
	}
}

/*
 * Handle a polling disk interrupt.
 */
ippoll() 
{
	register struct ctlr *c;
	register int ctlr, serviced = 0;

	for (ctlr = 0, c = ipctlrs; ctlr < NIPC; ctlr++, c++) {
		if (!c->c_present || (IP_R0(c) & IP_COMPLETE) == 0)
			continue;
		serviced = 1;
		ipint(ctlr);
	}
	return (serviced);
}

/*
 * Clean up queues, free resources, and start next I/O
 * all done after I/O finishes.  Called by mbdone.
 */
ipdone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct unit *un;

	bp = mc->mc_tab.b_actf->b_actf;
	un = &ipunits[UNIT(bp->b_dev)];

	/* advance controller queue */
	dp = mc->mc_tab.b_actf;
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_actf = dp->b_forw;

	/* advance unit queue */
	dp->b_active = 0;
	dp->b_actf = bp->av_forw;

	iodone(bp);
	if (bp == &un->un_sbuf && un->un_sasync)
		bp->b_flags &= ~B_BUSY;

	/* start next I/O on unit */
	if (dp->b_actf)
		(void) ipustart(un);

	/* start next I/O on controller */
	if (mc->mc_tab.b_actf && mc->mc_tab.b_active == 0)
		(void) ipstart(mc);
}

#define	CMDNAME(n)	ipcmdnames[n-IP_READ]
char *ipcmdnames[] = {
	"read",
	"write",
	"verify",
	"format",
	"map",
	"switch",
	"init",
	"<bad cmd>",
	"restore",
	"seek",
	"rtz",
	"spindown",
	"<bad cmd>",
	"<bad cmd>",
	"reset",
};

static
errmsg(cmd, bp, e, action)
	struct buf *bp;
	struct error *e;
	char *action;
{

	printf("ip%d%c: %s %s (%s) blk %d\n", UNIT(bp->b_dev),
	    LPART(bp->b_dev)+'a', CMDNAME(cmd), action, e->e_name, bp->b_blkno);
}

ipread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = UNIT(dev);

	if (unit >= NIP)
		return (ENXIO);
	return (physio(ipstrategy, &ipunits[unit].un_rtab, dev, B_READ,
	    minphys, uio));
}

ipwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = UNIT(dev);

	if (unit >= NIP)
		return (ENXIO);
	return (physio(ipstrategy, &ipunits[unit].un_rtab, dev, B_WRITE,
	    minphys, uio));
}

/*ARGSUSED*/
ipioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct unit *un = &ipunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	struct dk_info *inf;

	switch (cmd) {

	case DKIOCINFO:
		inf = (struct dk_info *)data;
		inf->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		inf->dki_unit = un->un_md->md_slave;
		inf->dki_ctype = ipctlrs[un->un_mc->mc_ctlr].c_type;
		inf->dki_flags = DKI_FMTTRK+DKI_MAPTRK;
		break;

	case DKIOCGGEOM:
		*(struct dk_geom *)data = un->un_g;	/* struct assignment */
		break;

	case DKIOCSGEOM:
		if (!suser())
			return (u.u_error);
		un->un_g = *(struct dk_geom *)data;	/* struct assignment */
		break;

	case DKIOCGPART:
		*(struct dk_map *)data = *lp;		/* struct assignment */
		break;

	case DKIOCSPART:
		if (!suser())
			return (u.u_error);
		*lp = *(struct dk_map *)data;		/* struct assignment */
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}


/*
 * Wake up every so often and poke the disk to make
 * sure it hasn't disappeared 
 *
 * SOMEDAY WE SHOULD HANDLE LOST INTERRUPTS HERE (but no problems yet)
 */
ipwatch()
{
	register struct unit *un;

	ipticks++;

	for (un = &ipunits[0]; un < &ipunits[NIP]; un++) {
		if (!un->un_present)
			continue;
		if (ipticks - un->un_ltick < IPTIMO)
			continue;
		(void) ipcommand((un-ipunits)*NUNIT, IP_RESTORE,
		    (daddr_t)0, (caddr_t)0, 0, 1);
	}

	timeout(ipwatch, (caddr_t)0, hz);
}

ipdump(dev, addr, blkno, nblk)
	dev_t dev;
	caddr_t addr;
	daddr_t blkno, nblk;
{
	register struct unit *un = &ipunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	register struct ctlr *c = &ipctlrs[un->un_mc->mc_ctlr];
	int unit = un->un_md->md_slave;
	int nsect, sect, head, cyl;
	int err;

	if (!un->un_present)
		return (ENXIO);
	if (blkno >= lp->dkl_nblk || (blkno+nblk) > lp->dkl_nblk)
		return (EINVAL);

	do {
		nsect = un->un_g.dkg_nsect - (blkno % un->un_g.dkg_nsect);
		nsect = min((u_int)nsect, (u_int)nblk);
		nblk -= nsect;
		sect = blkno % un->un_g.dkg_nsect;
		head = blkno / un->un_g.dkg_nsect;
		cyl = head / un->un_g.dkg_nhead;
		head = head % un->un_g.dkg_nhead;
		head += un->un_g.dkg_bhead;
		cyl += un->un_g.dkg_bcyl;
		cyl += lp->dkl_cylno;
	
		ipcmd(c, IP_WRITE, (char *)addr - DVMA, unit,
		    cyl, head, sect, nsect);
		err = ipwait(c);
		blkno += nsect;
		addr += nsect*DEV_BSIZE;
	} while (nblk > 0 && !err);
	return (err ? EIO : 0);
}
