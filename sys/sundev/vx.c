/*
 * Copyright 1990 by Sun Microsystems, Inc.
 */

#ifndef lint
static char sccsid[] = "@(#)vx.c 1.1 94/10/31 Copyright 1990 Sun Micro VIP ";
#endif
#ifndef lint
static  char sccsid1[] = "@(#)vxseg.c 2.9 91/03/04 Copyr 1990 Sun Micro VIP";
#endif
#ifndef lint
static  char sccsid2[] = "@(#)vxsubr.c 1.2 91/03/04 Copyr 1990 Sun Micro VIP";
#endif

#include <vx.h>

#include <win.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/vmmac.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/debug.h>
#include <sys/kmem_alloc.h>

#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/eeprom.h>
#include <machine/vm_hat.h>
#include <machine/psl.h>

#include <sundev/mbvar.h>
#include <sundev/vxioctl.h>
#include <sundev/vxseg.h>
#include <sundev/vxvar.h>
#include <sundev/p4reg.h>
#include <sundev/vxhmvec.h>

#include <sun/fbio.h>
#include <sun/seg_kmem.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg6var.h>

#include <sundev/cg6reg.h>
#include <sundev/vxreg.h>

#include <vm/as.h>
#include <vm/seg.h>

#ifndef PGT_VME_D32
#define PGT_VME_D32	0xC000000	/* == VME_D32<<26 == 0x3<<26 */
#endif

/*
 * cgvx stuff
 */

int	cgvxdebug=0;	/* MUST be initialized to get into data segment
			 *  otherwise kernel file cannot be patched using adb
			 */

#define CG6_ID_SHIFT 24
#define CG6_RES_MASK 0x0f
#define CG6_PROBESIZE   (NBPG)

/* forward references */
static	void	cgvxPAC1000(), 
		cgvxASIC(), 
		cgvxFHCTHC(),
		cgvxRAMDAC();

static	void	savefbc();

static	void	cgvxCMap_get(),
		cgvxCMap_put();

static	int	segvx_timeout();

#define ByteRep(A)	(((A)<<24)|((A)<<16)|((A)<<8)|(A))
#define ColorFull(A)	((unsigned int)(((A)<<16)|((A)<<8)|(A)))
#define ColorRed(A)	((unsigned int)(A))
#define ColorGren(A)	((unsigned int)((A)<<8))
#define ColorBlue(A)	((unsigned int)((A)<<16))
#define Upper(A) (((A)-2)<<16)
#define Lower(A) ((A)-2)
#define FBCRESET 0x8000
#define MODE68 0x0200

/*
 * This flag indicates that we should use kcopy instead of copyin
 * for cursor ioctls.
 */
int fbio_kcopy;
extern int copyin(), kcopy();
extern	caddr_t	getpages();	/* FIXME: check the type */

/*
 * Direction flags for savefbc()
 */
#define DIR_FROM_FBC    0
#define DIR_TO_FBC      1

#ifdef sun3x
#define PGT_CG6 0
#endif
#ifdef sun3
#define PGT_CG6 PGT_OBMEM
#endif
#ifdef sun4
#define PGT_CG6 PGT_OBIO
#endif

struct mapping vx_addrs[MAP_NMAP] = {
	{ CG6_VADDR_CMAP,	VX_ADDR_CMAP,	CG6_CMAP_SZ,    0 },
	{ CG6_VADDR_FHCTHC,	VX_ADDR_FHCTHC,	CG6_FHCTHC_SZ,  0 },
	{ CG6_VADDR_FBCTEC,	VX_ADDR_FBC,	CG6_FBCTEC_SZ,  0 },
	{ CG6_VADDR_COLOR,	VX_ADDR_COLOR,	VX_FB_SZ,     0 },
	{ VX_VADDR_VIDCSR,	VX_ADDR_VIDCSR,	VX_VIDCSR_SZ, 0 },
	{ VX_VADDR_VIDPAC,	VX_ADDR_VIDPAC,	VX_VIDPAC_SZ, 0 },
	{ VX_VADDR_VIDMIX,	VX_ADDR_VIDMIX,	VX_VIDMIX_SZ, 0 },
	{ VX_VADDR_VMECTL,	VX_ADDR_VMECTL,	VX_VMECTL_SZ, 0 },
	{ CG6_VADDR_ROM,	VX_ADDR_ROM,	CG6_ROM_SZ,     0 },
	{ 0,			0,		0,		0 },
	{ 0,			0,		VX_VX_SZ,	0 },	
};

/*
 * A blank page for mapping CG4 overlay/enable planes
 */
caddr_t blankpage;

/*
 * driver per-unit data
 */
struct vx_softc vx_softc[NVX];

/*
 * default structure for FBIOGTYPE ioctl
 */
static struct fbtype vxtypedefault =  {
        /* type           h  w  depth cms size */
        FBTYPE_SUN4COLOR, 0, 0, 8,    256,   0
};

/*
 * default structure for FBIOGATTR ioctl
 */
static struct fbgattr vxattrdefault =  {
        /* real_type     owner */
        FBTYPE_SUNFAST_COLOR, 0,
        /* fbtype: type h  w  depth cms  size */
        { FBTYPE_SUNFAST_COLOR, 0, 0, 8,    256,  0 },
        /* fbsattr: flags       emu_type       dev_specific */
        { 0,                    FBTYPE_SUN4COLOR, { 0, 0, 0, 1 } },
        /* emu_types */
        { FBTYPE_SUNFAST_COLOR, FBTYPE_SUN4COLOR, -1, -1},
};

/*
 * handy macros
 */
#define BZERO(d,c)      bzero((caddr_t) (d), (u_int) (c))
#define COPY(f,s,d,c)   (f((caddr_t) (s), (caddr_t) (d), (u_int) (c)))
#define CG6BCOPY(s,d,c) COPY(bcopy,(s),(d),(c))
#define COPYIN(s,d,c)   COPY(copyin,(s),(d),(c))
#define COPYOUT(s,d,c)  COPY(copyout,(s),(d),(c))

/*
 * enable/disable interrupt
 */
#define TEC_EN_VBLANK_IRQ       0x20
#define TEC_HCMISC_IRQBIT       0x10

/* position value to use to disable HW cursor */
#define CGVX_CURSOR_OFFPOS       (0xffe0ffe0)

#define fbmapout(virt, size) \
        mapout(&Usrptmap[btokmx((struct pte *) (virt))], (int) btoc((size)));

#define vx_pgmint_enable(softc) 				\
    {  								\
	struct	vx_vmectl *vmectl = 				\
		(struct vx_vmectl *)softc->va[MAP_VMECTL]; 	\
	vmectl->pgm_csr = 0x100 | softc->pvec; 			\
    }

#define vx_pgmint_disable(softc)  				\
    {  								\
	struct	vx_vmectl *vmectl = 				\
		(struct vx_vmectl *)softc->va[MAP_VMECTL]; 	\
	vmectl->pgm_csr = 0; 					\
    }

#define vx_vrtint_enable(softc) 				\
    {                                                           \
        struct    vx_vmectl *vmectl =                           \
            (struct vx_vmectl *)softc->va[MAP_VMECTL];          \
        int     x;                                              \
        x = splr(pritospl(softc->pri));                         \
        vmectl->vrt_csr = 0x100 | softc->vvec;                  \
        softc->thc->l_thc_hcmisc =                              \
        (softc->thc->l_thc_hcmisc & ~THC_HCMISC_RESET) |  TEC_EN_VBLANK_IRQ; \
        (void)splx(x);                                \
    }

#define vx_vrtint_disable(softc)  				\
    {                                                           \
        struct    vx_vmectl *vmectl =                           \
            (struct vx_vmectl *)softc->va[MAP_VMECTL];          \
        int     x;                                              \
        x = splr(pritospl(softc->pri));                         \
        softc->thc->l_thc_hcmisc =  \
         (softc->thc->l_thc_hcmisc & ~(THC_HCMISC_RESET | TEC_EN_VBLANK_IRQ)) \
                         | TEC_HCMISC_IRQBIT;   \
        vmectl->vrt_csr = 0;                    \
        (void)splx(x);                                \
    }

/* check if color map update is pending */
#define cgvx_intr_pending(softc)	\
        ((softc)->cmap_count ||		\
	 (softc)->omap_update ||	\
	 (softc)->intr)

#if NWIN > 0

/*
 * SunWindows specific stuff
 */

static cgvx_mem_rop(), cgvx_putcolormap();

/*
 * kernel pixrect ops vector
 */
struct pixrectops cgvx_ops = {
        cgvx_mem_rop,
        cgvx_putcolormap,
        mem_putattributes,
};

#endif

/* defines needed for probing for VX's and MVX's
 * and mapping space for high memory access
 */

/* extracted from jetmap.h */
#define SO_INC	      0x100000  /* Semaphore Op address modifier for clear */
#define SO_CLEAR      0x200000  /* Semaphore Op address modifier for increment */
#define SO_DEC        0x300000  /* Semaphore Op address modifier for decrement */
#define SP_DPRAM    0x01000000
#define DPRAMSIZE       0x8000

/* extracted from vxio.h */
#define JPP_NOFF     0x400000
#define JPP_BOFF     0x2000000
#define JPP_OFFSET   0x4000000

/* extracted from sfbvme.h */
#define FB_AV_HSB 	0x01c00000	
#define FB_AV_QUAD1 	0x00400000	/* quadrant 1	      */

/* extracted from sfb.h */
#define FB_AV_VIDBT 	0x01a80000	/* Brooktree RAMDACs	*/

#define HMEM_SIZE		    0x2000

int	vxprobe(/* int reg, int unit */);
int	vxattach(/* struct mb_device *mb */ );
int	vxintr(/* struct vx_softc *vx*/);

static struct vx_procinfo *vxgetproc(/* dev_t dev */);

struct mb_device *vxinfo[NVX];
struct mb_driver vxdriver = { 
	vxprobe,	/* probe routine	*/
	NULL,		/* slave routine	*/
	vxattach,	/* attach routine	*/
	NULL,		/* go routine		*/
	NULL,		/* done routine         */
	NULL,		/* poll routine         */
	4,		/* amount of memory space needed     */
	"vx",		/* device name                       */
	vxinfo,		/* backpointer to device structs     */
	NULL,		/* name of controller                */
	NULL,		/* backpointer to controller structs */         
	NULL,		/* mainbus flags                     */
	NULL		/* interrupt routine linked list     */
};

/* if we ever go over 32 bits we change these guys
 */
#define VXSET(x,n)   ((x) |= (1 << (n)))
#define VXISSET(x,n) ((x) & (1 << (n)))
#define VXCLEAR(x,n) ((x) &= ~(1 << (n)))

/* mapvme2kadr allocates a page of kernel memory and maps it to the give vme adr
 * 	returns the kernel address that was mapped in
 */
long *
mapvme2kadr(adr, size)
    long       *adr;		/* where in VME space to map */
    int		size;		/* number of bytes to map */
{
    int		kmx, pageval;
    long       *kadr; 
    int		s;

    
    /* recompute size with padding added for wasted space between nearest
     * page boundary and the given address
     */
    /* size += ((int)adr % PAGESIZE); */
    
    /* get index of empty entry in kernel map of size size (page aligned) */

    s = splhigh();
    if ((kmx = rmalloc(kernelmap, (long)btopr(size))) == 0)
	panic("vx: probe: out of kernelmap for devices");
    (void)splx(s);
   
    /* convert it to kernel virtual address */
    kadr = (long *)kmxtob(kmx);	
    
    /* compute the physical page at the address */
    pageval = btop(adr) | PGT_VME_D32;
    
    /* map it to the allocated kernel mem
     * 		ptob(btopr(size)) rounds up to nearest page sized chunk
     */
    segkmem_mapin(&kseg, (addr_t)kadr, ptob(btopr(size)), 
		  (u_int)PROT_READ | PROT_WRITE, (u_int)pageval, 0);
    
    /* return pointer to allocated kmem, offset from page boundary 
       to proper adr */
    return(kadr + (((int)adr % PAGESIZE) / sizeof(long)));
}    


/* freekadr deallocates kernel space beginning at 'kadr' created by mapvme2kadr */
freekadr(kadr, size)
    long       *kadr;
    int		size;
{
    int	s;

    /* free up the kernel memory and return */
    segkmem_mapout(&kseg, (addr_t)ptob(btopr(kadr)), ptob(btopr(size)));
    s = splhigh();
    rmfree(kernelmap, (long)ptob(btopr(size)), (u_long)kadr);
    (void)splx(s);
}


/* sematouch attempts to poke 'val' into 'adr'
 * 	if the address exists, returns the value at the same location after 2 reads
 * 	otherwise returns -1
 */
long
sematouch(adr, val)
    long       *adr, val;
{
    long	res;
    
    /* if the address exists, put in a value and take one out */
    if (! pokel(adr, val)) {
	res = *adr;		/* read twice to trigger semaphore change */
	res = *adr;
    }
    
    /* otherwise, indicate nonexistent address */
    else
	res = -1;
    
    return(res);
}

