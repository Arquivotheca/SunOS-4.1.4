#ifndef lint
static	char	sccsid[] = "@(#)cgnine.c	1.1 94/10/31 SMI";
#endif

/*
 *	Copyright (c)  1989 by Sun Microsystems, Inc.
 */

#include "cgnine.h"			/* num of cg9 boards (NCGNINE)	*/
#include "win.h"			/* max num of windows (NWIN)	*/
#include <sys/param.h>			/* PZERO, NULL, and other .h	*/
#include <sys/buf.h>			/* sundev/mbvar.h		*/
#include <sys/errno.h>			/* EINVAL, ENOTTY		*/
#include <sys/ioctl.h>			/* ioctl definitions		*/
#include <sys/map.h>			/* kernelmap			*/
#include <sys/vmmac.h>			/* kmxtob			*/
#include <sys/time.h>			/* for kernel.h */
#include <sys/proc.h>			/* struct proc */
#include <sys/user.h>			/* u. */
#include <sys/kernel.h>			/* hz = # of ticks/second */
#include <machine/pte.h>		/* PGT_VME_D32			*/
#include <machine/psl.h>		/* pritospl			*/
#include <sundev/mbvar.h>		/* struct mb_driver, pritospl	*/
#include <sun/fbio.h>			/* fbgattr			*/
#include <pixrect/pr_planegroups.h>	/* PIXPG_*			*/
#include <pixrect/cg9var.h>		/* CG9_*			*/

#define	CG9_PROBESIZE		MMU_PAGESIZE
#ifdef sun3x
#define	CG9_PHYSICAL(p)	(btop(softc->cg9_physical + (p)))
#else
#define	CG9_PHYSICAL(p)	(PGT_VME_D32 | btop(softc->cg9_physical + (p)))
#endif

#ifdef	CG9_DEBUG
#define	D_PRINT(m,v)		ring_addseq(m,v)
#define	BAD_CG9()		goto bad_cg9; trial_count++
#else	!CG9_DEBUG
#define	D_PRINT(m,v)
#define	BAD_CG9()		goto bad_cg9
#endif	CG9_DEBUG

/* CG9 Interrupt Handling Macros */

/*
 * Not all architecture implement "setintrenable".  When they don't, there
 * will be a race condition on the read-modify-write statement.  We
 * encapsulate it with splx to prevent it.
 */
#define DISABLE_INTR(s) {                                       \
    	int x;                                                  \
    	x = splx(pritospl(md->md_intpri));                      \
	(s) &= ~CG9_REG_VB_INTR_ENAB;                           \
	(void) splx(x);                                         \
}
#define ENABLE_INTR(s)	((s) |= CG9_REG_VB_INTR_ENAB)

/* Driver information for auto-config routines: cgnineprobe,  cgnineattach */
int	cgnineprobe(), cgnineattach();

/*  Main Bus device instance for CG9	*/
struct	mb_device 	*cgnineinfo[NCGNINE];

/*  Main Bus device driver for CG9  */
struct	mb_driver	cgninedriver =
{
    cgnineprobe,		/* Check CG9 dev./controller install	*/
    0,				/* N/A - No CG9 Slave Device		*/
    cgnineattach,		/* CG9 Boot-time device installation	*/
    0,				/* N/A - start transfer			*/
    0,				/* N/A - finish transfer		*/
    0,				/* N/A - Polling Interrupt Routine	*/
    CG9_PROBESIZE,		/* Bytes of memory space needed for CG9	*/
    "cgnine",			/* Char String "name" of the CG9	*/
    cgnineinfo,			/* Back-pointer to mbdinit structs	*/
    0,				/* N/A - name of the controller		*/
    0,				/* N/A - pointer to mbcinit struct	*/
    0,				/* N/A - main bus usage flags		*/
    0				/* N/A - interrupt routine link list	*/
};


