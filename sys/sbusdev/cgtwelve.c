#ifndef lint
static char sccsid[] = "@(#)cgtwelve.c 1.1 94/10/31 SMI";
#endif

/*
 * Sbus accelerated 24 bit color frame buffer driver
 */

/*
 * EIC-COMMENTS
 * There are two hardware bugs in the EIC ASIC (SBus to Local bus interface
 * Chip). One requires a write to some non-eic location following any host
 * updates to an eic register. This requires special attention to any code
 * involving the eic and should be carefully examined before making any
 * changes. apu.res2 is the APU-ID read only register thus theoretically
 * safe to write at any time.
 * Second one requires a write to an eic location if a write access caused
 * an eic timer interrupt. Hence the dummy write to eic.eic_control which 
 * is the EIC_ID read only register in the cg12_poll routine.
 */

#include "cgtwelve.h"
#include "win.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vmmac.h>
#include <sys/mman.h>

#include <sun/fbio.h>
#include <sun/gpio.h>

#include <sundev/mbvar.h>
#include <sbusdev/cg12reg.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/seg_kmem.h>
#include <machine/vm_hat.h>
#include <machine/psl.h>
#include <machine/cpu.h>

#include <vm/as.h>
#include <vm/seg.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/pr_dblbuf.h>
#include <pixrect/memvar.h>
#include <pixrect/gp1reg.h>
#include <pixrect/cg12_var.h>


addr_t kmem_zalloc();
addr_t map_regs();
void unmap_regs();
void report_dev();

/* configuration options */
#ifndef OLDDEVSW
#define OLDDEVSW 1
#endif  OLDDEVSW

#define NOHWINIT 1

#ifndef NOHWINIT
#define NOHWINIT 0
#endif  NOHWINIT

#if NOHWINIT
int cg12_hwinit = 0;
#endif NOHWINIT

#define CG12DEBUG 1

#ifndef CG12DEBUG
#define CG12DEBUG 0
#endif CG12DEBUG

#if CG12DEBUG
int cg12_debug = 0;
#define	DEBUGF(level, args)	_STMT(if (cg12_debug >= (level)) printf args;)
#else CG12DEBUG
#define	DEBUGF(level, args)
#endif CG12DEBUG

#if OLDDEVSW
#define	STATIC /* nothing */
#define	cg12_open	cgtwelveopen
#define	cg12_close	cgtwelveclose
#define	cg12_mmap	cgtwelvemmap
#else OLDDEVSW
#define	STATIC	static
#endif OLDDEVSW

/* config info */
static int cg12_identify();
static int cg12_attach();
STATIC int cg12_open();
STATIC int cg12_close();
STATIC int cgtwelveioctl();
STATIC int cgtwelvesegmap();

#define CG12_OWNER_WANTS_COLOR 	4	
#define CG12_PHYSADDR(p) (softc->basepage + btop(p))
#define RAMDACWRITE(x)  ((x) << 16 | (x) << 8 | (x))
#define FBUNIT(unit)	((unit) &3)
/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
#define ENABLE_INTR(softc) \
	((softc)->ctl_sp->apu.msg0 |= CG12_APU_EN_HINT, \
	(softc)->ctl_sp->eic.interrupt |= CG12_EIC_EN_HINT, \
	(softc)->ctl_sp->apu.res2 = 0)
/* !!! */
#define	getsoftc(unit)	(&cg12_softc[unit])
#define	btob(n)		ctob(btoc(n))

#define NMINOR	128

struct dev_ops cgtwelve_ops = {
	0,		/* revision */
	cg12_identify,
	cg12_attach,
	cg12_open,
	cg12_close,
	0, 0, 0, 0, 0,
	cgtwelveioctl,
	0,
	cgtwelvesegmap,
};

struct cg12_mapped
{
    struct cg12_mapped *next;		/* -> next process */
    struct cg12_mapped *prev;		/* -> prior process */
    int        pid;			/* owner of the mappings */
    int        total;			/* total size mapped by this process */
};
#define	NULL_MAPPED	((struct cg12_mapped *) 0)

/* driver per-unit data */
struct cg12_softc
{
    int                 w;		/* width */
    int                 h;		/* height */
    int                 size;		/* size of frame buffer */
    int                 bw2size;	/* size of overlay */
    int                 gb;		/* for gp2 compatibility */
    addr_t		base;		/* mapped base address */
    int			basepage;	/* page # for base address */
    struct proc        *owner;		/* owner of the frame buffer */
    struct fbgattr     *gattr;		/* current attrributes */
    u_int		dev_id;		/* device id for gpconfig */
    int                 restart;	/* restart count */
    u_int               dsp_active;	/* dsp is active */
    u_int               ucode_ver;	/* version of gs.ucode */
    struct cg12_ctl_sp *ctl_sp;		/* control space */
    struct gp1_shmem   *shmem;		/* shmem pointer */
    char               *o_ptr;		/* overlay and enable pointers */
    char	       *e_ptr;		/* for cursor */
    char	       *i_ptr;		/* for 8 bit indexed pointer */
    char	       *w_ptr;		/* for wid plane pointer */
    char	       *d_ptr;		/* for dram pointer */
    int			reg_bustype;
    addr_t		reg_addr;
    int			cg12pri;
    struct cg12_mapped *total_mapped;
    struct seg	       *curcntxt;
    u_int		cntxt_regs[6];
    struct seg_tbl
    {
    	struct proc	*owner;
	int		gpminor;
    }			seg_tbl[NMINOR];
    union	fbunit	cmap_rgb[CG12_CMAP_SIZE];
    union	fbunit	cmap_indexed[CG12_CMAP_SIZE];
    union	fbunit	cmap_a12[CG12_CMAP_SIZE];
    union	fbunit	cmap_b12[CG12_CMAP_SIZE];
    union	fbunit	cmap_alt[CG12_CMAP_SIZE];
    union	fbunit	cmap_overlay[4];
    union	fbunit	cmap_4bit_ovr[CG12_OMAP_SIZE - 4];
    int			cmap_begin_rgb;
    int			cmap_end_rgb;
    int			cmap_begin_indexed;
    int			cmap_end_indexed;
    int			cmap_begin_a12;
    int			cmap_end_a12;
    int			cmap_begin_b12;
    int			cmap_end_b12;
    int			cmap_begin_alt;
    int			cmap_end_alt;
    int			cmap_begin_overlay;
    int			cmap_end_overlay;
    int			cmap_begin_4bit_ovr;
    int			cmap_end_4bit_ovr;
    struct cg12_wid_alloc
    {
	struct proc	*owner;
	int		type;
    }			wid_alloc[NWIDS];
    int			wid_tbl[NWIDS];
    int			wid_begin;
    int			wid_end;
    struct gp1_sblock
    {
	struct proc     *owner;
	int             gpminor;
    }                   sblock[CG12_STATIC_BLOCKS];
    struct cg12_data    cg12d;
#if NWIN > 0
    Pixrect             pr;		/* kernel pixrect and private data */
#endif					/* NWIN > 0 */
    struct addrmap	*addr_tbl;
};

struct cg9_fbdesc
{
    short               depth;	/* depth, bits */
    short               group;	/* plane group */
    int                 allplanes;	/* initial plane mask */
} cg12_fb_desc[CG12_NFBS] =
{
    {  1, PIXPG_OVERLAY,		0  },
    {  1, PIXPG_OVERLAY_ENABLE,		0  },
    { 32, PIXPG_24BIT_COLOR,	0x00ffffff },
    {  8, PIXPG_8BIT_COLOR,	0x000000ff },
    {  8, PIXPG_WID,		0x0000003f },
    { 16, PIXPG_ZBUF,		0x0000ffff }
};

/*
 * Table of CG12 addresses
 */
struct addrmap
{
    u_int               offset;		/* mmap offset */
    u_int               paddr;		/* physical address */
    u_int               size;		/* size */
    u_char              prot;		/* superuser flag */
};

struct addrmap addr_tbl_lr[] = 
{
    { CG12_VOFF_OVERLAY,	CG12_OFF_OVERLAY0,	CG12_OVERLAY_SIZE, 0 },
    { CG12_VOFF_ENABLE,		CG12_OFF_OVERLAY1,	CG12_ENABLE_SIZE,  0 },
    { CG12_VOFF_COLOR24,	CG12_OFF_INTEN,		CG12_COLOR24_SIZE, 0 },
    { CG12_VOFF_COLOR8,		CG12_OFF_INTEN,		CG12_COLOR8_SIZE,  0 },
    { CG12_VOFF_WID,		CG12_OFF_WID,		CG12_WID_SIZE,     0 },
    { CG12_VOFF_ZBUF,		CG12_OFF_DEPTH,		CG12_ZBUF_SIZE,    0 },
    { CG12_VOFF_CTL,		CG12_OFF_CTL,		0x2000, 	   0 },
    { CG12_VOFF_SHMEM,		CG12_OFF_SHMEM,		CG12_SHMEM_SIZE,   0 },
    { CG12_VOFF_DRAM,		CG12_OFF_DRAM,		CG12_DRAM_SIZE,    0 },
    { CG12_VOFF_PROM,		CG12_OFF_PROM,		CG12_PROM_SIZE,    0 },
    { 0 }
};

struct addrmap addr_tbl_hr[] = 
{
	{ CG12_VOFF_OVERLAY_HR,	CG12_OFF_OVERLAY0_HR,	CG12_OVERLAY_SIZE_HR,	0 },
    { CG12_VOFF_ENABLE_HR,	CG12_OFF_OVERLAY1_HR,	CG12_ENABLE_SIZE_HR,	0 },
    { CG12_VOFF_COLOR24_HR,	CG12_OFF_INTEN_HR,	CG12_COLOR24_SIZE_HR,	0 },
    { CG12_VOFF_COLOR8_HR,	CG12_OFF_INTEN_HR,	CG12_COLOR8_SIZE_HR,	0 },
    { CG12_VOFF_WID_HR,		CG12_OFF_WID_HR,	CG12_WID_SIZE_HR,	0 },
    { CG12_VOFF_ZBUF_HR,	CG12_OFF_DEPTH_HR,	CG12_ZBUF_SIZE_HR,	0 },
    { CG12_VOFF_CTL_HR,		CG12_OFF_CTL,		0x2000,				0 },
    { CG12_VOFF_SHMEM_HR,	CG12_OFF_SHMEM,		CG12_SHMEM_SIZE,	0 },
    { CG12_VOFF_DRAM_HR,	CG12_OFF_DRAM,		CG12_DRAM_SIZE,		0 },
    { CG12_VOFF_PROM_HR,	CG12_OFF_PROM,		CG12_PROM_SIZE,		0 },
    { 0 }
};

/* default structure for FBIOGTYPE ioctl */
static struct fbtype cg12typedefault =
{
    /* zero entries filled in attach */
    FBTYPE_SUN2GP,			/* type */
	0,				/* height */
	0,				/* width */
	CG12_DEPTH,			/* depth */
	CG12_CMAP_SIZE,			/* color map size */
	0				/* total mmap size */
};

/* default structure for FBIOGATTR ioctl */
static struct fbgattr cg12attrdefault =
{
    FBTYPE_SUNGP3,			/* real_type	 */
	0,				/* owner	 */
    {
	FBTYPE_SUN2GP,			/* type		 */
	    0,
	    0,
	    CG12_DEPTH,
	    CG12_CMAP_SIZE,
	    0
    },
    /* fbsattr */
    {
	0,				/* flags	 */
	-1,			/* emu_type	 */
	/* dev_specific */
	{
	    0,
	    0,
	    0,
	    FBTYPE_SUNGP3,		/* gp real type indication	*/
	    0,
	    GP2_SHMEM_SIZE		/* how much shared mem to mmap	*/
	}
    },
    /* emu_types */
    {
	FBTYPE_SUNGP3,
	FBTYPE_SUN2BW,		/* bwtwo */
	-1, -1
    }
};

#if NWIN > 0
/* SunWindows specific stuff */

/* kernel pixrect ops vector */
struct pixrectops   cg12_ops = {
    cg12_rop,
    cg12_putcolormap,
    cg12_putattributes,
    cg12_ioctl
};

#endif					/* NWIN > 0 */

static	int	ncg12;
static	struct	cg12_softc *cg12_softc;
static	void	cg12_intr();
extern	int	cpu;
static	cg12_poll();
int	cg12_sync_timeout = 1500 * 1000;
int	cg12_width;

static
cg12_identify(name)
	char	*name;
{
	DEBUGF(1, ("cg12_identify(%s)\n", name));

	if (strcmp(name, "cgtwelve") == 0)
		return ++ncg12;
	else
		return 0;
}

static
cg12_attach(devi)
	struct dev_info	*devi;
{
	struct	cg12_softc	*softc;
	addr_t			reg;
	int			bytes_8, bytes_1, i, d_size, wsc_val;
	static			int unit;
	union			fbunit *map, *imap, *a12map, *b12map;
	addr_t			devid;

	DEBUGF(1, ("cg12_attach: ncg12=%d unit=%d\n", ncg12, unit));

	/* Allocate softc structs on first attach */
	if (!cg12_softc)
	{
	    cg12_softc = (struct cg12_softc *)
		kmem_zalloc((u_int) sizeof (struct cg12_softc) * ncg12);
	    unit = 0;
	}

	softc = getsoftc(unit);

	/* get the device id for gpconfig */
        d_size = getproplen(devi->devi_nodeid, "dev_id");