/*ARGSUSED*/
static
cgvxprobe(reg, unit)
    caddr_t	reg;
    int		unit;
{
    register struct vx_softc *softc = getsoftc(unit);
    register struct mapping *mp;
    register caddr_t *va;
    register int i;
    int id;
    u_long kmx, rmalloc();
    long peekval;
    u_int hat_getkpfnum();
    register long *Addr;
    int resolution;
    int status;
    caddr_t	legoreg;

    legoreg = 
	(caddr_t)mapvme2kadr(
		(long *)(ptob(hat_getkpfnum((addr_t)reg) + btop(VX_LEGO))), 1);
    /*
     * Determine frame buffer resolution:
     */
#ifdef sun3
    /* this is for SunOS 3.x which does not know hat_petkpfnum */
    /* we should simply use fbgetpage in this case */
    softc->baseaddr = 0xFF000000;
#else
    softc->baseaddr = ptob((u_long) hat_getkpfnum((addr_t) reg));
#endif

    mmu_getkpte((addr_t) legoreg, (struct pte *)&status);

#if defined(SUN4_110)
    if (cpu == CPU_SUN4_110 && !(softc->baseaddr & 0xf0000000))
	    softc->baseaddr |= 0xf0000000;
#endif

    for(i = 0; i< MAP_NMAP; i++) {
	softc->addrs[i].vaddr = vx_addrs[i].vaddr;
	softc->addrs[i].paddr = vx_addrs[i].paddr;
	softc->addrs[i].size = vx_addrs[i].size;
	softc->addrs[i].prot = vx_addrs[i].prot;
    }
    softc->addrs[MAP_VX].vaddr = (u_int)ptob(hat_getkpfnum((addr_t)reg));
    softc->addrs[MAP_VX].paddr = 0;

    id = 0xffdeadff;
    if (peekl((long *)legoreg, (long *)&id) == -1) {
	DEBUGprint (2, "vxprobe: FBC CONFIG read failed!\n",0,0);
	return 0;
    }
    else {
	DEBUGprint1 (2, "vxprobe: FBC CONFIG=%x\n", id);
    }

    resolution = (id >> CG6_ID_SHIFT) & CG6_RES_MASK;
    DEBUGprint1 (2, "vxprobe: %d sensed on monitor cable.\n", resolution);
    switch (resolution) {
    case 1:
    case 3:
    case 5:
    case 4:         /* Possibly the 13W3-4BNC adapter */
	break;

    /* If "no monitor" or unsupported resolution, default to 1280x1024!! */
    case 2:
    case 6:
    case 7:
    default:
	printf ("UNSUPPORTED resolution sensed!\n");
	resolution = 0;
    case 0:
	break;
    }
    softc->videofmt = vx_monitor[resolution];
    softc->w = videofmt[softc->videofmt].x;
    softc->h = videofmt[softc->videofmt].y;
    softc->hz = videofmt[softc->videofmt].hz;
    softc->clk = videofmt[softc->videofmt].clk;

    fbmapin(legoreg, (int) VX_ADDR(VX_ADDR_CMAP), (int) NBPG);

    /*
     * Check for Brooktree RAMDACs; "0x4d" is BT461, "0x4c" is BT462.
     */
    Addr = (long *)(legoreg + VX_BTID*4);
    id = 0x80dead01;
    status = peekl(Addr, (long *)&id);
    if( !status ) {
	int rd1=(id&0xff), rd2=((id>>8)&0xff), rd3=((id>>16)&0xff);
	DEBUGprint1 (2, "vxprobe: RAMDAC ID = %x (", id&0xffffff);
	DEBUGprint1 (2, "%x,", rd1);
	DEBUGprint1 (2, "%x,", rd2);
	DEBUGprint1 (2, "%x)\n", rd3);
	if ( !(
	    (rd1==0x4c || rd1==0x4d) &&
	    (rd2==0x4c || rd2==0x4d) &&
	    (rd3==0x4c || rd3==0x4d)
	    ) ) { 
	    printf ("vxprobe: RAMDAC ID incorrect, = 0x%x!\n", id);
	    return 0;
	}
    }
    else {
	printf ("vxprobe: RAMDAC ID read failed!\n");
	return 0;
    }


    /*
     * Map in each section
     */
    for (va = softc->va,mp = vx_addrs, i = MAP_NPROBE; i; i--, mp++, va++) {
	/*
	 * allocate enough pages 
	 */
	kmx = rmalloc(kernelmap, (long) btoc(mp->size));
	if (kmx == 0) {
	    va--, mp--, i++;
	    printf ("vxprobe: No kernel pages available!\n");
	    goto die;
	}

	/*
	 * get kernel virtual address 
	 */
	*va = (caddr_t) kmxtob(kmx);

	/*
	 * Map in this section 
	 */
	fbmapin(*va, (int) VX_ADDR(mp->paddr), (int) mp->size);

	/*
	 * Check if this section is present. If
	 * not, map out all sections, and return an error.
	 */
	if (mp->paddr!=VX_ADDR_COLOR && peekl((long *) *va, &peekval) == -1) {
	    printf ("vxprobe: Memory mapping failure va=%x pa=%x!\n",
					    va, mp->paddr);
die:
		do {
			fbmapout(*va, (int) mp->size);
			va--;
			mp--;
		} while (i++ < MAP_NMAP);
		freekadr((long *)legoreg, 1);
		return 0;
	}
    }

    softc->found = 1;
#ifdef NOTDEF
    freekadr(reg, 1);
#endif
    return (CG6_PROBESIZE);
}

vxprobe(reg, unit)
    caddr_t	reg; 
    int		unit;
{
    int 	bd, nd;
    int		dpbase, vxbase = ptob(hat_getkpfnum((addr_t)reg));
    long        *kadr;
    register struct vx_softc *softc = getsoftc(unit);

    /*
     * Is there an SFB attached?
     */
    DEBUGprint1 (2, "vxprobe: (0x%x) checking for VX...\n", vxbase);
    if (cgvxprobe(reg, unit) == 0) 
	return(0);
    else {
	DEBUGprint1 (2, "vxprobe: identified VX on board 0\n", 0);
	VXSET(softc->vx_ndmask, 0);
    }

    /*
     * Test clear, inc and dec semaphores for presence of JPP's
     */
    DEBUGprint1 (2, "vxprobe: checking for MVX's...\n", 0);
    for (bd = 0; bd < (VX_MAXBOARDS - 1); bd++) {
	for (nd = 0; nd < 4; nd++) {
	    dpbase = vxbase + JPP_OFFSET +
		bd * JPP_BOFF + nd * JPP_NOFF + SP_DPRAM;
	    
	    kadr = mapvme2kadr((long *)(dpbase + SO_INC), 1);
	    if (sematouch(kadr, (long)2) != 3)
		break;
	    freekadr(kadr, 1);

	    kadr = mapvme2kadr((long *)(dpbase + SO_CLEAR), 1);
	    if (sematouch(kadr, (long)2) != 0)
		break;
	    freekadr(kadr, 1);

	    kadr = mapvme2kadr((long *)(dpbase + SO_DEC), 1);
	    if (sematouch(kadr, (long)2) != 1)
		break;
	    freekadr(kadr, 1);
	}
	
	/* if we're able to clear, inc, and dec all four nodes, we can assume
	 * a JPP is present, so set mask for all four at once
	 */
	if (nd == 4) {
	    DEBUGprint1 (2, "vxprobe: identified MVX on board %d\n", bd);
	    for (nd = 0; nd < 4; nd++)
		VXSET(softc->vx_ndmask, (bd+1) * 4 + nd);
	    }
    }

    if (softc->vx_ndmask)
	return 4;
    else
	return 0;
}

static
cgvxattach(md)
    register struct mb_device *md;
{
    register struct vx_softc *softc = getsoftc(md->md_unit);
    register  struct thc *thc;
    register  struct tec *tec;
    register unsigned int *cfg, *asic, *pac, *video, *cmap;
    u_int	*cmd = ((u_int *)softc->va[MAP_CMAP]) + VX_BTOVRDMASK;
    struct    vx_vmectl *vmectl;

    int bootdev;
    struct	vec	*vp;
    register unsigned int i;

DEBUGprint (0x1, "cgvxattach \n", 0, 0); 

    tec = (struct tec*) (softc->va[MAP_FBCTEC] + VX_TEC_POFF);
    thc = (struct thc*) (softc->va[MAP_FHCTHC] + VX_THC_POFF);
    cfg = (unsigned int *) (softc->va[MAP_FHCTHC]);
    video = (unsigned int *) (softc->va[MAP_VIDCSR]);
    pac = (unsigned int *) (softc->va[MAP_VIDPAC]);
    asic = (unsigned int *) (softc->va[MAP_VIDMIX]);
    cmap = (unsigned int *) (softc->va[MAP_CMAP]);
    vmectl = (struct vx_vmectl *)softc->va[MAP_VMECTL];

    if(!(*video & 1)) {	/* PAC video controller not active.
			 *  Starting it with any clock will allow
			 *  access to the firmware version #.  Stop
			 *  it when we're done and let start it
			 *  again when we configure it later. */
	DEBUGprint1 (2, "vxattach: VX is secondary device.\n", 0);
	bootdev=0;
	*video |= 0x0001;	/* Start PAC */
	cgvx_pac_rel = (*pac & 0xff00) >> 8;
	cgvx_pac_ver = (*pac & 0x00ff);
	DEBUGprint (2,
		"vxattach: VX video controller (PAC) s/w version %x.%x\n",
				    cgvx_pac_rel, cgvx_pac_ver);
	*video &= 0xfe;		/* Stop the PAC */
        DELAY (1000);           /* Wait sync to propagate to RAMDACs */
        *video |= 0x80;         /* Start Bt439 reset */
        *video &= 0x7f;         /* Stop  Bt439 reset */
        *(pac+1) = 0x0000;      /* Turn off C/H sync + blanking */
        *(pac+1) = 0x0e00;      /* Turn on C/H sync + blanking */
    } else {		/* PAC video controller already active.
			 *  Let's not mess with it and just get the
			 *  firmware version. */

	DEBUGprint1 (2, "vxattach: VX is boot device.\n", 0);
		    /* PAC running therefore assume we are boot device */
	bootdev=1;
	*video |= 0x20;			/* Set PAC req. */
	i = *(pac+4);			/* Clear input FIFO */
	while (*video & 0x8000) 
		i = *pac;		/* Spin on PAC ack  */
	*(pac+30) = 0;			/* Set PAC op to "Version#" */
	i = *pac;			/* Read any PAC reg to engage op */
	VXDELAY ( (!(*video & 0x8000)), 100); /* Spin until PAC ack  */
	i = *pac;			/* Read any version number */
	cgvx_pac_rel = (i & 0xff00) >> 8;
	cgvx_pac_ver = (i & 0x00ff);
	DEBUGprint (2,
		"vxattach: VX video controller (PAC) s/w version %x.%x\n",
				    cgvx_pac_rel, cgvx_pac_ver);
	VXDELAY ( (*video & 0x8000), 100);	/* Spin on PAC ack  */
	*video &= 0xdf;				/* Clear PAC req. */
    }

    softc->thc = thc;
    softc->owner = NULL;

    vp = md->md_intr;
    softc->pvec = vp->v_vec;
    *(vp->v_vptr) = (int)softc;
    vp++;
    softc->vvec = vp->v_vec;
    *(vp->v_vptr) = (int)softc;
    vmectl->pgm_csr = 0;
    vmectl->vrt_csr = 0;

    softc->pri = md->md_intpri;
    softc->size = VX_FB_SZ;
    softc->intr = 0;
    softc->gattr = vxattrdefault;
    softc->gattr.fbtype.fb_height = softc->h;
    softc->gattr.fbtype.fb_width =  softc->w;
    softc->gattr.fbtype.fb_size = softc->size;

    DEBUGprint (2, "vxattach: ASIC..", 0,0); 
    cgvxASIC(asic);
    DEBUGprint (2, "RAMDAC..", 0,0); 
    cgvxRAMDAC(cmap);
    DEBUGprint (2, "FHCTHC..", 0,0); 
    cgvxFHCTHC(softc, cfg, thc);
    tec->l_tec_mv	= 0;
    tec->l_tec_clip	= 0;
    tec->l_tec_vdc	= 0;

    /* Enable overlay plane display */
    *cmd = 0x070707;

    /* Turn off cursor, but do it myyyyyyyyyyyy way. JMP */
    softc->thc->l_thc_cursor = CGVX_CURSOR_OFFPOS;

    if (!bootdev) { 
	DEBUGprint (2, "VIDEO..", 0,0); 
	cgvxPAC1000(softc,video,pac); 
    }

    /*
     * initialize lockbase
     */
    softc->lockbase = (caddr_t)
	(softc->addrs[MAP_NMAP-1].vaddr + 
	softc->addrs[MAP_NMAP-1].size + PAGESIZE);

    /*
     * save fbc for PROM
     */
    savefbc(&softc->fbc, (struct fbc*)(softc->va[MAP_FBCTEC]), DIR_FROM_FBC);

    DEBUGprint (2,"\n", 0,0);
}

vxattach(md)
    struct mb_device *md;
{
    int	       		nd;
    struct vx_softc	*vx = getsoftc(minor(md->md_unit));
    struct vx_nodeinfo	*vn = vx->vx_nodeinfo;

    cgvxattach(md);
    
    /* for each node under this device, map some kernel memory to the high page
     * of the node's memory where important data is
     */
    for (nd = 0; nd < VX_MAXNODES; nd++, vn++) {
	if VXISSET(vx->vx_ndmask, nd) {
	    if (nd == 0)
		vn->vn_datap = (int)mapvme2kadr(
				(long *)(ptob(hat_getkpfnum(md->md_addr)) +
				FB_AV_HSB + FB_AV_QUAD1 - HMEM_SIZE),
				HMEM_SIZE);
	    else
		vn->vn_datap = (int)mapvme2kadr(
				(long *)(ptob(hat_getkpfnum(md->md_addr)) +
				JPP_OFFSET + ((nd - 4) / 4) * JPP_BOFF +
				((nd - 4) % 4) * JPP_NOFF +
				SP_DPRAM + DPRAMSIZE - HMEM_SIZE),
				HMEM_SIZE);
	    
	    /* initialize the high memory vectors
	     * note:  can only use 32 bit write/read requests
	     */
	    *(unsigned int *)(vn->vn_datap + HPCVEC_OFFSET) = 0;
	    *(unsigned int *)(vn->vn_datap + HSRVEC_OFFSET) = HS_READY;
	    
	    /* allocate space for the buffer used in DVMA transfers */
	    vn->vn_bp = (struct buf *)kmem_alloc(sizeof(struct buf));
	}
    }

/*
 * Output configuration details.
 */
    DEBUGprint1 (2, "vxattach: VX node mask = %x\n", vx->vx_ndmask);
    {	int unit=minor(md->md_unit), mvx=0;
	printf ("  vx%d is", unit);
	if VXISSET(vx->vx_ndmask, 0) {
	    printf (" a VX");
	    if (cgvx_pac_rel<=4) printf ("(rev1.1)");
	    else if (cgvx_pac_rel==5) printf ("(rev1.2)");
	    else if (cgvx_pac_rel==6) printf ("");
	    printf (" at %dx%d resolution", vx->w, vx->h);
	    DEBUGprint1 (2, " @ %dHz", vx->hz);
	    DEBUGprint1 (2, " (fmt=%d)", vx->videofmt);
	}
	for (nd=4; nd<VX_MAXNODES; nd+=4) if VXISSET(vx->vx_ndmask, nd) mvx++;
	if (VXISSET(vx->vx_ndmask,0) & mvx) printf (", plus ");
	if(mvx) printf ("%c MVX%c", (mvx>1)?(mvx+'0'):'a', (mvx>1)?'s':' ');

	printf ("\n");
    }
    return 0;
}