/* Build separate structures for each cg9 board in the system. */
struct cg9_softc
{
    int		w;			/* width			*/
    int		h;			/* height			*/
    int		fb32size;		/* total size of frame buffer	*/
    int		bw2size;		/* total size of overlay plane	*/
    struct	proc	*owner;		/* owner of the frame buffer	*/
    struct	fbgattr	*gattr;		/* current attributes		*/
    caddr_t	cg9_gp_shmem;
    int		cg9_gp_fd;
    u_long	cg9_physical;
    short	*ovlptr;
    short	*enptr;
    union	fbunit	cmap_rgb[CG9_CMAP_SIZE];
    int		cmap_begin;
    int		cmap_end;
    union	fbunit	omap_rgb[CG9_OMAP_SIZE];
    int		omap_begin;
    int		omap_end;
    u_char	em_red[CG9_CMAP_SIZE], em_green[CG9_CMAP_SIZE],
		    em_blue[CG9_CMAP_SIZE];	/* indexed cmap emulation */
    struct	cg9_control_sp	*ctrl_sp;
    struct	ramdac		*lut;
#if NWIN > 0
    Pixrect	pr;
#endif NWIN > 0
    struct	cg9_data	cg9d;
} cg9_softc[NCGNINE];


/*	Default CG9 structure for FBIOGTYPE ioctl	*/
static	struct	fbtype	cg9typedefault =
{
    FBTYPE_SUNROP_COLOR,		/* Frame Buffer type		*/
    CG9_HEIGHT,				/* Height (Pixels)		*/
    CG9_WIDTH,				/* Width (Pixels)		*/
    CG9_DEPTH,				/* Depth (Bits)			*/
    CG9_CMAP_SIZE,			/* Color Map Size		*/
    0					/* Size				*/
}; 


/*	Default CG9 structure for FBIOGATTR ioctl	*/
static	struct	fbgattr	cg9attrdefault =
{
    FBTYPE_SUNROP_COLOR,	       /* Real Type			 */
	0,			       /* PID of Owner (0=self)	 */
    {
	FBTYPE_SUNROP_COLOR,	       /* Frame Buffer type		 */
	    0,			       /* Height (Pixels)		 */
	    0,			       /* Width (Pixels)		 */
	    32,			       /* Depth (Bits)			 */
	    256,		       /* Color Map Size		 */
	    0			       /* Size				 */
    },
    {
	0,			       /* fbsattr:  flags		 */
	    FBTYPE_SUN2BW,	       /* bwtwo */
	{
	    0, 0, 0, 0
	}			       /* dev_specific			 */
    },
    {
	FBTYPE_SUNROP_COLOR,	       /* DEVICE emulates itself!!!	 */
	    FBTYPE_SUN2BW,	       /* bwtwo */
	    FBTYPE_MEMCOLOR,	       /* cgeight */
	    -1			       /* No device emulation		 */
    }
};

struct	addrmap
{
    u_int vaddr;
    u_int paddr;
    int   size;
}	addr_table[] =
{
    CG9_ADDR_OVERLAY, CG9_OVERLAY_BASE, 0,
    0, CG9_ENABLE_BASE, 0,
    0, CG9_COLOR_FB_BASE, 0,
    0, CG9_CTRL_BASE, ctob(1)
};

/* frame buffer description table */
static	struct	cg9_fbdesc
{
    short           depth;	       /* depth, bits			*/
    short           group;	       /* plane group			*/
    int             allplanes;	       /* initial plane mask		*/
}	cg9_fbdesc[CG9_NFBS] =
{
    { 1, PIXPG_OVERLAY, 0 },
    { 1, PIXPG_OVERLAY_ENABLE, 0 } ,
    { 32, PIXPG_24BIT_COLOR, 0x00ffffff }
};

/* cg9 private data frame buffer descriptor indices (see cg9_fbdesc above) */
#define	CG9_FBINDEX_OVERLAY		0
#define	CG9_FBINDEX_ENABLE		1
#define	CG9_FBINDEX_COLOR		2

/* initial active frame buffer index */
#ifndef	CG9_INITFB
#define	CG9_INITFB		CG9_FBINDEX_OVERLAY
#endif

#if NWIN > 0
struct	pixrectops	cg9_ops =
{
    cg9_rop,
    cg9_putcolormap,
    cg9_putattributes,
#   ifdef _PR_IOCTL_KERNEL_DEFINED
    cg9_ioctl
#   endif
};
#endif NWIN > 0

/*************************************************************************/

/*
 *	CG9 Probe
 *      	1.  Determine if a CG9 exists at expected address
 * 		2.  Map 1st section of CG9 (Control Registers) into Kernel
 *		3.  Allocate kernel space for all sections  
 *		4.  Complete virtual addr. of first section  
 *		5.  Map into Kernel:  Overlay Memory, Enable Plane Memory
 */