	if (d_size && (devid = getlongprop(devi->devi_nodeid, "dev_id")))
	{
	    if (strcmp(devid, "gsxr") == 0)
	    {
		softc->dev_id = 3;
		softc->addr_tbl = addr_tbl_hr;
	    }
	    else if (strcmp(devid, "cg12+") == 0)
	    {
		softc->dev_id = 2;
		softc->addr_tbl = addr_tbl_lr;
	    }
	    else if ((strncmp(devid, "eg", 2) == 0) ||
		(strcmp(devid, "cg12") == 0))
	    {
		softc->dev_id = 1;
		softc->addr_tbl = addr_tbl_lr;
	    }
	    kmem_free(devid, (u_int)d_size);
	}

	/* grab properties from PROM */
	if (softc->dev_id <= 2)
	{
	    softc->w = getprop(devi->devi_nodeid, "width", 1152);
	    softc->h = getprop(devi->devi_nodeid, "height", 900);
	}
	else if (softc->dev_id == 3)
	{
	    softc->w = getprop(devi->devi_nodeid, "width", 1280);
	    softc->h = getprop(devi->devi_nodeid, "height", 1024);
	}
	cg12_width = softc->w;

	bytes_8 = mpr_linebytes(softc->w,8);

	DEBUGF(2, ("cg12_attach: w =%d, h =%d, linebytes =%d, dev_id=%d\n",
	    softc->w, softc->h, bytes_8, softc->dev_id));

	/* compute size of frame buffer */
	bytes_8 = btob(bytes_8 * softc->h);
	bytes_1 = btob(mpr_linebytes(softc->w,1) * softc->h);
	softc->bw2size = bytes_1;

#if NWIN > 0
	softc->o_ptr = (char *) getprop(devi->devi_nodeid, "overlay", 0);
	softc->e_ptr = (char *) getprop(devi->devi_nodeid, "enable", 0);

	if(softc->o_ptr && softc->e_ptr) {
	    DEBUGF(2, ("cg12_attach: o=%x, e=%x\n", softc->o_ptr,softc->e_ptr));
	    bytes_1 = 0;
	}

#else 			/* NWIN > 0 */
	bytes_8 = 0;
	bytes_1 = 0;
#endif 			/* NWIN > 0 */

	/*
	 * Allocate virtual space for registers, shared memory
	 * and may be frame buffer. 
	 */

	DEBUGF(2, ("cg12_attach: bytes_8= %x, bytes_1= %x\n", bytes_8,bytes_1));

	if (!(reg = map_regs(devi->devi_reg[0].reg_addr,
	    (u_int) NBPG * 6, devi->devi_reg[0].reg_bustype)))
	    return -1;

	softc->base = reg;
	softc->basepage = fbgetpage(reg);

	if (!(reg = map_regs(devi->devi_reg[0].reg_addr+CG12_OFF_CTL,
	    (u_int) (NBPG * 2 + bytes_8 + bytes_1 * 2 + CG12_SHMEM_SIZE),
	    devi->devi_reg[0].reg_bustype)))
	    return -1;

	softc->reg_bustype = devi->devi_reg[0].reg_bustype;
	softc->reg_addr = devi->devi_reg[0].reg_addr;

	DEBUGF(1, ("cg12_attach: reg=0x%x/0x%x (0x%x)\n", (u_int) reg,
	    fbgetpage((addr_t) reg) << PGSHIFT, fbgetpage((addr_t) reg)));

	softc->ctl_sp = (struct cg12_ctl_sp *) reg;

	softc->shmem =
	    (struct gp1_shmem *)((caddr_t)softc->ctl_sp 
	    + btob(sizeof(struct cg12_ctl_sp)));

	(void) fbmapin((char *) softc->shmem,
	    (int) CG12_PHYSADDR(CG12_OFF_SHMEM), (int)CG12_SHMEM_SIZE);

	softc->d_ptr =
  	    (char *)((caddr_t)softc->shmem + btob(sizeof(struct gp1_shmem)));

	(void) fbmapin(softc->d_ptr,
	    (int) CG12_PHYSADDR(CG12_OFF_DRAM), NBPG);

	DEBUGF(2, ("cg12_attach: shmem=0x%x, dram=0x%x\n",
	    softc->shmem, softc->d_ptr));

#if NWIN > 0
	softc->i_ptr =
  	    (char *)((caddr_t)softc->d_ptr + NBPG);

	if (softc->dev_id <= 2)
	{
	    (void) fbmapin(softc->i_ptr,
	    (int) CG12_PHYSADDR(CG12_OFF_INTEN), bytes_8);
	}
	else if (softc->dev_id == 3)
	{
	    (void) fbmapin(softc->i_ptr,
	    (int) CG12_PHYSADDR(CG12_OFF_INTEN_HR), bytes_8);
	}

	if (!softc->o_ptr) 
	{
	    softc->o_ptr = softc->i_ptr + bytes_8;

	    if (softc->dev_id <= 2)
	    {
		(void) fbmapin(softc->o_ptr,
		    (int) CG12_PHYSADDR(CG12_OFF_OVERLAY1), bytes_1);
	    }
	    else if (softc->dev_id == 3)
	    {
		(void) fbmapin(softc->o_ptr,
		    (int) CG12_PHYSADDR(CG12_OFF_OVERLAY1_HR), bytes_1);
	    }
	}

	if (!softc->e_ptr) 
	{
	    softc->e_ptr = softc->o_ptr + bytes_1;

	    if (softc->dev_id <= 2)
	    {
		(void) fbmapin(softc->e_ptr,
		    (int) CG12_PHYSADDR(CG12_OFF_OVERLAY0), bytes_1);
	    }
	    else if (softc->dev_id == 3)
	    {
		(void) fbmapin(softc->e_ptr,
		    (int) CG12_PHYSADDR(CG12_OFF_OVERLAY0_HR), bytes_1);
	    }
	}

	DEBUGF(2, ("cg12_attach: o_ptr=%x, e_ptr=%x, i_ptr=%x, w_ptr=%x\n",
	    softc->o_ptr, softc->e_ptr, softc->i_ptr, softc->w_ptr));

#endif			/* NWIN > 0 */

#if NOHWINIT
    if (cg12_hwinit) 
	cg12_binit(softc->ctl_sp);

#endif			/* NOHWINIT */

	cg12_init(softc->ctl_sp, softc->dev_id);

	/* save unit number */
	devi->devi_unit = unit;

	/* attach interrupt */
	softc->cg12pri = ipltospl(devi->devi_intr[0].int_pri);
	addintr(devi->devi_intr[0].int_pri, cg12_poll, devi->devi_name, unit);

	/* save back pointer to softc */
	devi->devi_data = (addr_t) softc;

	printf("cgtwelve%d: screen pixel resolution %dx%d\n", 
	    unit, softc->w, softc->h);

	/* prepare for next attach */
	unit++;

	report_dev(devi);

	softc->restart = 0;
	softc->ucode_ver = 0;

	bzero((caddr_t) softc->sblock,sizeof softc->sblock);

	softc->gattr = &cg12attrdefault;
	softc->gattr->fbtype.fb_height = softc->h;
	softc->gattr->fbtype.fb_width = softc->w;
	if (softc->dev_id <= 2)
	{
	    softc->gattr->fbtype.fb_size = CG12_FBCTL_SIZE;
	    softc->size = CG12_FBCTL_SIZE;
	}
	else if (softc->dev_id == 3)
	{
	    softc->gattr->fbtype.fb_size = CG12_FBCTL_SIZE_HR;
	    softc->size = CG12_FBCTL_SIZE_HR;
	}
	softc->gb = 0;
	softc->dsp_active = 0;
	softc->cmap_begin_overlay = 4;
	softc->cmap_end_overlay = 0;
	softc->cmap_begin_indexed = CG12_CMAP_SIZE;
	softc->cmap_end_indexed = 0;
	softc->cmap_begin_rgb = CG12_CMAP_SIZE;
	softc->cmap_end_rgb = 0;
	softc->cmap_begin_4bit_ovr = CG12_OMAP_SIZE - 4;
	softc->cmap_end_4bit_ovr = 0;
	softc->wid_begin = NWIDS;
	softc->wid_end = 0;

	/*
	 * wsc 1 (dev_id == 0 or 1) = 3-color overlay, 8-bit;
	 * wsc 2 = just 8-bit
	 */

	wsc_val =  (softc->dev_id == 0 || softc->dev_id == 1) ? 9 : 1;

    	/* initialize the shadow window-id table */
    	for (i = 0; i < NWIDS; i++)
	    softc->wid_tbl[i] = wsc_val;

   	/* initialize the window-id allocation table */
   	bzero((caddr_t) softc->wid_alloc, sizeof softc->wid_alloc);

	/* initilaize the soft copies of the colormaps */
	map = softc->cmap_rgb;
	imap = softc->cmap_indexed;
	a12map = softc->cmap_a12;
	b12map = softc->cmap_b12;

	for ( i = 0 ; i < 256; i++, map++, imap++, a12map++, b12map++)
	{
	    map->packed = RAMDACWRITE(i);
	    imap->packed = RAMDACWRITE((i) ? 0x00 : 0xff);
	    a12map->packed = RAMDACWRITE((i & 0xf0) | ((i & 0xf0) >> 4));
	    b12map->packed = RAMDACWRITE(((i & 0x0f) << 4) | (i & 0x0f));
	}

	map = softc->cmap_overlay;
	map->packed = 0xffffff;		/* white */
	map++;
	map->packed = 0xffffff;		/* white */
	map++;
	map->packed = 0x00ff00;		/* green */
	map++;
	map->packed = 0x000000;		/* black */

	map = softc->cmap_4bit_ovr;

	for ( i = 0; i <27; i++, map++)
	    map->packed = 0;
	map->packed = 0xffffff;

	map = softc->cmap_alt;

	map->packed = 0xffffff;
	for ( i = 0; i <255; i++, map++)
	    map->packed = 0;

	softc->ctl_sp->apu.msg0 = 0;
	softc->ctl_sp->apu.msg1 = 0;
	softc->ctl_sp->apu.ien0 = 0;

#ifdef	sun4c
	/* sun4c-75 cache bug workaround */
	if(cpu == CPU_SUN4C_75)
	    on_enablereg(1);
#endif			/* sun4c */

	/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	softc->ctl_sp->eic.interrupt |= CG12_EIC_EN_EICINT;
	softc->ctl_sp->apu.res2 = 0;
	softc->ctl_sp->eic.host_control |= CG12_EIC_WSTATUS;
	softc->ctl_sp->apu.res2 = 0;
	/* !!! */

	softc->cntxt_regs[0] = softc->ctl_sp->apu.hpage;
	softc->cntxt_regs[1] = softc->ctl_sp->apu.haccess;
	softc->cntxt_regs[2] = softc->ctl_sp->dpu.pln_sl_host;
	softc->cntxt_regs[3] = softc->ctl_sp->dpu.pln_rd_msk_host;
	softc->cntxt_regs[4] = softc->ctl_sp->dpu.pln_wr_msk_host;
	softc->cntxt_regs[5] = softc->ctl_sp->eic.host_control;

	return 0;
}

STATIC
cg12_open(dev, flag)
	dev_t	dev;
	int	flag;
{
	int	gpminor = minor(dev);

	DEBUGF(4, ("cg12_open(%d), flag:%x\n", gpminor, flag));

	return fbopen(dev, flag, ncg12);
}

/*ARGSUSED*/
STATIC
cg12_close(dev, flag)
	dev_t dev;
	int flag;
{
	struct	cg12_softc	*softc;
	int	gpminor = minor(dev);

	softc = getsoftc(minor(dev));

	DEBUGF(3, ("cg12_close(%d)\n", gpminor));

	{
	    struct gp1_sblock *sbp = softc->sblock;

	    PR_LOOPP(CG12_STATIC_BLOCKS - 1,
	    	if (sbp->owner)
		    sbp->owner = 0;
	    	sbp++);
	}

	{
	    struct cg12_wid_alloc *widp = softc->wid_alloc;

	    PR_LOOPP(NWIDS - 1,
	    	if (widp->owner)
		{
		    widp->owner = 0;
		    widp->type = 0;
		}
	    	widp++);
	}

	{
	    struct seg_tbl *segtbl = softc->seg_tbl;

	    PR_LOOPP(NMINOR - 1,
		if(segtbl->owner && (segtbl->owner->p_pid == u.u_procp->p_pid))
		{
		    DEBUGF(3, ("cg12_close:segtbl_pid:%x,gpminor:%x\n",
			segtbl->owner->p_pid,segtbl->gpminor));

		    segtbl->owner = 0;
		    (void) cg12_cleanup(softc, segtbl->gpminor);
		    segtbl->gpminor = 0;
		}
		segtbl++);
	}

	/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	softc->ctl_sp->eic.host_control = softc->cntxt_regs[5];
	/* !!! */
	softc->ctl_sp->apu.hpage = softc->cntxt_regs[0];
	softc->ctl_sp->apu.haccess = softc->cntxt_regs[1];
	softc->ctl_sp->dpu.pln_sl_host = softc->cntxt_regs[2];
	softc->ctl_sp->dpu.pln_rd_msk_host = softc->cntxt_regs[3];
	softc->ctl_sp->dpu.pln_wr_msk_host = softc->cntxt_regs[4];
	/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	softc->ctl_sp->eic.host_control |= CG12_EIC_WSTATUS;
	softc->ctl_sp->apu.res2 = 0;
	/* !!! */

	return 0;
}