/*ARGSUSED*/
int
cgvxmmap(dev, off, prot)
    dev_t	dev;
    register	off_t off;
    int		prot;
{
    register struct vx_softc *softc = getsoftc(minor(dev));
    register struct mapping *mp;
    register int i;

    /*
     * See if the offset specifies a 'real' CG6 address
     */
    for (mp = softc->addrs, i = 0; i < MAP_NMAP; i++, mp++) {
	if (off >= (off_t) mp->vaddr && off < (off_t) (mp->vaddr + mp->size)) {

	    /*
	     * See if this section may only be mapped by root.
	     */
	    if (mp->prot && !suser()) {
		    return (-1);
	    }
	    return (VX_ADDR(mp->paddr + (off - mp->vaddr)));
	}
    }

    /*
     * Check if this is within the CG4 Overlay/Enable/Color
     * Frame buffer regions. Map overlay & enable to a page in
     * the kernel.
     */
    if (off < (off_t) softc->bw1size) {
	if (blankpage == 0) {
	    blankpage = getpages(1, KMEM_SLEEP);
	}
	return (btop(blankpage));
    }

    return (-1);
}

/*ARGSUSED*/
int
vxmmap(dev, off, prot)
    dev_t	dev; 
    off_t	off; 
    int		prot;
{
    u_int addr;

    addr = (u_int)hat_getkpfnum((addr_t)vxinfo[minor(dev)]->md_addr);
    addr += btop(off);
    
    return(addr);
}


/* vxhsr_service is called by vxintr to handle potential Host Segment Requests
 * if there is a request, vxhsr_service unlocks any host pages which had been
 * requested earlier and locks down the new page
 * it then sets up a buffer structure for the DVMA transfer and then handles all
 * the mapping using mbsetup()
 *
 * returns with hs_flag set to either HS_READY or HS_NOSPACE
 * if the DVMA setup was successful, it returns READY along with hs_mapadr set
 * to the VME address available to the vx node for DVMA transfers
 */
void
vxhsr_service(vx, vn)
    struct vx_softc    *vx;
    struct vx_nodeinfo *vn;
{
    struct hsrvec      *hs = (struct hsrvec *)(vn->vn_datap + HSRVEC_OFFSET);
    struct mb_device   *md = vxinfo[minor(vx->vx_dev)];
    struct vx_mseg     *vms = &vn->vn_mseg;
    struct vx_mseg     *ovms = &vn->vn_omseg;
    struct buf 	       *bp = vn->vn_bp;

    int fc, addr;
    caddr_t a;

#ifdef notdef
printf("vxintr: vxhsr_service: hsrvec at kernel mem %x = %d\n", hs, hs->hs_flag);
#endif notdef
    
    /* does the node have an hsr request? */
    if (hs->hs_flag != HS_WANTPAGE)
	return;
    
#ifdef notdef
    printf("vxintr: vxhsr_service: phase 1\n");
    printf("vxintr: vxhsr_service: a=%x s=%x p=%x\n",vms->vms_addr,vms->vms_size,vms->vms_pp);
    printf("vxintr: vxhsr_service: a=%x s=%x p=%x\n",ovms->vms_addr,ovms->vms_size,ovms->vms_pp);
#endif notdef

    /* if we had locked down a host page earlier,
     * unlock it now and release the DVMA mapping
     */
    if (ovms->vms_addr != NULL){
	fc = (int)as_fault(ovms->vms_pp->p_as,ovms->vms_addr,PAGESIZE,F_SOFTUNLOCK,S_WRITE);
        if (fc)
	    panic("vxhsr_service");
	ovms->vms_addr = NULL;
	mbrelse(md->md_hd, &vn->vn_mbinfo);
    }

#ifdef notdef
    printf("vxintr: vxhsr_service: phase 2, hs->hs_off=%x\n", hs->hs_off);
#endif notdef

    /* compute the new virtual address offset from the exported segment's base */
    a = vms->vms_addr + hs->hs_off;

    /* make sure it's a valid address */
    if (a > vms->vms_addr + vms->vms_size) {
	printf("vxintr: page out of range off = %x\n", hs->hs_off);
	hs->hs_flag = HS_READY;
	return;
    }
 
    /* lock down the host page & store the address so we can unlock it later */
    fc = (int)as_fault(vms->vms_pp->p_as,a,PAGESIZE,F_SOFTLOCK,S_WRITE);
    if (fc)
	panic("vxhsr_service");
    ovms->vms_addr = a;

    /* setup a buffer for the DVMA transfer */
    bp->b_proc = vms->vms_pp;
    bp->b_un.b_addr = a;
    bp->b_dev = vx->vx_dev;
    bp->b_bcount = PAGESIZE;
    bp->b_blkno = 0;
    bp->b_flags = B_BUSY | B_PHYS | B_WRITE;

#ifdef notdef
    printf("vxintr: vxhsr_service: phase 3\n");
#endif notdef

    /* try and setup a memory map for a DVMA transfer */
    if (vn->vn_mbinfo = mbsetup(md->md_hd, bp, 1)) {
	addr = MBI_ADDR(vn->vn_mbinfo);		/* convert the cookie */
	hs->hs_mapaddr = addr;			/* let caller know where it is */
	hs->hs_flag = HS_READY;
    }
    else
	hs->hs_flag = HS_NOSPACE;		/* indicate a map couldn't be found */

#ifdef notdef
    printf("vxintr: vxhsr_service: fc=%x addr=%x\n", fc, addr);
#endif notdef
}

vxintr(vx)
    struct vx_softc *vx;
{
    struct vx_nodeinfo *vn = vx->vx_nodeinfo;
    struct vx_procinfo *vp;
    int nd, inproc;
    
/* TODO: on interrupt, shift ndmask and && with 1 as part of loop to avoid VXISSET
 */

    if (vx->intr & VX_VRTWAIT) {
	vx->intr &= ~VX_VRTWAIT;
	vx_pgmint_disable(vx);
	wakeup((caddr_t)vx);
	return(0);
    }

    /* check each node to determine if it called for an interrupt */
    for (nd = 0; nd < VX_MAXNODES; nd++, vn++) {
	if (VXISSET(vx->vx_ndmask, nd)) {
	    
#ifdef notdef
printf("vxintr: checking for hpc request\n");
#endif notdef
	    
	    /* does the node have and hpc request? */
	    if (*(unsigned int *)(vn->vn_datap + HPCVEC_OFFSET)) {
		/* yes, so check each process
		 * and find out if its interruptable for this node
		 */
#ifdef notdef
printf("HPC flag set for node %d.  Checking processes...\n", nd);
#endif notdef
		vp = vx->vx_procinfo;
		for (inproc = 0; inproc < VX_MAXPROCS; inproc++, vp++) {
		    if (vp->vp_procp && VXISSET(vp->vp_intrmask, nd)) {
			/* yes, it is, so go ahead and send it and interrupt signal */
#ifdef notdef
			printf("sending signal to %d\n", vp->vp_procp->p_pid);
#endif
			psignal(vp->vp_procp, SIGIO);
		    }
		}
	    }

#ifdef notdef
printf("vxintr: checking for hsr request\n");
#endif notdef
	    
	    /* handle possible hsr request by this node */
	    vxhsr_service(vx, vn);
	}
    }
    return(0);
}
    
#ifdef notdef
vxselect()
{
    return(0);
}

#endif

vxopen(dev, flag) 
    dev_t dev; int flag;
{
    return fbopen(dev, flag, NVX, vxinfo);
}

/*ARGSUSED*/
vxclose(dev, flag)
    dev_t	dev;
    int		flag;
{
    register struct vx_softc *softc = getsoftc(minor(dev));

    softc->owner = NULL;

    softc->gattr = vxattrdefault;
    softc->gattr.fbtype.fb_height = softc->h;
    softc->gattr.fbtype.fb_width =  softc->w;
    softc->gattr.fbtype.fb_size = softc->size;
    softc->cur.enable = 0;
    cgvx_setcurpos(softc);

    /*
     * Restore fbc for PROM
     */
    savefbc((struct fbc*)(softc->va[MAP_FBCTEC]), &softc->fbc, DIR_TO_FBC);

    /* also, reprep the fhc/thc */

    cgvxFHCTHC(softc, (unsigned int *) (softc->va[MAP_FHCTHC]),
	(struct thc*) (softc->va[MAP_FHCTHC] + VX_THC_POFF));
}

/* XXX - we use vx_segfree to simulate a close per process rather
   than a close on last process.
   */
int
vxfreethings(dev)
    dev_t	dev; 
{
    struct vx_softc *vx = getsoftc(minor(dev));
    struct vx_nodeinfo *vn = vx->vx_nodeinfo;
    struct vx_procinfo *vp;
    struct vx_mseg     *ovms;
    int i, fc;
    
    for(i = 0; i < VX_NKEYS; i++) {
	if (vx->vx_keyprocs[i] == u.u_procp)
	    fc = vxunkey(vx, i);
    }

    if ((vp = vxgetproc(dev)) == NULL)
	return(ENXIO);

/* XXX - if this is called by segfree it will unlock ALL nodes associated with
 * the freeing process.  We may only want to unlock a particular node!
 * segfree really needs its own clean up code, so we only unlock one at a time
 */
    for (i=0; i<VX_MAXNODES; i++,vn++){
	if (VXISSET(vx->vx_ndmask, i)) {
	    if (VXISSET(vp->vp_lockmask, i)) {
		fc = vxunlock(vn,vp,i);
	    }

	    /* if there's an old memory segment still connected for DMA, release it */
	    ovms = &vn->vn_omseg;
	    if (ovms->vms_addr != NULL){
		fc = (int)as_fault(ovms->vms_pp->p_as,ovms->vms_addr,PAGESIZE,F_SOFTUNLOCK,S_WRITE);
		if (fc)
		    panic("vxfreethings");
		ovms->vms_addr = NULL;
		mbrelse(vxinfo[minor(dev)]->md_hd, &vn->vn_mbinfo);
	    }
	}
    }
    vp->vp_lockmask = 0;
    vp->vp_procp = 0;
    
    return 0;
}

static struct vx_procinfo *
vxgetproc(dev)
{
    struct vx_softc *vx = getsoftc(minor(dev));
    struct vx_procinfo *vp = vx->vx_procinfo;
    int i;
    
    for (i = 0; i < VX_MAXPROCS; i++, vp++) {
	if (vp->vp_procp == u.u_procp)
	    return(vp);
    }
    return(NULL);
}

/*ARGSUSED*/
static
cgvxioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct vx_softc *softc = getsoftc(unit);
	int 		cursor_cmap;
	int		(*copyfun)();
	extern int	segvx_graballoc();
	extern int	segvx_grabattach();
	extern int	segvx_grabfree();
	extern int	segvx_grabinfo();

	static union {
		u_char cmap[3*CG6_CMAP_ENTRIES];
		u_long cur[32];
	} iobuf;
	register  struct thc *thc;
	register unsigned int *cfg, *asic, *pac, *video;