cgnineprobe(reg, unit)
caddr_t         reg;
int             unit;
{
    u_int           hat_getkpfnum ();
    u_long          rmalloc ();
    register struct cg9_control_sp *fb;
    register struct cg9_softc *softc = &cg9_softc[unit];
    int             pages1,
                    pages32;
    register caddr_t fbaddr;
    long            poke_value;
    u_long          kmx;
    int	s;

#   ifdef CG9_DEBUG
    int             trial_count = 0;
#   endif


    fb = (struct cg9_control_sp *) reg;

#   ifdef CG9_DEBUG
    ring_init ();
#   endif

    /* if CG9 not found */
    if (peekl ((long *) &fb->ident, &poke_value))
	return (0);
    if (!(poke_value & CG9_ID))
	return (0);

    if (peekl ((long *) fb, &poke_value) ||
	pokel ((long *) fb, ~poke_value) ||
	pokel ((long *) fb, poke_value))	/* restore old value */
	BAD_CG9 ();

    softc->cg9_physical = ((u_long) hat_getkpfnum ((addr_t) reg) << PAGESHIFT) |
	((u_long) reg & PAGEOFFSET);
    softc->ctrl_sp = fb;
    softc->lut = &softc->ctrl_sp->ramdac_reg;
    softc->w = CG9_WIDTH;
    softc->h = CG9_HEIGHT;

    /* Calculate page size for 1-bit frame buffer */
    pages1 = btoc (mpr_linebytes (softc->w, 1) * softc->h);
    softc->bw2size = ctob (pages1);

    /* Calculate page size for 32-bit frame buffer */
    pages32 = btoc (mpr_linebytes (softc->w, 32) * softc->h);
    softc->fb32size = ctob (pages32);

    /* Map CG9 sections into Kernel:  Overlay and Enable Planes */
    s = splhigh();
    kmx = rmalloc(kernelmap, (long) (pages1 + pages1));
    (void)splx(s);
    if (kmx == 0) {
	(void) printf ("cgnineprobe: out of kernelmap\n");
	return (0);		       /* must free from now on */
    }

    /* compute virtual addr. of first section */
    fbaddr = (caddr_t) kmxtob (kmx);

    /* POKE No. 2  -  Top of Overlay Memory  */
    fbmapin (fbaddr, (int) CG9_PHYSICAL (CG9_OVERLAY_BASE), ctob (pages1));
    if (peekl ((long *) fbaddr, &poke_value) ||
	pokel ((long *) fbaddr, ~poke_value) ||
	pokel ((long *) fbaddr, poke_value))
	BAD_CG9 ();
    /* Successful POKE No. 2  -  Let's keep this address  */
    softc->ovlptr = (short *) fbaddr;
    fbaddr += softc->bw2size;

    /* POKE No. 3  -  Top of Enable Plane Memory  */
    fbmapin (fbaddr, (int) CG9_PHYSICAL (CG9_ENABLE_BASE), ctob (pages1));
    if (peekl ((long *) fbaddr, &poke_value) ||
	pokel ((long *) fbaddr, ~poke_value) ||
	pokel ((long *) fbaddr, poke_value))
	BAD_CG9 ();
    /* Successful POKE No. 3  -  Let's keep this address  */
    softc->enptr = (short *) fbaddr;

    return (CG9_PROBESIZE);	       /* probe succeeded */

bad_cg9:			       /* where BAD_CG9 comes */
    s = splhigh();
    rmfree (kernelmap, (long) (pages1 + pages1), kmx);
    (void)splx(s);
    return (0);
}

