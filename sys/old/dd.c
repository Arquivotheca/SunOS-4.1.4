#ifndef lint
static	char sccsid[] = "@(#)dd.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "dd.h"
#if NDDC > 0

/*
 * Driver for Data Systems Design 5215 controller
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"

#include "../sun/pte.h"
#include "../sun/psl.h"
#include "../sun/cpu.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../sundev/mbvar.h"
#include "../sundev/dsdreg.h"

#define Dprintf if(dddebug)printf

int dddebug = 0;   /* Set to 1 for lots of verbiage */

int ddprobe(), ddslave(), ddattach(), ddgo(), dddone(), ddintr();

u_long dsdaddrs[] = { 0xf0, 0xf2, 0xf4, 0xf6, 0};
#define WUBADDR(n)      (dsdaddrs[ctlr]*16)

struct	mb_ctlr *ddcinfo[NDDC];
struct	mb_device *dddinfo[NDD];
struct mb_driver ddcdriver = {
	ddprobe, ddslave, ddattach, ddgo, dddone, ddintr, 
	dsdaddrs, 0, 0, 
	"dd", dddinfo, "ddc", ddcinfo, MDR_DMA,
};

#define	NUNIT		2		/* max # of units per controller */
#define	NLPART		NDKMAP		/* # of logical partitions (8) */
#define	UNIT(dev)	((dev>>3) % NUNIT)
#define	LPART(dev)	(dev % NLPART)

#define	INTERLEAVE 1	/* XXX this should be passed to ioctl as an arg */
#define	SECSIZE	512
#define INTERRUPT 0
#define NOINTERRUPT 1

/*
 * Definition of a unit
 * A unit is a physical partition of a drive,
 * usually the whole drive 
 */
struct	unit {
	struct	dk_map un_map[NLPART];	/* logical partitions */
	char	un_present;		/* physical partition is present */
	struct	dk_geom un_g;		/* disk geometry */
	struct	buf un_utab;		/* for queuing per unit */
	struct	buf un_rtab;		/* for raw I/O */
	struct	buf un_sbuf;		/* for special commands */
	struct	dsdiopb *un_iopb;	/* the iopb for this unit */
	u_char	un_scmd;		/* the special command */
	u_char	un_serr;		/* the error from special command */
	u_char	un_sasync;		/* noone is waiting for scmd */
	int	un_ltick;		/* last time active */ 
	struct	mb_device *un_md;	/* generic unit */
	struct	mb_ctlr *un_mc;		/* generic controller */
	short	un_ctlr;		/* index of controller */
} ddunits[NDD];

#define	MAXHEAD		4	/* max heads to search for a label */

/*
 * Data per controller
 */
struct	ctlr {
	struct unit	*c_units[NUNIT];/* units on controller */
	struct dsddevice *c_io;		/* ptr to I/O space data */
	struct dsdccb  *c_ccb;		/* ptr to CCB */
	short		c_type;		/* controller type */
	char		c_present;	/* controller is present */
	/* current transfer: */
/* XXX
 * should some of these be associated with the unit instead of the ctlr? 
 */
	caddr_t		c_bufaddr;	/* virtual buffer address */
	daddr_t		c_blkno;	/* current block */
	short		c_secnt;	/* block(sector) count */
	short		c_retries;	/* retry count */
	short		c_restores;	/* restore count */
} ddctlrs[NDDC];

#define	R0(c)	(c)->c_io->dsd_r0
#define BUSY(c)	((c)->c_ccb->dsd_busy == DSD_BUSY)

/*
 * Error message control
 */
#define	EL_RETRY	3
#define	EL_REST		2
#define	EL_FAIL		1

int dderrlvl = EL_RETRY;

#define	DSDERROR(err)	dsderrors[(err>>4)*8 + (err&0x0f) - 1]

struct	error {
	char	e_retry;	/* # of times to retry */
	char 	e_restore;	/* # of times to restore after retries */
	char	*e_name;	/* error message */
} dsderrors[] = {
	0,      0,      "unknown error",			/* 0x11 */
	0,      0,      "unknown error",			/* 0x12 */
	0,      0,      "unknown error",			/* 0x13 */
	0,	0,	"RAM error",				/* 0x14 */
	0,	0,	"ROM error",				/* 0x15 */
	0,	0,	"seek in progress",			/* 0x16 */
	1,	1,	"Illegal format type",			/* 0x17 */
	1,	1,	"End of media",				/* 0x18 */
	1,	1,	"Illegal sector size",			/* 0x21 */
	0,	1,	"Diagnostic Fault",			/* 0x22 */
	3,	8,	"No index",				/* 0x23 */
	0,	0,	"Invalid command",			/* 0x24 */
	3,	1,	"Sector not found",			/* 0x25 */
	0,	0,	"Invalid address",			/* 0x26 */
	0,	1,	"Unit not ready",			/* 0x27 */
	0,	0,	"Write protect",			/* 0x28 */
	0,      0,      "unknown error",			/* 0x31 */
	0,      0,      "unknown error",			/* 0x32 */
	0,      0,      "unknown error",			/* 0x33 */
	3,	1,	"Data ECC or CRC error",		/* 0x34 */
	3,	1,	"ID ECC or CRC error",			/* 0x35 */
	0,	3,	"Drive fault",				/* 0x36 */
#define	E_UFAULT 0x36
	1,	8,	"Cylinder address miscompare",		/* 0x37 */
	1,	8,	"Seek error",				/* 0x38 */
	3,	1,	"Data field not found",			/* 0x41 */
	3,	1,	"Wrong type of data field",		/* 0x42 */
	3,	1,	"Drive spinning too fast",		/* 0x43 */
	3,	1,	"Drive spinning too slow",		/* 0x44 */
	3,	1,	"Read/write controller error",		/* 0x45 */
	0,      0,      "unknown error",			/* 0x46 */
	0,      0,      "unknown error",			/* 0x47 */
	0,      0,      "unknown error",			/* 0x48 */
	0,	0,	"Tape cartridge not in place",		/* 0x51 */
	0,	0,	"Tape cartridge write protected",	/* 0x52 */
	0,	0,	"Tape drive not on line",		/* 0x53 */
	0,	0,	"Tape unrecoverable data error",	/* 0x54 */
	0,	0,	"No data on tape",			/* 0x55 */
	0,	0,	"Data Miscompare during diagnostic",	/* 0x56 */
	0,	0,	"Unknown tape error",			/* 0x57 */
	0,	0,	"unknown error",			/* 0x58 */
};