DEBUGprint (0x1, "cgvxioctl %x\n", cmd, 0); 

	/* default to updating normal colormap */
	cursor_cmap = 0;

	/* figure out if kernel or user mode call */
	copyfun = fbio_kcopy ? kcopy : copyin;

	switch (cmd) {

	case FBIOPUTCMAP:
	case FBIOGETCMAP: 
	{
	    struct fbcmap *cmap;
	    u_int index;
	    u_int count;
	    u_char *map;
#ifdef notdef
	    u_int entries;
#endif
	    int	error;
	    register u_int *colorlut = (u_int *)softc->va[MAP_CMAP];
	    static void cgvx_update_cmap();

cmap_ioctl:
	    cmap = (struct fbcmap *) data;
	    index = (u_int) cmap->index;
	    count = (u_int) cmap->count;

DEBUGprint (0x4, "cgvxioctl FBIOP/GCMAP idx=%d cnt=%d\n", index, count);

            if (cmd == FBIOPUTCMAP) {

                if (cgvx_intr_pending(softc)) {
                        if (!servicing_interrupt())
                            synctovrt(softc);
                        vx_vrtint_disable(softc);
                }

                if (cursor_cmap == 0)
                        switch (PIX_ATTRGROUP(index)) {
                        case 0:
                        case PIXPG_8BIT_COLOR:
                                map = softc->cmap_rgb;
#ifdef notdef
                                entries = CG6_CMAP_ENTRIES;
#endif
                                break;
                        default:
                                return EINVAL;
                        }
                else {
                        map = softc->omap_rgb;
#ifdef notdef
                        entries = 2;
#endif
                }

		do {
		    if (error = COPY(copyfun, cmap->red, iobuf.cmap, count))
                        break;
		    CG6BCOPY(iobuf.cmap, map, count);

		    if (error = COPY(copyfun, cmap->green, iobuf.cmap, count))
                        break;
		    CG6BCOPY(iobuf.cmap, map+count, count);

		    if (error = COPY(copyfun, cmap->blue, iobuf.cmap, count))
                        break;
		    CG6BCOPY(iobuf.cmap, map+2*count, count);
		} while(_ZERO_);

                if (error) {
                    if (cgvx_intr_pending(softc))
                            vx_vrtint_enable(softc);
                    return EFAULT;
                }

                if (cursor_cmap)
                    count = 0;

                cgvx_update_cmap(softc, index, count);
                vx_vrtint_enable(softc);

            } else {
                cgvxCMap_get (0, index, count, iobuf.cmap, colorlut);
                (void)copyout ((caddr_t)iobuf.cmap, (caddr_t)cmap->red, count);
                cgvxCMap_get (1, index, count, iobuf.cmap, colorlut);
                (void)copyout ((caddr_t)iobuf.cmap, (caddr_t)cmap->green, count);
                cgvxCMap_get (2, index, count, iobuf.cmap, colorlut);
                (void)copyout ((caddr_t)iobuf.cmap, (caddr_t)cmap->blue, count);
            }
	}
	break;

	case FBIOSATTR: 
	{
		register struct fbsattr *sattr = (struct fbsattr *) data;

#ifdef ONLY_OWNER_CAN_SATTR
		/* this can only happen for the owner */
		if (softc->owner != u.u_procp)
			return (ENOTTY);
#endif ONLY_OWNER_CAN_SATTR

		softc->gattr.sattr.flags = sattr->flags;

		if (sattr->emu_type != -1)
			softc->gattr.sattr.emu_type = sattr->emu_type;

		if (sattr->flags & FB_ATTR_DEVSPECIFIC) {
			bcopy((char *) sattr->dev_specific,
				(char *) softc->gattr.sattr.dev_specific,
				sizeof (sattr->dev_specific));

			if (softc->gattr.sattr.dev_specific
				[FB_ATTR_CG6_SETOWNER_CMD] == 1) {
				struct proc *pfind();
				register struct proc *newowner = 0;

				if (softc->gattr.sattr.dev_specific
					[FB_ATTR_CG6_SETOWNER_PID] > 0 &&
					(newowner = pfind(
					softc->gattr.sattr.dev_specific
						[FB_ATTR_CG6_SETOWNER_PID]))) {
					softc->owner = newowner;
					softc->gattr.owner = newowner->p_pid;
				}

				softc->gattr.sattr.dev_specific
					[FB_ATTR_CG6_SETOWNER_CMD] = 0;
				softc->gattr.sattr.dev_specific
					[FB_ATTR_CG6_SETOWNER_PID] = 0;

				if (!newowner)
					return (ESRCH);
			}
			
		}

	}
	break;

	case FBIOGATTR: 
	{
	    register struct fbgattr *gattr = (struct fbgattr *) data;


	    /* set owner if not owned or previous owner is dead */
	    if (softc->owner == NULL ||
		softc->owner->p_stat == NULL ||
		softc->owner->p_pid != softc->gattr.owner) {

		    softc->owner = u.u_procp;
		    softc->gattr.owner = u.u_procp->p_pid;
	    }

	    *gattr = softc->gattr;

	    if (u.u_procp == softc->owner) {
		gattr->owner = 0;
	    }
	}
	break;

	case FBIOGTYPE: 
	{
	    register struct fbtype *fb = (struct fbtype *) data;

	    /* set owner if not owned or previous owner is dead */
	    if (softc->owner == NULL ||
		softc->owner->p_stat == NULL ||
		softc->owner->p_pid != softc->gattr.owner) {

		    softc->owner = u.u_procp;
		    softc->gattr.owner = u.u_procp->p_pid;
	    }

	    switch (softc->gattr.sattr.emu_type) {
	    case FBTYPE_SUN4COLOR:
		*fb = vxtypedefault;
		fb->fb_height = softc->h;
		fb->fb_width  = softc->w;
		fb->fb_size = softc->size;
		break;

	    case FBTYPE_SUNFAST_COLOR:
	    default:
		*fb = softc->gattr.fbtype;
		break;
	    }
	}
	break;

#if NWIN > 0

	case FBIOGPIXRECT: 
	{
	    ((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

	    softc->pr.pr_data = (caddr_t) &softc->prd;

	    /* initialize pixrect */

	    softc->pr.pr_ops = &cgvx_ops;
	    softc->pr.pr_size.x = softc->w;
	    softc->pr.pr_size.y = softc->h;
	    softc->pr.pr_depth = CG6_DEPTH;

	    /* initialize private data */

	    softc->prd.mpr.md_linebytes = mpr_linebytes(softc->w, CG6_DEPTH);
	    softc->prd.mpr.md_image = (short *) (softc->va[MAP_COLOR]);
	    softc->prd.mpr.md_offset.x = 0;
	    softc->prd.mpr.md_offset.y = 0;

/* Need unit number added to 'prd'!!! */
	    softc->prd.mpr.md_primary = unit;

	    softc->prd.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
	    softc->prd.planes = 255;

	    /* enable video */
/*
	    setvideoenable(1); /* does this do anything for cg6 ? */

	    cgvx_videnable(softc, 1);
	}
	break;

#endif NWIN > 0

	/* set video flags */
	case FBIOSVIDEO: 
	{
	    register int *nvideo = (int *) data;
            unsigned int *dac = (unsigned int*) (softc->va[MAP_CMAP]);
            /*
             * VX video never really goes "off".  This just switches the
             * RAMDACs to display to/from "overlay color 0" or the color
             * palette.  This requires that "overlay color 0" has previously
             * been set to black!!
             */ 
            if (*nvideo & FBVIDEO_ON) {
                *(dac+VX_BTCR0) = 0x505050;
                softc->cur.video_blanking = 0;	/* Enable video */
                softc->cur.enable = 1;		/* Turn on cursor */
                cgvx_setcurpos(softc);		/* Do it */
            }
            else {
                *(dac+VX_BTCR0) = 0x707070;
                softc->cur.enable = 0;		/* Turn off cursor */
                cgvx_setcurpos(softc);		/* Do it */
                softc->cur.video_blanking = 1;	/* Then disable video */
            }
	}
	break;

	/* get video flags */
	case FBIOGVIDEO: 
	{
            unsigned int *dac = (unsigned int*) (softc->va[MAP_CMAP]);
            *(int *)data = (*(dac+VX_BTCR0)==0x505050) ? FBVIDEO_ON : FBVIDEO_OFF;
	}
	break;

	case GRABPAGEALLOC: 	/* Window Grabber */
		return (segvx_graballoc(data, softc));
	case GRABATTACH:	/* Window Grabber */
		return (segvx_grabattach(data));
	case GRABPAGEFREE:	/* Window Grabber */
		return (segvx_grabfree(data, softc));
        case GRABLOCKINFO:      /* Window Grabber */
                return (segvx_grabinfo(data));

	/* HW cursor control */
	case FBIOSCURSOR: {
		struct fbcursor *cp = (struct fbcursor *) data;
		int set = cp->set;
		int cbytes;

		if (set & FB_CUR_SETCUR)
			softc->cur.enable = cp->enable;

		if (set & FB_CUR_SETPOS)
			softc->cur.pos = cp->pos;

		if (set & FB_CUR_SETHOT)
			softc->cur.hot = cp->hot;

		/* update hardware */
		cgvx_setcurpos(softc);

		if (set & FB_CUR_SETSHAPE) {
		    if ((u_int) cp->size.x > 32 || (u_int) cp->size.y > 32)
			    return EINVAL;
		    softc->cur.size = cp->size;

		    /* compute cursor bitmap bytes */
		    cbytes = softc->cur.size.y * sizeof softc->cur.image[0];

		    /* copy cursor image into softc */
		    if (COPY(copyfun, cp->image, iobuf.cur, cbytes))
			    return EFAULT;
		    BZERO(softc->cur.image, sizeof softc->cur.image);
		    CG6BCOPY(iobuf.cur, softc->cur.image, cbytes);
		    if (COPY(copyfun, cp->mask, iobuf.cur, cbytes))
			    return EFAULT;
		    BZERO(softc->cur.mask, sizeof softc->cur.mask);
		    CG6BCOPY(iobuf.cur, softc->cur.mask, cbytes);

		    /* load into hardware */
		    cgvx_setcurshape(softc);
		}

		/* load colormap */
		if (set & FB_CUR_SETCMAP) {
		    cursor_cmap = 1;
		    cmd = FBIOPUTCMAP;
		    data = (caddr_t) &cp->cmap;
		    goto cmap_ioctl;
		}

		break;
	}

	case FBIOGCURSOR: 
	{
	    struct fbcursor *cp = (struct fbcursor *) data;
	    int cbytes;

	    cp->set = 0;
	    cp->enable = softc->cur.enable;
	    cp->pos = softc->cur.pos;
	    cp->hot = softc->cur.hot;
	    cp->size = softc->cur.size;

	    /* compute cursor bitmap bytes */
	    cbytes = softc->cur.size.y * sizeof softc->cur.image[0];

	    /* if image pointer is non-null copy both bitmaps */
	    if (cp->image &&
		    (COPYOUT(softc->cur.image, cp->image, cbytes) ||
		    COPYOUT(softc->cur.mask, cp->mask, cbytes)))
		return EFAULT;

	    /* if red pointer is non-null, copy colormap */
	    if (cp->cmap.red) {
		cursor_cmap = 1;
		cmd = FBIOGETCMAP;
		data = (caddr_t) &cp->cmap;
		goto cmap_ioctl;
	    } else { /* just trying to find out colormap size */
		cp->cmap.index = 0;
		cp->cmap.count = 2;
	    }
	    break;
	}

	case FBIOSCURPOS:
	    softc->cur.pos = * (struct fbcurpos *) data;
	    cgvx_setcurpos(softc);
	    break;

	case FBIOGCURPOS:
	    * (struct fbcurpos *) data = softc->cur.pos;
	    break;

	case FBIOGCURMAX: 
	{
	    static struct fbcurpos curmax = { 32, 32 };

	    * (struct fbcurpos *) data = curmax;
	    break;
	}

	case FBRESET:
	{

DEBUGprint (0x2, "vxioctl RESET\n", 0, 0);
	    thc = (struct thc*) (softc->va[MAP_FHCTHC]+VX_THC_POFF);
	    cfg = (unsigned int *) (softc->va[MAP_FHCTHC]);
	    video = (unsigned int *) (softc->va[MAP_VIDCSR]);
	    pac = (unsigned int *) (softc->va[MAP_VIDPAC]);
	    asic = (unsigned int *) (softc->va[MAP_VIDMIX]);

	    softc->w = softc->gattr.fbtype.fb_width =
					videofmt[softc->videofmt].x;
	    softc->h = softc->gattr.fbtype.fb_height =
					videofmt[softc->videofmt].y;
	    softc->clk = videofmt[softc->videofmt].clk;
	    softc->hz = videofmt[softc->videofmt].hz;
	    softc->gattr.fbtype.fb_size = softc->w * softc->h;

	    DEBUGprint (2, "vxioctl: Init ASIC..\n",0,0); 
	    cgvxASIC(asic);
	    DEBUGprint (2, "FHCTHC..\n",0,0); 
	    cgvxFHCTHC(softc,cfg, thc);
	    DEBUGprint (2, "VIDEO..\n",0,0); 
	    cgvxPAC1000(softc,video,pac);
	    DEBUGprint (2, "FBRESET complete\n",0,0);
	    break;
	}

        case FBIOVERTICAL:

	    switch(*(int *)data) {

#ifdef notdef
	    case VX_VRTSTOP:
		softc->intr = 0;
                softc->owner = NULL;
                softc->gattr.owner = 0;
		vx_vrtint_disable(softc);
		break;

	    case VX_VRTSTART:
		softc->intr = VX_VRTSTART;
                softc->owner = u.u_procp;
                softc->gattr.owner = u.u_procp->p_pid;
		vx_vrtint_enable(softc);
		break;
#endif

	    case VX_VRTWAIT:
	    {
		int nx;

#ifdef notdef
                timeout(vxtout, softc, 2*hz);
#endif
                nx = splr(pritospl(softc->pri));
                softc->intr |= VX_VRTWAIT;
                vx_vrtint_enable(softc);
                (void)sleep((caddr_t)softc, PWAIT);
                (void)splx(nx);
	    }
		break;

	    default:
		return(EINVAL);
	    }
	    break;

        case FBVIDEOFMT:

	    if((*data >= VX_VIDFMT_1280_76Hz) && (*data <= VX_VIDFMT_USER)) {
		softc->videofmt = *data;
		softc->w = softc->gattr.fbtype.fb_width =
					    videofmt[softc->videofmt].x;
		softc->h = softc->gattr.fbtype.fb_height =
					    videofmt[softc->videofmt].y;
		softc->clk = videofmt[softc->videofmt].clk;
		softc->hz = videofmt[softc->videofmt].hz;
		softc->gattr.fbtype.fb_size = softc->w * softc->h;

            } else {
                printf ("ioctl: vx: FBVIDEOFMT illegal type\n");
                return (EINVAL);
            }
            break;

	default:
	    return (-1);

	} /* switch (cmd) */
	return (0);
}

#ifdef notdef
static
vxtout(arg)
    caddr_t   arg;
{
     vx_vrtint_enable(((struct vx_softc *)arg));
}
#endif

static
synctovrt(softc)
        struct vx_softc *softc;
{
    int nx;

    while(cgvx_intr_pending(softc)) {
#ifdef notdef
        timeout(vxtout, softc, hz/30);
#endif
        nx = splr(pritospl(softc->pri));
        softc->intr |= VX_VRTWAIT;
        vx_vrtint_enable(softc);
        (void)sleep((caddr_t)softc, PWAIT);
        (void)splx(nx);
    }
}




int
vxioctl(dev, cmd, data, flag)
    dev_t	dev;
    caddr_t	data;
    int		cmd, flag;
{
    struct vx_softc 	*vx = getsoftc(minor(dev));
    struct vx_nodeinfo	*vn;
    struct vx_procinfo	*vp;
    struct vx_parms	*vip;
    int			rval;
    int			vxunkey();
    int			fc;

    if ((rval = cgvxioctl(dev, cmd, data, flag)) != -1)
	return(rval);
    
    switch (cmd) {
    case VXNDMAP:
    case VXLOCK:
    case VXTEST:
    case VXALLOCKEY:
    case VXFREEKEY:
    case VXFREELOCK:
	break;
    default:
	vip = (struct vx_parms *)data;
    
	if (vip->vip_unit < 0 || vip->vip_unit > VX_MAXNODES)
	    return ENODEV;
	vn = (struct vx_nodeinfo *)&(vx->vx_nodeinfo[vip->vip_unit]);
	
	if ((vp = vxgetproc(dev)) == NULL)
	    return ENXIO;
	break;
    }
    
    switch (cmd){
    case VXTEST:
    {
        int x;
        struct  vx_vmectl *nvmectl = (struct vx_vmectl *)vx->va[MAP_VMECTL];

        x = splr(pritospl(vx->pri));
        vx->intr |= VX_VRTWAIT;
        vx_pgmint_enable(vx);
        nvmectl->intr = 0;
        (void)sleep((caddr_t)vx, PWAIT);
        (void)splx(x);
        return(0);
    }

    case VXENINTR:
#ifdef notdef
	printf("enabling interrupt for node=%x u = %x\n",vip->vip_unit,u.u_procp->p_pid);
#endif
	vx_pgmint_enable(vx);
	VXSET(vp->vp_intrmask, vip->vip_unit);
	break;
	
    case VXLOCK: 
    {
	struct vx_procinfo *nvp;
	int i;

	vip = (struct vx_parms *)data;
	
	if (vip->vip_unit < 0 || vip->vip_unit > VX_MAXNODES)
	    return(ENODEV);

	if (VXISSET(vx->vx_ndmask, vip->vip_unit) == 0)
	    return(ENODEV);

	vn = (struct vx_nodeinfo *)&(vx->vx_nodeinfo[vip->vip_unit]);

	if ((nvp = vxgetproc(dev)) == NULL) {

	    nvp = vx->vx_procinfo;
	    for (i = 0; i < VX_MAXPROCS; i++, nvp++){
		if (nvp->vp_procp == NULL)
		    break;
	    }
	
	    if (i >= VX_MAXPROCS)
		return ENXIO;

	    nvp->vp_procp = u.u_procp;
	    nvp->vp_lockmask = 0;
	    vx->vx_dev = dev;
	}

	return(vxlock(vn, nvp, vip->vip_unit));
    }
	
    case VXUNLOCK:
	return(vxunlock(vn, vp, vip->vip_unit));

    case VXFREELOCK:
    {
	int	n, p, ndmask;

	ndmask = *(int *)data;
	vn = vx->vx_nodeinfo;
	for (n = 0; n < VX_MAXNODES; n++, vn++)
	    if (VXISSET(vx->vx_ndmask, n) && VXISSET(ndmask, n)) {
		vp = vx->vx_procinfo;
		for (p = 0; p < VX_MAXPROCS; p++, vp++) 
		    if (VXISSET(vp->vp_lockmask, n)) {
			VXCLEAR(vp->vp_lockmask, n);
			vn->vn_lockflag = 0;
			vn->vn_procp = NULL;
			break;
		    }
	    }
	break;
    }
	
    case VXEXPORTSEG:
	vn->vn_mseg.vms_addr = vip->vip_addr;
	vn->vn_mseg.vms_size = vip->vip_size;
	vn->vn_mseg.vms_pp = u.u_procp;
	
	vn->vn_omseg.vms_addr = NULL;
	vn->vn_omseg.vms_pp = u.u_procp;
	break;
   
    case VXUNEXPORTSEG:
	if (vn->vn_omseg.vms_addr != NULL){
	    fc = (int)as_fault(vn->vn_omseg.vms_pp->p_as,vn->vn_omseg.vms_addr,PAGESIZE,F_SOFTUNLOCK,S_WRITE);
	    if (fc)
		panic("vxioctl: fault");
	    vn->vn_omseg.vms_addr = NULL;
	    mbrelse(vxinfo[minor(dev)]->md_hd, &vn->vn_mbinfo);
	}
	break;

    case VXNDMAP:
	{
	    int *tmp=(int*)data;
	    *tmp = vx->vx_ndmask;
	}
	break;
    case VXALLOCKEY:
	{
	    int	key = *(int *)data;

	    if (key < 0 || key >= VX_NKEYS)
		return(ENODEV);

	    if (vx->vx_keyprocs[key] == NULL) {
		vx->vx_keyprocs[key] = u.u_procp;
		return(0);
	    }
	    return(EBUSY);
	}
    case VXFREEKEY:
	{
	    int	key = *(int *)data;

	    return(vxunkey(vx, key));
	}

    default:
	return(-1);

    }
    
    return (0);
} 

int
vxunkey(vx, key)
    struct	vx_softc	*vx;
    int				key;
{
    struct cgvx_vidmix *vidmix = 
	(struct cgvx_vidmix *)vx->va[MAP_VIDMIX];

    if (key < 0 || key >= VX_NKEYS)
	return(ENODEV);

    if (vx->vx_keyprocs[key] != u.u_procp)
	return(EPERM);

    vx->vx_keyprocs[key] = NULL;
    vidmix->vidmixcsr &= ~(0x01010101 << key);
    return(0);
}

vxlock(vn, vp, unit)
    struct vx_nodeinfo *vn; 
    struct vx_procinfo *vp;
{
    if (vn->vn_lockflag)
	return(EBUSY);
    
    vn->vn_lockflag++;
    vn->vn_procp = u.u_procp;
    VXSET(vp->vp_lockmask, unit);
    return 0;
}


int
vxunlock(vn, vp, unit)
    struct vx_nodeinfo	*vn; 
    struct vx_procinfo	*vp;
    int			unit;
{
    if (vn->vn_procp != u.u_procp)
	return(EPERM);
    if (!vn->vn_lockflag)
	return(0);
    
    VXCLEAR(vp->vp_lockmask, unit);
    vn->vn_lockflag = 0;
    vn->vn_procp = NULL;
    
    return 0;
}


/*ARGSUSED*/
vxread(dev,uio)
    dev_t dev; struct uio *uio;
{
    return(0);
}


/*ARGSUSED*/
vxwrite(dev,uio)
    dev_t dev; struct uio *uio;
{
    return(0);
}


#if NWIN > 0

/*
 * SunWindows specific stuff
 */

/*
 * Mem_rop which waits for idle FBC
 * If we do not wait, it seems that
 * bus timeout errors occur.
 * The wait is limited to 50 millisecs.
 */

static cg6_rop_wait = 50;

static
cgvx_mem_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh, op;
	Pixrect *spr;
	int sx, sy;
{
	Pixrect mpr;
	int unit;

/* Parallax tvone fix
/*	if (spr && spr->pr_ops == &cg6_ops) {*/
	/* FIXME: bugginess here... */
        if (spr && spr->pr_ops->pro_rop == cgvx_mem_rop) {
/* end Parallax tvone fix
 */
		unit = mpr_d(spr)->md_primary;
		mpr = *spr;
		mpr.pr_ops = &mem_ops;
		spr = &mpr;
	} else
		unit = mpr_d(dpr)->md_primary;

	if (cg6_rop_wait) {
		register u_int *statp =&((struct fbc *)
			vx_softc[unit].va[MAP_FBCTEC])->l_fbc_status;
		CDELAY(!(*statp & L_FBC_BUSY), cg6_rop_wait << 10);
	}
	return (mem_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy));
}