/* cgnineattach: set up CG9 board to known state */
cgnineattach(md)
register struct	mb_device	*md;
{
    register struct cg9_control_sp *ctrl_sp;
    register u_int  cg9_ctrl_reg;
    register struct cg9_softc *softc = &cg9_softc[md->md_unit];


    /* set up CG9 control register space */
    ctrl_sp = softc->ctrl_sp;

    /* insure vertical blank interrupts are disabled on the board */
    DISABLE_INTR(ctrl_sp->ctrl);

    /* setup and initialize   CG9/VME interrupt vector */
    if (!(ctrl_sp->cg9_intr = md->md_intr->v_vec))
	printf("WARNING: no CG9 interrupt vector found in config file\n");

    cg9_ctrl_reg = ctrl_sp->ctrl;      /* read from hw */
    cg9_ctrl_reg &= (~CG9_REG_GP2_INTR_SET &	/* gp2 does not interrupt */
		     ~CG9_REG_GP2_INTR_ENAB &
		     ~CG9_REG_P2_ENABLE &	/* disable p2 first */
		     ~CG9_REG_STEREO & /* no stereo */
		     ~CG9_REG_DAC_DIAG);
    cg9_ctrl_reg |= CG9_REG_CMAP_MODE; /* 3-color overlay style */
    ctrl_sp->ctrl = cg9_ctrl_reg;      /* write to hw */
    ctrl_sp->dbl_reg = CG9_NO_WRITE_A | CG9_NO_WRITE_B;

    INIT_BT458(softc->lut);

    /* sync up ramdac */
    cg9_ctrl_reg &= ~CG9_REG_DAC_SYNC;
    ctrl_sp->ctrl = cg9_ctrl_reg;
    cg9_ctrl_reg |= CG9_REG_DAC_SYNC;
    ctrl_sp->ctrl = cg9_ctrl_reg;
    cg9_ctrl_reg &= ~CG9_REG_DAC_SYNC;
    ctrl_sp->ctrl = cg9_ctrl_reg;

    ctrl_sp->bitblt = 0;
    ctrl_sp->pixel_count = 0;

    ctrl_sp->plane_mask = PIX_ALL_PLANES;

    INIT_OCMAP(softc->omap_rgb);
    INIT_CMAP(softc->cmap_rgb, CG9_CMAP_SIZE);

    softc->omap_begin = 0;
    softc->omap_end = 4;
    softc->cmap_begin = 0;
    softc->cmap_end = CG9_CMAP_SIZE;
    softc->cg9d.flags |= CG9_OVERLAY_CMAP | CG9_24BIT_CMAP;

    ENABLE_INTR(softc->ctrl_sp->ctrl);

    /* set up address translation tables */
    addr_table[0].size = softc->bw2size;
    addr_table[1].size = softc->bw2size;
    addr_table[2].size = softc->fb32size;
    addr_table[1].vaddr = addr_table[0].vaddr + addr_table[0].size;
    addr_table[2].vaddr = addr_table[1].vaddr + addr_table[1].size;
    addr_table[3].vaddr = addr_table[2].vaddr + addr_table[2].size;

    cg9typedefault.fb_width = softc->w;
    cg9typedefault.fb_height = softc->h;
    cg9typedefault.fb_size = softc->fb32size + (2 * softc->bw2size) +
	sizeof (struct cg9_control_sp);
    cg9attrdefault.fbtype = cg9typedefault;
    softc->gattr = &cg9attrdefault;

    return (0);
}

/* CG9 "Virtual-Physical" Memory Mapping */
/* ARGSUSED */
cgninemmap (dev, off, flag)
dev_t           dev;
off_t           off;
int             flag;
{
    register struct addrmap *tblptr;
    register	struct	cg9_softc	*softc = &cg9_softc[minor(dev)];

    for (tblptr=addr_table; tblptr <= addr_table+CG9_ADDR_TABLE_SIZE; tblptr++)
	if (off >= tblptr->vaddr && (off < (tblptr->vaddr + tblptr->size)))
	    return (CG9_PHYSICAL ((u_int) off - tblptr->vaddr + tblptr->paddr));

    return -1;
}


cgnineopen(dev, flag)
dev_t	dev;
int	flag;
{ return (fbopen (dev, flag, NCGNINE, cgnineinfo)); }

/* ARGSUSED */
cgnineclose(dev, flag)
dev_t	dev;
int	flag;
{ return (0); }


/*
 *	cgnineioctl:	Parse out to the eight FBIO system calls,  and
 *			take seperate action for each one.
 */