#define	CMDNAME(n)	DSD_cmdlist[n]

char *DSD_cmdlist[] = {
	"initialize",
	"error status",
	"format",
	"read ID",
	"read",
	"verify",
	"write",
	"write from buffer",
	"seek",
	NULL, NULL, NULL, NULL, NULL, NULL, 
	"restore",
};

#define ONE_MILLISECOND		1000 /* argument for DELAY, ~1 msec I hope */
#define DSD_WAIT_TIMEOUT	5000 /* 5 seconds, more or less */

#define	b_cylin b_resid

#define RESET(c)	R0(c) = DSD_RESET
#define CLRINT(c)	R0(c) = DSD_CLEAR
#define START(c)	R0(c) = DSD_START

swlong_t
swlong(n)
{
	swlong_t s;

	s.swl_lo = n & 0xFFFF;
	s.swl_hi = n >> 16;
	return (s);
}
/* address in multibus address space */
#define swadd(n)	(swlong(((int)n) & 0xFFFFF)) 

#ifdef INTRLVE
daddr_t dkblock();
#endif

int	ddwstart, ddwatch();		/* Have started guardian */
int	ddticks;			/* timer for guardian */
#define	DSDTIMO	20			/* time until disk check */

/* 
 * TODO: 
 * 
 * 2) Add overlapped seek capability.
 * 5) Use the automatic retry capability of the controller.
 * 7) Build a boot driver.
 * 8) Clean up the dsd portion of diag.
 */
/*
 * Determine existence and type of controller
 */
ddprobe(reg, ctlr)
	caddr_t reg;
{
	register struct ctlr *c = &ddctlrs[ctlr];
	register int nms;
	/* next line generates correct address for wake-up block */
	register struct dsdwub *wub = 
	      (struct dsdwub *)&iopbs[WUBADDR(ctlr)];

	c->c_io = (struct dsddevice *)reg;
	if (pokec((caddr_t)&R0(c), DSD_RESET)) 
		return (0);

	if ( rmget(iopbmap, sizeof (struct dsdwub), (int)wub) == 0) {
                printf(
		  "ddc%d: can't get MBmem at %x to initialize controller\n",
		   ctlr, WUBADDR(ctlr));
		return (0);
	}
	c->c_ccb = (struct dsdccb *)rmalloc(iopbmap, (long)DSDCCBSZ);
	if (c->c_ccb == NULL) {
		printf("ddprobe: no ccb space\n");
		return (0);
	}
	c->c_present = 1;

	/* determine controller type */
	bzero((caddr_t)c->c_ccb, DSDCCBSZ);
	ddwakeup(c->c_ccb,wub,c);
	c->c_type = DKC_UNKNOWN;
	for (nms = 0; nms < 4000; nms++) {
		DELAY(1000);
		if (!BUSY(c)) {
/* printf("DSD-5215 ST-506 disk controller at mbio 0x%x\n",reg - mbio); */
			c->c_type = DKC_DSD5215; 
			break;
		}
	}
	if (c->c_type == DKC_UNKNOWN) {
		printf("ddc%d: Unknown controller type at mbio 0x%x\n",
		    ctlr, reg - mbio);
		rmfree(iopbmap, (long)DSDCCBSZ, (long)c->c_ccb);
		c->c_present = 0;
		return (0);
	}
	return (sizeof (struct dsddevice) );
}

ddwakeup(ccb,wub,c)
	register struct dsdccb * ccb;
	register struct dsdwub  * wub;
	register struct ctlr * c;
{

	RESET(c);
	wub->dsw_reserved = 0;
	wub->dsw_extdiag = 1;		/* extended diagnostics */
	wub->dsw_linaddr = 1;		/* linear addressing */
	wub->dsw_emul    = 1;		/* isbx215 emulation, must be 1 */
	wub->dsw_ccbp  = swadd((caddr_t) vtop(ccb)); /* physical add */

	ccb->dsd_busy = DSD_BUSY;
	ccb->dsd_01h  = 0x01;
	ccb->dsd_01h1 = 0x01;
	ccb->dsd_04h  = 0x04;
#ifdef COMPILERBUG
	ccb->dsd_cibp = swadd(vtop(&ccb->dsd_handle));
#else
{
	int x = (int)&ccb->dsd_handle;
	ccb->dsd_cibp = swadd(vtop(x));
}
#endif

	CLRINT(c);
	DELAY(1);
	START(c);
}