static
cgvx_putcolormap(pr, index, count, red, green, blue)
    Pixrect	*pr;
    int		index, count;
    unsigned char *red, *green, *blue;
{
    register struct vx_softc *softc = getsoftc((mpr_d(pr)->md_primary));
    static void cgvx_update_cmap();

    register u_int rindex = (u_int) index;
    register u_int rcount = (u_int) count;
    register u_char *map;
    register u_int entries;

    map = softc->cmap_rgb;
    entries = CG6_CMAP_ENTRIES;

    /* check arguments */
    if (rindex >= entries || rindex + rcount > entries)  {
	    return (PIX_ERR);
    }

    if (rcount == 0)
	    return (0);

    /* lock out updates of the hardware colormap */
    if (cgvx_intr_pending(softc)) {
        if (!servicing_interrupt())
            synctovrt(softc);
        vx_vrtint_disable(softc);
    }

    CG6BCOPY(red, map, rcount);
    CG6BCOPY(green, map+rcount, rcount);
    CG6BCOPY(blue, map+2*rcount, rcount);

    cgvx_update_cmap(softc, rindex, rcount);

    /* enable interrupt so we can load the hardware colormap */
    vx_vrtint_enable(softc);

    return (0);
}

#endif	NWIN > 0

/*
 * enable/disable/update HW cursor
 */
static
cgvx_setcurpos(softc)
	struct vx_softc *softc;
{
    if (softc->cur.video_blanking == 0) {
	softc->thc->l_thc_cursor =
	    softc->cur.enable ?
		((softc->cur.pos.x - softc->cur.hot.x) << 16) |
		((softc->cur.pos.y - softc->cur.hot.y) & 0xffff) :
		CGVX_CURSOR_OFFPOS;
    }
}

/*
 * load HW cursor bitmaps
 */
static
cgvx_setcurshape(softc)
	struct vx_softc *softc;
{
	u_long tmp, edge = 0;
	u_long *image, *mask, *hw;
	int i;

	/* compute right edge mask */
	if (softc->cur.size.x)
		edge = (u_long) ~0 << (32 - softc->cur.size.x);

	image = softc->cur.image;
	mask = softc->cur.mask;
	hw = (u_long *) &softc->thc->l_thc_cursora00;

	for (i = 0; i < 32; i++) {
		hw[i] = (tmp = mask[i] & edge);
		hw[i + 32] = tmp & image[i];
	}
}

cgvxintr(softc)
	register struct vx_softc *softc;
{
	u_int		*cmap = (u_int *)softc->va[MAP_CMAP];
	u_int		*omap = ((u_int *)softc->va[MAP_CMAP]) + VX_BTOVERLAY;
	struct	fbc     *fbc = (struct fbc *)softc->va[MAP_FBCTEC];
	char	*somap =  (char *)softc->omap_image.omap_char;

#ifdef notdef
	untimeout(vxtout, softc);
#endif

	/* load cursor color map */
	if (softc->omap_update) {
	    VXDELAY ((fbc->l_fbc_status & L_FBC_BUSY), 100);
	    *omap++ = 0;
	    *omap++ = (*somap | (*(somap+2)<<8) | (*(somap+4)<<16));
	    *omap++ = 0;
	    *omap++ = (*(somap+1) | (*(somap+3)<<8) | (*(somap+5)<<16));
	    softc->omap_update = 0;
	}

	/* load main color map */
	if (softc->cmap_count) {
	    VXDELAY ((fbc->l_fbc_status & L_FBC_BUSY), 100);
	    cgvxCMap_put(0, softc->cmap_index, softc->cmap_count, 
		softc->cmap_rgb, cmap);
	    softc->cmap_count = 0;
	}

	if (softc->intr & VX_VRTWAIT) {
	    softc->intr &= ~VX_VRTWAIT;
	    wakeup((caddr_t)softc);
	}

	vx_vrtint_disable(softc);
}

/*
 * Initialize a colormap: background = white, all others = black
 */
static void
cgvx_reset_cmap(cmap, entries)
register caddr_t cmap;
register u_int entries;
{
	bzero(cmap, entries * 3);
	*cmap++ = 255;
	*cmap++ = 255;
	*cmap   = 255;
}

/*
 * Compute color map update parameters: starting index and count.
 * If count is already nonzero, adjust values as necessary.
 * Zero count argument indicates cursor color map update desired.
 */
static void
cgvx_update_cmap(softc, index, count)
	struct vx_softc *softc;
	u_int index, count;
{
	u_int high, low;

	if (count == 0) {
		softc->omap_update = 1;
		return;
	}

	if (high = softc->cmap_count) {
		high += (low = softc->cmap_index);

		if (index < low)
			softc->cmap_index = low = index;

		if (index + count > high)
			high = index + count;

		softc->cmap_count = high - low;
	}
	else {
		softc->cmap_index = index;
		softc->cmap_count = count;
	}
}

/*
 * Copy colormap entries between red, green, or blue array and
 * interspersed rgb array.
 *
 * count > 0 : copy count bytes from buf to rgb
 * count < 0 : copy -count bytes from rgb to buf
 */
static void
cgvx_cmap_bcopy(bufp, rgb, count)
register u_char *bufp, *rgb;
u_int count;
{
	register int rcount = count;

	if (--rcount >= 0)
		do {
			*rgb = *bufp++;
			rgb += 3;
		} while (--rcount >= 0);
	else {
		rcount = -rcount - 2;
		do {
			*bufp++ = *rgb;
			rgb += 3;
		} while (--rcount >= 0);
	}
}

/*
 * Save fbc pointed to by fbc into fbc0
 * Have to do individual fields because
 * some registers can not be read (eg. read activated).
 */
static
void
savefbc(fbc0, fbc, dir)
	struct fbc *fbc0, *fbc;
{
	if (fbc == 0 || fbc0 == 0) {
		return;
	}

	/*
	 * Wait until the cg6 is idle.
	 * If not, may get a bus timeout error
	 */
	if (dir == DIR_FROM_FBC) {
	       VXDELAY ( (fbc->l_fbc_status & L_FBC_BUSY), 200);
	} else {
	       VXDELAY ( (fbc0->l_fbc_status & L_FBC_BUSY), 200);
	}

	ShowIt (fbc0->l_fbc_misc, fbc->l_fbc_misc);
	ShowIt (fbc0->l_fbc_rasteroffx, fbc->l_fbc_rasteroffx);
	ShowIt (fbc0->l_fbc_rasteroffy, fbc->l_fbc_rasteroffy);
	ShowIt (fbc0->l_fbc_rasterop, fbc->l_fbc_rasterop);
	ShowIt (fbc0->l_fbc_pixelmask, fbc->l_fbc_pixelmask);
	ShowIt (fbc0->l_fbc_planemask, fbc->l_fbc_planemask);
	ShowIt (fbc0->l_fbc_fcolor, fbc->l_fbc_fcolor);
	ShowIt (fbc0->l_fbc_bcolor, fbc->l_fbc_bcolor);
	ShowIt (fbc0->l_fbc_clipminx, fbc->l_fbc_clipminx);
	ShowIt (fbc0->l_fbc_clipminy, fbc->l_fbc_clipminy);
	ShowIt (fbc0->l_fbc_clipmaxx, fbc->l_fbc_clipmaxx);
	ShowIt (fbc0->l_fbc_clipmaxy, fbc->l_fbc_clipmaxy);
	ShowIt (fbc0->l_fbc_autoincx, fbc->l_fbc_autoincx);
	ShowIt (fbc0->l_fbc_autoincy, fbc->l_fbc_autoincy);
	ShowIt (fbc0->l_fbc_clipcheck, fbc->l_fbc_clipcheck);
}

/*
 * Enable cg6 video if on non-zero,
 * else disable.
 */
cgvx_videnable(softc, on)
	struct vx_softc *softc;
	u_int on;
{
	register struct thc *thc;

	thc = (struct thc*) (softc->va[MAP_FHCTHC] + CG6_THC_POFF);

	if (on)
	    thc->l_thc_hcmisc	= 0x0004cf;
	else
	    thc->l_thc_hcmisc	= 0x0000cf;
}