/* ARGSUSED */
cgnineioctl (dev, cmd, data, flag)
dev_t           dev;
int             cmd;
caddr_t         data;
int             flag;
{
    register		int		i,j;
    register	struct	cg9_softc	*softc = &cg9_softc[minor(dev)];

#ifdef CG9_DEBUG
    ring_dump();
#endif    

    switch (cmd)
    {
	case FBIOGTYPE:
	{
	    register struct fbtype *fb = (struct fbtype *) data;
	    /* set owner if not owned, or if prev. owner=dead  */

	    if (softc->owner == NULL ||
		softc->owner->p_stat == NULL ||
		(int) softc->owner->p_pid != softc->gattr->owner) {

		softc->owner = u.u_procp;
		softc->gattr->owner = (int) u.u_procp->p_pid;
	    }
	    *fb = cg9typedefault;
	    fb->fb_height = softc->h;
	    fb->fb_width = softc->w;
	    if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW) {
		fb->fb_type = FBTYPE_SUN2BW;
		fb->fb_size = softc->bw2size;
		fb->fb_depth = 1;
		fb->fb_cmsize = 2;
	    }
	    else if (softc->gattr->sattr.emu_type == FBTYPE_MEMCOLOR) {
		fb->fb_type = FBTYPE_MEMCOLOR;
		fb->fb_size = softc->fb32size;
		fb->fb_depth = CG9_DEPTH;
		fb->fb_cmsize = CG9_CMAP_SIZE;
	    }
	}
	break;

#if NWIN > 0
	case FBIOGPIXRECT:
	{
	    struct	fbpixrect	*fbpix = (struct fbpixrect *) data;
	    register	struct	cg9fb		*fbp;
	    register	struct	cg9_fbdesc	*descp;

	    /* "Allocate" pixrect and private data */
	    fbpix->fbpr_pixrect = &softc->pr;
	    softc->pr.pr_data = (caddr_t) & softc->cg9d;
	    softc->cg9d.ctrl_sp = softc->ctrl_sp;

	    /* initialize pixrect */
	    softc->pr.pr_ops = &cg9_ops;
	    softc->pr.pr_size.x = softc->w;
	    softc->pr.pr_size.y = softc->h;

	    /* initialize private data */
	    softc->cg9d.flags = 0;
	    softc->cg9d.planes = 0;
	    softc->cg9d.fd = minor (dev);

	    /* set up everything except for the image pointer */
	    for (fbp=softc->cg9d.fb, descp=cg9_fbdesc, i=0; i<CG9_NFBS;
		fbp++, descp++, i++)
	    {
		fbp->group = descp->group;
		fbp->depth = descp->depth;
		fbp->mprp.mpr.md_linebytes=mpr_linebytes(softc->w,descp->depth);
		fbp->mprp.mpr.md_offset.x = 0;
		fbp->mprp.mpr.md_offset.y = 0;
		fbp->mprp.mpr.md_primary = 0;
		fbp->mprp.mpr.md_flags = descp->allplanes != 0
		    ? MP_DISPLAY | MP_PLANEMASK
		    : MP_DISPLAY;
		fbp->mprp.planes = descp->allplanes;
	    }

	    /*
	     * switch to each plane group, set up image pointers and
	     * initialzed the images
	     */
	    {
		u_int           initplanes;
		Pixrect        *tmppr = &softc->pr;

		initplanes = PIX_GROUP (cg9_fbdesc[CG9_FBINDEX_COLOR].group)
		     | cg9_fbdesc[CG9_FBINDEX_COLOR].allplanes;

		(void) cg9_putattributes (tmppr, &initplanes);
		cg9_d (tmppr)->fb[CG9_FBINDEX_COLOR].mprp.mpr.md_image =
		    cg9_d (tmppr)->mprp.mpr.md_image = (short *) -1;
		cg9_d (tmppr)->mprp.mpr.md_linebytes = 0;

		initplanes = PIX_GROUP(cg9_fbdesc[CG9_FBINDEX_ENABLE].group)
		     | cg9_fbdesc[CG9_FBINDEX_ENABLE].allplanes;
		(void) cg9_putattributes (tmppr, &initplanes);

		cg9_d (tmppr)->fb[CG9_FBINDEX_ENABLE].mprp.mpr.md_image =
		    cg9_d (tmppr)->mprp.mpr.md_image = softc->enptr;
		pr_rop (&softc->pr, 0, 0,
			softc->pr.pr_size.x, softc->pr.pr_size.y,
			PIX_SRC | PIX_COLOR (1), (Pixrect *) 0, 0, 0);

		initplanes = PIX_GROUP (cg9_fbdesc[CG9_INITFB].group)
		     | cg9_fbdesc[CG9_INITFB].allplanes;
		(void) cg9_putattributes (&softc->pr, &initplanes);

		cg9_d (tmppr)->fb[CG9_INITFB].mprp.mpr.md_image =
		    cg9_d (tmppr)->mprp.mpr.md_image = softc->ovlptr;
		pr_rop (tmppr, 0, 0,
			softc->pr.pr_size.x, softc->pr.pr_size.y,
			PIX_SRC, (Pixrect *) 0, 0, 0);

	    }
	}
	break;
#endif NWIN > 0

	case FBIOGINFO:
	{
	    register	struct	fbinfo	*fbinfo = (struct fbinfo *) data;

	    fbinfo->fb_physaddr = CG9_PHYSICAL(CG9_COLOR_FB_BASE);
	    fbinfo->fb_hwwidth = softc->w;
	    fbinfo->fb_hwheight = softc->h;
	    fbinfo->fb_ropaddr = (unsigned char *) softc->ctrl_sp;    /* bad name "ropadr" */
	}
	break;

	case FBIOPUTCMAP:
	case FBIOGETCMAP:
	{
	    register	struct	fbcmap	*cmap = (struct fbcmap *) data;
	    register	u_int		index = (u_int) cmap->index;
	    register	u_int		count = (u_int) cmap->count;
	    register	u_int		pgroup = PIX_ATTRGROUP(index);
	    register	union	fbunit	*map;
	    register	union	fbunit 	*lutptr;
	    register	int		*begin;
	    register	int		*end;
	    register	int		entries;
	    register	u_int		intr_flag;
	    u_char	tmp[3][CG9_CMAP_SIZE];

	    if (count == 0 || !cmap->red || !cmap->green || !cmap->blue)
		return (EINVAL);

	    switch (pgroup)
	    {
		case 0:
		case PIXPG_24BIT_COLOR:
		    lutptr	= &softc->lut->lut_data;
		    begin	= &softc->cmap_begin;
		    end		= &softc->cmap_end;
		    map		= softc->cmap_rgb;
		    intr_flag	= CG9_24BIT_CMAP;
		    entries	= CG9_CMAP_SIZE;
		    break;

		case PIXPG_OVERLAY:
		    lutptr	= &softc->lut->overlay;
		    begin	= &softc->omap_begin;
		    end		= &softc->omap_end;
		    map 	= softc->omap_rgb;
		    intr_flag	= CG9_OVERLAY_CMAP;
		    entries	= CG9_OMAP_SIZE;
		    break;

		default:	return (EINVAL);
	    }

	    if ((index &= PIX_ALL_PLANES) >= entries || index+count > entries)
		return (EINVAL);

	    if (cmd == FBIOPUTCMAP)
	    {
		if (softc->cg9d.flags & CG9_KERNEL_UPDATE)
		{
		    D_PRINT ("kernel putcmap\n", 0);
		    softc->cg9d.flags &= ~CG9_KERNEL_UPDATE;
		    bcopy ((caddr_t) cmap->red, (caddr_t) tmp[0], count);
		    bcopy ((caddr_t) cmap->green, (caddr_t) tmp[1], count);
		    bcopy ((caddr_t) cmap->blue, (caddr_t) tmp[2], count);
		}
		else
		{
		    D_PRINT("user putcmap\n",0);
		    if (copyin((caddr_t)cmap->red,(caddr_t)tmp[0],count) ||
			copyin((caddr_t)cmap->green,(caddr_t)tmp[1],count) ||
			copyin((caddr_t)cmap->blue,(caddr_t)tmp[2],count))
			return (EFAULT);
		}

		if (!(cmap->index & PR_FORCE_UPDATE) &&
		    intr_flag == CG9_24BIT_CMAP)	/* emulated index */
		{
		    for (i=0; i<count; i++,index++)
		    {
			softc->em_red[index] = tmp[0][i];
			softc->em_green[index] = tmp[1][i];
			softc->em_blue[index] = tmp[2][i];
		    }
		    return (0);
		}

		softc->cg9d.flags |= intr_flag;
		*begin = MIN(*begin,index);
		*end = MAX(*end,index+count);
		D_PRINT("pcmap: f=%d ",softc->cg9d.flags);
		D_PRINT("b=%d ",*begin);
		D_PRINT("e=%d ",*end);
		D_PRINT("i=%d ",index);
		D_PRINT("c=%d\n",count);

		for (i=0,map+=index; count; i++,count--,map++)
		    map->packed =
			tmp[0][i]|(tmp[1][i]<<8)|(tmp[2][i]<<16);

		ENABLE_INTR(softc->ctrl_sp->ctrl);
		break;
	    }
	    else		/* FBIOGETCMAP */
	    {
		if (!(cmap->index & PR_FORCE_UPDATE) &&
		    intr_flag == CG9_24BIT_CMAP)	/* emulated index */
		{
		    for (i=0; i<count; i++,index++)
		    {
			tmp[0][i] = softc->em_red[index];
			tmp[1][i] = softc->em_green[index];
			tmp[2][i] = softc->em_blue[index];
		    }
		}
		else
		{
		    if (softc->cg9d.flags & intr_flag) cgnine_wait(minor(dev));
		    ASSIGN_LUT(softc->lut->addr_reg, index);
		    for (i=0,j=count; j; i++,j--)
		    {
			tmp[0][i] = (lutptr->packed & 0x0000ff);
			tmp[1][i] = (lutptr->packed & 0x00ff00)>>8;
			tmp[2][i] = (lutptr->packed & 0xff0000)>>16;
		    }
		}
		if (copyout((caddr_t)tmp[0],(caddr_t)cmap->red,count) ||
		    copyout((caddr_t)tmp[1],(caddr_t)cmap->green,count) ||
		    copyout((caddr_t)tmp[2],(caddr_t)cmap->blue,count))
		    return (EFAULT);
		break;
	    }
	}

	case FBIOSATTR:
	{

		register struct fbsattr *sattr = (struct fbsattr *) data;
		softc->gattr->sattr.flags = sattr->flags;

		if (sattr->emu_type != -1)
			softc->gattr->sattr.emu_type = sattr->emu_type;

		if (sattr->flags & FB_ATTR_DEVSPECIFIC)
			bcopy((char *) sattr->dev_specific,
				(char *) softc->gattr->sattr.dev_specific,
				sizeof sattr->dev_specific);
		/* under new ownership */
		softc->owner = u.u_procp;
		softc->gattr->owner = (int) u.u_procp->p_pid;

	}
	break;
	case FBIOGATTR:
	{
	    register struct fbgattr *gattr = (struct fbgattr *) data;
	    /* set owner if not owned, or if prev.  owner is dead  */
	    if (softc->owner == NULL ||
		softc->owner->p_stat == NULL ||
		(int) softc->owner->p_pid != softc->gattr->owner) {
		softc->owner = u.u_procp;
		softc->gattr->owner = (int) u.u_procp->p_pid;
	    }

	    *gattr = *(softc->gattr);
	    if (u.u_procp == softc->owner)
		gattr->owner = 0;
	}
	break;

	case FBIOSVIDEO:
	{
	    register	int	on = *(int *) data & FBVIDEO_ON;
	    register	struct	cg9_control_sp	*csp;

	    csp = softc->ctrl_sp;
	    if (csp->ctrl)
	    {
		csp->ctrl = on	? csp->ctrl | CG9_REG_VIDEO_ON
				: csp->ctrl & ~CG9_REG_VIDEO_ON; 
	    }
	    else
		setvideoenable ((u_int) on);
	}
	break;

	case FBIOGVIDEO:
	    *(int *) data = softc->ctrl_sp->ctrl & CG9_REG_VIDEO_ON
		? FBVIDEO_ON : FBVIDEO_OFF;
	    break;

	case FBIOVERTICAL:
	    if (*(int *) data == 1) softc->cg9d.flags |= CG9_DOUBLE_BUF_A;
	    else if (*(int *) data == 2) softc->cg9d.flags |= CG9_DOUBLE_BUF_B;
	    cgnine_wait(minor(dev));
	    break;

	default:		return (ENOTTY);
	}