/*
 * See if a slave unit exists
 */
/*ARGSUSED*/
ddslave(md, reg)
	struct mb_device *md;
	caddr_t reg;
{
	return (1);
}

ddattach(md)
	register struct mb_device *md;
{
	register struct ctlr *c = &ddctlrs[md->md_ctlr];
	register struct unit *un = &ddunits[md->md_unit];
	register struct dsdiopb *ip;
	struct dk_label *l;
	int head, i;
	int unit = md->md_slave;

Dprintf("ddattach ");
if( !dddebug ) { /* XXX don't do this if debugging-it gets in the way */
	if (ddwstart == 0) {
		timeout(ddwatch, (caddr_t)0, hz);
		ddwstart++;
	}
} /* XXX */

	/* Allocate space for iopb in Multibus memory */
	ip = (struct dsdiopb *)rmalloc(iopbmap, (long)IOPBSIZE);
	if (ip == NULL) {
		printf("ddattach: no space for iopb\n");
		return;
	} 
	un->un_iopb = ip;
	c->c_units[unit] = un;
	un->un_ctlr = md->md_ctlr;
	un->un_md = md;
	un->un_mc = md->md_mc;

	/* Allocate space for label in Multibus memory */
	l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
	if (l == NULL) {
		printf("ddattach: no space for disk label\n");
		un->un_present = 0;
		return;
	}

	/* 
 	 * Initialize unit (physical drive)
	 */
	un->un_g.dkg_ncyl = 1;
	un->un_g.dkg_bcyl = 0;
	un->un_g.dkg_acyl = 0;
	un->un_g.dkg_nhead = MAXHEAD;
	un->un_g.dkg_bhead = 0;
	un->un_g.dkg_nsect = 2;
	un->un_g.dkg_intrlv = 1;
	un->un_g.dkg_gap1 = 0;
	un->un_g.dkg_gap2 = 0;
	
	if( ddusegeom(un, 1) != 0 ) {
printf("ddattach: err %x\n", dderror(c));
		un->un_present = 0;
		return;
	}

	/* Search for a label */
	for (head = 0; head < MAXHEAD; head++) {
		l->dkl_magic = 0;	/* clear from previous try */
		if (ddgetlabel(c, unit, head, l) == 0)
			continue;
		if (ddislabel(un - ddunits, unit, head, l) == 0)
			continue;
		dduselabel(un, l);
		ddusegeom(un, 1);
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
dduselabel(un, l)
	register struct unit *un;
	register struct dk_label *l;
{
	int i, intrlv;

	printf("dd%d: <%s>\n", un - ddunits, l->dkl_asciilabel);
	un->un_present = 1;
	un->un_g.dkg_ncyl   = l->dkl_ncyl;
	un->un_g.dkg_bcyl   = 0;              /* label doesn't have this */
	un->un_g.dkg_acyl   = l->dkl_acyl;
	un->un_g.dkg_nhead  = l->dkl_nhead;
	un->un_g.dkg_bhead  = l->dkl_bhead;
	un->un_g.dkg_nsect  = l->dkl_nsect;
	un->un_g.dkg_gap1   = l->dkl_gap1;
	un->un_g.dkg_gap2   = l->dkl_gap2;
	un->un_g.dkg_intrlv = l->dkl_intrlv;
	for (i = 0; i < NLPART; i++)
		un->un_map[i] = l->dkl_map[i]; /* struct assignment */
	if (un->un_md->md_dk >= 0) {
		intrlv = un->un_g.dkg_intrlv;
		dk_mspw[un->un_md->md_dk] = 1.0 /
		    ((SECSIZE/sizeof (short))*un->un_g.dkg_nsect*60/intrlv);
	}
}

/*
 * Initialize a unit's geometry 
 */
static int
ddusegeom(un, init)
	register struct unit *un;
{
	register struct ctlr *c = &ddctlrs[un->un_ctlr];
	register struct dsdinit *ib;
	register int unit = un->un_md->md_slave;
	register int nhead = un->un_g.dkg_nhead + un->un_g.dkg_bhead;
	register int err;

	ib = (struct dsdinit *)rmalloc(iopbmap, (long)sizeof (struct dsdinit));
	if (ib == NULL) {
		printf("dd%d: can't get initialization memory\n",
		    un - ddunits);
		return (-1);
	}
	ib->dsx_ncyl  = un->un_g.dkg_ncyl + un->un_g.dkg_acyl;
	ib->dsx_acyl  = un->un_g.dkg_acyl;
	ib->dsx_rheads = 0;                  /* no removable media */
	ib->dsx_fheads = nhead;
	ib->dsx_nsect = un->un_g.dkg_nsect;
	ib->dsx_bpslo = SECSIZE & 0xff;
	ib->dsx_bpshi = SECSIZE >> 8;

	err = init
	      ? ddcmd(c, DSD_INIT, (caddr_t)ib, unit,
		       0, 0, 0, 0, NOINTERRUPT)
	      : ddcommand((un-ddunits)*NLPART, DSD_INIT, 
		           (daddr_t)0, (caddr_t)ib, 0, 0);
	rmfree(iopbmap, (long)sizeof (struct dsdinit), (long)ib);
	return( err );
}

static
ddgetlabel(c, unit, head, l)
	register struct ctlr *c;
	register struct dk_label *l;
{
	int error, retries, restores;
	register int ddn = (c->c_units[unit] - ddunits);

	l->dkl_magic = 0;
	restores = 0;
	error = 0;
	do {
		retries = 0;
		do {
			if( ddcmd(c, DSD_READ, (caddr_t)l, unit,
			    0, head, 0, 1,NOINTERRUPT) )
				error = dderror(c);
		} while (error && retries++ < DSDERROR(error).e_retry);
	} while (error && restores++ < DSDERROR(error).e_restore);
	if (error) {
		printf("dd%d: error %x reading label on head %d: %s\n", 
			ddn, error, head, DSDERROR(error).e_name);
		return (0);
	}
	return (1);
}

dderror(c)
	register struct ctlr *c;
{

	ddcmd(c, DSD_STATUS, 
	 	(caddr_t)&c->c_ccb->dsd_error[0], c->c_ccb->dsd_unit,
	 	0,0,0,0,NOINTERRUPT);
	return( c->c_ccb->dsd_exterr );
}

static
ddislabel(ddn, unit, head, l)
	register struct dk_label *l;
{

	if (l->dkl_magic != DKL_MAGIC)
		return (0);
	if (!ddck_cksum(l)) {
		printf("dd%d: Corrupt label on head %d\n", ddn, head);
		return (0);
	}
	if (head != l->dkl_bhead) {
		printf("dd%d: Misplaced label on head %d\n", ddn, head);
		return (0);
	}
	if (l->dkl_ppart > 1) {
		printf("dd%d: Unsupported phys partition # %d\n", 
			ddn, l->dkl_ppart);
		return (0);
	}
	return (1);
}

/*
 * Check the checksum of the label
 */
static
ddck_cksum(l)
	struct dk_label *l;
{
	short *sp, sum = 0;
	short count = sizeof(struct dk_label)/sizeof(short);

	sp = (short *)l;
	while (count--) 
		sum ^= *sp++;
	return (sum ? 0 : 1);
}

ddcmd (c, cmd, dmaddr, unit, cylinder, head, sector, secnt, nointerrupt) 
	struct ctlr *c;
	caddr_t dmaddr;
{
	register struct dsdccb *cp = c->c_ccb;
	register struct dsdiopb *ip = c->c_units[unit]->un_iopb;

	/* restore uses the diagnostic command, modified for seek to 0 */
	ip->dsi_diagmod = (cmd == DSD_RESTORE) ? DSD_REST_MOD : 0;

	ip->dsi_device = DSD_WINCH;
	ip->dsi_cmd = cmd;
	ip->dsi_unit = unit&1;
	ip->dsi_nointr = nointerrupt;
	ip->dsi_noretry = 1;	/* XXX */
	ip->dsi_deldata = 0;
	ip->dsi_cylinder = cylinder;
	ip->dsi_sector = sector;
	ip->dsi_head = head;
	ip->dsi_bufp = swadd((caddr_t)vtop(dmaddr));
	ip->dsi_count = swlong(secnt*SECSIZE);
	cp->dsd_iopbp = swadd((caddr_t)vtop(ip));

	while( BUSY(c) ) /* callers should check for busy if they can't wait */
		;
	cp->dsd_busy = DSD_BUSY;
	cp->dsd_stsema = 0;
Dprintf("ddcmd: %s nointr=%d\n",CMDNAME(cmd),nointerrupt);

	START(c);
	return ( nointerrupt ? ddwait(c) : 0 );
}

ddwait(c)
	register struct ctlr *c;
{
	register int cnt = DSD_WAIT_TIMEOUT;

	while (BUSY(c)) {
		DELAY(ONE_MILLISECOND);
		if( cnt-- <= 0 ) {
			printf("dd timeout\n");
			CLRINT(c);
			return(1);
		}
	}
	CLRINT(c);
	return ( c->c_ccb->dsd_sumerr );
}

ddopen(dev, flag)
	dev_t dev;
{
	struct unit *un;
	struct dk_label *l;
	int i, head, unit;
	
Dprintf("ddopen: dev %d flag %X\n",dev,flag);
	unit = UNIT(dev);
	un = &ddunits[unit];
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
		
		if( ddusegeom(un, 0) != 0 ) {
			uprintf("dd%d: unit not online\n", unit);
			return (EIO);
		}
		un->un_present = 1;
	
		/* Allocate space for label in Multibus memory */
		l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
		if (l == NULL) {
			printf("ddopen: no buffer for disk label\n");
			return (EIO);
		}
	
		/* Search for a label */
		for (head = 0; head < MAXHEAD; head++) {
			l->dkl_magic = 0;	/* clear from previous try */
			un->un_g.dkg_bhead = head;
			if (ddcommand(dev, DSD_READ, 
				(daddr_t)0, (caddr_t)l, SECSIZE, 0) != 0)
				continue;
			if (!ddislabel(un-ddunits, un->un_md->md_slave, head, l))
				continue;
			dduselabel(un, l);
			ddusegeom(un, 0);
			break;
		}
		if (head >= MAXHEAD)
			un->un_g.dkg_bhead = 0;
		rmfree(iopbmap, (long)SECSIZE, (long)l);
	}
	return (0);
}
 
daddr_t
ddsize(dev)
	dev_t dev;
{
	struct unit *un = &ddunits[UNIT(dev)];
	struct dk_map *lp = &un->un_map[LPART(dev)];

	if (!un->un_present)
		return (-1);
	return (lp->dkl_nblk);
}
 
ddstrategy(bp)
	register struct buf *bp;
{
	register struct unit *un;
	register struct dk_map *lp;
	register struct buf *dp;
	register int unit, s;
	daddr_t bn;

Dprintf("ddstratetgy\n");
	unit = dkunit(bp);
	if (unit >= NDD)
		goto bad;

	un = &ddunits[unit];
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
	disksort(dp, bp);
	if (dp->b_active == 0) {
		(void) ddustart(un);
		bp = &un->un_mc->mc_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) ddstart(un->un_mc);
	}
	(void) splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

ddcommand(dev, cmd, daddr, bufaddr, blen, nowait)
	dev_t dev;
	daddr_t daddr;
	caddr_t bufaddr;
{
	register struct unit *un = &ddunits[UNIT(dev)];
	register struct buf *bp;

Dprintf("ddcommand: dev=%d %s\n",dev, CMDNAME(cmd));
	bp = &un->un_sbuf;
	(void) splx(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags&B_BUSY) {
		if (nowait)
			return (0);
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	spl0();

	bp->b_dev = dev;
	un->un_scmd = cmd;
	un->un_sasync = nowait;
	bp->b_bcount = blen;
	bp->b_un.b_addr = bufaddr;
	bp->b_blkno = daddr;

	ddstrategy(bp);
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
ddustart(un)
	register struct unit *un;
{
	register struct buf *dp;
	register struct mb_device *md;
	register struct mb_ctlr *mc;

	if (un ==0)
		return ;
	md = un->un_md;
	mc = un->un_mc;
	dk_busy &= ~(1<<md->md_dk);

	dp = &un->un_utab;
	if ( dp->b_actf == NULL)
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
ddstart(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct unit *un;
	register struct ctlr *c;
	register struct dk_map *lp;
	daddr_t bn;
	int nblk, secnt;

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

	un = &ddunits[dkunit(bp)];
	c = &ddctlrs[mc->mc_ctlr];

	bn = dkblock(bp);
	lp = &un->un_map[LPART(bp->b_dev)];
	nblk = howmany(bp->b_bcount, SECSIZE);
	secnt = MIN(nblk, (int)(lp->dkl_nblk - bp->b_blkno));
	bn += (lp->dkl_cylno * un->un_g.dkg_nhead * un->un_g.dkg_nsect);
	c->c_blkno = bn;
	c->c_bufaddr = bp->b_un.b_addr;
	c->c_secnt = secnt;
	bp->b_resid = bp->b_bcount;
	ddrstart(mc);
}

/*
 * Start or restart a piece of a transfer
 * Figure out how big the transfer can be
 * and call mbgo to get the buffer into Multibus
 * memory.
 */
ddrstart(mc)
	register struct mb_ctlr *mc;
{
	register struct ctlr *c;
	register struct unit *un;
	register struct buf *bp;
	register int nsect;

	bp = mc->mc_tab.b_actf->b_actf;
	un = &ddunits[dkunit(bp)];
	c = &ddctlrs[mc->mc_ctlr];
	/*
	 * WDC-2880 doesn't increment by head;
 	 * SMD-2180 doesn't always work when you try
	 * SMD-2181 ????
 	 */
	nsect = un->un_g.dkg_nsect - (c->c_blkno % un->un_g.dkg_nsect);
	nsect = MIN(255, nsect);
	nsect = MIN(nsect, c->c_secnt);

	mc->mc_baddr = c->c_bufaddr;
	mc->mc_blen = MIN(nsect*SECSIZE, bp->b_resid);
	bp->b_resid -= mc->mc_blen;
	if (bp == &un->un_sbuf  &&
	    un->un_scmd != DSD_READ && un->un_scmd != DSD_WRITE)
		mc->mc_blen = 0;
	mc->mc_rw = (bp->b_flags & B_READ);
	mbgo(mc);
}

/*
 * Start up a transfer
 * Called via mbgo after buffer is in Multibus memory
 */
ddgo(mc)
	register struct mb_ctlr *mc;
{
	register struct unit *un;
	register struct ctlr *c;
	register struct buf *bp, *dp;
	int cyl, head, sect, nsect, cmd;
	int unit, addr;

	c = &ddctlrs[mc->mc_ctlr];
	dp = mc->mc_tab.b_actf;
	if (dp == NULL || dp->b_actf == NULL)
		panic("ddgo queueing error 1");
	bp = dp->b_actf;
	unit = UNIT(bp->b_dev);
	un = &ddunits[unit];
	if (dp != &un->un_utab)
		panic("ddgo queueing error 2");
	
	sect = c->c_blkno % un->un_g.dkg_nsect;
	head = c->c_blkno / un->un_g.dkg_nsect;
	cyl = head / un->un_g.dkg_nhead;
	head = head % un->un_g.dkg_nhead;

	head += un->un_g.dkg_bhead;
	cyl += un->un_g.dkg_bcyl;

	if (bp == &un->un_sbuf) {
		nsect = howmany(bp->b_bcount, SECSIZE);
		cmd = un->un_scmd;
		if (cmd == DSD_READ || cmd == DSD_WRITE || cmd == DSD_INIT)
			addr = (int)mc->mc_mbaddr;
		else
			addr = (int)bp->b_un.b_addr;	/* not an address */
	} else {
		nsect = howmany(mc->mc_blen, SECSIZE);
		if (bp->b_flags & B_READ)
			cmd = DSD_READ;
		else
			cmd = DSD_WRITE;
		addr = (int)mc->mc_mbaddr;
	}

	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<unit;
		dk_xfer[unit]++;
		dk_wds[unit] += mc->mc_blen>>6;
	}

	unit = un->un_md->md_slave;
	ddcmd(c, cmd, (caddr_t)addr, unit, cyl, head, sect, nsect, INTERRUPT);
}

int ddintrkludge = 0xb0;
/*
 * Handle a disk interrupt.
 */
ddintr() 
{
	register struct ctlr *c;
	register struct unit *un;
	register struct buf *bp;
	register struct dsdccb *cp;
	register struct mb_ctlr *mc;
	register struct mb_device *md;
	register struct dsdiopb *ip;
	struct error *e;
	register int serviced = 0, status, cmd, secnt, unit;

Dprintf("ddintr ");
	for (c = ddctlrs; c < &ddctlrs[NDDC]; c++) {
/*
 * Here it is necessary that controllers do not interrupt unless
 * the nointr bit in the iopb is false.
 * This may be satisfied by ensuring that:
 *  Seek commands are always started with INTERRUPT (because the 
 *  dsd controller ALWAYS interrupts on seek completion and we need
 *  to look at the nointr bit later to tell if this is the interrupting
 *  device)
 */
		if (!c->c_present ||
		    (cp = c->c_ccb)->dsd_stsema != DSD_STATUS_POSTED)
			continue;

/* XXX
 * This may not be needed.  The condition that concerns me is:
 * Suppose seeks were started on drives 0 and 1.  Drive 0 finished
 * its seek, and interrupted.  We started a read (for example) on
 * drive 0, and than went away.  Later on, drive 1 finishes its seek
 * and interrupts.  Meanwhile, drive0 is still busy with its read, so
 * we can't start up a read on drive 1 until the drive 0 read is 
 * finished.  The dsd controller only allows one non-seek operation at
 * a time.
 */
		while( BUSY(c) ) 
			;   
		DELAY(ddintrkludge);
		CLRINT(c);

		cp->dsd_stsema = 0;
		serviced = 1;

/*
 * Find out which unit the status refers to and verify that that unit
 * was started with interrupts on.
 */
		if (c->c_units[cp->dsd_unit] == 0) {
			printf("dd: intr from unknown unit\n");
			continue;
		}
		ip = c->c_units[cp->dsd_unit]->un_iopb;
		if( ip->dsi_nointr ) {
			printf("ddintr: unexpected interrupt\n");	
			continue;
		}

		/*
		 * Get device and block structures, and a pointer
		 * to the mb_device for the drive.
		 */
		mc = ddcinfo[c - ddctlrs];
		bp = mc->mc_tab.b_actf->b_actf;
		md = dddinfo[dkunit(bp)];
		un = &ddunits[dkunit(bp)];
		un->un_ltick = ddticks;
		dk_busy &= ~(1 << md->md_dk);

		status = cp->dsd_sumerr;
		cmd = ip->dsi_cmd;  /* last command executed for this unit */
		secnt = ip->dsi_xcount.swl_lo/SECSIZE;

		if (bp == NULL) {
			printf("ddintr: bad bp\n");
			continue;
		}

		if (status) {		/* error handling! */
			e = &DSDERROR(dderror(c));
			if (c->c_retries++ >= e->e_retry) {
				/* retries exhausted, try restore */
				c->c_retries = 0;
				if (c->c_restores++ >= e->e_restore) {
					/* complete failure */
					if (dderrlvl >= EL_FAIL)
						dderrmsg(cmd, bp, e, "failed");
					bp->b_flags |= B_ERROR;
					if (bp == &un->un_sbuf &&
					    un->un_scmd == DSD_RESTORE) {
						un->un_present = 0;
						if (un->un_md->md_dk >= 0)
						  dk_mspw[un->un_md->md_dk] = 0;
						printf("dd%d: offline\n", 
								dkunit(bp));
					}
					mbdone(mc);
				} else {
/* XXX
 * it may be better to do an initialize here instead of a restore.
 * perhaps the label could be checked to make sure the disk is the same ? 
 * Maybe restore should be redefined to mean Initialize 
 */
				 /* do a restore */
					if (dderrlvl >= EL_REST)
						dderrmsg(cmd, bp, e, "restore");
					unit = UNIT(bp->b_dev);
					unit = ddunits[unit].un_md->md_slave;
					ddcmd(c, DSD_RESTORE, (caddr_t)0,
						 unit,0,0,0,0,INTERRUPT);
				}
			} else {
				/* retry */
				if (dderrlvl >= EL_RETRY)
					dderrmsg(cmd, bp, e, "retry");
				ddgo(mc);
			}
			continue;
		}

		/* status == OK */
		if (cmd == DSD_RESTORE && 
		    !(bp == &un->un_sbuf && un->un_scmd == DSD_RESTORE)) {
			/* error recovery */
			ddgo(mc);
			continue;
		}

		/* transfer worked */
		if (cmd == DSD_READ || cmd == DSD_WRITE)
			c->c_secnt -= secnt;
		else
			c->c_secnt = 0;
		c->c_blkno += secnt;
		c->c_bufaddr += (secnt * SECSIZE);
		if (c->c_secnt == 0)		/* all of bp done */
			c->c_retries = c->c_restores = 0;
		mbdone(mc);
	}

	return (serviced);
}

/*
 * Clean up queues, free resources, and start next I/O
 * all done after I/O finishes
 * Called by mbdone after moving read data from Multibus
 */
dddone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct ctlr *c;
	register struct unit *un;

	c = &ddctlrs[mc->mc_ctlr];
	bp = mc->mc_tab.b_actf->b_actf;
	un = &ddunits[UNIT(bp->b_dev)];
	if ((bp->b_flags & B_ERROR) == 0 && c->c_secnt > 0) {
		/* start next piece */
		ddrstart(mc);
		return;
	}

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
		ddustart(un);

	/* start next I/O on controller */
	if (mc->mc_tab.b_actf && mc->mc_tab.b_active == 0)
		ddstart(mc);
}

static
dderrmsg(cmd, bp, e, action)
	struct buf *bp;
	struct error *e;
	char *action;
{

	printf("dd%d%c: %s %s (%s) blk %d\n", UNIT(bp->b_dev),
	    LPART(bp->b_dev)+'a', CMDNAME(cmd), action, e->e_name, bp->b_blkno);
}

ddread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = UNIT(dev);

	if (unit >= NDD)
		return (ENXIO);
	return (physio(ddstrategy, &ddunits[unit].un_rtab, dev, B_READ,
	    minphys, uio));
}

ddwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = UNIT(dev);

	if (unit >= NDD)
		return (ENXIO);
	return (physio(ddstrategy, &ddunits[unit].un_rtab, dev, B_WRITE,
	    minphys, uio));
}