/*ARGSUSED*/
static
void
cgvxCMap_put (cmd, index, count, cmap, colormap)
    int cmd;
    unsigned int count, index, *colormap;
    u_char *cmap;
{
    register int i, ii;

    if(count+index <= 256) {
	for (i=0, ii=index; i<count; i++, ii++) {
	    colormap[ii] = (u_int)
	      ( *(cmap+i) | (*(cmap+count+i)<<8) | (*(cmap+2*count+i)<<16) );
	}
    }
}

static
void
cgvxCMap_get (cmd, index, count, cmap, colormap)
	int cmd;
	unsigned int count, index, *colormap;
	u_char *cmap;
{
    register int i, ii;

    if(count+index <= 256) {
	for (i=0, ii=index; i<count; i++, ii++) {
	    switch (cmd) {
	    case 0:
		*(cmap+i) = (u_char)(  colormap[ii] & 0x0000ff	);
		break;
	    case 1:
		*(cmap+i) = (u_char)( (colormap[ii] & 0x00ff00) >> 8);
		break;
	    case 2:
		*(cmap+i) = (u_char)( (colormap[ii] & 0xff0000) >> 16);
		break;
	    }
	}
    }
}

#ifdef notdef
static
void
cgvxColors_put (index, count, red, green, blue, colormap)
    unsigned int count, index, *colormap;
    u_char *red, *green, *blue;
{
    register int i, ii, error;

    if(count+index <= 256) {
	for (i=0, ii=index; i<count; i++, ii++) {
	    colormap[ii] = (unsigned int)
		( *(red+i) | (*(green+i)<<8) | (*(blue+i)<<16) );
	}
    }
}

static
void
cgvxColors_get (index, count, red, green, blue, colormap)
    unsigned int count, index, *colormap;
    u_char *red, *green, *blue;
{
    register int i, ii, error;

    if(count+index <= 256) {
	for (i=0, ii=index; i<count; i++, ii++) {
	    *(red+i)   = (u_char)( (colormap[ii] & 0x0000ff)		);
	    *(green+i) = (u_char)( (colormap[ii] & 0x00ff00) >> 8	);
	    *(blue+i)  = (u_char)( (colormap[ii] & 0xff0000) >> 16	);
	}
    }
}
#endif

/* Zero all registers first!!!!! */					
static
void
cgvxPAC1000 (softc, video, pac)
    struct vx_softc *softc;
    unsigned int *video, *pac;
{

    struct cgvx_vidctl4 *t =(struct cgvx_vidctl4 *)pac;
    *(video) &= 0xc0;	    /* stop the PAC video controller */
    *(video) |= (softc->clk<<1);	/* Change oscillator */
    *(video) |= 0x01;	 /* restart the PAC video controller */
    ShowIt(t->vc_h_syncsize	, cgvx_vidfmt4[softc->videofmt].vc_h_syncsize);
    ShowIt(t->vc_h_bporch	, cgvx_vidfmt4[softc->videofmt].vc_h_bporch);
    ShowIt(t->vc_h_active	, cgvx_vidfmt4[softc->videofmt].vc_h_active);
    ShowIt(t->vc_h_fporch	, cgvx_vidfmt4[softc->videofmt].vc_h_fporch);
    ShowIt(t->vc_csynclo	, cgvx_vidfmt4[softc->videofmt].vc_csynclo);
    ShowIt(t->vc_v_fporch	, cgvx_vidfmt4[softc->videofmt].vc_v_fporch);
    ShowIt(t->vc_v_sync		, cgvx_vidfmt4[softc->videofmt].vc_v_sync);
    ShowIt(t->vc_v_bporch	, cgvx_vidfmt4[softc->videofmt].vc_v_bporch);
    ShowIt(t->vc_v_active	, cgvx_vidfmt4[softc->videofmt].vc_v_active);
    ShowIt(t->vc_displaymode	, cgvx_vidfmt4[softc->videofmt].vc_displaymode);
    ShowIt(t->vc_casoffset	, cgvx_vidfmt4[softc->videofmt].vc_casoffset);
    ShowIt(t->vc_ras_tof	, cgvx_vidfmt4[softc->videofmt].vc_ras_tof);
    ShowIt(t->vc_cas_tof	, cgvx_vidfmt4[softc->videofmt].vc_cas_tof);
    ShowIt(t->vc_go		, 0xffff);
}

static
void
cgvxASIC(asic)
unsigned int *asic;
{
    ShowIt (*(asic + 0x09) , 0x0);
    ShowIt (*(asic + 0x00) , 0x0);
    ShowIt (*(asic + 0x01) , 0x0);
    ShowIt (*(asic + 0x02) , 0x0);
    ShowIt (*(asic + 0x03) , 0x0);
    ShowIt (*(asic + 0x04) , 0x0);
    ShowIt (*(asic + 0x05) , 0x0);
    ShowIt (*(asic + 0x06) , 0x0);
    ShowIt (*(asic + 0x07) , 0x0);
    ShowIt (*(asic + 0x08) , 0x0);
}


/*
 *    Initialize 8-bit colormap 0=white, 1-255=black
 *    Initialize 32-bit colormap to ramps
 */
static
void
cgvxRAMDAC (cmap)
unsigned int *cmap;
{
    int i;

    *(cmap + VX_BTCR0     ) = 0x505050;
    *(cmap + VX_BTCR1     ) = 0x000000;
    *(cmap + VX_BTCR2     ) = 0x080808;
    *(cmap + VX_BTRDMSKLO ) = 0xffffff;
    *(cmap + VX_BTRDMSKHI ) = 0x030303;
    *(cmap + VX_BTBKMSKLO ) = 0x000000;
    *(cmap + VX_BTBKMSKHI ) = 0x000000;
    *(cmap + VX_BTOVRDMASK) = 0x000000;
    *(cmap + VX_BTOVBKMASK) = 0x000000;

    *(cmap) = ColorFull(0xff);
    for (i=1; i<0x120; i++) *(cmap+i) = 0;
    for (i=0x400; i<0x500; i++) *(cmap+i) = ColorFull(i & 0xff);
    for (i=0x500; i<0x600; i++) *(cmap+i) = ColorRed (i & 0xff);
    for (i=0x600; i<0x700; i++) *(cmap+i) = ColorGren(i & 0xff);
    for (i=0x700; i<0x800; i++) *(cmap+i) = ColorBlue(i & 0xff);
}


static
void
cgvxFHCTHC (softc, cfg, thc)
struct vx_softc *softc;
unsigned int *cfg;
struct thc *thc;
{
    int i=softc->videofmt;
    *cfg = FBCRESET | MODE68 | cgvx_vid_data[i].HORZ;
    *cfg = MODE68 | cgvx_vid_data[i].HORZ;
    thc->l_thc_hchs	= cgvx_vid_data[i].hchs;
    thc->l_thc_hchsdvs 	= cgvx_vid_data[i].hchsdvs;
    thc->l_thc_hchd	= cgvx_vid_data[i].hchd;
    thc->l_thc_hcvs	=
	Upper(cgvx_vid_data[i].VFP+1) |
		    Lower(cgvx_vid_data[i].VSYN +
			    cgvx_vid_data[i].VFP);
    thc->l_thc_hcvd 	=
    Upper(cgvx_vid_data[i].VBP + cgvx_vid_data[i].VSYN + cgvx_vid_data[i].VFP)
	| Lower(cgvx_vid_data[i].VVIS + cgvx_vid_data[i].VBP +
			cgvx_vid_data[i].VSYN + cgvx_vid_data[i].VFP);
    thc->l_thc_hcr	= 363;
    i = thc->l_thc_hcmisc;
    thc->l_thc_hcmisc	= (i | THC_HCMISC_VIDEO |
	    THC_HCMISC_SYNCEN | THC_HCMISC_CURSOR_RES);
}

/*
 * from here on down, is the cgvx segment driver.  this virtualizes the
 * lego register file by associating a register save area with each
 * mapping of the cgvx device (each cgvx segment).  only one of these
 * mappings is valid at any time; a page fault on one of the invalid
 * mappings saves off the current cgvx context, invalidates the current
 * valid mapping, restores the former register contents appropriate to
 * the faulting mapping, and then validates it.
 *
 * this implements a graphical context switch that is transparent to the user.
 *
 * the TEC and FBC contain the interesting context registers.
 */

/*
 * many registers in the tec and fbc do
 * not need to be saved/restored.
 */
struct cgvx_cntxt {
	struct {
		u_int	mv;
		u_int	clip;
		u_int	vdc;
		u_int	data[64][2];
	} tec;
	struct {
		u_int	status;
		u_int	clipcheck;
		struct	l_fbc_misc	misc;
		u_int	x0, y0, x1, y1, x2, y2, x3, y3;
		u_int	rasteroffx, rasteroffy;
		u_int	autoincx, autoincy;
		u_int	clipminx, clipminy, clipmaxx, clipmaxy;
		u_int	fcolor, bcolor;
		struct	l_fbc_rasterop	rasterop;
		u_int	planemask, pixelmask;
		union	l_fbc_pattalign	 pattalign;
		u_int	pattern0, pattern1, pattern2, pattern3,
			pattern4, pattern5, pattern6, pattern7;
	} fbc;
};

struct segvx_data {
	int			flag;
	struct cgvx_cntxt	*cntxtp; 	/* pointer to context */
	struct	segvx_data	*link;		/* shared_list */
	dev_t			dev;
	u_int			offset;
};

struct  segvx_data     *vxshared_list;
struct  cgvx_cntxt       vxshared_context;

#define	segvx_crargs segvx_data

#define	SEG_CG6PRIV	(1)	/* cg6 private context */
#define	SEG_CG6SHARED	(2)	/* cg6 shared context */
#define	SEG_LOCK	(3)	/* lock page */
#define	SEG_VX		(4)	/* VX segment (whole vx) */
#define	SEG_MISC	(5)	/* other cg6/VX bits */


/*
 *      Window Grabber lock management
 */

#define	NLOCKS		16
#define	LOCKTIME	5	/* seconds */
static	int	locktime = LOCKTIME;

/* flag bits */
#define LOCKMAP         0x01
#define UNLOCKMAP       0x02
#define ATTACH          0x04
#define TRASHPAGE       0x08
#define UNGRAB          0x10    /* ungrab called */

struct  segproc {
	int	     flag;
	struct  proc    *procp;
	caddr_t	 lockaddr;
	struct  seg     *locksegp;
	caddr_t	 unlockaddr;
	struct  seg     *unlocksegp;
};

struct  seglock {
	short		sleepf;	/* sleep flag */
	short		allocf;	/* allocated flag */
	off_t		offset;	/* offset into device */
	int		*page;	/* virtual address of lock page */
	struct  segproc cr;     /* creator */
	struct  segproc cl;     /* client */
	struct  segproc *owner;	/* current owner of lock */
	struct  segproc *other;	/* not owner of lock */
};

caddr_t vxtrashpage;	      /* for trashing unwanted writes */

struct  seglock vxlock_list[NLOCKS];
struct  seglock *segvx_findlock();

/*
 * This routine is called through the cdevsw[] table to handle
 * the creation of cgvx and vx segments.
 */
/*ARGSUSED*/
int
vxsegmap(dev, offset, as, addr, len, prot, maxprot, flags, cred)
	dev_t dev;
	register u_int offset;
	struct as *as;
	addr_t *addr;
	register u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segvx_crargs dev_a;
	int	segvx_create();
	register	i;
	caddr_t		seglock;
	int		ctxflag = 0;
	int		vxflag = 0;

	register int unit = minor(dev);
	struct	vx_softc	*softc = getsoftc(unit);

	enum	as_res	as_unmap();
	int	as_map();
 
#ifndef _sys_mman_h
        int     old_mmap;
#endif
 
DEBUGprint (0x1, "vxsegmap \n", 0, 0); 

	/*
	 * we now support MAP_SHARED and MAP_PRIVATE:
	 *
	 *	MAP_SHARED means you get the shared context
	 *	which is the traditional mapping method.
	 *
	 *	MAP_PRIVATE means you get your very own
	 *	LEGO context.
	 *
	 *	Note that you can't get to here without
	 *	asking for one or the other, but not both.
	 */
 
#ifndef _sys_mman_h
        /*
         * See if this is an "old mmap call".  If so, remember this
         * fact and convert the flags value given to mmap to indicate
         * the specified address in the system call must be used.
         * _MAP_NEW is turned set by all new uses of mmap.
         */
        old_mmap = (flags & _MAP_NEW) == 0;
        if (old_mmap)
                flags |= MAP_FIXED;
        flags &= ~_MAP_NEW;
#endif

	/*
	 * Check to see if this is an allocated lock page.
	 */

	if ((seglock = 
		(caddr_t)segvx_findlock((off_t)offset, softc)) != 
		(caddr_t)NULL) {
DEBUGprint (0x1, "cgsixsegmap findlock=%x len=%d\n", seglock, len); 
		if (len != PAGESIZE)
			return (EINVAL);
	} else {
	    /*
	     * Check to insure that the entire range is
	     * legal and we are not trying to map in
	     * more than the device will let us.
	     */
	    for (i = 0; i < len; i += PAGESIZE) {
		if (cgvxmmap(dev, (off_t)offset + i, (int)maxprot) == -1) {
			return (ENXIO);
		}
		if ((offset+i >= (off_t)softc->addrs[MAP_FBCTEC].vaddr) &&
		    (offset+i < (off_t)softc->addrs[MAP_FBCTEC].vaddr+
		    softc->addrs[MAP_FBCTEC].size))
			ctxflag++;
	    }

	    /* check to see if this is the VX portion of the device */

	    if (offset >= (off_t)softc->addrs[MAP_VX].vaddr && 
		offset <= (off_t)softc->addrs[MAP_VX].vaddr + 
			  softc->addrs[MAP_VX].size)
		vxflag++;
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * Pick an address w/o worrying about
		 * any vac alignment contraints.
		 */
		map_addr(addr, len, (off_t)0, 0);
DEBUGprint (0x1, "cgsixsegmap map_addr=%x len=%d\n", *addr, len); 
		if (*addr == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
DEBUGprint (0x1, "cgsixsegmap as_unmap MAP_FIXED=%x len=%d\n", *addr, len); 
		 i = (int)as_unmap((struct as *)as, *addr, len);
	}

	/*
	 * record device number; mapping function doesn't do anything smart
	 * with it, but it takes it as an argument.  the offset is needed
	 * for mapping objects beginning some ways into them.
	 */

	dev_a.dev = dev;
	dev_a.offset = offset;
	dev_a.cntxtp = NULL;
	dev_a.link = NULL;

	/*
	 * determine whether this is a shared mapping, or a private
 	 * one. if shared, link it onto the shared list, if private,
	 * create a private LEGO context.
	 */
	if (seglock) {
	    dev_a.flag = SEG_LOCK;
	} else
	if (vxflag) {
	    dev_a.flag = SEG_VX;
	} else
	if (ctxflag) {
	    if (flags & MAP_SHARED) { /* shared mapping */
		struct segvx_crargs *a;
		a = vxshared_list;
		vxshared_list = &dev_a;
		dev_a.flag = SEG_CG6SHARED;
		dev_a.link = a;
		dev_a.cntxtp = &vxshared_context;
	    } else {		/* private mapping */
		dev_a.flag = SEG_CG6PRIV;
		dev_a.cntxtp = 
		    (struct cgvx_cntxt *)new_kmem_zalloc(
					    sizeof (struct cgvx_cntxt), 
					    KMEM_SLEEP); 
	    }
	} else
	    dev_a.flag = SEG_MISC;
	return (as_map((struct as *)as, *addr, len, segvx_create, (caddr_t)&dev_a));
}