#	ifdef CG9_DEBUG
	ring_dump();
#	endif    

	return (0);
}

cgnine_wait(unit)
int     unit;
{
    register    struct  mb_device       *md = cgnineinfo[unit & 255];
    register    struct  cg9_softc       *softc = &cg9_softc[unit & 255];
    int         s;

    if (softc->ctrl_sp->cg9_intr == 0) return;
    D_PRINT("cgnine wait, entered\n", 0);

    s = splx(pritospl(md->md_intpri));
    ENABLE_INTR(softc->ctrl_sp->ctrl);
    softc->cg9d.flags |= CG9_SLEEPING;
    (void) sleep((caddr_t) softc, PZERO - 1);
    (void) splx(s);
    D_PRINT("cgnine wait, waken up\n", 0);
}

/*
 *	"cgnineintr",  invoked through vertical blank vector interrupt.
 */
cgnineintr(unit)
int	unit;
{
    register int    i;
    register union fbunit *lutptr;
    register union fbunit *map;
    register struct cg9_softc *softc = &cg9_softc[unit];
    register struct mb_device *md = cgnineinfo[unit & 255];
    int             serviced = 0;

    DISABLE_INTR(softc->ctrl_sp->ctrl);

    if (softc->cg9d.flags & (CG9_DOUBLE_BUF_A | CG9_DOUBLE_BUF_B)) {
	softc->ctrl_sp->dbl_disp = (softc->cg9d.flags & CG9_DOUBLE_BUF_A) ?
		0 : -1;
	softc->cg9d.flags &= ~(CG9_DOUBLE_BUF_A | CG9_DOUBLE_BUF_B);
	serviced++;
    }
    if (softc->cg9d.flags & CG9_OVERLAY_CMAP) {
	D_PRINT("overlay cmap s=%d", softc->omap_begin);
	D_PRINT(" e=%d\n", softc->omap_end);
	softc->cg9d.flags &= ~CG9_OVERLAY_CMAP;
	ASSIGN_LUT(softc->lut->addr_reg, softc->omap_begin);
	lutptr = &softc->lut->overlay;
	map = softc->omap_rgb + softc->omap_begin;
	for (i = softc->omap_begin; i < softc->omap_end; i++, map++) {
	    lutptr->packed = map->packed;
	    lutptr->packed = map->packed;
	    lutptr->packed = map->packed;
	}
	softc->omap_begin = CG9_OMAP_SIZE;
	softc->omap_end = 0;
	serviced++;
    }
    if (softc->cg9d.flags & CG9_24BIT_CMAP) {
	D_PRINT("cmap s=%d", softc->cmap_begin);
	D_PRINT(" e=%d\n", softc->cmap_end);
	softc->cg9d.flags &= ~CG9_24BIT_CMAP;
	ASSIGN_LUT(softc->lut->addr_reg, softc->cmap_begin);
	lutptr = &softc->lut->lut_data;
	map = softc->cmap_rgb + softc->cmap_begin;
	for (i = softc->cmap_begin; i < softc->cmap_end; i++, map++) {
	    lutptr->packed = map->packed;
	    lutptr->packed = map->packed;
	    lutptr->packed = map->packed;
	}
	softc->cmap_begin = CG9_CMAP_SIZE;
	softc->cmap_end = 0;
	serviced++;
    }
    if (softc->cg9d.flags & CG9_SLEEPING) {
	softc->cg9d.flags &= ~CG9_SLEEPING;
	wakeup((caddr_t) softc);
	serviced++;
    }

#   ifdef lint
    (void) cgnineintr(unit);	/* oh, shut up */
#   endif
    return (serviced);
}