/*ARGSUSED*/
ddioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct unit *un = &ddunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	struct dk_fmt *f;
	struct dk_mapr *m;
	struct dk_info *inf;
	daddr_t sblk, bblk, nblk, tblk;
	int cyl, head, nsect;

	switch (cmd) {

	case DKIOCINFO:
		inf = (struct dk_info *)data;
		inf->dki_ctlr = un->un_mc->mc_addr - mbio;
		inf->dki_unit = un->un_md->md_slave;
		inf->dki_ctype = ddctlrs[un->un_mc->mc_ctlr].c_type;
		inf->dki_flags = DKI_FMTTRK+DKI_MAPTRK;
		break;

	case DKIOCGGEOM:
		*(struct dk_geom *)data = un->un_g;	/* struct assignment */
		break;

	case DKIOCSGEOM:
		if (!suser())
			return (u.u_error);
		un->un_g = *(struct dk_geom *)data;	/* struct assignment */
		ddusegeom(un, 0);
		break;

	case DKIOCGPART:
		*(struct dk_map *)data = *lp;		/* struct assignment */
		break;

	case DKIOCSPART:
		if (!suser())
			return (u.u_error);
		*lp = *(struct dk_map *)data;		/* struct assignment */
		break;
/* XXX
 * The interleave factor really should be passed in the argument block 
 * instead of being taken from the label.
 */
	case DKIOCFMT:
		if (!suser())
			return (u.u_error);
		f = (struct dk_fmt *)data;		/* struct assignment */
		nsect = un->un_g.dkg_nsect;
		nblk = f->dkf_nblk;
		bblk = lp->dkl_cylno * un->un_g.dkg_nhead * nsect;
		sblk = f->dkf_blkno + bblk;
		if ((sblk % nsect) != 0 ||
		    nblk != nsect)
			return (EINVAL);
		if ((sblk + nblk) > (bblk + lp->dkl_nblk))
			return (EINVAL);
/* XXX the following assumes that 1 sector = 1 block  */
		if (ddformat(dev, f->dkf_blkno, f->dkf_fill, nblk,
			INTERLEAVE,DSD_NORMAL,nsect) == 0)
			ddcommand(dev, DSD_VERIFY, f->dkf_blkno, (caddr_t)0, 
					nblk*SECSIZE, 0);
		/* EIO set on failure */
		break;

	case DKIOCMAP:
		if (!suser())
			return (u.u_error);
		m = (struct dk_mapr *)data;
		bblk = lp->dkl_cylno * un->un_g.dkg_nhead * un->un_g.dkg_nsect;
		sblk = m->dkm_fblk + bblk;
		nsect = un->un_g.dkg_nsect;
		nblk = m->dkm_nblk;
		tblk = m->dkm_tblk;
		if ((sblk % nsect) != 0 ||  /* sblk on trk boundary ? */
		    nblk != nsect)   /* request entire track ? */
			return (EINVAL);
		if ((sblk + nblk) > (bblk + lp->dkl_nblk)) /* req>part. size */
			return (EINVAL);
		cyl = tblk / (un->un_g.dkg_nhead * nsect);
		head = tblk % (un->un_g.dkg_nhead * nsect);

		if( !ddformat(dev, tblk, m->dkm_fill, nblk,
				INTERLEAVE,DSD_ALTERNATE,nsect)
		&&  !ddcommand(dev,DSD_VERIFY,tblk,(caddr_t)0,nblk*SECSIZE,0)
		&&  !dd_def_trk(dev,m->dkm_fblk,nblk,cyl,head,INTERLEAVE)
		)
			ddcommand(dev, DSD_VERIFY, m->dkm_fblk, (caddr_t)0, 
					nblk*SECSIZE, 0);
	
		/* EIO set on failure */
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

static
ddformat(dev,blkno,fill,nblk,interleave,type,blks_per_trk)
	dev_t dev;
	daddr_t blkno;
	daddr_t nblk;
{
	register struct dsdfmt *fb;
	register int stat;
	register daddr_t endblk;

	fb = (struct dsdfmt *)rmalloc(iopbmap, (long)sizeof (struct dsdfmt));
	if (fb == NULL) 
		return(EIO);

	fb->dsf_type   = type; /* either DSD_NORMAL or DSD_ALTERNATE */
	fb->dsf_intrlv = interleave;
	fb->dsf_fill1  = fill;
	fb->dsf_fill2  = fill;
	fb->dsf_fill3  = fill;
	fb->dsf_fill4  = fill;

	for(endblk=blkno+nblk; blkno<endblk; blkno+=(daddr_t)blks_per_trk ) {
		stat = ddcommand(dev, DSD_FORMAT, 
			 (daddr_t)blkno, (caddr_t)fb, blks_per_trk, 0); 
		if( stat != 0 )
			break;
	}
	rmfree(iopbmap, (long)sizeof(struct dsdfmt), (long)fb);
	return(stat);
}

static
dd_def_trk(dev,blkno,nblk,altcyl,althead,interleave)
	dev_t dev;
	daddr_t blkno;
	daddr_t nblk;
{
	register struct dsdbad *fb;
	register int stat;

	fb = (struct dsdbad *)rmalloc(iopbmap, (long)sizeof (struct dsdbad));
	if (fb == NULL) 
		return(EIO);

	fb->dsb_type   = DSD_DEFECTIVE;  
	fb->dsb_intrlv = interleave;	
	fb->dsb_acyllo = altcyl & 0xFF;
	fb->dsb_acylhi = (altcyl >> 8) & 0xFF;
	fb->dsb_ahead  = althead;
	fb->dsb_0x00   = 0;
	stat = ddcommand(dev, DSD_FORMAT, 
			 (daddr_t)blkno, (caddr_t)fb, (int)nblk, 0); 
	rmfree(iopbmap, (long)sizeof(struct dsdbad), (long)fb);
	return(stat);
}

/*
 * Wake up every so often and poke the disk to make
 * sure it hasn't disappeared 
 *
 * SOMEDAY WE SHOULD HANDLE LOST INTERRUPTS HERE (but no problems yet)
 */
ddwatch()
{
	register struct unit *un;

	ddticks++;

	for (un = &ddunits[0]; un < &ddunits[NDD]; un++) {
		if (!un->un_present)
			continue;
		if (ddticks - un->un_ltick < DSDTIMO)
			continue;
/* really should do a "read next sector header" and check the error stat */
/* there may be a special case somewhere about checking for restore and */
/* discarding the command if something useful is going on.  If so, it */
/* should be fixed to not trigger off of RESTORE XXX */
		ddcommand((un-ddunits)*NUNIT, DSD_RESTORE, 
				   (daddr_t)0, (caddr_t)0, 0, 1);
	}

	timeout(ddwatch, (caddr_t)0, hz);
}

dddump(dev, addr, blkno, nblk)
	dev_t dev;
	caddr_t addr;
	daddr_t blkno, nblk;
{
	register struct unit *un = &ddunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	register struct ctlr *c = &ddctlrs[un->un_mc->mc_ctlr];
	int unit = un->un_md->md_slave;
	int sect, head, cyl;
	int err;

	if (!un->un_present)
		return (ENXIO);
	if (blkno >= lp->dkl_nblk || (blkno+nblk) > lp->dkl_nblk)
		return (EINVAL);

	sect = blkno % un->un_g.dkg_nsect;
	head = blkno / un->un_g.dkg_nsect;
	cyl = head / un->un_g.dkg_nhead;
	head = head % un->un_g.dkg_nhead;
	head += un->un_g.dkg_bhead;
	cyl += un->un_g.dkg_bcyl;
	cyl += lp->dkl_cylno;

	err = ddcmd(c, DSD_WRITE,(caddr_t)addr,unit,
		     cyl,head,sect,(int)nblk,NOINTERRUPT);
	return (err ? EIO : 0);
}
#endif NDDC