/*ARGSUSED*/
STATIC
cg12_mmap(dev, off, prot)
	dev_t		dev;
	register	off_t	off;
	int		prot;
{
	register	struct	cg12_softc *softc = getsoftc(minor(dev));
	register	struct	addrmap *mp;

	DEBUGF(off ? 9 : 4, ("cg12_mmap(%d, 0x%x)\n", minor(dev), (u_int) off));

	if (softc->gattr->sattr.flags & FB_ATTR_AUTOINIT)
	{
	    int i, j, k;

	    /*
	     * !!! see cgtwelve.c EIC-COMMENTS before changing !!!
	     * set WSTATUS bit so that the PROM will save the context
	     * before displaying any character to the console.
	     */
	    softc->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    /* !!! */
	    softc->ctl_sp->apu.hpage =
		(softc->w == 1280) ? CG12_HPAGE_ENABLE_HR : CG12_HPAGE_ENABLE;
	    softc->ctl_sp->apu.haccess = CG12_HACCESS_ENABLE;
	    softc->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_ENABLE;
	    softc->ctl_sp->dpu.pln_wr_msk_host = CG12_PLN_WR_ENABLE;
	    softc->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_ENABLE;

	    /* set enable if monochrome, clear if color */
	    for (i = 0, j = softc->bw2size,
		k = (softc->gattr->sattr.flags &
		CG12_OWNER_WANTS_COLOR) ? 0 : 0xff; i < j; i++)
		    *(softc->e_ptr + i) = k;

	    /*
	     * !!! see cgtwelve.c EIC-COMMENTS before changing !!!
	     * set WSTATUS bit so that the PROM will save the context
	     * before displaying any character to the console.
	     */
	    softc->ctl_sp->eic.host_control = CG12_EIC_WSTATUS;
	    /* !!! */
	    softc->ctl_sp->apu.hpage =
		(softc->w == 1280) ? CG12_HPAGE_OVERLAY_HR : CG12_HPAGE_OVERLAY;
	    softc->ctl_sp->apu.haccess = CG12_HACCESS_OVERLAY;
	    softc->ctl_sp->dpu.pln_sl_host = CG12_PLN_SL_OVERLAY;
	    softc->ctl_sp->dpu.pln_wr_msk_host = CG12_PLN_WR_OVERLAY;
	    softc->ctl_sp->dpu.pln_rd_msk_host = CG12_PLN_RD_OVERLAY;

	    for (i = 0; i < j; i++)
		*(softc->o_ptr + i) = 0x0;	/* clear the overlay */
	    softc->gattr->sattr.flags &= ~FB_ATTR_AUTOINIT;
	}

	for ( mp = softc->addr_tbl; mp->size; mp++) {
	   if( off >= (off_t) mp->offset &&
	       off < (off_t) (mp->offset + mp->size)) {

		/* Checkup if only root can map this section */
 	   	if (mp->prot && !suser()) {
 			return (-1);
 		}
 
	   	DEBUGF((off == mp->offset) ? 4 : 9, ("\toff:%x pa:%x\n",
		    off, CG12_PHYSADDR(mp->paddr + (off-mp->offset))));

	   	return (CG12_PHYSADDR(mp->paddr + (off-mp->offset)));
	   }
	}

	DEBUGF(1, ("cg12_mmap returning error\n"));

	return (-1);
}