#ifdef CG9_DEBUG
/*
 * Ring buffer utilities for debugging (especially for kernel debugging).
 * The idea is simple: have a ring buffer which stores one integer value
 * and the format to print the value.  Dump the content of the ring once in
 * a while. 
 *
 * The idea is from Greg Limes.  I make them a little object oriented.
 */

#define	MAX_EVENT	(1<<10)
#define	EVENT_MASK	(MAX_EVENT-1)


struct ringbuf {
    char           *fmt;
    int             arg;
};

struct {
    int             prtflg;
    int             lastdump;
    int             next_event;
    struct ringbuf      ringbuf[MAX_EVENT]; /* avoid malloc for kernel debugging */
}               ring;

ring_init ()
{
    ring.prtflg = 1;	/* default is to print */
    ring.lastdump = 0;
    ring.next_event = 0;
}


ring_trigger ()
{
    ring.prtflg = 1;		       /* make dump do printing */
}

ring_addseq (format, value)
    char           *format;
    int             value;
{
    ring.ringbuf[ring.next_event & EVENT_MASK].fmt = format;
    ring.ringbuf[ring.next_event & EVENT_MASK].arg = value;
    ring.next_event++;
    if (ring.next_event >= MAX_EVENT)
	ring.next_event &= EVENT_MASK;
    if (ring.next_event == ring.lastdump)
	ring.lastdump++;    /* trashed */
}


ring_dump ()
{
    int             endprt;
    int            *last = &ring.lastdump;
    struct ringbuf     *bufptr = ring.ringbuf;


    if (!ring.prtflg)
	return;
    /*ring.prtflg = 0;*/	       /* has to be triggered everytime */
    /*ring_addseq ("dumping seq\n", 0);*/
    endprt = ring.next_event;

    for (; *last != endprt;
	 *last = (*last + 1) & EVENT_MASK) {	/* handle wraparound */
	printf (bufptr[*last].fmt, bufptr[*last].arg);
    }
    /*ring_addseq ("seq dump done\n", 0);*/
}

#endif CG9_DEBUG