static	int segvx_dup(/* seg, newsegp */);
static	int segvx_unmap(/* seg, addr, len */);
static	int segvx_free(/* seg */);
static	faultcode_t segvx_fault(/* seg, addr, len, type, rw */);
static	int segvx_checkprot(/* seg, addr, size, prot */);
static	int segvx_badop();
static  int segvx_incore();

struct	seg_ops segvx_ops =  {
	segvx_dup,	/* dup */
	segvx_unmap,	/* unmap */
	segvx_free,	/* free */
	segvx_fault,	/* fault */
	segvx_badop,	/* faulta */
	segvx_badop,	/* unload */
	segvx_badop,	/* setprot */
	segvx_checkprot, /* checkprot */
	segvx_badop,	/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	segvx_badop,	/* sync */
	segvx_incore	/* incore */
};

/*
 * Window Grabber Lock Page Management
 */

/*
 * search the vxlock_list list for the specified offset
 *
 */

/*ARGSUSED*/
struct seglock *
segvx_findlock(off, softc)
    off_t	off;
    struct	vx_softc *softc;
{
	register int i;

DEBUGprint (0x1, "segvx_findlock \n", 0, 0); 

	if ((unsigned long)off < (unsigned long)softc->lockbase)
		return ((struct seglock *)NULL);
	for (i = 0; i < NLOCKS; i++)
	    if  (off == vxlock_list[i].offset)
		return (vxlock_list + i);
	return ((struct seglock *)NULL);
}

/*ARGSUSED*/
int
segvx_grabattach(cookiep)      /* IOCTL */
caddr_t cookiep;
{
DEBUGprint (0x1, "segvx_grabattach \n", 0, 0); 

	return (EINVAL);
}

/*ARGSUSED*/
int
segvx_grabinfo(pp)    /* IOCTL */
caddr_t pp;
{
DEBUGprint (0x1, "segvx_grabinfo \n", 0, 0); 

        *(int *)pp = 0;
        return (0);
}

/*ARGSUSED*/
int
segvx_graballoc(pp, softc)    /* IOCTL */
	caddr_t pp;
	struct	vx_softc	*softc;
{
	register	int i;
	register	struct seglock	*lp = vxlock_list;

DEBUGprint (0x1, "segvx_graballoc \n", 0, 0); 

	if (vxtrashpage == NULL) {
	    vxtrashpage = getpages(1, KMEM_SLEEP);
	    for (i = 0; i < NLOCKS; i++)
		vxlock_list[i].offset = (off_t)(softc->lockbase+PAGESIZE*i);
	}

	for (i = 0; i < NLOCKS; i++, lp++)
	    if (lp->allocf == 0) {
		lp->allocf = 1;
		if (lp->page == (int *)NULL)
		    lp->page = (int *)getpages(1, KMEM_SLEEP);
		lp->cr.flag = 0;
		lp->cl.flag = 0;
		lp->cr.procp = u.u_procp;
		lp->cl.procp = NULL;
		lp->cr.locksegp = NULL;
		lp->cl.locksegp = NULL;
		lp->cr.unlocksegp = NULL;
		lp->cl.unlocksegp = NULL;
		lp->owner = NULL;
		lp->other = NULL;
		lp->sleepf = 0;
		*lp->page = 0;
		*(int *)pp = (int)lp->offset;
		return (0);
	    }
	return (ENOMEM);
}

/*ARGSUSED*/
int
segvx_grabfree(pp, softc)     /* IOCTL */
    caddr_t pp;
    struct vx_softc *softc;
{
    register    struct seglock  *lp;
    static	int	segvx_timeout();

DEBUGprint (0x1, "segvx_grabfree \n", 0, 0); 

    if ((lp = segvx_findlock((off_t)*(int *)pp, softc)) == NULL)
        return(EINVAL);
    lp->cr.flag |= UNGRAB;
    lp->allocf = 0;
    if (lp->sleepf) {   /* client will acquire the lock in this case */
	untimeout(segvx_timeout, (caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
    return(0);
}    

/*
 * Create a device segment.  the lego context is initialized by the create args.
 */
static
segvx_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segvx_data *dp;

DEBUGprint (0x1, "segvx_create \n", 0, 0); 

	dp = (struct segvx_data *)
		new_kmem_zalloc(sizeof (struct segvx_data), KMEM_SLEEP);
	*dp = *((struct segvx_crargs *)argsp);

	seg->s_ops = &segvx_ops;
	seg->s_data = (char *)dp;

	return (0);
}

/*
 * Duplicate seg and return new segment in newsegp. copy original lego context.
 */
static
segvx_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segvx_data *sdp = (struct segvx_data *)seg->s_data;
	register struct segvx_data *ndp;

DEBUGprint (0x1, "segvx_dup \n", 0, 0); 

	(void) segvx_create(newseg, (caddr_t)sdp);
	ndp = (struct segvx_data *)newseg->s_data;

	switch(sdp->flag) {
	case SEG_CG6PRIV:
	    ndp->cntxtp = (struct cgvx_cntxt *)
			new_kmem_zalloc(sizeof (struct cgvx_cntxt), KMEM_SLEEP);
	    *ndp->cntxtp = *sdp->cntxtp;
	    break;
	case SEG_CG6SHARED:
	{
	    struct segvx_crargs *a;

	    a = vxshared_list;
	    vxshared_list = ndp;
	    ndp->link = a;
	    ndp->cntxtp = &vxshared_context;
	    break;
	}
	case SEG_LOCK:
	case SEG_VX:
	case SEG_MISC:
	    break;
	}
	return (0);
}

static
segvx_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
        register struct segvx_data *sdp = (struct segvx_data *)seg->s_data;
	struct	vx_softc	*vx = getsoftc(minor(sdp->dev));
        register struct seg *nseg;
        addr_t nbase;
        u_int nsize;
	void	hat_unload();
	void	hat_newseg();
        static  segvx_lockfree();

DEBUGprint (0x1, "segvx_unmap \n", 0, 0); 

        segvx_lockfree(seg, vx);

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
		seg->s_base += len;
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
 		panic("segvx_unmap seg_alloc");
 
 	nseg->s_ops = seg->s_ops;
 
 	/* figure out what to do about the new context */
 
 	nseg->s_data = (caddr_t)
		new_kmem_alloc(sizeof (struct segvx_data),KMEM_SLEEP);
 
	switch(sdp->flag) {
	case SEG_CG6PRIV:
	     ((struct segvx_data *)nseg->s_data)->cntxtp =
                                 (struct cgvx_cntxt *)
                                 new_kmem_zalloc(sizeof (struct cgvx_cntxt),KMEM_SLEEP);
	     *((struct segvx_data *)nseg->s_data)->cntxtp = *sdp->cntxtp;

	case SEG_CG6SHARED:
	{
	     struct segvx_crargs *a;

	     a = vxshared_list;
	     vxshared_list = (struct segvx_data *)nseg->s_data;
	     ((struct segvx_data *)nseg->s_data)->link = a;
	}
	case SEG_LOCK:
	case SEG_MISC:
	case SEG_VX:
	    ((struct segvx_data *)nseg->s_data)->cntxtp = NULL;
	    break;
         }
 
 	/*
 	 * Now we do something so that all the translations which used
 	 * to be associated with seg but are now associated with nseg.
 	 */
 	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);
	return(0);
}

/*
 * free a lego segment and clean up its context.
 */
static
int
segvx_free(seg)
	struct seg *seg;
{
	register struct segvx_data *sdp = (struct segvx_data *)seg->s_data;
        register struct vx_softc *softc = getsoftc(minor(sdp->dev));

DEBUGprint (0x1, "segvx_free \n", 0, 0); 

	if (seg == softc->curcntxt) 
	    softc->curcntxt = NULL;

	switch(sdp->flag) {
	case SEG_CG6PRIV:
	    kmem_free((char *)sdp->cntxtp, sizeof (struct cgvx_cntxt));
	    break;
	case SEG_LOCK:
	    segvx_lockfree(seg, softc);
	    break;
	case SEG_VX:
	    (void)vxfreethings(sdp->dev);
	    break;
	case SEG_MISC:
	case SEG_CG6SHARED:
	    break;
	}
	kmem_free((char *)sdp, sizeof (*sdp));
	return(0);
}


#define	CG6MAP(sp, addr, page)	    \
	hat_devload(sp,		 \
		    addr,	       \
		    fbgetpage((caddr_t)page),    \
		    PROT_READ|PROT_WRITE|PROT_USER, 0)

#define	CG6UNMAP(segp)	  \
	hat_unload(segp,		\
		   (segp)->s_base,      \
		   (segp)->s_size);

/*ARGSUSED*/
static
segvx_lockfree(seg, softc)
    struct seg *seg;
    struct vx_softc *softc;
{
    register struct segvx_data *sdp = (struct segvx_data *)seg->s_data;
    struct      seglock *lp;
    static	int	segvx_timeout();
 
DEBUGprint (0x1, "segvx_lockfree \n", 0, 0); 