/*ARGSUSED*/
STATIC
cgtwelveioctl(dev, cmd, data, flag)
dev_t		dev;
int		cmd;
caddr_t		data;
int		flag;
{
    register	int	gpminor = minor(dev);
    register	struct	cg12_softc *softc = getsoftc(minor(dev));
    struct		gp1_shmem *shp = (struct gp1_shmem *)softc->shmem;
    register	int	i, j;

    DEBUGF(4, ("cgtwelveioctl(%d, 0x%x)\n", gpminor, cmd));

    switch (cmd)
    {

	case FBIO_DEVID:
	    *(int *) data = softc->dev_id;
	    break;

	case FBIO_U_RST:
	    {
	    	int        dsphlt = *(int *) data;

    		DEBUGF(3, ("cgtwelveioctl: URST called with %x\n", dsphlt));

	    	if (!dsphlt) {
		    struct gp1_sblock *sbp = softc->sblock;

		    PR_LOOPP(CG12_STATIC_BLOCKS - 1,
		    	if (sbp->owner)
			    psignal(sbp->owner, SIGXCPU);
			    sbp++);

		    softc->dsp_active = 0xff;
		    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    	    softc->ctl_sp->eic.reset &= ~CG12_EIC_DSPRST;
	    	    softc->ctl_sp->apu.res2 = 0;
		    /* !!! */
	    	}
	    	else
		{
	    	    shp->fill280[0] = 0x00ff;

		    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    	    softc->ctl_sp->eic.reset |= CG12_EIC_DSPRST;
	    	    softc->ctl_sp->apu.res2 = 0;
		    /* !!! */
		}

	    	break;
	    }

	case GP1IO_GET_STATIC_BLOCK:
	    {
		register int        block;

		{
		    register struct gp1_sblock *sbp = softc->sblock;

		    DEBUGF(3, ("CG12_GET_STATIC_BLOCK:sbp:%x\n", sbp));

		    block = 0;

		    while (sbp->owner)
		    {
			if (++block >= CG12_STATIC_BLOCKS)
			{
			    *(int *) data = -1;
			    return 0;
			}
			sbp++;
		    }

		    sbp->owner = u.u_procp;
		    sbp->gpminor = minor(dev);
		    *(int *) data = block;
		}
		break;
	    }

	case GP1IO_FREE_STATIC_BLOCK:
	    {
		register int        block = *(int *) data;
		register struct gp1_sblock *sbp = &softc->sblock[block];

		DEBUGF(3, ("CG12_FREE_STATIC_BLOCK:block:%x,sbp:%x\n",
			block, sbp));

		if ((u_int) block >= CG12_STATIC_BLOCKS ||
		    sbp->owner == 0 ||
		    sbp->gpminor != minor(dev))
		    return EINVAL;

		sbp->owner = 0;
	    }
	    break;

	case GP1IO_INFO_STATIC_BLOCK:
	    {
		char                tmp[CG12_STATIC_BLOCKS];
		struct static_block_info *binfo =
		    (struct static_block_info *) data;
		struct gp1_sblock  *sbp = softc->sblock;

		DEBUGF(3, ("CG12_INFO_STATIC_BLOCK:sbp:%x\n", sbp));

		for (i = 0, binfo->sbi_count = 0; i < CG12_STATIC_BLOCKS;
		    i++, sbp++)
		{
		    if (sbp->owner && (sbp->owner->p_pid == u.u_procp->p_pid))
		    {
			tmp[binfo->sbi_count] = i;

			DEBUGF(3, ("CG12_INFO_STATIC_BLOCK: tmp[%x]:%x\n",
				binfo->sbi_count, tmp[binfo->sbi_count]));
			binfo->sbi_count++;
		    }
		}

		if (copyout(tmp, binfo->sbi_array, (u_int) binfo->sbi_count))
		    return (EFAULT);
	    }
	    break;

	case GP1IO_CHK_GP:
	    if (!(*(int *) data && cg12_cleanup(softc, gpminor)))
		cg12_restart(gpminor, "");
	    break;

	case GP1IO_GET_RESTART_COUNT:
	    *(int *) data = softc->restart;
	    break;

	case FBIOGTYPE:
	    {
		register struct fbtype *fb = (struct fbtype *) data;

		if (softc->owner == NULL ||
		    softc->owner->p_stat == NULL ||
		    (int) softc->owner->p_pid != softc->gattr->owner)
		{
		    softc->owner = u.u_procp;
		    softc->gattr->owner = (int) u.u_procp->p_pid;
		}

		*fb = cg12typedefault;
		fb->fb_height = softc->h;
		fb->fb_width = softc->w;

		if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW)
		{
		    fb->fb_type = FBTYPE_SUN2BW;
		    fb->fb_depth = 1;
		    fb->fb_size = softc->bw2size;
		    fb->fb_cmsize = 2;
		}
		else
		{
		    fb->fb_type = FBTYPE_SUN2GP;
		    fb->fb_depth = CG12_DEPTH;
		    fb->fb_size = softc->size;
		    fb->fb_cmsize = CG12_CMAP_SIZE;
		}
		break;
	    }

#if NWIN > 0
	case FBIOGPIXRECT:
	    {
		struct fbpixrect   *fbpix = (struct fbpixrect *) data;
		unsigned int        initplanes;

		/* point to the softc pixrect and private data */
		fbpix->fbpr_pixrect = &softc->pr;
		softc->pr.pr_data = (caddr_t) & softc->cg12d;

		/* initialize pixrect */
		softc->pr.pr_ops = &cg12_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;

		/* initialize private data */
		softc->cg12d.gp_shmem = (char *) softc->shmem;
		softc->cg12d.flags = 0;
		softc->cg12d.planes = 0;
		softc->cg12d.fd = gpminor;
		softc->cg12d.ctl_sp = softc->ctl_sp;
		softc->cg12d.wid_dbl_info.dbl_wid.wa_type = 0;
		softc->cg12d.wid_dbl_info.dbl_wid.wa_index = -1;
		softc->cg12d.wid_dbl_info.dbl_wid.wa_count = 1;
		softc->cg12d.wid_dbl_info.dbl_fore = PR_DBL_A;
		softc->cg12d.wid_dbl_info.dbl_back = PR_DBL_B;
		softc->cg12d.wid_dbl_info.dbl_read_state = PR_DBL_A;
		softc->cg12d.wid_dbl_info.dbl_write_state = PR_DBL_BOTH;
		/* set up the plane group data */
		for (i = 0; i < CG12_NFBS; i++)
		{
		    softc->cg12d.fb[i].group = cg12_fb_desc[i].group;
		    softc->cg12d.fb[i].depth = cg12_fb_desc[i].depth;
		    softc->cg12d.fb[i].mprp.mpr.md_offset.x = 0;
		    softc->cg12d.fb[i].mprp.mpr.md_offset.y = 0;
		    softc->cg12d.fb[i].mprp.mpr.md_primary = 0;
		    softc->cg12d.fb[i].mprp.mpr.md_flags =
			(cg12_fb_desc[i].allplanes != 0)
			    ? MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
		    softc->cg12d.fb[i].mprp.planes = cg12_fb_desc[i].allplanes;
		    softc->cg12d.fb[i].mprp.mpr.md_linebytes =
			mpr_linebytes(softc->w, cg12_fb_desc[i].depth);

		    initplanes = PIX_GROUP(cg12_fb_desc[i].group) |
			cg12_fb_desc[i].allplanes;

		    if (softc->cg12d.fb[i].group == PIXPG_OVERLAY)
		    {
			softc->cg12d.fb[i].mprp.mpr.md_image =
			    (short *) softc->o_ptr;
			(void) cg12_putattributes(&softc->pr, &initplanes);
			pr_rop(&softc->pr, 0, 0, softc->pr.pr_size.x,
			    softc->pr.pr_size.y, PIX_CLR, (Pixrect *) 0, 0, 0);
		    }
		    else if (softc->cg12d.fb[i].group == PIXPG_OVERLAY_ENABLE)
		    {
			softc->cg12d.fb[i].mprp.mpr.md_image =
			    (short *) softc->e_ptr;
			(void) cg12_putattributes(&softc->pr, &initplanes);
			pr_rop(&softc->pr, 0, 0, softc->pr.pr_size.x,
			    softc->pr.pr_size.y, PIX_SET, (Pixrect *) 0, 0, 0);
		    }
		    else if (softc->cg12d.fb[i].group == PIXPG_8BIT_COLOR)
		    {
			softc->cg12d.fb[i].mprp.mpr.md_image =
			    (short *) softc->i_ptr;
			(void) cg12_putattributes(&softc->pr, &initplanes);
			pr_rop(&softc->pr, 0, 0, softc->pr.pr_size.x,
			    softc->pr.pr_size.y, PIX_CLR, (Pixrect *) 0, 0, 0);
		    }
		    else
			softc->cg12d.fb[i].mprp.mpr.md_image = (short *) -1;
		}

		/* leave it in overlay */
		initplanes = PIX_GROUP(PIXPG_OVERLAY) | PIX_ALL_PLANES;
		(void) cg12_putattributes(&softc->pr, &initplanes);
	    }
	    break;
#endif					/* NWIN > 0 */

	case FBIOSATTR:
	    {
		struct fbsattr     *sattr = (struct fbsattr *) data;

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
		break;
	    }

	case FBIOGATTR:
	    {
		register struct fbgattr *gattr = (struct fbgattr *) data;

		/* set owner if not owned, or if prev. owner is dead */
		if (softc->owner == NULL ||
		    softc->owner->p_stat == NULL ||
		    (int) softc->owner->p_pid != softc->gattr->owner)
		{

		    softc->owner = u.u_procp;
		    softc->gattr->owner = (int) u.u_procp->p_pid;
		}

		*gattr = *(softc->gattr);
		if (u.u_procp == softc->owner)
		{
		    gattr->owner = 0;
/*		    softc->gattr->sattr.flags |= CG12_OWNER_WANTS_COLOR; */
		}
		break;
	    }

	case FBIOSVIDEO:
	    {
		register int        on = *(int *) data & FBVIDEO_ON;

		if (!on)
		    softc->ctl_sp->apu.vsg_ctl &= ~CG12_APU_EN_VID;
		else if (on && !(softc->ctl_sp->apu.vsg_ctl & CG12_APU_EN_VID))
		    softc->ctl_sp->apu.vsg_ctl |= CG12_APU_EN_VID;
		break;
	    }

	case FBIOGVIDEO:
	    *(int *) data = softc->ctl_sp->apu.vsg_ctl & CG12_APU_EN_VID
		? FBVIDEO_ON : FBVIDEO_OFF;
	    break;

	case FBIOVERTICAL:
	    cg12_wait(minor(dev));
	    break;

	case FBIOVRTOFFSET:
	    if (softc->dev_id <= 2)
		*(int *) data = CG12_VOFF_VERTCOUNT;
	    else if (softc->dev_id == 3)
		*(int *) data = CG12_VOFF_VERTCOUNT_HR;
	    break;

	case FBIOGINFO:
	    {
		register struct fbinfo *fbinfo = (struct fbinfo *) data;

		fbinfo->fb_physaddr = CG12_OFF_INTEN;
		fbinfo->fb_addrdelta = softc->size;
		fbinfo->fb_unit = FBUNIT(gpminor);
		fbinfo->fb_hwwidth = softc->w;
		fbinfo->fb_hwheight = softc->h;
		break;
	    }

	case GP1IO_CHK_FOR_GBUFFER:
	    *(int *) data = (softc->gb >= 0);
	    break;

	case GP1IO_GET_GBUFFER_STATE:
	    *(int *) data = (softc->gb == FBUNIT(gpminor));
	    break;

	case GP1IO_GET_TRUMINORDEV:
	    {
		struct seg_tbl *segtbl = softc->seg_tbl;

		DEBUGF(3, ("GP1IO_GET_TRUMINORDEV:segtbl:%x\n", segtbl));

		for (i = 0; i < NMINOR; i++, segtbl++)
		{
		    if (segtbl->owner &&
		        (segtbl->owner->p_pid == u.u_procp->p_pid)) 
		    {
			DEBUGF(3, ("TRUMINORDEV:pid:%x,gpminor:%x\n",
			    segtbl->owner->p_pid, segtbl->gpminor));

		        *(char *) data = segtbl->gpminor;
		    }
		}
	    }
	    break;

	case GP1IO_PUT_INFO:
	case GP1IO_SET_USING_GBUFFER:
	case GP1IO_REDIRECT_DEVFB:
	case GP1IO_GET_REQDEV:
	    break;

	case FBIOPUTCMAP:
	case FBIOGETCMAP:
	{
	    struct fbcmap      *cmap = (struct fbcmap *) data;
	    u_int               index = (u_int) cmap->index;
	    u_int               count = (u_int) cmap->count;
	    u_int               pgroup = PIX_ATTRGROUP(index);
	    union fbunit       *map;
	    union fbunit       *mapa12;
	    union fbunit       *mapb12;
	    int                *begin;
	    int                *end;
	    int                 entries;
	    u_int               intr_flag;
	    u_char              tmp[3][CG12_CMAP_SIZE];
	    u_int               color;
	    u_int               lo_addr;
	    u_int               hi_addr;
	    u_int              *map_addr;

	    if (count == 0 || !cmap->red || !cmap->green || !cmap->blue)
		return (EINVAL);

	    switch (pgroup)
	    {
		case PIXPG_OVERLAY:
		case PIXPG_OVERLAY_ENABLE:
		    begin = &softc->cmap_begin_overlay;
		    end = &softc->cmap_end_overlay;
		    map = softc->cmap_overlay;
		    intr_flag = CG12_OVERLAY_CMAP;
		    lo_addr = 0;
		    hi_addr = 1;
		    map_addr = &softc->ctl_sp->ramdac.control;
		    entries = 4;
		    break;

		case PIXPG_4BIT_OVERLAY:
		    if((softc->dev_id != 2) && (softc->dev_id !=3))
			return (EINVAL);

		    begin = &softc->cmap_begin_4bit_ovr;
		    end = &softc->cmap_end_4bit_ovr;
		    map = &softc->cmap_4bit_ovr[CG12_OMAP_SIZE - 5];
		    intr_flag = CG12_4BIT_OVR_CMAP;
		    lo_addr = 0;
		    hi_addr = 1;
		    map_addr = &softc->ctl_sp->ramdac.control;
		    entries = CG12_OMAP_SIZE - 4;
		    break;

		case PIXPG_8BIT_COLOR:
		    begin = &softc->cmap_begin_indexed;
		    end = &softc->cmap_end_indexed;
		    map = softc->cmap_indexed;
		    intr_flag = CG12_8BIT_CMAP;
		    lo_addr = 0;
		    hi_addr = 1;
		    map_addr = &softc->ctl_sp->ramdac.color;
		    entries = CG12_CMAP_SIZE;
		    break;

		case PIXPG_24BIT_COLOR:
		    begin = &softc->cmap_begin_rgb;
		    end = &softc->cmap_end_rgb;
		    map = softc->cmap_rgb;
		    intr_flag = CG12_24BIT_CMAP;
		    lo_addr = 0;
		    hi_addr = 0;
		    map_addr = &softc->ctl_sp->ramdac.color;
		    entries = CG12_CMAP_SIZE;
		    break;

		case PIXPG_ALT_COLOR:
		    begin = &softc->cmap_begin_alt;
		    end = &softc->cmap_end_alt;
		    map = softc->cmap_alt;
		    intr_flag = CG12_ALT_CMAP;
		    lo_addr = 0;
		    hi_addr = 0;
		    map_addr = &softc->ctl_sp->ramdac.control;
		    entries = CG12_CMAP_SIZE;
		    break;

		case PIXPG_A12BIT_COLOR:
		    begin = &softc->cmap_begin_a12;
		    end = &softc->cmap_end_a12;
		    map = softc->cmap_a12;
		    intr_flag = CG12_A12BIT_CMAP;
		    lo_addr = 0;
		    hi_addr = 2;
		    map_addr = &softc->ctl_sp->ramdac.color;
		    entries = CG12_CMAP_SIZE;
		    break;

		case PIXPG_B12BIT_COLOR:
		    begin = &softc->cmap_begin_b12;
		    end = &softc->cmap_end_b12;
		    map = softc->cmap_b12;
		    intr_flag = CG12_B12BIT_CMAP;
		    lo_addr = 0;
		    hi_addr = 3;
		    map_addr = &softc->ctl_sp->ramdac.color;
		    entries = CG12_CMAP_SIZE;
		    break;

		default:
		    return (EINVAL);
	    }

	    if ((index &= PIX_ALL_PLANES) >= entries || index + count > entries)
		return (EINVAL);

	    if (cmd == FBIOPUTCMAP)
	    {
		if (softc->cg12d.flags & CG12_KERNEL_UPDATE)
		{
		    DEBUGF(3, ("kernel putcmap\n"));
		    softc->cg12d.flags &= ~CG12_KERNEL_UPDATE;
		    bcopy((caddr_t) cmap->red, (caddr_t) tmp[0], count);
		    bcopy((caddr_t) cmap->green, (caddr_t) tmp[1], count);
		    bcopy((caddr_t) cmap->blue, (caddr_t) tmp[2], count);
		}
		else
		{
		    DEBUGF(3, ("user putcmap\n"));
		    if (copyin((caddr_t) cmap->red, (caddr_t) tmp[0], count) ||
			copyin((caddr_t) cmap->green, (caddr_t) tmp[1],count) ||
			copyin((caddr_t) cmap->blue, (caddr_t) tmp[2], count))
			return (EFAULT);
		}

		*begin = MIN(*begin, index);
		*end = MAX(*end, index + count);

		if (pgroup == PIXPG_24BIT_COLOR)
		{
		    DEBUGF(3, ("24bit cmap: index:%x,count:%x\n",index,count));

		    mapa12 = softc->cmap_a12;
		    mapb12 = softc->cmap_b12;

		    for (i = 0, map += index; count; i++, count--, map++)
		    {
		    	map->packed =
			    tmp[0][i] | (tmp[1][i] << 8) | (tmp[2][i] << 16);

			if (((i+index) & 0xf) == (((i+index) & 0xf0) >> 4))
			{
			    mapa12 = softc->cmap_a12 + (i & 0xf0);
			    mapb12 = softc->cmap_b12 + (i & 0x0f);

		            DEBUGF(3, ("24bit: i:%x, mapa12:%x, mapb12:%x\n",
				i, mapa12, mapb12));

			    for (j = 0; j < 16; j++, mapa12++, mapb12 += 16)
			    {
			        mapa12->packed = map->packed;
			        mapb12->packed = map->packed;
			    }
			}
		    }

		    if ((mapa12 >softc->cmap_a12) || (mapb12 > softc->cmap_b12))
		    {
		        DEBUGF(3, ("24bit cmap: mapa12:%x, mapb12:%x\n",
			    mapa12,mapb12));

		        softc->cmap_begin_a12 = *begin & 0xf0;
		        softc->cmap_begin_b12 = 0;
		        softc->cmap_end_a12 = mapa12 - softc->cmap_a12;
		        softc->cmap_end_b12 = mapb12 - softc->cmap_b12 - 15;
			softc->cg12d.flags |= CG12_A12BIT_CMAP;
			softc->cg12d.flags |= CG12_B12BIT_CMAP;
		    }
		}
		else if (pgroup == PIXPG_4BIT_OVERLAY)
		{
		    DEBUGF(3, ("4bit cmap: index:%x,count:%x\n",index,count));
		    for (i = 0, map -= index; count; i++, count--, map--)
		    	map->packed =
			    tmp[0][i] | (tmp[1][i] << 8) | (tmp[2][i] << 16);
		}
		else
		{
		    for (i = 0, map += index; count; i++, count--, map++)
		    	map->packed =
			    tmp[0][i] | (tmp[1][i] << 8) | (tmp[2][i] << 16);
		}

		softc->cg12d.flags |= intr_flag;

		if((shp->ver_release >> 8) <= 6)
		    enable_intr(softc);
		else
		    ENABLE_INTR(softc);
	    }
	    /* FBIOGETCMAP */
	    else
	    {
		if (softc->cg12d.flags & 
		    (CG12_8BIT_CMAP | CG12_24BIT_CMAP | CG12_A12BIT_CMAP|
		    CG12_B12BIT_CMAP | CG12_ALT_CMAP | CG12_OVERLAY_CMAP |
		    CG12_4BIT_OVR_CMAP))
		    cg12_wait(minor(dev));

		if (pgroup == PIXPG_4BIT_OVERLAY)
		{
		    softc->ctl_sp->ramdac.addr_lo =
			RAMDACWRITE((lo_addr + CG12_OMAP_SIZE - (index+count)));
		    softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		    for (i = 0, j = count; j; i++, j--)
		    {
		        color = *map_addr;
		        tmp[0][j - 1] = (color & 0x0000ff);
		        tmp[1][j - 1] = (color & 0x00ff00) >> 8;
		        tmp[2][j - 1] = (color & 0xff0000) >> 16;
		    }
		}
		else
		{
		    softc->ctl_sp->ramdac.addr_lo =
			RAMDACWRITE((lo_addr + index));
		    softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		    for (i = 0, j = count; j; i++, j--)
		    {
		        color = *map_addr;
		        tmp[0][i] = (color & 0x0000ff);
		        tmp[1][i] = (color & 0x00ff00) >> 8;
		        tmp[2][i] = (color & 0xff0000) >> 16;
		    }
		}

		if (copyout((caddr_t) tmp[0], (caddr_t) cmap->red, count) ||
		    copyout((caddr_t) tmp[1], (caddr_t) cmap->green, count) ||
		    copyout((caddr_t) tmp[2], (caddr_t) cmap->blue, count))
		    return (EFAULT);
	    }
	}
	break;
		     
	case FBIO_WID_ALLOC:
	{
	    struct fb_wid_alloc *widalloc = (struct fb_wid_alloc *) data;
	    u_int               type = widalloc->wa_type;
	    u_int               count = widalloc->wa_count;
	    u_int               index;
	    int			pwr_of_2, found;
	    struct cg12_wid_alloc *widp = softc->wid_alloc;

	    DEBUGF(3, ("FBIO_WID_ALLOC: type:%x, count:%x\n", type, count));

	    switch (type)
	    {
		case FB_WID_SHARED_8:
		case FB_WID_SHARED_24:
		{
		    for (index = 0; index < NWIDS; widp++, index++)
		    {
		        if(widp->type == type +1)
		        {
			    widalloc->wa_index = index;
			    return(0);
		        }
		    }

	            widp = softc->wid_alloc;

		    for (index = 0; index < NWIDS; widp++, index++)
		    {
		        if(!widp->owner)
		        {
			    widp->owner = u.u_procp;
			    widp->type = type + 1;
		            widalloc->wa_index = index;
		            return(0);
		        }
		    }

		    widalloc->wa_index = -1;
	        }
	        break;

	        case FB_WID_DBL_24:
	        {
		    if ((int)count < 0 || count > NWIDS  ||
		        (pwr_of_2 = cg12_find_power(count)) == -1) 
		    {
		        widalloc->wa_index = -1;
		        return(EINVAL);
		    }

	            for (j = 0; j < NWIDS; j += pwr_of_2) 
	            {
		        found = 1;

	                widp = &softc->wid_alloc[j];

	                for (i = 0; i < count; i++, widp++)
	                {
		            if (widp->owner)
		            {
			        found = 0;
			        break;
		            }
		        }

		        if (found)
		        {
	                    widp = &softc->wid_alloc[j];

		            for ( i = 0; i < count; i++, widp++)
		            {
		                widp->owner = u.u_procp;
		                widp->type = type + 1;
		            }

	                    widalloc->wa_index = j;
		            return(0);
	                }
	            }

	            widalloc->wa_index = -1;
	        }
		break;

	        default:
		    widalloc->wa_index = -1;
		    return(EINVAL);
	    }
	    if (widalloc->wa_index == -1)
		return (ENOMEM);
	}
	break;

	case FBIO_WID_FREE:
	{
	    struct fb_wid_alloc *widalloc = (struct fb_wid_alloc *) data;
	    u_int               type = widalloc->wa_type;
	    u_int               count = widalloc->wa_count;
	    u_int               index = widalloc->wa_index;
	    struct cg12_wid_alloc *widp = &softc->wid_alloc[index];
	    int cc;

	    DEBUGF(3, ("FBIO_WID_FREE: type:%x, count:%x, index:%x \n",
		type, count, index));

	    if ((type == FB_WID_SHARED_8) || (type == FB_WID_SHARED_24))
	        return(0);

	    if (index >= NWIDS)
		return EINVAL;

	    for (i = cc = 0; i < count; i++, widp++)
	    {
		if (widp->owner == u.u_procp)
		{
		    widp->owner = 0;
		    widp->type = 0;
		}
		else
		    cc = EINVAL;
	    }
	    if (cc == EINVAL)
	        return (cc);
	}
	break;

	case FBIO_WID_GET:
	case FBIO_WID_PUT:
	{
	    struct fb_wid_list *widlist = (struct fb_wid_list *) data;
	    u_int               index;
	    u_int               count = widlist->wl_count;
	    struct fb_wid_item  tmp[NWIDS];
	    u_int		value;

	    DEBUGF(3, ("FBIO_WID_PUT/GET: count:%x, flags:%x\n", count,
		    widlist->wl_flags));

	    if ((count == 0) || (!widlist->wl_list))
		return (EINVAL);

	    if (copyin((caddr_t) widlist->wl_list, (caddr_t) & tmp[0],
	        sizeof(struct fb_wid_item) * count))
		return (EFAULT);

	    if (cmd == FBIO_WID_PUT)
	    {
	    	for (i = 0; i < count; i++)
	    	{
		    if (tmp[i].wi_type)
		   	return (EINVAL);

		    index = tmp[i].wi_index & 0x3f;
		    softc->ctl_sp->wsc.addr = index;
		    softc->wid_tbl[index] = softc->ctl_sp->wsc.data;

		    for (j = 0; j < CG12_WID_ATTRS && tmp[i].wi_attrs;
			j++, tmp[i].wi_attrs >>= 1)
		    {
		        if (tmp[i].wi_attrs & 1)
		        {
			    switch (j)
			    {
			        case CG12_WID_8_BIT:
			        case CG12_WID_24_BIT:
			        case CG12_WID_DBL_BUF_DISP_A:
			        case CG12_WID_DBL_BUF_DISP_B:
				    softc->wid_tbl[index] =
				        (softc->wid_tbl[index] & 0xc) |
					(tmp[i].wi_values[j] & 3);
				    break;

			        case CG12_WID_ENABLE_2:
			        case CG12_WID_ENABLE_3:
				    softc->wid_tbl[index] =
				        (softc->wid_tbl[index] & 0x7) |
					(tmp[i].wi_values[j] & 8);
				    break;

			        case CG12_WID_ALT_CMAP:
				    softc->wid_tbl[index] =
				        (softc->wid_tbl[index] & 0xb) |
					(tmp[i].wi_values[j] & 4);
				    break;

			        default:
				    return (EINVAL);
			    }
		        }
		    }
	        }

	        softc->wid_begin = MIN(softc->wid_begin, tmp[0].wi_index);
	        softc->wid_end = MAX(softc->wid_end, tmp[0].wi_index + count);

	    	softc->cg12d.flags |= CG12_WID_UPDATE;

	    	if (widlist->wl_flags & FBDBL_DONT_BLOCK) {
		    if ((shp->ver_release >> 8) <= 6)
	    		enable_intr(softc);
		    else
			ENABLE_INTR(softc);
		}
		else
		    cg12_wait(minor(dev));
	    }
	    else
	    {
	    	if (softc->cg12d.flags & CG12_WID_UPDATE)
	    	    cg12_wait(minor(dev));

	        for (i = 0; i < count; i++)
	        {
		    if (tmp[i].wi_type)
		   	return (EINVAL);

		    softc->ctl_sp->wsc.addr = tmp[i].wi_index & 0x3f;

		    tmp[i].wi_attrs = 0xffff;

		    value = softc->ctl_sp->wsc.data;
		    for (j = 0; j < (NBBY*sizeof(int)); j++)
		        tmp[i].wi_values[j] = value;
	        }

	        if (copyout((caddr_t)tmp, (caddr_t) widlist->wl_list, 
	            sizeof(struct fb_wid_item) * count))
		    return (EFAULT);
	    }
	}
	break;

	default:
	    return ENOTTY;
    }
    return 0;
}

cg12_find_power(count)
u_int	count;
{
    int                 i;

    for (i = 1; i <= NWIDS; i = i << 1)
	if (i >= count)
	    return (i);
    return (-1);
}

cg12_wait(unit)
int	unit;
{
	struct	cg12_softc *softc = getsoftc(unit);
	struct	gp1_shmem *shp = (struct gp1_shmem *)softc->shmem;
	int	s;

	DEBUGF(3, ("cg12_wait: entered\n"));

	s = splr(softc->cg12pri);
	softc->cg12d.flags |= CG12_SLEEPING;

	if ((shp->ver_release >> 8) <= 6)
	    enable_intr(softc);
	else
	    ENABLE_INTR(softc);

	(void) sleep((caddr_t) softc, PZERO - 1);
	(void) splx(s);
	DEBUGF(3, ("cg12_wait: woken up\n"));
}


static
cg12_poll()
{
	register int	i, serviced = 0;
	register struct	cg12_softc *softc;
	u_int		cg12ctl;

	DEBUGF(7, ("cg12_poll\n"));

	for (softc = cg12_softc, i = ncg12; --i >= 0; softc++) {
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    softc->ctl_sp->eic.eic_control = 0xffffffff;
	    softc->ctl_sp->apu.res2 = 0;
	    cg12ctl = softc->ctl_sp->eic.interrupt;
	    /* !!! */

	    if(cg12ctl & CG12_EIC_INT){
		cg12_intr(softc);
		serviced++;
	    }
	}

	return serviced;
}


static void
cg12_intr(softc)
	register struct	cg12_softc *softc;
{
	register	union fbunit *map;
	u_int		lo_addr, hi_addr, data;
	int		i, j;
	u_int		eic_int, istatus;
	u_int		*oprl, *oprh, *umemacc, *ctrl, *dram;
	u_int		*prom_ptr, *buffer, *buffer2;
	u_char		*prom_cptr, *cbuffer;
	struct	gp1_shmem *shp = (struct gp1_shmem *)softc->shmem;
	struct gp1_sblock *sbp = softc->sblock;

	eic_int = softc->ctl_sp->eic.interrupt;

	if (eic_int & CG12_EIC_EICINT )
	{
	    DEBUGF(1, ("cg12_intr: timer interrupt,c30_control:%x,int_reg:%x\n",
		softc->ctl_sp->eic.c30_control, eic_int));

	    softc->ctl_sp->eic.timeout_reg = 0;	/* ### */

	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    softc->ctl_sp->eic.reset = 0x03000000;
	    softc->ctl_sp->eic.reset = 0x02000000;
	    softc->ctl_sp->eic.interrupt = 0;
	    softc->ctl_sp->apu.res2 = 0;
	    /* !!! */

	    softc->ctl_sp->ramdac.addr_lo = RAMDACWRITE(01);
	    softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(02);
	    softc->ctl_sp->ramdac.control = RAMDACWRITE(0x40);

	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    softc->ctl_sp->eic.reset = 0x03000000;
	    softc->ctl_sp->eic.reset = 0x02000000;
	    softc->ctl_sp->apu.res2 = 0;
	    /* !!! */

	    shp->fill280[0] = 0xffff;

	    PR_LOOPP(GP1_STATIC_BLOCKS - 1,
	        if (sbp->owner)
		psignal(sbp->owner, SIGXCPU);
	        sbp++);

	    prom_ptr = (unsigned int *)(softc->base + 4);
	    prom_cptr = (unsigned char *)(softc->base + *prom_ptr);

	    DEBUGF(1, ("cg12_intr: prom_cptr:%x, *prom_cptr:%x\n",
		prom_cptr, *prom_cptr));

	    oprl = (unsigned int *)softc->ctl_sp;
	    oprh = (unsigned int *)(4 + (unsigned int)softc->ctl_sp);
	    umemacc = (unsigned int *)(8 + (unsigned int)softc->ctl_sp);
	    ctrl = (unsigned int *)(12 + (unsigned int)softc->ctl_sp);

	    buffer =
		(unsigned int *) new_kmem_zalloc(sizeof(data)*512*8, 
		KMEM_NOSLEEP);

	    buffer2 = buffer;
	    cbuffer = (u_char *)buffer;

	    DEBUGF(1, ("buffer:%x\n",buffer));

	    for (i = 0; i < 512 * 4; i++)
	    {
		for (j = 0; j < 8; j++)
		{
		    *cbuffer++ = *prom_cptr++;
		}
	    }

	    DEBUGF(1, ("cbuffer:%x\n",cbuffer));

	    *ctrl |= 0x00000202;
	    *ctrl &= ~0x00000808;

	    *oprl = 0;
	    *oprh = 0;

	    *ctrl &= ~0x00000404;

	    for (i = 0; i <512; i++)
	    {
		for (j = 0; j < 8; j++)
		{
		    while(*ctrl & 0x00002020);
		    *umemacc = *buffer2++;
		}
	    }

	    *oprh = 0x00000707;
	    *oprl = 0x00002a2a;
	    *ctrl &= ~0x00000303;

	    dram = (unsigned int *)softc->d_ptr;
	    *dram = 0x00001f0;
	    *(dram + 0x00001ee) = 0x08780006;
	    *(dram + 0x00001ef) = 0x16000000;
	    *(dram + 0x00001f0) = 0x6a00ffff;

	    softc->ctl_sp->eic.reset = 0x00000000;
	    softc->ctl_sp->apu.res2 = 0;

	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    softc->ctl_sp->eic.interrupt |= CG12_EIC_EN_EICINT;
	    softc->ctl_sp->apu.res2 = 0;
	    softc->ctl_sp->eic.host_control |= CG12_EIC_WSTATUS;
	    softc->ctl_sp->apu.res2 = 0;
	    /* !!! */

            kmem_free((char *)buffer, sizeof(data)*512*8);

	    cg12_binit(softc->ctl_sp);
	    cg12_init(softc->ctl_sp, softc->dev_id);
	    softc->ctl_sp->apu.vsg_ctl |= CG12_APU_EN_VID;

	    DEBUGF(0, ("Graphic accelerator panic.\n"));
	    DEBUGF(0, ("gpconfig called to download /usr/lib/gs.ucode.\n"));

	    return;
	}

	istatus = softc->ctl_sp->apu.istatus;

	DEBUGF(3, ("cg12_intr:istatus:%x,int_reg:%x\n",
	   istatus, eic_int));

	/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	softc->ctl_sp->eic.interrupt &= ~CG12_EIC_EN_HINT;
	softc->ctl_sp->apu.res2 = 0;
	/* !!! */

	if ((eic_int & CG12_EIC_HINT) && (istatus & CG12_APU_EN_HINT)) 
	{
	    softc->ctl_sp->apu.ien0 &= ~CG12_APU_EN_HINT;
	    if ((shp->ver_release >> 8) >= 7)
	        softc->ctl_sp->apu.msg0 &= ~CG12_APU_EN_HINT;

	    if (softc->cg12d.flags & CG12_OVERLAY_CMAP) 
	    {
		map = softc->cmap_overlay + softc->cmap_begin_overlay;
		lo_addr	= 0;
		hi_addr	= 1;

	        softc->cg12d.flags &= ~CG12_OVERLAY_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + softc->cmap_begin_overlay));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_overlay; i < softc->cmap_end_overlay;
		    i++, map++)
		    softc->ctl_sp->ramdac.control = map->packed;

		softc->cmap_begin_overlay = 4;
		softc->cmap_end_overlay = 0;
	    }
	    if (softc->cg12d.flags & CG12_4BIT_OVR_CMAP) 
	    {
		map = softc->cmap_4bit_ovr + 28 - softc->cmap_end_4bit_ovr;
		lo_addr	= 4;
		hi_addr	= 1;

	        softc->cg12d.flags &= ~CG12_4BIT_OVR_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + 28 - softc->cmap_end_4bit_ovr));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_4bit_ovr; i< softc->cmap_end_4bit_ovr;
		    i++, map++) 
		    softc->ctl_sp->ramdac.control = map->packed;

		softc->cmap_begin_4bit_ovr = CG12_OMAP_SIZE - 4;
		softc->cmap_end_4bit_ovr = 0;
	    }
	    if (softc->cg12d.flags & CG12_8BIT_CMAP) 
	    {
		map = softc->cmap_indexed + softc->cmap_begin_indexed;
		lo_addr	= 0;
		hi_addr	= 1;

	        softc->cg12d.flags &= ~CG12_8BIT_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + softc->cmap_begin_indexed));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_indexed; i < softc->cmap_end_indexed;
		    i++, map++)
		    softc->ctl_sp->ramdac.color = map->packed;

		softc->cmap_begin_indexed = CG12_CMAP_SIZE;
		softc->cmap_end_indexed = 0;
	    }
	    if (softc->cg12d.flags & CG12_ALT_CMAP) 
	    {
		map = softc->cmap_alt + softc->cmap_begin_alt;
		lo_addr	= 0;
		hi_addr	= 0;

	        softc->cg12d.flags &= ~CG12_ALT_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + softc->cmap_begin_alt));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_alt; i < softc->cmap_end_alt;
		    i++, map++)
		    softc->ctl_sp->ramdac.control = map->packed;

		softc->cmap_begin_alt = CG12_CMAP_SIZE;
		softc->cmap_end_alt = 0;
	    }
	    if (softc->cg12d.flags & CG12_WID_UPDATE)
	    {
		softc->ctl_sp->wsc.addr = softc->wid_begin & 0x3f;
		for (i = softc->wid_begin; i < softc->wid_end; i++)
		{
		    softc->ctl_sp->wsc.data = softc->wid_tbl[i];
		}

		softc->cg12d.flags &= ~CG12_WID_UPDATE;
		softc->wid_begin = NWIDS;
		softc->wid_end = 0;
	    }
	    if (softc->cg12d.flags & CG12_24BIT_CMAP) 
	    {
		map = softc->cmap_rgb + softc->cmap_begin_rgb;
		lo_addr	= 0;
		hi_addr	= 0;

	        softc->cg12d.flags &= ~CG12_24BIT_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + softc->cmap_begin_rgb));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_rgb; i < softc->cmap_end_rgb;
		    i++, map++)
		    softc->ctl_sp->ramdac.color = map->packed;

		softc->cmap_begin_rgb = CG12_CMAP_SIZE;
		softc->cmap_end_rgb = 0;
	    }
	    if (softc->cg12d.flags & CG12_A12BIT_CMAP) 
	    {
		map = softc->cmap_a12 + softc->cmap_begin_a12;
		lo_addr	= 0;
		hi_addr	= 2;

	        softc->cg12d.flags &= ~CG12_A12BIT_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + softc->cmap_begin_a12));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_a12; i < softc->cmap_end_a12;
		    i++, map++)
		    softc->ctl_sp->ramdac.color = map->packed;

		softc->cmap_begin_a12 = CG12_CMAP_SIZE;
		softc->cmap_end_a12 = 0;
	    }
	    if (softc->cg12d.flags & CG12_B12BIT_CMAP) 
	    {
		map = softc->cmap_b12 + softc->cmap_begin_b12;
		lo_addr	= 0;
		hi_addr	= 3;

	        softc->cg12d.flags &= ~CG12_B12BIT_CMAP;
		softc->ctl_sp->ramdac.addr_lo =
		    RAMDACWRITE((lo_addr + softc->cmap_begin_b12));
		softc->ctl_sp->ramdac.addr_hi = RAMDACWRITE(hi_addr);

		for( i= softc->cmap_begin_b12; i < softc->cmap_end_b12;
		    i++, map++)
		    softc->ctl_sp->ramdac.color = map->packed;

		softc->cmap_begin_b12 = CG12_CMAP_SIZE;
		softc->cmap_end_b12 = 0;
	    }
	    if (softc->cg12d.flags & CG12_SLEEPING) 
	    {
	        softc->cg12d.flags &= ~CG12_SLEEPING;
		wakeup((caddr_t) softc);
	    }

	    if((shp->ver_release >> 8) <=6)
		softc->ctl_sp->apu.iclear |= CG12_APU_EN_HINT;
	}
	/* stary interrupts ? */
	else if ((eic_int & CG12_EIC_HINT) && !(istatus & CG12_APU_EN_HINT)) 
	{
	    istatus &= ~CG12_APU_EN_HINT;
	    softc->ctl_sp->apu.ien0 &= ~istatus;
	    softc->ctl_sp->apu.iclear |= istatus;
	}

	return;
}