    if ((lp = segvx_findlock((off_t)sdp->offset, softc))==NULL)
        return;
    lp->cr.flag |= UNGRAB;
    if (lp->sleepf) {
	untimeout(segvx_timeout, (caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
}

static
int
segvx_timeout(lp)
struct	seglock *lp;
{
    struct  segproc	*np;
    void        hat_devload();

DEBUGprint (0x1, "segvx_timeout \n", 0, 0); 

    np = &lp->cr;
    if (np->flag & LOCKMAP) {
	CG6UNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->flag & UNLOCKMAP)
	CG6UNMAP(np->unlocksegp);
    CG6MAP(np->unlocksegp, np->unlockaddr, lp->page);
    np->flag |= UNLOCKMAP;
    np->flag &= ~TRASHPAGE;

    np = &lp->cl;
    if (np->locksegp && np->flag & LOCKMAP) {
	CG6UNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->unlocksegp) {
	if (np->flag & UNLOCKMAP)
	    CG6UNMAP(np->unlocksegp);
	CG6MAP(np->unlocksegp, np->unlockaddr, vxtrashpage);
	np->flag |= (UNLOCKMAP | TRASHPAGE);
    }
    wakeup((caddr_t)lp->page);
}

/*
 * Handle lock segment faults here...
 *
 */

static
int
segvx_lockfault(seg, addr, softc)
    register struct seg *seg;
    addr_t  addr;
    struct	vx_softc *softc;
{
	register struct seglock *lp;
	register struct segvx_data *current
		= (struct segvx_data *)seg->s_data;
	struct  segproc *sp;
	void	hat_devload();
	void	hat_unload();
	int	s;
	extern	int	hz;

DEBUGprint (0x1, "segvx_lockfault \n", 0, 0); 


	/* look up the segment in the vxlock_list */

	lp = segvx_findlock((off_t)current->offset, softc);
	if (lp == NULL)
	    return (FC_MAKE_ERR(EFAULT));

	s = splsoftclock();
	
        if (lp->cr.flag & UNGRAB) {
            CG6MAP(seg,addr,vxtrashpage);
	    (void)splx(s);
            return(0);
        }    

	/* initialization necessary? */

	if (lp->cr.procp == u.u_procp && lp->cr.locksegp == NULL) {
	    lp->cr.locksegp = seg;
	    lp->cr.lockaddr = addr;
	} else if (lp->cr.procp != u.u_procp && lp->cl.locksegp == NULL) {
	    lp->cl.procp = u.u_procp;
	    lp->cl.locksegp = seg;
	    lp->cl.lockaddr = addr;
	} else if (lp->cr.procp == u.u_procp && lp->cr.locksegp != seg) {
	    lp->cr.unlocksegp = seg;
	    lp->cr.unlockaddr = addr;
	} else if (lp->cl.procp == u.u_procp && lp->cl.locksegp != seg) {
	    lp->cl.unlocksegp = seg;
	    lp->cl.unlockaddr = addr;
	}

	if (*(int *)lp->page == 0) {
	    if (!lp->sleepf) {
		if (lp->owner == NULL) {
		    if (lp->cr.procp == u.u_procp) {
			lp->owner = &lp->cr;
			lp->other = &lp->cl;
		    } else {
			lp->owner = &lp->cl;
			lp->other = &lp->cr;
		    }
		}
		/* switch ownership? */
		if (lp->other->locksegp == seg) {
		    if (lp->owner->flag & LOCKMAP) {
			CG6UNMAP(lp->owner->locksegp);
			lp->owner->flag &= ~LOCKMAP;
		    }
		    if (lp->owner->unlocksegp != NULL)
			if (lp->owner->flag & UNLOCKMAP) {
			    CG6UNMAP(lp->owner->unlocksegp);
			    lp->owner->flag &= ~TRASHPAGE;
			    lp->owner->flag &= ~UNLOCKMAP;
			}
		    sp = lp->owner;
		    lp->owner = lp->other;
		    lp->other = sp;
		}
		/* map lock segment to page */
		CG6MAP(lp->owner->locksegp, lp->owner->lockaddr, lp->page);
		lp->owner->flag |= LOCKMAP;
		if (lp->owner->unlocksegp != NULL) {
		    if (lp->owner->flag & UNLOCKMAP) /***/
			CG6UNMAP(lp->owner->unlocksegp);
		    CG6MAP(lp->owner->unlocksegp,
			   lp->owner->unlockaddr, lp->page);
		    lp->owner->flag &= ~TRASHPAGE;
		    lp->owner->flag |= UNLOCKMAP;
		}
		(void)splx(s);
		return (0);
	    }
	    (void)splx(s);
	    return (0);
	}

	if (*(int *)lp->page == 1) {

	    if (lp->sleepf) {
		if (lp->owner->flag & UNLOCKMAP)	/***/
		    CG6UNMAP(lp->owner->unlocksegp);
		CG6MAP(lp->owner->unlocksegp, lp->owner->unlockaddr, vxtrashpage);
		lp->owner->flag |= TRASHPAGE;
		lp->owner->flag |= UNLOCKMAP;
		if (lp->owner->flag & LOCKMAP) {
		    CG6UNMAP(lp->owner->locksegp);
		    lp->owner->flag &= ~LOCKMAP;
		}
		untimeout(segvx_timeout, (caddr_t)lp);
		wakeup((caddr_t)lp->page);       /* wake up sleeper */
		(void)splx(s);
		return (0);
	    }

	    if (lp->owner->procp==u.u_procp && lp->owner->unlocksegp==seg) {
		if (lp->owner->flag & UNLOCKMAP)	/***/
		    CG6UNMAP(lp->owner->unlocksegp);
		CG6MAP(lp->owner->unlocksegp, lp->owner->unlockaddr, lp->page);
		lp->owner->flag &= ~TRASHPAGE;
		lp->owner->flag |= UNLOCKMAP;
		(void)splx(s);
		return (0);
	    }

	    if (lp->owner->flag & UNLOCKMAP) {
		CG6UNMAP(lp->owner->unlocksegp);
		lp->owner->flag &= ~TRASHPAGE;
		lp->owner->flag &= ~UNLOCKMAP;
	    }

	    lp->sleepf = 1;

	    if (lp->cr.procp == u.u_procp)	/* creator about to sleep */
		timeout(segvx_timeout, (caddr_t)lp, locktime*hz);

            (void)sleep((caddr_t)lp->page,PRIBIO);      /* goodnight, gracie */
                                        /* we wake up */
            lp->sleepf = 0;             /* clear sleepf */
            if (lp->cr.flag & UNGRAB) {
		CG6MAP(seg,addr,vxtrashpage);
		(void)splx(s);
                return(0);
            }    

	    sp = lp->owner;	     /* switch owner and other */
	    lp->owner = lp->other;
	    lp->other = sp;

	    /* map new owner to page */
	    CG6MAP(lp->owner->locksegp, lp->owner->lockaddr, lp->page);
	    lp->owner->flag |= LOCKMAP;

	    if (lp->owner->flag & TRASHPAGE) {
		CG6UNMAP(lp->owner->unlocksegp);
		lp->owner->flag &= ~TRASHPAGE;
		lp->owner->flag |= UNLOCKMAP;
		CG6MAP(lp->owner->unlocksegp, lp->owner->unlockaddr, lp->page);
	    }
	}
	(void)splx(s);
	return (0);
}

/*
 * Handle a fault on a lego segment.  The only tricky part is that only one
 * valid mapping at a time is allowed.  Whenever a new mapping is called for, the
 * current values of the TEC and FBC registers in the old context are saved away,
 * and the old values in the new context are restored.
 */

/*ARGSUSED*/
static
segvx_fault(seg, addr, len, type, rw)
	register struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register addr_t adr;
	register struct segvx_data *current = (struct segvx_data *)seg->s_data;
	register struct segvx_data *old;
	int pf;
        register struct vx_softc *softc = getsoftc(minor(current->dev));
	void    hat_devload();
	void    hat_unload();

DEBUGprint (0x1, "segvx_fault \n", 0, 0); 

	if (type != F_INVAL) {
		return (FC_MAKE_ERR(EFAULT));
	}

	switch(current->flag) {
        case SEG_CG6PRIV:       /* cg6 private context */
        case SEG_CG6SHARED:     /* cg6 shared context */
        case SEG_VX:            /* VX segment (whole vx) */
        case SEG_MISC:          /* other cg6/VX bits */
	    break;
        case SEG_LOCK:          /* lock page */
	    return (segvx_lockfault(seg, addr, softc));
	}

	if (current->cntxtp && softc->curcntxt != seg) {
		/*
		 * time to switch lego contexts.
		 */
		if (softc->curcntxt != NULL) {
			old = (struct segvx_data *)softc->curcntxt->s_data;
			/*
			 * wipe out old valid mapping.
			 */

			    hat_unload(softc->curcntxt,
				       softc->curcntxt->s_base,
				       softc->curcntxt->s_size);
			    /*
			     * switch hardware contexts
			     */

			    if (cgvx_cntxsave(
				    (struct fbc *)softc->va[MAP_FBCTEC],
				    (struct tec *)(softc->va[MAP_FBCTEC]+CG6_TEC_POFF),
				    old->cntxtp) == 0)
				return (FC_HWERR);
		    }

		    if (cgvx_cntxrestore(
			    (struct fbc *)softc->va[MAP_FBCTEC],
			    (struct tec *)(softc->va[MAP_FBCTEC]+CG6_TEC_POFF),
			    current->cntxtp) == 0)
			return (FC_HWERR);

		/*
		 * switch software "context"
		 */
		softc->curcntxt = seg;
	}
	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
		pf = cgvxmmap(current->dev, (off_t)current->offset+(adr-seg->s_base),
					PROT_READ|PROT_WRITE);
		if (pf == -1)
			return (FC_MAKE_ERR(EFAULT));

		hat_devload(seg, adr, pf, PROT_READ|PROT_WRITE|PROT_USER, 0 /* don't lock */);
	}
	return (0);
}

/*ARGSUSED*/
static
segvx_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
DEBUGprint (0x1, "segvx_checkprot \n", 0, 0); 

	return (PROT_READ|PROT_WRITE);
}


/*
 * segdev pages are always "in core".
 */
/*ARGSUSED*/
static 
segvx_incore(seg, addr, len, vec)
        struct seg *seg;
        addr_t addr;
        register u_int len;
        register char *vec;
{
        u_int v = 0;

DEBUGprint (0x1, "segvx_incore \n", 0, 0); 

        for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE,
            v += PAGESIZE)
                *vec++ = 1;
        return (v);
}


static
segvx_badop()
{
DEBUGprint (0x1, "segvx_badop \n", 0, 0); 

	/*
	 * silently fail.
	 */
	return (0);
}

#undef L_TEC_VDC_INTRNL0
#define	L_TEC_VDC_INTRNL0 0x8000
#undef L_TEC_VDC_INTRNL1
#define	L_TEC_VDC_INTRNL1 0xa000


cgvx_cntxsave(fbc, tec, saved)
	struct fbc	*fbc;
	struct tec	*tec;
	struct cgvx_cntxt	*saved;
{
	int	dreg;	/* counts through the data registers */
	u_int	*dp;	/* points to a tec data register */
	register spin;

DEBUGprint (0x1, "cgvx_cntxsave \n", 0, 0); 

	/*
	 * wait for fbc not busy; but not forever
	 */
	for (spin = 100000; fbc->l_fbc_status & L_FBC_BUSY && --spin > 0;)
		;
	if (spin == 0) {
		return (0);
	}

	/*
	 * start dumping stuff out.
	 */
	saved->fbc.status = fbc->l_fbc_status;
	saved->fbc.clipcheck = fbc->l_fbc_clipcheck;
	saved->fbc.misc = fbc->l_fbc_misc;
	saved->fbc.x0 = fbc->l_fbc_x0;
	saved->fbc.y0 = fbc->l_fbc_y0;
	saved->fbc.x1 = fbc->l_fbc_x1;
	saved->fbc.y1 = fbc->l_fbc_y1;
	saved->fbc.x2 = fbc->l_fbc_x2;
	saved->fbc.y2 = fbc->l_fbc_y2;
	saved->fbc.x3 = fbc->l_fbc_x3;
	saved->fbc.y3 = fbc->l_fbc_y3;
	saved->fbc.rasteroffx = fbc->l_fbc_rasteroffx;
	saved->fbc.rasteroffy = fbc->l_fbc_rasteroffy;
	saved->fbc.autoincx = fbc->l_fbc_autoincx;
	saved->fbc.autoincy = fbc->l_fbc_autoincy;
	saved->fbc.clipminx = fbc->l_fbc_clipminx;
	saved->fbc.clipminy = fbc->l_fbc_clipminy;
	saved->fbc.clipmaxx = fbc->l_fbc_clipmaxx;
	saved->fbc.clipmaxy = fbc->l_fbc_clipmaxy;
	saved->fbc.fcolor = fbc->l_fbc_fcolor;
	saved->fbc.bcolor = fbc->l_fbc_bcolor;
	saved->fbc.rasterop = fbc->l_fbc_rasterop;
	saved->fbc.planemask = fbc->l_fbc_planemask;
	saved->fbc.pixelmask = fbc->l_fbc_pixelmask;
	saved->fbc.pattalign = fbc->l_fbc_pattalign;
	saved->fbc.pattern0 = fbc->l_fbc_pattern0;
	saved->fbc.pattern1 = fbc->l_fbc_pattern1;
	saved->fbc.pattern2 = fbc->l_fbc_pattern2;
	saved->fbc.pattern3 = fbc->l_fbc_pattern3;
	saved->fbc.pattern4 = fbc->l_fbc_pattern4;
	saved->fbc.pattern5 = fbc->l_fbc_pattern5;
	saved->fbc.pattern6 = fbc->l_fbc_pattern6;
	saved->fbc.pattern7 = fbc->l_fbc_pattern7;

	/*
	 * the tec matrix and clipping registers are easy.
	 */
	saved->tec.mv = tec->l_tec_mv;
	saved->tec.clip = tec->l_tec_clip;
	saved->tec.vdc = tec->l_tec_vdc;

	/*
	 * the tec data registers are a little more non-obvious.
	 * internally, they are 36 bits.  what we see in the
	 * register file is a 32-bit window onto the underlying
	 * data register.  changing the data-type in the VDC
	 * gets us either of two parts of the data register.
	 * the internal format is opaque to us.
	 */
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL0;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		saved->tec.data[dreg][0] = *dp;
	}
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL1;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		saved->tec.data[dreg][1] = *dp;
	}
	return (1);
}

cgvx_cntxrestore(fbc, tec, saved)
	struct fbc	*fbc;
	struct tec	*tec;
	struct cgvx_cntxt	*saved;
{
	int	dreg;
	u_int	*dp;
	register spin;

DEBUGprint (0x1, "cgvx_cntxrestore \n", 0, 0); 

	/*
	 * wait for fbc not busy; but not forever
	 */
	for (spin = 100000; fbc->l_fbc_status & L_FBC_BUSY && --spin > 0;)
		;
	if (spin == 0) {
		return (0);
	}

	/*
	 * reload the tec data registers.  see above for
	 * "how do they get 36 bits in that itty-bitty int"
	 */
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL0;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		*dp = saved->tec.data[dreg][0];
	}
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL1;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		*dp = saved->tec.data[dreg][1];
	}

	/*
	 * the tec matrix and clipping registers are next.
	 */
	tec->l_tec_mv = saved->tec.mv;
	tec->l_tec_clip = saved->tec.clip;
	tec->l_tec_vdc = saved->tec.vdc;

	/*
	 * now the FBC vertex and address registers
	 */
	fbc->l_fbc_x0 = saved->fbc.x0;
	fbc->l_fbc_y0 = saved->fbc.y0;
	fbc->l_fbc_x1 = saved->fbc.x1;
	fbc->l_fbc_y1 = saved->fbc.y1;
	fbc->l_fbc_x2 = saved->fbc.x2;
	fbc->l_fbc_y2 = saved->fbc.y2;
	fbc->l_fbc_x3 = saved->fbc.x3;
	fbc->l_fbc_y3 = saved->fbc.y3;
	fbc->l_fbc_rasteroffx = saved->fbc.rasteroffx;
	fbc->l_fbc_rasteroffy = saved->fbc.rasteroffy;
	fbc->l_fbc_autoincx = saved->fbc.autoincx;
	fbc->l_fbc_autoincy = saved->fbc.autoincy;
	fbc->l_fbc_clipminx = saved->fbc.clipminx;
	fbc->l_fbc_clipminy = saved->fbc.clipminy;
	fbc->l_fbc_clipmaxx = saved->fbc.clipmaxx;
	fbc->l_fbc_clipmaxy = saved->fbc.clipmaxy;

	/*
	 * restoring the attribute registers
	 */
	fbc->l_fbc_fcolor = saved->fbc.fcolor;
	fbc->l_fbc_bcolor = saved->fbc.bcolor;
	fbc->l_fbc_rasterop = saved->fbc.rasterop;
	fbc->l_fbc_planemask = saved->fbc.planemask;
	fbc->l_fbc_pixelmask = saved->fbc.pixelmask;
	fbc->l_fbc_pattalign = saved->fbc.pattalign;
	fbc->l_fbc_pattern0 = saved->fbc.pattern0;
	fbc->l_fbc_pattern1 = saved->fbc.pattern1;
	fbc->l_fbc_pattern2 = saved->fbc.pattern2;
	fbc->l_fbc_pattern3 = saved->fbc.pattern3;
	fbc->l_fbc_pattern4 = saved->fbc.pattern4;
	fbc->l_fbc_pattern5 = saved->fbc.pattern5;
	fbc->l_fbc_pattern6 = saved->fbc.pattern6;
	fbc->l_fbc_pattern7 = saved->fbc.pattern7;

	fbc->l_fbc_clipcheck = saved->fbc.clipcheck;
	fbc->l_fbc_misc = saved->fbc.misc;

	/*
	 * lastly, let's restore the status
	 */
	fbc->l_fbc_status = saved->fbc.status;
	return (1);
}