cg12_cleanup(softc, gpunit)
	struct cg12_softc *softc;
	int gpunit;
{
	register struct gp1_shmem *shp = (struct gp1_shmem *) softc->shmem;
	register int allo, free, bit;
	register u_char *own, m;

	DEBUGF(3, ("cg12_cleanup:shp:%x,softc:%x,gpunit:%x\n",
	    shp, softc, gpunit));

	if (shp->alloc_sem || cg12_sync((caddr_t) shp, 0))
	    return 0;

	allo = IF68000(* (long *) &shp->host_alloc_h ^ 
		    * (long *) &shp->gp_alloc_h,
		(shp->host_alloc_h ^ shp->gp_alloc_h) << 16 | 
		    (shp->host_alloc_l ^ shp->gp_alloc_l));

	free = 0;
	bit = GP1_ALLOC_BIT(23);

	own = shp->block_owners + GP1_OWNER_INDEX(23);

	do {
	    if (allo & bit &&
		((m = *own) >= NMINOR ||
		m == (u_char) gpunit))
		free |= bit;
	    own++;
	} while ((bit <<= 1) >= 0);

	IF68000(* (int *) &shp->host_alloc_h ^= free,
		shp->host_alloc_h ^= free >> 16;
		shp->host_alloc_l ^= free);

	DEBUGF(3, ("cg12_cleanup: free:%x\n",free));
	return free;
}

static 
cg12_restart(gpminor, msg)
	int gpminor;
	char *msg;
{
	register struct cg12_softc *softc = getsoftc(gpminor);
	register struct gp1_sblock *sbp = softc->sblock;
	register u_short *cp = (u_short *) softc->shmem;
	register u_short *p =
		(u_short *) &((struct gp1_shmem *)softc->shmem)->gp_alloc_h;
	
	DEBUGF(3, ("cg12_restart:sbp:%x,cp:%x,p:%x,softc:%x,gpminor:%x,m:%s\n",
	    sbp,cp,p,softc,gpminor,msg));

	PR_LOOPP(CG12_STATIC_BLOCKS - 1,
	    if (sbp->owner) 
		psignal(sbp->owner, SIGXCPU);
		sbp++);

	/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	softc->ctl_sp->eic.reset |= CG12_EIC_DSPRST;
	softc->ctl_sp->apu.res2 = 0;
	/* !!! */

	PR_LOOPP(GP1_BLOCK_SHORTS - 1, *cp++ = 0);

	*p++ = 0x8000;
/*	*p = 0x00ff;	*/

	if (softc->dsp_active)
	{
	    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	    softc->ctl_sp->eic.reset &= ~CG12_EIC_DSPRST;
	    softc->ctl_sp->apu.res2 = 0;
	    /* !!! */
	}

	softc->restart++;

	printf("Graphics processor restarted%s after time limit exceeded.\n\
\t You may see display garbage because of this action.\n", msg);
}

cg12_sync(shmem, fd)
	caddr_t shmem;
	int fd;
{
    register short *shp,
                    host,
                    diff;

    DEBUGF(3, ("cg12_sync: shmem:%x,fd:%x\n",shmem,fd));

    if (!shmem)
	return 0;

    shp = (short *) &((struct gp1_shmem *) shmem)->host_count;
    host = *shp++;

    CDELAY ((diff = *shp - host, diff >= 0), cg12_sync_timeout);

    if (diff < 0)
	cg12_restart (fd >> 8, " by kernel");

    return 0;
}

#if NOHWINIT
cg12_binit(ctl_sp)
struct cg12_ctl_sp *ctl_sp;
{

    /* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
    ctl_sp->eic.host_control = 0;	/* no z-buf or dbl-buf */
    ctl_sp->apu.res2 = 0;
    /* !!! */

    ctl_sp->apu.h_sync	= 0x000b700b;
    ctl_sp->apu.hblank	= 0x0009d00d;
    ctl_sp->apu.v_sync	= 0x003ae00f;
    ctl_sp->apu.vblank	= 0x003ab027;
    ctl_sp->apu.vdpyint	= 0x00000000;
    ctl_sp->apu.vssyncs	= 0x000000af;
    ctl_sp->apu.hdelays	= 0x00013013;
    ctl_sp->apu.hpitches = 0x00370090;
    ctl_sp->apu.vsg_ctl	= 0x0013181b;
    ctl_sp->apu.stdaddr	= 0x000a0000;
    ctl_sp->apu.zoom	= 0x00000000;

    /*
     * pixel swap within byte. the least significant pixel should go in the
     * most significant bit.
     */
    ctl_sp->dpu.control = 0x10000;
    ctl_sp->dpu.reload_stb = 1;

    ctl_sp->ramdac.addr_lo = RAMDACWRITE(0x01);
    ctl_sp->ramdac.addr_hi = RAMDACWRITE(0x02);
    ctl_sp->ramdac.control = RAMDACWRITE(0x50);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(0x02);
    ctl_sp->ramdac.control = RAMDACWRITE(0);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(3);
    ctl_sp->ramdac.control = RAMDACWRITE(0x40);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(4);
    ctl_sp->ramdac.control = RAMDACWRITE(0xff);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(5);
    ctl_sp->ramdac.control = RAMDACWRITE(3);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(6);
    ctl_sp->ramdac.control = RAMDACWRITE(0);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(7);
    ctl_sp->ramdac.control = RAMDACWRITE(0);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(8);
    ctl_sp->ramdac.control = RAMDACWRITE(0x1f);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(9);
    ctl_sp->ramdac.control = RAMDACWRITE(0);
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(0xc);
    ctl_sp->ramdac.control = RAMDACWRITE(0);
}
#endif					/* NOHWINIT */

cg12_init(ctl_sp, dev_id)
struct cg12_ctl_sp *ctl_sp;
unsigned int       dev_id;
{
    int                 i, wsc_val;

    /* if cg12-B, overlay read mask for 5 bit overlay, else 2 */
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(8);
    ctl_sp->ramdac.addr_hi = RAMDACWRITE(0x02);
    if ((dev_id == 3) || (dev_id == 2))
	ctl_sp->ramdac.control = RAMDACWRITE(0x1f);
    else if ((dev_id == 1) || (dev_id == 0))
	ctl_sp->ramdac.control = RAMDACWRITE(0x3);
	
    /* Here come the colormaps */

    /* overlay */
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(0);
    ctl_sp->ramdac.addr_hi = RAMDACWRITE(1);
    ctl_sp->ramdac.control = 0xffffff;		/* white */
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(1);
    ctl_sp->ramdac.control = 0xffffff;		/* white */
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(2);
    ctl_sp->ramdac.control = 0x00ff00;		/* green */
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(3);
    ctl_sp->ramdac.control = 0x000000;		/* black */

    if ((dev_id == 2) || (dev_id ==3))
    {
	for (i = 0; i < 27; i++)
	    ctl_sp->ramdac.control = 0x000000;	/* black */
	ctl_sp->ramdac.control = 0xffffff;	/* white */
    }

    /* init the gamma, indexed, buff-a, and buf-b */
    for (i = 0; i < 256; i++)
    {
	/* gamma = grayscale ramp */
	ctl_sp->ramdac.addr_lo = RAMDACWRITE(i);
	ctl_sp->ramdac.addr_hi = RAMDACWRITE(0);
	ctl_sp->ramdac.color = RAMDACWRITE(i);

	/* indexed has entry 0 white ... all others black */
	ctl_sp->ramdac.addr_lo = RAMDACWRITE(i);
	ctl_sp->ramdac.addr_hi = RAMDACWRITE(1);
	ctl_sp->ramdac.color = RAMDACWRITE((i) ? 0x00 : 0xff);

	/*
	 * 12 bit buffer-a is a color repeated 16 times for 16 different
	 * colors
	 */
	ctl_sp->ramdac.addr_lo = RAMDACWRITE(i);
	ctl_sp->ramdac.addr_hi = RAMDACWRITE(2);
	ctl_sp->ramdac.color = RAMDACWRITE((i & 0xf0) | ((i & 0xf0) >> 4));

	/* 12 bit buffer-b is 16 colors repeated 16 times */
	ctl_sp->ramdac.addr_lo = RAMDACWRITE(i);
	ctl_sp->ramdac.addr_hi = RAMDACWRITE(3);
	ctl_sp->ramdac.color = RAMDACWRITE(((i & 0x0f) << 4) | (i & 0x0f));
    }

    /* init the alternate cmap */
    ctl_sp->ramdac.addr_lo = RAMDACWRITE(0);
    ctl_sp->ramdac.addr_hi = RAMDACWRITE(0);
    ctl_sp->ramdac.control = 0xffffff;		/* white */
    for (i = 0; i < 255; i++)
	ctl_sp->ramdac.control = 0x000000;	/* black */

    /* and now the window-ids */

    /* wsc 1 (dev_id == 0 or 1) = 3-color overlay, 8-bit; wsc 2 = just 8-bit */
    wsc_val =  (dev_id == 0 || dev_id == 1) ? 9 : 1;
	
    ctl_sp->wsc.addr = 0;
    for (i = 0; i < 64; i++)
	ctl_sp->wsc.data = wsc_val;
    ctl_sp->wsc.addr = 0;
}

struct cg12_cntxt {
    struct {
	u_int	haccess;
	u_int	hpage;
    } apu;
    struct {
	u_int	pln_rd_msk_host;
	u_int	pln_wr_msk_host;
	u_int	pln_sl_host;
    } dpu;
    struct {
	u_int	host_control;
    } eic;
};

cg12_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
Pixrect            *dpr;
int                 dx, dy, dw, dh;
int                 op;
Pixrect            *spr;
int                 sx, sy;
{
    Pixrect             dmempr, smempr;
    int                 cc, d_group, s_group;
    struct cg12_cntxt   context;
    struct cg12_ctl_sp *ctl_sp;

    /*
     * only overlay, overlay_enable, 8bit_color, and control are mapped into
     * the kernel.  don't allow other planes to be manipulated in the kernel.
     */

    d_group = s_group = PIXPG_CURRENT;
    if (dpr->pr_ops == &cg12_ops)
    {
	d_group = PIX_ATTRGROUP(cg12_d(dpr)->planes);
	ctl_sp = cg12_d(dpr)->ctl_sp;
    }
    if (spr && spr->pr_ops == &cg12_ops)
    {
	s_group = PIX_ATTRGROUP(cg12_d(spr)->planes);
	ctl_sp = cg12_d(spr)->ctl_sp;
    }
    (void) cg12_cntxsave(ctl_sp, &context);

    switch (d_group)
    {
	case PIXPG_CURRENT:
	case PIXPG_8BIT_COLOR:
	case PIXPG_OVERLAY:
	case PIXPG_OVERLAY_ENABLE:
	    CG12_PR_TO_MEM(dpr, dmempr);
	    break;

	default:
	    return 0;
    }

    switch (s_group)
    {
	case PIXPG_CURRENT:
	case PIXPG_8BIT_COLOR:
	case PIXPG_OVERLAY:
	case PIXPG_OVERLAY_ENABLE:
	    CG12_PR_TO_MEM(spr, smempr);
	    break;

	default:
	    return 0;
    }

    /* punt to mem_rop */

    cc = cg12_no_accel_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);
    (void) cg12_cntxrestore(ctl_sp, &context);
    return cc;
}


/*	Segment driver	*/

struct segcg12_data {
    int				flag;
    struct    cg12_cntxt	*cntxtp; 	/* pointer to context */
    struct    segcg12_data	*link;		/* shared_list */
    dev_t			dev;
    u_int			offset;
};

struct  segcg12_data     *cg12_shared_list;
struct  cg12_cntxt       cg12_shared_context;

#define segcg12_crargs segcg12_data

#define	SEG_CTXINIT	(1)	/* not initted */
#define	SEG_SHARED	(2)	/* shared context */
#define	SEG_LOCK	(4)	/* lock page */

/*ARGSUSED*/
int
cgtwelvesegmap(dev, offset, as, addr, len, prot, maxprot, flags, cred)
    dev_t dev;
    register u_int offset;
    struct as *as;
    addr_t *addr;
    register u_int len;
    u_int prot, maxprot;
    u_int flags;
    struct ucred *cred;
{
    register struct cg12_softc *softc;
    struct segcg12_crargs dev_a;
    int	segcg12_create();
    register    i;
    int    ctxflag = 0;

    enum	as_res	as_unmap();
    int	as_map();

    DEBUGF(3, ("cg12segmap:dev:%x,off:%x,as:%x, addr:%x, len:%x,flags:%x\n",
	dev, offset, as, addr, len, flags));

    softc = getsoftc(minor(dev));

    for (i = 0; i < len; i += PAGESIZE) 
    {
	if (cgtwelvemmap(dev, (off_t)offset + i, (int)maxprot) == -1)
	     return (ENXIO);

	if ((offset+i >= (off_t)softc->addr_tbl[0].offset) &&
	    (offset+i < (off_t)softc->addr_tbl[5].offset +
	    softc->addr_tbl[5].size))
	    ctxflag++;
    }

    if ((flags & MAP_FIXED) == 0) 
    {

	/*
	 * Pick an address w/o worrying about
	 * any vac alignment contraints.
 	 */

	map_addr(addr, len, (off_t)0, 0);
	if (*addr == NULL)
	    return (ENOMEM);

    } 
    else
    {
	/*
	 * User specified address -
	 * Blow away any previous mappings.
	 */

	 i = (int)as_unmap((struct as *)as, *addr, len);
    }

    /*
     * record device number; mapping function doesn't do anything smart
     * with it, but it takes it as an argument.  the offset is needed
     * for mapping objects beginning some ways into them.
     */

    dev_a.dev = dev;
    dev_a.offset = offset;

    /*
     * determine whether this is a shared mapping, or a private
     * one. if shared, link it onto the shared list, if private,
     * create a private cg12 context.
     */

    if (flags & MAP_SHARED) { /* shared mapping */
	struct segcg12_crargs *a;

	/* link it onto the shared list */
	a = cg12_shared_list;
	cg12_shared_list = &dev_a;
	dev_a.link = a;
	dev_a.flag = SEG_SHARED; 
	dev_a.cntxtp = (ctxflag) ? &cg12_shared_context : NULL;

    } else {		/* private mapping */
	dev_a.link = NULL;
	dev_a.flag = 0;
	dev_a.cntxtp =  
	    (ctxflag) ? 
		(struct cg12_cntxt *) 
		new_kmem_zalloc(sizeof (struct cg12_cntxt),KMEM_NOSLEEP) : NULL;
    }

    DEBUGF(3, ("cg12segmap: as:%x,*addr:%x,len:%x,&dev_a:%x,ctxflag:%x\n",
	as, *addr, len, &dev_a,ctxflag));

    return(as_map((struct as *)as, *addr,len, segcg12_create, (caddr_t)&dev_a));
}

static	int segcg12_dup(/* seg, newsegp */);
static	int segcg12_unmap(/* seg, addr, len */);
static	int segcg12_free(/* seg */);
static	faultcode_t segcg12_fault(/* seg, addr, len, type, rw */);
static	int segcg12_checkprot(/* seg, addr, size, prot */);
static	int segcg12_badop();
static	int segcg12_incore();

struct	seg_ops segcg12_ops =  {
	segcg12_dup,	/* dup */
	segcg12_unmap,	/* unmap */
	segcg12_free,	/* free */
	segcg12_fault,	/* fault */
	segcg12_badop,	/* faulta */
	segcg12_badop,	/* unload */
	segcg12_badop,	/* setprot */
	segcg12_checkprot,/* checkprot */
	segcg12_badop,	/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	segcg12_badop,	/* sync */
	segcg12_incore	/* incore */
};

/*
 * Gets a cg12_mapped entry for the specified process.  Either returns
 * pointer to existing entry or allocates new one if not found.
 */
static struct cg12_mapped *
get_cg12_mapped(softc, pid)
    register struct cg12_softc *softc;
    register int pid;
{
    register struct cg12_mapped *mp, *new_mp;
    
    if ((new_mp = softc->total_mapped) != NULL_MAPPED)
	do {
	    if ((mp = new_mp)->pid == pid)
		return(mp);
	} while((new_mp = mp->next) != NULL_MAPPED);

    new_mp = (struct cg12_mapped *) 
		new_kmem_zalloc(sizeof (struct cg12_mapped),KMEM_NOSLEEP);

    if (new_mp == NULL_MAPPED)
	panic("Can't allocate cg12_mapped structure");

    DEBUGF(3, ("get_cg12_mapped: new_mp:%x, pid:%x\n", new_mp, pid));

    if (softc->total_mapped == NULL_MAPPED)
	softc->total_mapped = new_mp;
    else {
	mp->next = new_mp;
	new_mp->prev = mp;
    }

    new_mp->pid = pid;
    return(new_mp);
}

static void
free_cg12_mapped(softc, mp)
    register struct cg12_softc *softc;
    register struct cg12_mapped *mp;
{
    DEBUGF(3, ("free_cg12_mapped: mp:%x, pid:%x, size left=%d\n", mp, mp->pid, mp->total));

    if (mp->next != NULL_MAPPED)
	mp->next->prev = mp->prev;
    if (mp->prev != NULL_MAPPED)
	mp->prev->next = mp->next;
    else
	softc->total_mapped = mp->next;
    kmem_free((char *) mp, sizeof (struct cg12_mapped));
}

static
int
segcg12_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segcg12_data *dp;
        register struct cg12_softc *softc;
	register struct cg12_mapped *mp;
	int i;

	DEBUGF(3, ("segcg12_create: seg:%x, argsp:%x\n", seg, argsp));

	dp = (struct segcg12_data *)
		new_kmem_zalloc(sizeof (struct segcg12_data),KMEM_NOSLEEP);
	*dp = *((struct segcg12_crargs *)argsp);

	seg->s_ops = &segcg12_ops;
	seg->s_data = (char *)dp;
	softc = getsoftc(minor(dp->dev));
	mp = get_cg12_mapped(softc, u.u_procp->p_pid);
	mp->total += seg->s_size;
	DEBUGF(3, ("segcg12_create: mp:%x, pid:%x, size now=%d\n", mp, mp->pid, mp->total));

	{
	    struct seg_tbl *segtbl = softc->seg_tbl;

	    for ( i = 0; i < NMINOR; i++, segtbl++)
	    {
		if (segtbl->owner && (segtbl->owner->p_pid == u.u_procp->p_pid))
		{
		    DEBUGF(3, ("segcg12_create: pid:%x, gpminor:%x, i:%x\n",
			segtbl->owner->p_pid, segtbl->gpminor, i));

		    return (0);
		}
	    }

	    segtbl = softc->seg_tbl;

	    for ( i = 4; i < NMINOR; i++, segtbl++)
	    {
		if (!segtbl->owner)
		{
		    segtbl->owner = u.u_procp;
		    segtbl->gpminor = i;
		    DEBUGF(3, ("segcg12_create: pid:%x,gpminor:%x\n",
			segtbl->owner->p_pid, segtbl->gpminor));

		    return (0);
		}
	    }
	}
	mp->total -= seg->s_size;
	DEBUGF(3, ("segcg12_create failed: mp:%x, pid:%x, size left=%d\n", mp, mp->pid, mp->total));
	if (mp->total == 0)
	    free_cg12_mapped(softc, mp);
	return (ENXIO);
}

static
int
segcg12_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segcg12_data *sdp = (struct segcg12_data *)seg->s_data;
	register struct segcg12_data *ndp;

	DEBUGF(3, ("segcg12_dup: seg:%x, newseg:%x\n", seg, newseg));

	(void) segcg12_create(newseg, (caddr_t)sdp);
	ndp = (struct segcg12_data *)newseg->s_data;

	if (sdp->flag & SEG_SHARED) {
		struct segcg12_crargs *a;

		a = cg12_shared_list;
		cg12_shared_list = ndp;
		ndp->link = a;
        } else if ((sdp->flag & SEG_LOCK) == 0) {
		if (sdp->cntxtp == NULL)
			ndp->cntxtp = NULL;
		else {
			ndp->cntxtp =
				(struct cg12_cntxt *)
				new_kmem_zalloc(sizeof (struct cg12_cntxt),KMEM_NOSLEEP);
			*ndp->cntxtp = *sdp->cntxtp;
		}
	}
	return(0);
}

static
int
segcg12_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
        register struct segcg12_data *sdp = (struct segcg12_data *)seg->s_data;
        register struct seg *nseg;
        addr_t nbase;
        u_int nsize;
	void	hat_unload();
	void	hat_newseg();

	DEBUGF(3, ("segcg12_unmap: seg:%x, addr:%x, len:%x\n", seg, addr, len));

	/*
 	 * Check for bad sizes
 	 */
 	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
 	    (len & PAGEOFFSET) || ((u_int)addr & PAGEOFFSET))
 		panic("segdev_unmap");

        /*
         * Unload any hardware translations in the range to be taken out.
         */

	hat_unload(seg,addr,len);

        /*
         * Check for entire segment
         */
 
 	if (addr == seg->s_base && len == seg->s_size) {
 		seg_free(seg);
 		return (0);
	}

	/*
	 * Partial unmap - keep track of how much is still mapped by this
	 * process
	 */
	{
	    register struct cg12_softc *softc;
	    register struct cg12_mapped *mp;

	    softc = getsoftc(minor(sdp->dev));
	    mp = get_cg12_mapped(softc, u.u_procp->p_pid);
	    mp->total -= len;
	    DEBUGF(3, ("segcg12_unmap: mp:%x, pid:%x, size left=%d\n", mp, mp->pid, mp->total));
	}

 	/*
 	 * Check for beginning of segment
 	 */
 
 	if (addr == seg->s_base) {
 		seg->s_base += len;
 		seg->s_size -= len;
 		return (0);
 	}
 
 	/*
 	 * Check for end of segment
 	 */
 	if (addr + len == seg->s_base + seg->s_size) {
 		seg->s_size -= len;
 		return (0);
 	}
 
 	/*
 	 * The section to go is in the middle of the segment,
 	 * have to make it into two segments.  nseg is made for
 	 * the high end while seg is cut down at the low end.
 	 */
 	nbase = addr + len;				/* new seg base */
 	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
 	seg->s_size = addr - seg->s_base;		/* shrink old seg */
 	nseg = seg_alloc(seg->s_as, nbase, nsize);
 	if (nseg == NULL)
 		panic("segcg12_unmap seg_alloc");
 
 	nseg->s_ops = seg->s_ops;
 
 	/* figure out what to do about the new context */
 
 	nseg->s_data = (caddr_t)new_kmem_alloc(sizeof (struct segcg12_data),KMEM_NOSLEEP);
 
         if (sdp->flag & SEG_SHARED) {
                 struct segcg12_crargs *a;
  
                 a = cg12_shared_list;
                 cg12_shared_list = (struct segcg12_data *)nseg->s_data;
                 ((struct segcg12_data *)nseg->s_data)->link = a;
         } else {
                 if (sdp->cntxtp == NULL)
 			((struct segcg12_data *)nseg->s_data)->cntxtp = NULL;
                 else {
                         ((struct segcg12_data *)nseg->s_data)->cntxtp =
                                 (struct cg12_cntxt *)
                                 new_kmem_zalloc(sizeof (struct cg12_cntxt),KMEM_NOSLEEP);
                         *((struct segcg12_data *)nseg->s_data)->cntxtp =
 				 *sdp->cntxtp;
                 }
         }
 
 	/*
 	 * Now we do something so that all the translations which used
 	 * to be associated with seg but are now associated with nseg.
 	 */
 	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);
	return(0);
}

static
int
segcg12_free(seg)
	struct seg *seg;
{
        register struct segcg12_data *sdp = (struct segcg12_data *)seg->s_data;
        register struct cg12_softc *softc = getsoftc(minor(sdp->dev));

	DEBUGF(3, ("segcg12_free: seg:%x,pid:%x\n", seg, u.u_procp->p_pid));

	/* If this process has anything else mapped then skip cleanup */
	{
	    register struct cg12_mapped *mp;

	    mp = get_cg12_mapped(softc, u.u_procp->p_pid);
	    mp->total -= seg->s_size;
	    DEBUGF(3, ("segcg12_free: mp:%x, pid:%x, size left=%d\n", mp, mp->pid, mp->total));
	
	    /* if still has mappings, just clean up segment only */
	    if (mp->total > 0) {
		if ((sdp->flag & SEG_SHARED) == 0 && (sdp->cntxtp != NULL))
		    kmem_free((char *)sdp->cntxtp, sizeof(struct cg12_cntxt));
		if (seg == softc->curcntxt)
		    softc->curcntxt = NULL;
		kmem_free((char *)sdp, sizeof (*sdp));
		return(0);
	    }

	    free_cg12_mapped(softc, mp);
	}

        if (seg == softc->curcntxt) {
            softc->curcntxt = NULL;
        }

	{
	    struct gp1_sblock *sbp = softc->sblock;

	    PR_LOOPP(CG12_STATIC_BLOCKS - 1,
		if(sbp->owner) {
		    DEBUGF(4, ("segcg12_free:sbp_pid:%x\n",sbp->owner->p_pid));

	    	    if (sbp->owner->p_pid == u.u_procp->p_pid)
			sbp->owner = 0;
		}
	    	sbp++);
	}

	{
	    struct cg12_wid_alloc *widp = softc->wid_alloc;

	    PR_LOOPP(NWIDS - 1,
		if(widp->owner) {
		    DEBUGF(4, ("segcg12_free:wid_pid:%x\n",widp->owner->p_pid));

		    if ((widp->type != FB_WID_SHARED_8 + 1) &&
			(widp->type != FB_WID_SHARED_24 + 1) &&
		        (widp->owner->p_pid == u.u_procp->p_pid)) 
		    {
			widp->owner = 0;
			widp->type = 0;
		    }
		}
		widp++);
	}

	{
	    struct seg_tbl *segtbl = softc->seg_tbl;

	    PR_LOOPP(NMINOR - 1,
		if(segtbl->owner && (segtbl->owner->p_pid == u.u_procp->p_pid))
		{
		    DEBUGF(3, ("segcg12_free:segtbl_pid:%x,gpminor:%x\n",
			segtbl->owner->p_pid,segtbl->gpminor));

		    segtbl->owner = 0;
		    (void) cg12_cleanup(softc, segtbl->gpminor);
		    segtbl->gpminor = 0;
		}
		segtbl++);
	}

        if ((sdp->flag & SEG_SHARED) == 0 && (sdp->cntxtp != NULL))
            kmem_free((char *)sdp->cntxtp, sizeof(struct cg12_cntxt));
        kmem_free((char *)sdp, sizeof (*sdp));
	return(0);
}
 
/*ARGSUSED*/
static
faultcode_t
segcg12_fault(seg, addr, len, type, rw)
	register struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register addr_t adr;
	register struct segcg12_data *current
	 	= (struct segcg12_data *)seg->s_data;
	register struct segcg12_data *old;
      	int pf;
	register struct cg12_softc *softc = getsoftc(minor(current->dev)); 
	void	hat_devload();
	void	hat_unload();

	if (type != F_INVAL) {
		return(FC_MAKE_ERR(EFAULT));
	}

	if (current->cntxtp && softc->curcntxt != seg) {
		if (softc->curcntxt != NULL) {
		    old = (struct segcg12_data *)softc->curcntxt->s_data;

		    /*
		     * wipe out old valid mapping.
		     */

		    hat_unload(softc->curcntxt, 
			softc->curcntxt->s_base, 
			softc->curcntxt->s_size);

		    if (!(current->flag == SEG_SHARED &&
			old->flag == SEG_SHARED))
		    {
			if (cg12_cntxsave(
			    (struct cg12_ctl_sp *)softc->ctl_sp,
			    old->cntxtp) == 0)
			    return(FC_HWERR);

			if (cg12_cntxrestore(
			    (struct cg12_ctl_sp *)softc->ctl_sp, 
			    current->cntxtp) == 0)
			    return(FC_HWERR);
		    }
		} else {
		    if ((current->flag != SEG_SHARED) &&
			(current->cntxtp->apu.haccess != 0))
		    {
			if (cg12_cntxrestore(
			    (struct cg12_ctl_sp *)softc->ctl_sp, 
			    current->cntxtp) == 0)
			    return(FC_HWERR);
		    } else {
			DEBUGF(2, ("segcg12_fault: seg:%x, addr:%x, len:%x\n", 
			    seg, addr, len));
		    }
		}

		/*
		 * switch software "context"
		 */
		softc->curcntxt = seg;
	}

	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
	    pf = cgtwelvemmap(current->dev, 
	    (off_t)current->offset+(adr-seg->s_base), 
	    PROT_READ|PROT_WRITE);

	    if (pf == -1)
		return (FC_MAKE_ERR(EFAULT));

	    hat_devload(seg, adr, pf, PROT_READ|PROT_WRITE|PROT_USER, 0 /*don't lock*/);
	}
	return(0);
}

/*ARGSUSED*/
static
int
segcg12_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	DEBUGF(3, ("segcg12_checkprot: seg:%x, addr:%x, len:%x, prot:%x\n", 
	    seg, addr, len, prot));

	return(PROT_READ|PROT_WRITE);
}

/*
 * segdev pages are always "in core".
 */
/*ARGSUSED*/
static
segcg12_incore(seg, addr, len, vec)
        struct seg *seg;
        addr_t addr;
        register u_int len;
        register char *vec;
{
        u_int v = 0;

	DEBUGF(3, ("segcg12_incore: seg:%x, addr:%x, len:%x, vec:%x\n", 
	    seg, addr, len, vec));

        for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE,
            v += PAGESIZE)
                *vec++ = 1;
        return (v);
}

static
segcg12_badop()
{
	/*
	 * silently fail.
	 */
	DEBUGF(3, ("segcg12_badop:\n"));

	return(0);
}

cg12_cntxsave(ctl_sp, saved)
	struct cg12_ctl_sp	*ctl_sp;
	struct cg12_cntxt	*saved;
{
	saved->eic.host_control = ctl_sp->eic.host_control;

	saved->apu.hpage = ctl_sp->apu.hpage;
	saved->apu.haccess = ctl_sp->apu.haccess;

	saved->dpu.pln_sl_host = ctl_sp->dpu.pln_sl_host;
	saved->dpu.pln_rd_msk_host = ctl_sp->dpu.pln_rd_msk_host;
	saved->dpu.pln_wr_msk_host = ctl_sp->dpu.pln_wr_msk_host;

	return(1);
}

cg12_cntxrestore(ctl_sp, saved)
	struct cg12_ctl_sp	*ctl_sp;
	struct cg12_cntxt	*saved;
{
	/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	ctl_sp->eic.host_control = saved->eic.host_control;
	/* !!! */

	ctl_sp->apu.hpage = saved->apu.hpage;
	ctl_sp->apu.haccess = saved->apu.haccess;

	ctl_sp->dpu.pln_sl_host = saved->dpu.pln_sl_host;
	ctl_sp->dpu.pln_rd_msk_host = saved->dpu.pln_rd_msk_host;
	ctl_sp->dpu.pln_wr_msk_host = saved->dpu.pln_wr_msk_host;

	return(1);
}

/*
 * The following code is only necessary because the cg12 8-bit and 24-bit
 * frame buffers share the same vram.  The normal mem_rop code (properly)
 * handles left and right end non-32-bit-aligned entries by reading a 32-bit
 * int and leaving the unused portion unaffected, then writing back the
 * 32-bit int.  On the cg12, this has the unfortunate effect of converting
 * the unused portion to 8-bit values which is bad if the underlying pixels
 * were 24-bit values.
 *
 * we can only flow to this code if the operation could not be accelerated
 * and we will only use this code if the destination is 8-bit.  otherwise,
 * mem_rop will still be used.  this code is optimized more for space than
 * for speed.  the user code version is better optimized for speed than space.
 */

cg12_no_accel_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect            *dpr, *spr;
int                 dx, dy, w, h, op, sx, sy;
{
    int                 i, j, incr, end;
    int                 wsx, src_offset, src_bytes, src_line;
    int                 wdx, dst_offset, dst_bytes, dst_line;
    unsigned char      *src, src_val, *dst;
    struct mprp_data   *src_data, *dst_data;

    /*
     * we only need this routine for 8-bit sharing memory with 24-bit. if the
     * destination is not 8-bit, then mem_rop can handle things just fine.
     */

    if (dpr->pr_depth != 8)
	return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);

    /* make sure all coordinates are within screen or region bounds */

    if (!(op & PIX_DONTCLIP))
    {
	struct pr_subregion d;
	struct pr_prpos     s;
	extern int          pr_clip();

	d.pr = dpr;
	d.pos.x = dx;
	d.pos.y = dy;
	d.size.x = w;
	d.size.y = h;

	s.pr = spr;
	if (spr)
	{
	    s.pos.x = sx;
	    s.pos.y = sy;
	}

	(void) pr_clip(&d, &s);

	dx = d.pos.x;
	dy = d.pos.y;
	w = d.size.x;
	h = d.size.y;

	if (spr)
	{
	    sx = s.pos.x;
	    sy = s.pos.y;
	}
    }

    if (w <= 0 || h <= 0)
	return 0;

    /* translate the origin */

    dx += mprp_d(dpr)->mpr.md_offset.x;
    dy += mprp_d(dpr)->mpr.md_offset.y;

    if (spr)
    {
	sx += mprp_d(spr)->mpr.md_offset.x;
	sy += mprp_d(spr)->mpr.md_offset.y;
    }

    --w;

    /*
     * Because of overlapping copies, must decide bottom up or top down. If
     * null source, then initialize the first destination line as the source
     * and start the copy from the second line.
     */

    if ((sy < dy) && (spr && spr->pr_ops == dpr->pr_ops))
    {
	incr = -1;
	end = dy - 1;
	if (spr)
	{
	    dst_line = dy + h - 1;
	    src_line = sy + h - 1;
	}
	else
	{
	    dst_line = dy + h - 2;
	    sx = dx;
	    src_line = dy + h - 1;
	}
    }
    else
    {
	incr = 1;
	end = dy + h;
	if (spr)
	{
	    dst_line = dy;
	    src_line = sy;
	}
	else
	{
	    dst_line = dy + 1;
	    sx = dx;
	    src_line = dy;
	}
    }

    /* data structure and destination memory address */

    dst_data = mprp_d(dpr);
    dst = (unsigned char *) dst_data->mpr.md_image;
    dst_bytes = dst_data->mpr.md_linebytes;

    if (spr)
    {
	src_data = mprp_d(spr);
	src = (unsigned char *) src_data->mpr.md_image;
	src_bytes = src_data->mpr.md_linebytes;
    }
    else
    {
	src_bytes = 0;
	dst_line -= incr;		/* didn't seed for bitblt */
    }

    if (sx > dx)			/* forward */
    {
	i = sx + w + 1;
	j = 1;
    }
    else
    {
	i = sx - 1;
	j = -1;
    }

    for (; dst_line != end; src_line += incr, dst_line += incr)
    {
	src_offset = src_line * src_bytes;
	dst_offset = dst_line * dst_bytes;

	if (j == 1)
	{
	    wsx = sx;
	    wdx = dx + dst_offset;
	}
	else
	{
	    wsx = sx + w;
	    wdx = dx + w + dst_offset;
	}


	for (; wsx != i; wsx += j, wdx += j)
	{
	    if (spr)
	    {
		switch (spr->pr_depth)
		{
		    case 1:
			src_val = (src[(wsx>>3)+src_offset] >> (7-(wsx%8)))&1;
			if (src_data->mpr.md_flags & MP_REVERSEVIDEO)
			    src_val = ~src_val & 1;
			src_val = src_val
			    ? ((PIX_OPCOLOR(op)) ? PIX_OPCOLOR(op) : ~0) : 0;
			break;

		    case 8:
			src_val = src[wsx + src_offset];
			break;
		}
	    }
	    else
		src_val = PIX_OPCOLOR(op);

	    switch (PIX_OP(op) >> 1)
	    {
		case 0:		/* CLR */
		    dst[wdx] = 0;
		    break;

		case 1:		/* ~s&~d or ~(s|d) */
		    dst[wdx] = ~(src_val | dst[wdx]);
		    break;

		case 2:		/* ~s&d */
		    dst[wdx] = ~src_val & dst[wdx];
		    break;

		case 3:		/* ~s */
		    dst[wdx] = ~src_val;
		    break;

		case 4:		/* s&~d */
		    dst[wdx] = src_val & ~dst[wdx];
		    break;

		case 5:		/* ~d */
		    dst[wdx] = ~dst[wdx];
		    break;

		case 6:		/* s^d */
		    dst[wdx] = src_val ^ dst[wdx];
		    break;

		case 7:		/* ~s|~d or ~(s&d) */
		    dst[wdx] = ~(src_val & dst[wdx]);
		    break;

		case 8:		/* s&d */
		    dst[wdx] = src_val & dst[wdx];
		    break;

		case 9:		/* ~(s^d) */
		    dst[wdx] = ~(src_val ^ dst[wdx]);
		    break;

		case 10:		/* DST */
		    break;

		case 11:		/* ~s|d */
		    dst[wdx] = ~src_val | dst[wdx];
		    break;

		case 12:		/* SRC */
		    dst[wdx] = src_val;
		    break;

		case 13:		/* s|~d */
		    dst[wdx] = src_val | ~dst[wdx];
		    break;

		case 14:		/* s|d */
		    dst[wdx] = src_val | dst[wdx];
		    break;

		case 15:		/* SET */
		    dst[wdx] = PIX_ALL_PLANES;
		    break;
	    }
	}
    }
    return 0;
}

enable_intr(softc)
struct cg12_softc *softc;
{
    if (!(softc->ctl_sp->apu.ien0 & CG12_APU_EN_HINT))
	softc->ctl_sp->apu.iclear |= CG12_APU_EN_HINT;
	softc->ctl_sp->apu.ien0 |= CG12_APU_EN_HINT;
/* !!! see cgtwelve.c EIC-COMMENTS before changing !!! */
	softc->ctl_sp->eic.interrupt |= CG12_EIC_EN_HINT;
	softc->ctl_sp->apu.res2 = 0;
/* !!! */
}
