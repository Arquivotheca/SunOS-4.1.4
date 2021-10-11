#ifdef	SYSV
#pragma	ident	"@(#)tcx.c 1.1 94/10/31 SMI"
#else
#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)tcx.c 1.1 94/10/31 SMI";
#endif
#endif

/*
 * Copyright 1993 1994, Sun Microsystems, Inc.
 */

/*
 * A24 24-bit framebuffer.
 */

/*
 * A24 theory of operation:
 *
 * The A24 is driven by mapping framebuffer memory into process address
 * space and writing to it directly.  There are some control registers
 * that are also accessed by mapping into process address space, but
 * access to these registers are normally limited to the window system
 * only.
 *
 * The A24 is a (mostly) stateless framebuffer.  This means that
 * most operations are atomic to the extent that if a process accessing
 * the framebuffer is interrupted, and then resumes later after the
 * framebuffer state has been altered, there are no bad effects.
 *
 * Different operations on the A24 are achieved by referencing different
 * address spaces.  There is a 24-bit address space where 32-bit writes
 * reference individual 24-bit pixels.  There is an 8-bit address space
 * where 8-bit writes reference the red channel of individual pixels
 * (and 32-bit writes reference four pixels at a time.)
 *
 * In addition, there are special address spaces:  The "stipple" address
 * space allows a 64-bit control word to be written containing a color (24
 * or 8 bit) and a 32-bit pixel mask.  This allows 32 pixels to be written
 * in a single operation.  The address of the 64-bit write determines the
 * starting pixel for the operation.  The "blit" address space is similar
 * to the "stipple" address space, except that the 64-bit control word
 * specifies a source pixel address and a pixel mask rather than a color
 * and a pixel mask.
 *
 * In addition, there are "raw 32", "raw stipple" and "raw blit" address
 * spaces.  These are identical to the 24-bit, stipple and blit address
 * spaces, except that they also access the 2 control planes which determine
 * how underlying pixels are interpreted by the dacs (modes are: 8-bit
 * indexed, 24-bit indexed, 24-bit gamma-corrected and 24-bit raw).
 *
 * Virtual addresses and lengths for mmap() are defined in tcxreg.h.  Since
 * the framebuffer size is not engraved in stone, applications should
 * determine mmap() sizes by multiplying linebytes*height*depth.  Pixel depth
 * is as follows:
 *
 *	dfb8	1
 *	dfb24	4
 *	stip	8
 *	blit	8
 *	rdfb32	4
 *	rstip	8
 *	rblit	8
 *
 * linebytes is returned via the FBIOGXINFO ioctl.
 *
 * Registers used by the driver:
 *
 *	HCMISC		chiprev, reset, video enable, vblank, vbank interrupt
 *	LINECOUNT	current line counter
 *	CURSOR_ADDRESS	cursor address register
 *
 *
 *
 *
 * Note on softc locking:  Bug 1142190 requires that the softc struct
 * *not* be locked during copyin/copyout.  This is because copyin/copyout
 * could generate disk I/O to swap in the user address space.  If
 * a vertical retrace interrupt should occur during this I/O, the tcx
 * interrupt routine would block if softc were locked.  This could block
 * the disk I/O, causing system deadlock.
 */

#ifndef	SYSV
#include "win.h"
#endif	/* SYSV */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/stat.h>

#ifdef	SYSV
#include <sys/file.h>
#include <sys/kmem.h>
#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sysmacros.h>	/* for ctob(), used by cg8reg.h */
#else
#include <sundev/mbvar.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>
extern	int	nulldev() ;
#endif

#ifdef	SYSV
#include <sys/fbio.h>
#include <sys/visual_io.h>
#include <sys/tcxreg.h>
#include <sys/cg8reg.h>
#include <sys/cg3var.h>
#else
#include <sun/fbio.h>
#include <sundev/tcxreg.h>
#include <sbusdev/memfb.h>
#include <sundev/cg8reg.h>
#include <pixrect/cg3var.h>
#endif	SYSV



#ifndef	SYSV
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#endif	/* SYSV */

#ifndef	PIX_ATTRGROUP	/* TODO: where does this come from? */
#define	PIX_ATTRGROUP(attr)	((unsigned)(attr) >> 25)
#define	PIX_ALL_PLANES		0x00FFFFFF
#define	PIXPG_8BIT_COLOR	2
#endif


#ifndef	SYSV
/*
 * This flag indicates that we should use kcopy instead of copyin
 * for cursor ioctls.
 */
extern	int	fbio_kcopy;
extern	int	copyout(), copyin(), kcopy();
#endif	/* !SYSV */


/* configuration options */

#ifndef	TCXDEBUG
#define	TCXDEBUG 0
#endif

#ifndef	POWER_MGT		/* TODO: change this to SYSV */
#define	POWER_MGT
#else
#endif

#ifndef SYSV
#ifdef	POWER_MGT
#undef	POWER_MGT
#endif
#endif

#ifndef	NWIN
#define	NWIN	0
#endif

#if NOHWINIT
int tcx_hwinit = 1;
#endif NOHWINIT

#define	TCXUNIT(dev)	getminor(dev)

/*
 *  The Xterminal kernel is linked static (i.e. no
 *  loadable drivers. Therefore some driver entry
 *  points must be visable externally.
 */
#ifndef	LOADABLE
#define	STATIC
#else
#define	STATIC	static
#endif	LOADABLE


	/* stupid 4.x/5.x incompatibilities */

#ifdef	SYSV
typedef	_VOID		*malloc_t ;
#else
typedef	caddr_t		malloc_t ;
typedef	struct dev_info	dev_info_t ;
#define	volatile
#endif




	/* Debugging-related stuff starts here */

#ifdef	SYSV

static	void
tcx_printf(char *format, ...)
{
	va_list	ap;
	va_start(ap, format);
	vcmn_err(CE_CONT, format, ap) ;
	va_end(ap) ;
}

#else
#define	tcx_printf		printf
#endif


#if TCXDEBUG

static
tcx_assfail(line,ass)
	int	line ;
	char	*ass ;
{
	tcx_printf("tcx: line %d: assertion failed \"%s\"\n", line,ass);
}


#ifdef	SYSV

#define	ASRT(e)	do{if(!(e)) tcx_assfail(__LINE__,#e) ;}while(0)

#else

#define	ASRT(e)	do{if(!(e)) tcx_assfail(__LINE__,"e") ;}while(0)

#endif	/* SYSV */



	int	tcx_debug = TCXDEBUG ;
	int	tcx_mutex_debug = 0 ;
	int	tcx_mem_debug = 0 ;

	/* 0=no vrt retrace interrupts
	 * 1=only from ioctl
	 * 2=ioctl + vrtpage
	 * 3= + cmap sync
	 */

	/*
	 * 4= + mmap return values
	 * 5= + memory debug + mutex debug + colormap debug
	 * 6= + interrupt wait debug
	 * 7= + interrupt debug
	 */

#define	DEBUGF(level, args)	\
		do{if (tcx_debug >= (level)) tcx_printf args;}while(0)

#define	MuDEBUGF(level, args)	\
		do{if (tcx_mutex_debug >= (level)) tcx_printf args;}while(0)

#define	MDEBUGF(level, args)	\
		do{if (tcx_mem_debug >= (level)) tcx_printf args;}while(0)



	/* memory-leak testing */

static	int	total_memory = 0 ;

static	malloc_t
my_kmem_zalloc(size, flag, line)
	u_long	size, flag, line ;
{
	total_memory += size ;
	MDEBUGF(1, ("zalloc line %d: %d bytes, total=%d\n",
		line, size, total_memory));
	return kmem_zalloc(size,flag) ;
}

static	malloc_t
my_kmem_alloc(size, flag, line)
	u_long	size, flag, line ;
{
	total_memory += size ;
	MDEBUGF(1, ("alloc line %d: %d bytes, total=%d\n",
		line, size, total_memory));
	return kmem_alloc(size,flag) ;
}

static
my_kmem_free(ptr, size, line)
	malloc_t ptr ;
	size_t	size ;
	int	line ;
{
	total_memory -= size ;
	ASRT(total_memory >= 0) ;
	MDEBUGF(1, ("kmem_free line %d: %d bytes, total=%d\n",
		line, size, total_memory));
	kmem_free(ptr, size) ;
}

#define	tcx_kmem_zalloc(s,f)	my_kmem_zalloc(s,f,__LINE__)
#define	tcx_kmem_alloc(s,f)	my_kmem_alloc(s,f,__LINE__)
#define	tcx_kmem_free(p,s)	my_kmem_free(p,s,__LINE__)



/* mutex testing */

#ifdef	SYSV

static int last_enter;
static int last_exit;

debug_mutex_enter(mp, line)
	kmutex_t *mp ;
	int	line ;
{
	MuDEBUGF(1, ("entering mutex %x at %d\n", mp, line));

	if (MUTEX_HELD(mp) && mutex_owned(mp))
	  cmn_err(CE_WARN,
		"tcx: mutex %x already held at %d, last entered at %d\n",
		mp, line, last_enter);
	else
	  mutex_enter(mp);
	last_enter = line;
}

debug_mutex_exit(mp, line)
	kmutex_t *mp ;
	int	line ;
{
	MuDEBUGF(1, ("exiting mutex %x at %d\n", mp, line));
	if (mutex_owned(mp))
	    mutex_exit(mp);
	else
	    cmn_err(CE_WARN,
		"tcx: mutex %x not held at %d, last exited at %d\n",
		mp, line, last_exit);
	last_exit = line;
}

#define	tcx_mutex_assert(s)	ASRT(mutex_owned(&s->mutex))
#define	tcx_mutex_nassert(s)	ASRT(!mutex_owned(&s->mutex))
#define	tcx_mutex_enter(mp)	debug_mutex_enter(mp,__LINE__)
#define	tcx_mutex_exit(mp)	debug_mutex_exit(mp,__LINE__)

#endif	/* SYSV */

#else	/* !TCXDEBUG */

#define	DEBUGF(level, args)	/* nothing */
#define	MDEBUGF(level, args)	/* nothing */
#define	MuDEBUGF(level, args)	/* nothing */
#define	ASRT(e)			/* nothing */
#define	tcx_kmem_zalloc(s,f)	kmem_zalloc(s,f)
#define	tcx_kmem_alloc(s,f)	kmem_alloc(s,f)
#define	tcx_kmem_free(p,s)	kmem_free(p,s)

#ifdef	SYSV
#define	tcx_mutex_assert(s)	/* nothing */
#define	tcx_mutex_nassert(s)	/* nothing */
#define	tcx_mutex_enter		mutex_enter
#define	tcx_mutex_exit		mutex_exit
#endif

#endif	TCXDEBUG





	/* tcx definitions */

static	void	*statep = NULL ;	/* units */



/*
 * Table of TCX virtual addresses, sorted roughly by frequency of use.
 */
static	struct mapping {
	int	vaddr;		/* virtual (mmap offset) address */
	short	flags ;		/* flags */
#define	SUSER	1		  /* super-user only */
	short	idx ;		/* index into mappings */
} tcx_map[] = {
  { TCX_VADDR_DFB8,	0, TCX_REG_DFB8},
  { TCX_VADDR_DFB24,	0, TCX_REG_DFB24},
  { TCX_VADDR_STIP,	0, TCX_REG_STIP},
  { TCX_VADDR_BLIT,	0, TCX_REG_BLIT},
  { TCX_VADDR_RDFB32,	0, TCX_REG_RDFB32},
  { TCX_VADDR_RSTIP,	0, TCX_REG_RSTIP},
  { TCX_VADDR_RBLIT,	0, TCX_REG_RBLIT},
  { TCX_VADDR_TEC,	0, TCX_REG_TEC},
  { TCX_VADDR_CMAP,	SUSER, TCX_REG_CMAP},
  { TCX_VADDR_THC,	0, TCX_REG_THC},
  { TCX_VADDR_ROM,	0, TCX_REG_ROM},
  { TCX_VADDR_DHC,	SUSER, TCX_REG_DHC},
  { TCX_VADDR_ALT,	0, TCX_REG_ALT},
};

#define	N_TCX_MAP	(sizeof(tcx_map)/sizeof(struct mapping))



#ifdef	POWER_MGT
	/* power management components */

#define	PM_BOARD	0	/* board is 0 */
#define	PM_MONITOR	1	/* monitor is 1 */
#endif	POWER_MGT


	/* video state: boolean OR of these bits */

#define	VS_VIDEO	1	/* video enabled */
#define	VS_SYNC		2	/* sync enabled */


typedef	struct {
	  volatile caddr_t addr ;	/* kernel virtual address */
	  u_long	size ;		/* object size in bytes */
	  off_t		map_size ;	/* how much is mapped into kernel */
	  int		pfn ;		/* hat_getkpfnum() value */
	} MapInfo ;


/* per-unit data */
struct tcx_softc
{
#if NWIN > 0
	Pixrect		pr;		/* kernel pixrect */
	struct mprp_data prd;		/* pixrect private data */
#define	_w		pr.pr_size.x
#define	_h		pr.pr_size.y
#define	_fb		prd.mpr.md_image
#define	_linebytes	prd.mpr.md_linebytes
#else NWIN > 0
	int		_w, _h;		/* resolution */
	int		_linebytes ;
	void		*_fb;		/* frame buffer address */
#endif NWIN > 0
	MapInfo		maps[TCX_REG_MAX] ;

	off_t		last_vaddr ;	/* cached mapping */
	u_long		last_size ;
	int		last_pfn ;

	/* TODO: dummy overlay planes for cg4, cg8 emulation? */

	struct softcur {
		short enable;		/* cursor enable */
		short pad1;
		struct fbcurpos pos;	/* cursor position */
		struct fbcurpos hot;	/* cursor hot spot */
		struct fbcurpos size;	/* cursor bitmap size */
		u_long image[32];	/* cursor image bitmap */
		u_long mask[32];	/* cursor mask bitmap */
	} cur;

	u_char		cmap_buffer[4][TCX_CMAP_ENTRIES] ;	/* frgb */
	u_char		omap_buffer[4][2] ;			/* rgb */

	u_int		vrtflag;	/* vrt interrupt flag */
#define	TCXVRTIOCTL	0x1	/* FBIOVERTICAL in effect */
#define	TCXVRTCTR	0x2	/* vertical retrace counter */
#define	TCXCMAPUD	0x4	/* colormap update pending */
#define	TCXOMAPUD	0x8	/* overlay colormap update pending */
#define	TCX_CMAP_WAIT	0x10	/* waiting for colormap update */
#define	TCXCURUPD	0x20	/* cursor shape update pending */
	int		*vrtpage;	/* pointer to VRT page */
	int		emulation;	/* emulation type, normally tcx */
	int		em_depth ;	/* emulated depth, normally 24 */
	int		depth ;		/* real depth, depends on A24/FSV */
	int		capabilities ;	/* alignment, rop, etc. */
	int		owner ;		/* pid of owner */
	int		revision ;	/* chip revision */
	struct mon_info	moninfo;	/* info about this monitor */
	dev_info_t	*dip ;		/* back pointer */
	int		unit ;
#ifdef	SYSV
	kmutex_t	mutex ;
	kcondvar_t	sleepcv ;
	ddi_iblock_cookie_t iblock ;
#endif	/* SYSV */
	int		initialized ;	/* prevents interrupt race cond. */
	int		vidstate ;
};



	/* default structure for FBIOGATTR ioctl */

static	struct fbgattr	tcx_attr =  {
	FBTYPE_TCX,
	0,				/* owner */

	/* fbtype structure, subject to change according to emulation mode */
	/* type,width,height,depth,cmap size, fb size */
	{FBTYPE_TCX, 0,0,24, TCX_CMAP_ENTRIES,0},

	/* fbsattr structure: flags, emu_type, dev_specific */
	{0, FBTYPE_TCX, { 0,0,0,0,0,0,0,0 } },

	/* available emulation types */
	{FBTYPE_SUN3COLOR, FBTYPE_MEMCOLOR, -1,-1}
};



	/* Colormap structure */

typedef	volatile struct {
	  u_long  addr ;
	  u_long  cmap ;
	  u_long  ctrl ;
	  u_long  omap ;
	  u_long  f[2] ;
	  u_long  ctrl2 ;
	} Tcx_Cmap ;




/*
 * forward references
 */
#ifdef	SYSV
static	int	tcx_identify(dev_info_t *);
static	int	tcx_getinfo(dev_info_t *, ddi_info_cmd_t, void *, void**);
static	int	tcx_attach(dev_info_t *, ddi_attach_cmd_t);
static	int	tcx_detach(dev_info_t *, ddi_detach_cmd_t);
#ifdef	POWER_MGT
static	int	tcx_power(dev_info_t *, int, int);
#endif	POWER_MGT
static	int	tcx_reset(dev_info_t *, ddi_reset_cmd_t);
static	int	tcx_open(dev_t *, int, int, cred_t *);
static	int	tcx_close(dev_t, int, int, cred_t *);
static	int	tcx_ioctl(dev_t, int, int, int, cred_t *, int *);
static	int	tcx_mmap(dev_t dev, off_t off, int prot);
static	u_int	tcx_intr(caddr_t);

static	void	tcx_reset_reg(struct tcx_softc *);
static	void	tcx_reset_cmap(struct tcx_softc *);
static	void	tcx_update_cmaps(struct tcx_softc *);
static	int	tcx_kmap(struct tcx_softc *, int, u_int) ;
static	int	tcx_unmap_all(struct tcx_softc *) ;
static	void	set_control_plane(struct tcx_softc *,int) ;
static	void	read_colormap(struct tcx_softc *,int,int,int,u_char *,
			u_char *,u_char *) ;
#else
static	int	tcx_identify();
static	int	tcx_getinfo();
static	int	tcx_attach();
static	int	tcx_reset();
STATIC	int	tcx_open();
STATIC	int	tcx_close();
STATIC	int	tcx_ioctl();
STATIC	int	tcx_mmap();
static	int	tcx_poll();
static	u_int	tcx_intr();

static	void	tcx_reset_reg();
static	void	tcx_reset_cmap();
static	void	tcx_update_cmaps();
static	int	tcx_kmap() ;
static	int	tcx_unmap_all() ;
static	void	set_control_plane() ;
static	void	read_colormap() ;
#endif



/*
 * Tuning parameters
 */

#define	MAX_VRT_LINE	10	/* if scanline count is less than this,
				 * we still have some useful vrt time
				 * remaining
				 */


/*
 * handy macros
 */

#ifdef	SYSV
#define	getsoftc(unit)	((struct tcx_softc *)ddi_get_soft_state(statep,(unit)))
#else
#define	getsoftc(unit)	(&tcx_softc[unit])
#endif

#define	TEC(s,r)	((volatile u_long *)(s)->maps[TCX_REG_TEC].addr + (r))
#define	THC(s,r)	((volatile u_long *)(s)->maps[TCX_REG_THC].addr + (r))

#define	HCMISC(softc)		(*THC(softc,TCX_THC_HCMISC))
#define	LINECOUNT(softc)	(*TEC(softc,TCX_TEC_LINECOUNT))
#define	CURSOR_ADDRESS(softc)	(*THC(softc,TCX_THC_CURSOR_ADDRESS))
#define	CONFIG(softc)		(*THC(softc,TCX_THC_CONFIG))

#define	BZERO(d,c)	bzero((caddr_t) (d), (size_t) (c))
#define	BCOPY(s,d,c)	bcopy((caddr_t) (s), (caddr_t) (d), (u_int) (c))


/* position value to use to disable HW cursor */
#define	TCX_CURSOR_OFFPOS	(0xffe0ffe0)

#define	tcx_get_video(softc)	((softc)->vidstate & VS_VIDEO)

#ifdef	TODO		/* TODO: re-enable when rev0/1 chips obsolete */
#define	tcx_int_enable(softc)	(HCMISC(softc) |= TCX_HCMISC_EN_IRQ)
#endif	/* TODO */

#define	tcx_int_disable(softc)	(HCMISC(softc) &= ~TCX_HCMISC_EN_IRQ)

#define	tcx_int_pending(softc)	(!(HCMISC(softc) & TCX_HCMISC_IRQ))

/* check if color map update is pending */
#define	tcx_update_pending(softc)   ((softc)->vrtflag & (TCXCMAPUD|TCXOMAPUD))



static	void
tcx_set_vidstate(softc, state)
	struct	tcx_softc *softc ;
	int	state ;
{
register u_long hcmisc = HCMISC(softc) ;
register u_long bits ;

	switch( state ) {
	  case 0:
	  case VS_VIDEO:
	    bits = TCX_HCMISC_DISABLE_VSYNC|TCX_HCMISC_DISABLE_HSYNC ;
	    break ;
	  case VS_SYNC:
	    bits = 0 ;
	    break ;
	  case VS_SYNC|VS_VIDEO:
	    bits = TCX_HCMISC_ENABLE_VIDEO ;
	    break ;
	}
	hcmisc &= ~(TCX_HCMISC_VSYNC_LEVEL|
		    TCX_HCMISC_HSYNC_LEVEL|
		    TCX_HCMISC_DISABLE_VSYNC|
		    TCX_HCMISC_DISABLE_HSYNC|
		    TCX_HCMISC_ENABLE_VIDEO) ;
	hcmisc |= bits ;
	HCMISC(softc) = hcmisc ;
	softc->vidstate = state ;
}

static	void
tcx_set_video(softc,f)
	struct	tcx_softc *softc ;
	int	f ;
{
	tcx_set_vidstate(softc, f ? softc->vidstate|VS_VIDEO :
				    softc->vidstate&~VS_VIDEO) ;
}

static	void
tcx_int_enable(softc)
	struct tcx_softc *softc ;
{
	volatile u_long *hcmisc = &HCMISC(softc) ;

	if( softc->revision == 0 )	/* wait for end of vrt */
	{
	  while( (*hcmisc & TCX_HCMISC_VBLANK) != 0 )
#ifdef	SYSV
	    drv_usecwait(100) ;
#else
	    DELAY(100) ;
#endif
	}

	*hcmisc |= TCX_HCMISC_EN_IRQ ;
}




	/* simulate SVr4-specific functions in 4.x */

#ifndef	SYSV
typedef	int			cred_t ;	/* dummy for tcx_close */
typedef	int			ddi_attach_cmd_t ;	/* dummy for attach()*/
static	int			ntcx=0, next_unit=0 ;
static	struct tcx_softc	*tcx_softc = NULL ;

#define	getpid()		(u.u_procp->p_pid)
#define	pagesize		PAGESIZE
#define	pageoffset		PAGEOFFSET
#define	pageshift		PAGESHIFT
#define	tcx_getpages		getpages
#define	tcx_freepages		freepages

#define	DDI_IDENTIFIED		++ntcx
#define	DDI_NOT_IDENTIFIED	0
#define	DDI_SUCCESS		(0)
#define	DDI_FAILURE		(-1)
#define	DDI_INTR_CLAIMED	1
#define	DDI_INTR_UNCLAIMED	0

#define	getminor		minor

#define	mutex_init(mp,name,type,cookie)
#define	tcx_mutex_enter(mp)
#define	tcx_mutex_exit(mp)
#define	tcx_mutex_assert(s)
#define	tcx_mutex_nassert(s)

#define	cv_init(cp,name,type,ptr)

static	int
ddi_map_regs(dip, rnumber, kaddrp, offset, len)
	dev_info_t	*dip ;
	u_int		rnumber ;
	caddr_t		*kaddrp ;
	off_t		offset ;
	off_t		len ;
{
	struct dev_reg *reg = &dip->devi_reg[rnumber] ;
	*kaddrp = map_regs(reg->reg_addr + offset, len, reg->reg_bustype);
	return *kaddrp != NULL ? DDI_SUCCESS : DDI_FAILURE ;
}

#define	ddi_unmap_regs(dip, rnumber, kaddrp, offset, len)	\
		unmap_regs(*kaddrp, len, dip->devi_reg[rnumber].reg_bustype)

int
ddi_soft_state_zalloc(statep, unit)
	void	*statep ;
	int	unit ;
{
	if( tcx_softc == NULL )
	    tcx_softc = (struct tcx_softc *)
		tcx_kmem_zalloc((u_int) sizeof (struct tcx_softc) * ntcx,1);

	return tcx_softc != NULL ? DDI_SUCCESS : DDI_FAILURE ;
}

#define	ddi_soft_state_free(s,u)		0

#define	ddi_set_driver_private(dip,softc)	(dip)->devi_data = (softc)

#define	ddi_report_dev	report_dev

#define	tgetprop(dip, name, def)  getprop((dip)->devi_nodeid, (name),(def))

static	char *
tgetlongprop(dip, name, rlen)
	dev_info_t *dip ;
	char	*name ;
	int	*rlen ;
{
	*rlen = getproplen(dip->devi_nodeid, name) ;
#if	TCXDEBUG
	total_memory += *rlen ;
#endif	TCXDEBUG
	return getlongprop(dip->devi_nodeid, name) ;
}

static	int
tgetboolprop(dip,name)
	dev_info_t *dip ;
	char	*name ;
{
	int	len ;
	return getproplen(dip->devi_nodeid, name) != -1 ;
}

#define	FKIOCTL	1

int
ddi_copyin(src, dst, len, flag)
	caddr_t	src, dst ;
	int	len, flag ;
{
	return flag ? kcopy(src,dst,len) : copyin(src,dst,len) ;
}

int
ddi_copyout(src, dst, len, flag)
	caddr_t	src, dst ;
	int	len, flag ;
{
	return flag ? kcopy(src,dst,len) : copyout(src,dst,len) ;
}

	/* copy ioctl arguments in/out.  These are no-ops in 4.x since
	 * the kernel does this for us. */
#define	ARG_COPYIN(dst)				0
#define	ARG_COPYOUT(src)			0
#define	IMM_COPYIN(dst,type)			((dst) = *(type *)arg)
#define	IMM_COPYOUT(src,type)			(*(type *)arg = (src))

	/* 4.x interrupt control */
#define	splbio4x		splbio
#define	splx4x			splx
#define	tcx_sleep(softc)	sleep((caddr_t)softc, PRIBIO)
#define	tcx_wakeup(softc)	wakeup((caddr_t)softc)

#else	/* SVr4 */



static	u_long
getpid()
{
	u_long	rval ;
	if( drv_getparm(PPID, &rval) == -1 )
	  return -1 ;
	else
	  return rval ;
}

static	int	pagesize ;
static	int	pageoffset ;
static	int	pageshift ;

static  caddr_t
tcx_getpages(npages)
	int    npages ;
{
	register caddr_t p1;

	p1 = kmem_zalloc(ptob(npages), KM_SLEEP);
	ASRT(p1 != NULL && !((u_int) p1 & pageoffset));
	return (p1);
}

static void
tcx_freepages(ptr, size)
	void	*ptr ;
	int	size ;
{
	ASRT (ptr != NULL && !((u_int)ptr & pageoffset));
	kmem_free(ptr, ptob(size));
}

static	int
tgetprop(dip, name, def)
	dev_info_t *dip ;
	char	*name ;
	int	def ;
{
	return ddi_getprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS, name, def) ;
}

static	char *
tgetlongprop(dip, name, rlen)
	dev_info_t *dip ;
	char	*name ;
	int	*rlen ;
{
	char	*rval ;
	if( ddi_getlongprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
		name, (caddr_t) &rval, rlen) != DDI_PROP_SUCCESS )
	  rval = NULL ;
#if	TCXDEBUG
	else
	  total_memory += *rlen ;
#endif	TCXDEBUG
	return rval ;
}

static	int
tgetboolprop(dip,name)
	dev_info_t *dip ;
	char	*name ;
{
	int	len ;
	return ddi_getproplen(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
		name, &len) == DDI_PROP_SUCCESS ;
}

	/* copy ioctl arguments in/out. */
#define	ARG_COPYIN(dst)		DDI_COPYIN(arg, dst, sizeof(*dst), copyflag)
#define	ARG_COPYOUT(src)	DDI_COPYOUT(src, arg, sizeof(*src), copyflag)
#define	IMM_COPYIN(dst,type)	DDI_COPYIN(arg, &dst, sizeof(type), copyflag)
#define	IMM_COPYOUT(src,type)	DDI_COPYOUT(&src, arg, sizeof(type), copyflag)

	/* 5.x interrupt control.  spl* are no-ops because the mutex
	 * mechanism does this for us. */
#define	splbio4x()		0
#define	splx4x(i)		0
#define	tcx_sleep(softc)	cv_wait(&softc->sleepcv, &softc->mutex)
#define	tcx_wakeup(softc)	cv_broadcast(&softc->sleepcv) ;

#endif	/* !SYSV */






#if NWIN > 0

/*
 * SunWindows specific stuff
 */
static tcx_pr_putcolormap();

/* kernel pixrect ops vector */
static struct pixrectops tcx_pr_ops = {
	mem_rop,
	tcx_pr_putcolormap,
	mem_putattributes
};

#endif	NWIN > 0


	/* Physically load colormaps into dacs, normally called from
	 * interrupt.  Only load flagged values.  Clear flags as we
	 * go.  Must be called from within mutex.
	 */
static	void
tcx_update_cmaps(softc)
register struct tcx_softc *softc ;
{
	register unsigned char	*red, *green, *blue, *flags;
	register Tcx_Cmap	*cmap ;
	register int		index ;
	register int		oflag ;

	DEBUGF(5, ("tcx_update_cmaps softc=%x\n", softc));
	tcx_mutex_assert(softc) ;

	cmap = (Tcx_Cmap *) softc->maps[TCX_REG_CMAP].addr ;
	ASRT(cmap != NULL) ;

	if( softc->vrtflag & TCXCMAPUD )
	{
	  flags = softc->cmap_buffer[0] ;
	  red = softc->cmap_buffer[1] ;
	  green = softc->cmap_buffer[2] ;
	  blue = softc->cmap_buffer[3] ;

	  oflag = 0 ;
	  for(index=0; index<TCX_CMAP_ENTRIES; ++index)
	  {
	    if( *flags ) {
	      if( !oflag )
		cmap->addr = index<<24 ;
	      cmap->cmap = *red<<24 ;
	      cmap->cmap = *green<<24 ;
	      cmap->cmap = *blue<<24 ;
	    }
	    oflag = *flags ;
	    *flags++ = 0 ;
	    ++red ; ++green ; ++blue ;
	  }
	  cmap->addr = 0 ;		/* A24 seems to require this */
	}


	if( softc->vrtflag & TCXOMAPUD )
	{
	  /* now load overlay colormap */
	  cmap->addr = 2<<24 ;
	  cmap->omap = softc->omap_buffer[1][0]<<24 ;
	  cmap->omap = softc->omap_buffer[2][0]<<24 ;
	  cmap->omap = softc->omap_buffer[3][0]<<24 ;

	  cmap->addr = 3<<24 ;
	  cmap->omap = softc->omap_buffer[1][1]<<24 ;
	  cmap->omap = softc->omap_buffer[2][1]<<24 ;
	  cmap->omap = softc->omap_buffer[3][1]<<24 ;

	  cmap->addr = 0 ;		/* A24 seems to require this */
	}

	softc->vrtflag &= ~(TCXCMAPUD|TCXOMAPUD) ;
}




#if NWIN > 0
static
tcx_pr_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct tcx_softc *softc = getsoftc(mpr_d(pr)->md_primary);

	DEBUGF(5, ("tcx_pr_putcolormap unit=%d index=%d count=%d\n",
		mpr_d(pr)->md_primary, index, count));

	return put_colormap(softc, 0, index, count, red, green, blue, 1) ?
	    PIX_ERR : 0 ;
}

#endif	NWIN > 0




static
#ifdef	SYSV
tcx_identify(dip)
	dev_info_t *dip ;
#else
tcx_identify(name)
	char	*name ;
#endif
{
#ifdef	SYSV
	char	*name = ddi_get_name(dip) ;
#endif

	DEBUGF(1, ("tcx_identify(%s)\n", name));

	if( strcmp(name, "SUNW,tcx") == 0 )
	  return DDI_IDENTIFIED ;
	else
	  return DDI_NOT_IDENTIFIED;
}




	/* Utility: create a kernel mapping and return page-frame # */
static	int
tcx_kmap(softc,which,size)
	struct tcx_softc *softc ;
	int	which ;
	u_int	size ;
{
	DEBUGF(3, ("tcx_kmap: which=%d, size=%x\n",which,size));
	tcx_mutex_enter(&softc->mutex) ;
	if(ddi_map_regs(softc->dip, which, &softc->maps[which].addr, 0,size))
	{
	  softc->maps[which].map_size = 0 ;
	  softc->maps[which].pfn = -1 ;
	}
	else {
	  softc->maps[which].map_size = size ;
	  softc->maps[which].pfn = hat_getkpfnum(softc->maps[which].addr);
	}
	tcx_mutex_exit(&softc->mutex) ;
	DEBUGF(4, ("  which=%d, va=%x\n", which, softc->maps[which].addr));
	return softc->maps[which].pfn ;
}

#ifdef	POWER_MGT
static	int
tcx_resume(dip)
	dev_info_t *dip ;
{
	register struct tcx_softc *softc;

	if((softc = (struct tcx_softc *)ddi_get_driver_private(dip)) == NULL)
	  return DDI_FAILURE ;

	DEBUGF(1, ("tcx_resume\n"));

	/* restore hardware state from softc */
	mutex_enter(&softc->mutex);
	/* reload cursor */
	if( softc->capabilities & TCX_CAP_HW_CURSOR ) {
	  tcx_setcurshape(softc);
	  tcx_setcurpos(softc);
	}

	/* reload colormap */
	softc->vrtflag |= TCXCMAPUD|TCXOMAPUD ;
	tcx_update_cmaps(softc) ;

	/* reset video */
	/* anthing else? */
	mutex_exit(&softc->mutex);
	return DDI_SUCCESS ;
}

static	int
tcx_suspend(softc)
	register struct tcx_softc *softc;
{
	int	i ;
	DEBUGF(1, ("tcx_suspend\n"));
	mutex_enter(&softc->mutex);

	/* get colormap into shadow copy */
	read_colormap(softc,0,0,256, softc->cmap_buffer[1],
			softc->cmap_buffer[2],softc->cmap_buffer[3]) ;
	for(i=0; i<256; ++i) softc->cmap_buffer[0][i] = 1 ;
	read_colormap(softc,1,0,2, softc->omap_buffer[1],
			softc->omap_buffer[2],softc->omap_buffer[3]) ;
	for(i=0; i<2; ++i) softc->omap_buffer[0][i] = 1 ;

	/* anything else? */
	mutex_exit(&softc->mutex);
	return DDI_SUCCESS ;
}
#endif	POWER_MGT


static	int
tcx_attach(dip, cmd)
	dev_info_t *dip;
	ddi_attach_cmd_t cmd;		/* not used in 4.x */
{
	register struct tcx_softc *softc;
	register caddr_t reg;
	int	w, h ;
	u_long	size ;			/* # of pixels in fb */
	char	*tmp ;
	int	tmplen ;
	int	unit;
	extern dev_info_t *top_devinfo;	/* in autoconf.c */
	char	name[16] ;
#ifdef	POWER_MGT
	u_long	timestamp[2] ;
	int	power[2] ;
#endif	POWER_MGT

	DEBUGF(1, ("tcx_attach cmd=%d\n", cmd));

#ifdef	SYSV
	switch( cmd ) {
	  case DDI_ATTACH:
	    break ;

#ifdef	POWER_MGT
	  case DDI_RESUME:
	    return tcx_resume(dip) ;
#endif	POWER_MGT

	  default:
	    return DDI_FAILURE ;
	}

	unit = ddi_get_instance(dip) ;
#else
	unit = next_unit++ ;
#endif	SYSV

	DEBUGF(1, ("tcx_attach unit=%d\n", unit));

	if( ddi_soft_state_zalloc(statep, unit) != DDI_SUCCESS )
	  return DDI_FAILURE ;

	/* KLUDGE: on sparc and sun3 architecture, zalloc() sets pointers
	 * to NULL, so we won't do it here.  This wouldn't be true on
	 * some other architectures.
	 */

	softc = getsoftc(unit);
	softc->initialized = 0 ;
	softc->dip = dip;
	softc->unit = unit ;
	ddi_set_driver_private(dip, (caddr_t)softc) ;
#ifndef	SYSV
	dip->devi_unit = unit ;
#endif

	/* intialize softc */

	/* get stipple alignment, rop capabilities, etc. from prom */

	softc->capabilities =
		tgetprop(dip, "stipple-align", 5) |
		tgetprop(dip, "blit-width", 5) << TCX_CAP_BLIT_WIDTH_SH |
		tgetprop(dip, "blit-height", 0) << TCX_CAP_BLIT_HEIGHT_SH |
		tgetprop(dip, "control-planes", 2) << TCX_CAP_C_PLANES_SH |
		(tgetboolprop(dip, "stip-rop") ? TCX_CAP_STIP_ROP : 0) |
		(tgetboolprop(dip, "blit-rop") ? TCX_CAP_BLIT_ROP : 0) |
		(tgetboolprop(dip, "plane-mask") ? TCX_CAP_PLANE_MASK : 0) |
		(tgetboolprop(dip, "hw-cursor") ? TCX_CAP_HW_CURSOR : 0) |
		(!tgetboolprop(dip, "tcx-8-bit") ? TCX_CAP_24_BIT : 0) ;

	/* compute values to be returned by FBIOGTYPE, FBIOGATTR and
	 * FBIOMONINFO.  "size" fields are not really meaningful in
	 * a framebuffer that is mapped in multiple ways.  Applications
	 * will really have to figure this out from the screen dimensions.
	 * Applications get linebytes with the FBIOGXINFO ioctl.
	 */

	softc->_w = w = tgetprop(dip, "width", 1152);
	softc->_h = h = tgetprop(dip, "height", 900);
	softc->_linebytes = tgetprop(dip, "linebytes", w);

	/* TODO: get these from hardware somehow (prom?) */

	size = softc->_linebytes * h ;
	if( TCX_DFB8_SZ > size )
	  size = TCX_DFB8_SZ ;
	softc->maps[TCX_REG_DFB8].size = size ;
	softc->maps[TCX_REG_STIP].size = size * sizeof(u_long) * 2 ;
	softc->maps[TCX_REG_BLIT].size = size * sizeof(u_long) * 2 ;
	if( softc->capabilities & TCX_CAP_24_BIT ) {
	  softc->maps[TCX_REG_DFB24].size = size * sizeof(u_long) ;
	  softc->maps[TCX_REG_RDFB32].size = size * sizeof(u_long) ;
	  softc->maps[TCX_REG_RSTIP].size = size * sizeof(u_long) * 2 ;
	  softc->maps[TCX_REG_RBLIT].size = size * sizeof(u_long) * 2 ;
	  softc->depth = 24 ;
	}
	else
	{
	  softc->maps[TCX_REG_DFB24].size =
	  softc->maps[TCX_REG_RDFB32].size =
	  softc->maps[TCX_REG_RSTIP].size =
	  softc->maps[TCX_REG_RBLIT].size = 0 ;
	  softc->depth = 8 ;
	}

	/* these are engraved in stone */
	softc->maps[TCX_REG_TEC].size = TCX_TEC_SZ ;
	softc->maps[TCX_REG_CMAP].size = TCX_CMAP_SZ ;
	softc->maps[TCX_REG_THC].size = TCX_THC_SZ ;
	softc->maps[TCX_REG_ROM].size = TCX_ROM_SZ ;
	softc->maps[TCX_REG_DHC].size = TCX_DHC_SZ ;
	softc->maps[TCX_REG_ALT].size = TCX_ALT_SZ ;

	softc->owner = -1 ;


        /*
         * get monitor attributes
         */
        softc->moninfo.mon_type = tgetprop(dip, "montype", 0);
        softc->moninfo.pixfreq = tgetprop(dip, "pixfreq", 929405);
        softc->moninfo.hfreq = tgetprop(dip, "hfreq", 61795);
        softc->moninfo.vfreq = tgetprop(dip, "vfreq", 66);
        softc->moninfo.hfporch = tgetprop(dip, "hfporch", 32);
        softc->moninfo.vfporch = tgetprop(dip, "vfporch", 2);
        softc->moninfo.hbporch = tgetprop(dip, "hbporch", 192);
        softc->moninfo.vbporch = tgetprop(dip, "vbporch", 31);
        softc->moninfo.hsync = tgetprop(dip, "hsync", 128);
        softc->moninfo.vsync = tgetprop(dip, "vsync", 4);



	/*
	 * Map in registers.  Hold off on mapping framebuffers until
	 * needed by user process.  If any register set fails to map
	 * in, the attach fails.
	 */

	if( tcx_kmap(softc, TCX_REG_THC, TCX_THC_SZ) == -1  ||
	    tcx_kmap(softc, TCX_REG_TEC, TCX_TEC_SZ) == -1  ||
	    tcx_kmap(softc, TCX_REG_CMAP, TCX_CMAP_SZ) == -1 )
	{
	  tcx_unmap_all(softc) ;	/* free whatever we had */
	  return DDI_FAILURE ;
	}

	softc->vidstate = (HCMISC(softc) & TCX_HCMISC_ENABLE_VIDEO) ?
		VS_SYNC|VS_VIDEO : VS_SYNC ;

	/* arbitrarily set the cached mapping to one we know to be valid */

	softc->last_vaddr = TCX_ADDR_TEC ;
	softc->last_size = TCX_TEC_SZ ;
	softc->last_pfn = softc->maps[TCX_REG_TEC].pfn ;



	/* If prom has us mapped in, can we use that mapping?  If not,
	 * we need to map the first page here for SunPC to read
	 */

	/* only use address property if we are console fb */
	if( (reg = (caddr_t) tgetprop(dip, "address", -1)) != (caddr_t)-1 )
	{
	    softc->maps[TCX_REG_DFB8].addr = reg ;
	    softc->maps[TCX_REG_DFB8].map_size = size ;
	    softc->maps[TCX_REG_DFB8].pfn = hat_getkpfnum(reg);
	    DEBUGF(2, ("tcx mapped by PROM\n"));
	}
	else
	{
	    /* TODO: only map in 1 page? */
	    if ( tcx_kmap( softc, TCX_REG_DFB8, size ) == -1 )
	    {
		tcx_unmap_all( softc );
		return DDI_FAILURE;
	    }
	    DEBUGF(2, ("tcx mapped by driver\n"));
	}
#if	NWIN > 0
	softc->_fb = (MPR_T *)softc->maps[TCX_REG_DFB8].addr;
#endif

	/*
	 *  TCX devices are not supported under 4.x as
	 *  accelerated frame buffers. Therefore we
	 *  return that we are a cg3. This will allow
	 *  4.x apps to function unmodified. Note that
	 *  an exception is made for the Xterminal's
	 *  microkernel is based on 4.x yet the terminals
	 *  X server does support TCX.
	 */
#if	defined(SYSV) || defined(HAMLET)
	softc->emulation = FBTYPE_TCX ;
	softc->em_depth = softc->depth ;
#else
	softc->emulation = FBTYPE_SUN3COLOR ;
	softc->em_depth = 8 ;
#endif	SYSV || HAMLET

	softc->revision = (CONFIG(softc) >> TCX_CONFIG_REV_SHIFT) & 0xf ;

	if( (tmp = tgetlongprop(dip, "emulation", &tmplen)) != NULL ) {
		if ( strcmp ( tmp, "cgthree+" ) == 0 ) {
			softc->emulation = FBTYPE_SUN3COLOR;
			softc->em_depth = 8 ;
		}
		else if ( strcmp ( tmp, "cgeight+" ) == 0 )
			softc->emulation = FBTYPE_MEMCOLOR;
		tcx_kmem_free(tmp,tmplen) ;
	}

	/* attach interrupt */
#ifdef	SYSV
	if(ddi_add_intr(dip, 0, &softc->iblock, NULL, tcx_intr, (caddr_t)softc)
		!= DDI_SUCCESS )
	{
	  tcx_unmap_all(softc) ;
	  return DDI_FAILURE ;
	}
#else
	addintr(dip->devi_intr[0].int_pri, tcx_poll, dip->devi_name, unit);
#endif	SYSV

	mutex_init(&softc->mutex, "tcx mutex", MUTEX_DRIVER, softc->iblock) ;

	cv_init(&softc->sleepcv, "tcx sleep", CV_DRIVER, NULL) ;

#if NOHWINIT
	if (tcx_hwinit)
		tcx_init(softc);
#endif NOHWINIT

	tcx_reset_reg(softc);

#ifdef	SYSV
	sprintf(name,"tcx%d",unit) ;
	if( ddi_create_minor_node(dip, name, S_IFCHR,
		unit, DDI_NT_DISPLAY, 0) != DDI_SUCCESS )
	{
	  cv_destroy(&softc->sleepcv) ;
	  mutex_destroy(&softc->mutex) ;
	  ddi_remove_intr(dip, 0, softc->iblock) ;
	  tcx_unmap_all(softc) ;
	  return DDI_FAILURE ;
	}
#endif	SYSV

#ifdef	POWER_MGT
	/* set power-management properties.  item 0 is the a24
	 * board itself, which currently can't be powered down.
	 * item 1 is the monitor, which is powered down by removing
	 * sync.
	 *
	 * The pm_timestamp property is a time value indicating when
	 * the last activity was.  A time of 0 is a magic cookie that
	 * says this component is never eligible for shutdown.  A
	 * small number (representing a very old time) indicates that
	 * the device may be shut down at any time.
	 */

	timestamp[0] = 0 ;	/* flag board as never eligable */
	drv_getparm(TIME, &timestamp[1]);
	(void) ddi_prop_create(DDI_DEV_T_NONE, dip, DDI_PROP_CANSLEEP,
		"pm_timestamp", (caddr_t)timestamp, sizeof (timestamp));

	/* the pm_norm_pwr property defines the "on" power levels.
	 * currently, we can only turn the monitor on and off, but
	 * it's possible that there will be a later product that
	 * can set the brightness leve, so we'll define "on"
	 * level for the monitor as 255
	 */
	power[PM_BOARD] = 1 ;
	power[PM_MONITOR] = 255 ;
	(void) ddi_prop_create(DDI_DEV_T_NONE, dip, DDI_PROP_CANSLEEP,
		"pm_norm_pwr", (caddr_t)power, sizeof (power));
#endif	POWER_MGT


	ddi_report_dev(dip);

#ifdef	SYSV
	tcx_printf("?tcx%d: revision %d, screen %dx%d\n",
		unit, softc->revision, w,h) ;
#else
	printf("tcx%d: revision %d, screen %dx%d\n",
		unit, softc->revision, w,h) ;
#endif	SYSV

	softc->initialized = 1 ;

	return DDI_SUCCESS;
}


static
tcx_unmap_all(softc)
	register struct tcx_softc *softc;
{
	dev_info_t *dip = softc->dip ;
	int	i,j ;

	/* unmap all registers */
	for(i=0; i<N_TCX_MAP; ++i)
	{
	  j = tcx_map[i].idx ;
	  if( softc->maps[j].addr != NULL ) {
	    DEBUGF(3,("tcx_unmap_all: which = %d, addr=%x, size=%x\n",
	      j, softc->maps[j].addr, softc->maps[j].map_size));
	    ddi_unmap_regs(dip, 0, &softc->maps[j].addr,
	    	0, softc->maps[j].map_size) ;
	    softc->maps[j].addr = NULL ;
	    softc->maps[j].map_size = 0 ;
	  }
	}

	if( softc->vrtpage != NULL ) {
	  tcx_freepages(softc->vrtpage, 1) ;
	  softc->vrtpage = NULL ;
	}

#ifdef	POWER_MGT
	if( ddi_prop_remove(DDI_DEV_T_NONE, dip, "pm_timestamp")
		!= DDI_PROP_SUCCESS  ||
	    ddi_prop_remove(DDI_DEV_T_NONE, dip, "pm_norm_pwr")
		!= DDI_PROP_SUCCESS )
	  cmn_err(CE_WARN, "tcx_detach: Unable to remove property");
#endif	POWER_MGT

	(void) ddi_soft_state_free(statep, softc->unit);
}


#ifdef	SYSV
static	int
tcx_detach(dip, cmd)
	dev_info_t	*dip ;
	ddi_detach_cmd_t cmd ;
{
register struct tcx_softc *softc ;
	int	unit ;

	DEBUGF(1, ("tcx_detach cmd=%d\n", cmd));

	if((softc = (struct tcx_softc *)ddi_get_driver_private(dip)) == NULL)
	  return DDI_FAILURE ;

	DEBUGF(1, ("tcx_detach, softc=%x, mem used=%d\n",
		softc, total_memory));

	switch( cmd ) {
	  case DDI_DETACH:
	    break ;

#ifdef	POWER_MGT
	  case DDI_SUSPEND:
	    return tcx_suspend(softc) ;
#endif	POWER_MGT

	  default:
	    return DDI_FAILURE ;
	}

	tcx_reset_reg(softc);

	ddi_remove_minor_node(dip,NULL) ;
	cv_destroy(&softc->sleepcv) ;
	mutex_destroy(&softc->mutex) ;
	ddi_remove_intr(dip, 0, softc->iblock) ;
	tcx_unmap_all(softc) ;
	return DDI_SUCCESS ;
}


#ifdef	POWER_MGT
static	int
tcx_power(dip, component, level)
	dev_info_t	*dip ;
	int		component, level ;
{
register struct tcx_softc *softc ;
	int	rval ;

	DEBUGF(1, ("tcx_power, component=%d, level=%d\n", component, level));

	if( level < 0 )
	  return DDI_FAILURE ;

	if((softc = (struct tcx_softc *)ddi_get_driver_private(dip)) == NULL)
	  return DDI_FAILURE ;

	tcx_mutex_enter(&softc->mutex) ;

	switch(component) {
	  case PM_MONITOR:		/* monitor */
	    /* for now, the power level is meaningless other than as
	     * an on/off flag
	     */
	    tcx_set_vidstate(softc, level > 0 ? softc->vidstate|VS_SYNC :
						softc->vidstate&~VS_SYNC) ;
	    rval = DDI_SUCCESS ;
	    break ;

	  case PM_BOARD:	/* A24 board can't be shut down, just return */
	  default:
	    rval = DDI_FAILURE ;
	    break ;
	}
	tcx_mutex_exit(&softc->mutex) ;
	return rval ;
}
#endif	POWER_MGT


tcx_getinfo(dip, cmd, arg, result)
	dev_info_t *dip ;
	ddi_info_cmd_t cmd ;
	void *arg ;
	void **result ;
{
	dev_t	dev ;
	int	unit, rval ;
	struct tcx_softc *softc ;

	dev = (dev_t) arg ;
	unit = TCXUNIT(dev) ;

	switch( cmd ) {
	  case DDI_INFO_DEVT2DEVINFO:
	    softc = getsoftc(unit) ;
	    if( softc == NULL )
	      return DDI_FAILURE ;
	    *result = (void *) softc->dip ;
	    break ;

	  case DDI_INFO_DEVT2INSTANCE:
	    *result = (void *) unit ;
	    break ;

	  default:
	    return DDI_FAILURE ;
	}
	return DDI_SUCCESS ;
}
#endif	SYSV


#ifdef	SYSV
static	int
tcx_open(devp, flag, otyp, credp)
	dev_t	*devp;
	int	flag;
	int	otyp ;
	cred_t	*credp ;
#else
STATIC	int
tcx_open(dev, flag)
	dev_t	dev;
	int	flag;
#endif	SYSV
{
#ifdef	SYSV
	dev_t	dev = *devp ;
#endif	SYSV
	struct tcx_softc *softc = getsoftc(TCXUNIT(dev));

	DEBUGF(2, ("tcx_open(%d), mem used=%d, pid=%d\n",
		TCXUNIT(dev), total_memory, getpid()));

	tcx_mutex_enter(&softc->mutex) ;
	if( softc->owner == -1 )
	  softc->owner = getpid() ;
	HCMISC(softc) |= TCX_HCMISC_OPEN_FLAG ;
	tcx_mutex_exit(&softc->mutex) ;

	return 0 ;
}

/*ARGSUSED*/
STATIC
tcx_close(dev, flag, otyp, credp)
	dev_t	dev;
	int	flag;
	int	otyp ;		/* not used in 4.x */
	cred_t	*credp ;	/* not used in 4.x */
{
	struct tcx_softc *softc = getsoftc(TCXUNIT(dev));

	DEBUGF(2, ("tcx_close(%d), mem used=%d, pid=%d\n",
		TCXUNIT(dev), total_memory, getpid()));

	tcx_mutex_enter(&softc->mutex) ;
	softc->cur.enable = 0;
	softc->owner = -1 ;
	HCMISC(softc) &= ~TCX_HCMISC_OPEN_FLAG ;
	tcx_reset_reg(softc);
	softc->vrtflag &= ~TCXVRTCTR ;
	if( softc->vrtpage != NULL ) {
	  tcx_freepages(softc->vrtpage, 1) ;
	  softc->vrtpage = NULL ;
	}
	tcx_mutex_exit(&softc->mutex) ;
	set_control_plane(softc,0) ;

	return 0;
}



#define	GET_MAP(softc,which,size)	\
	  do{ if( (softc)->maps[which].addr == NULL ) \
		(void) tcx_kmap(softc,which,size) ; \
	  } while(0)


	/* used for mapping framebuffers and other big mappings.  Only the
	 * first page of the framebuffer is actually mapped into kernel
	 * space.  This function works by adding the page offset to the
	 * cached page-frame number.
	 *
	 * Note: this function will fail on architectures that do not
	 * store the page in the low-order bits of the page-frame number.
	 * What we really need is a kernel function to map physical
	 * addresses to page-frame numbers rather than mapping kernel
	 * virtual addresses to page-frame numbers.
	 */

/*ARGSUSED*/
STATIC	int
tcx_mmap(dev, off, prot)
	dev_t dev;
register off_t off;
	int prot;
{
	register struct tcx_softc *softc = getsoftc(TCXUNIT(dev));
	register int diff, i,idx ;
	register int rval = -1 ;


	if( softc->emulation != FBTYPE_TCX )
	  switch( softc->emulation ) {
	    case FBTYPE_MEMCOLOR:
	      if( (diff = off-CG8_VADDR_FB) >= 0 &&
		    diff < softc->maps[TCX_REG_DFB24].size )
		off += TCX_VADDR_DFB24 - CG8_VADDR_FB ;
	      else if( (diff = off-CG8_VADDR_DAC) >= 0 &&
		    diff < softc->maps[TCX_REG_CMAP].size )
		off += TCX_VADDR_CMAP - CG8_VADDR_DAC ;
#ifdef	SYSV
	      /* TODO: where is this defined in 4.x? */
	      else if( (diff = off-CG8_VADDR_PROM) >= 0 &&
		    diff < PROMSIZE )
		off += TCX_VADDR_ROM - CG8_VADDR_PROM ;
#endif	/* SYSV */
	      break ;
	    case FBTYPE_SUN3COLOR:
	      if( (diff = off-CG3_MMAP_OFFSET) >= 0 &&
		    diff < softc->maps[TCX_REG_DFB8].size )
		off += TCX_VADDR_DFB8 - CG3_MMAP_OFFSET ;
	      break ;
	  }


	/* Speedup: compare offset to cached values */

	if( (diff = off-softc->last_vaddr) >= 0  &&
	      diff < softc->last_size )
	{

	  DEBUGF(off ? 4 : 2, ("tcx_mmap(%d, 0x%x) (cached), pid=%d\n",
		TCXUNIT(dev), (u_int) off, getpid()));

	  rval = softc->last_pfn + (diff>>pageshift) ;
	}


	else if((diff = off-TCX_VADDR_VRT) >= 0 && diff < pagesize )
	{
	  DEBUGF(off ? 3 : 2, ("tcx_mmap(%d, 0x%x) (vrt), pid=%d\n",
		TCXUNIT(dev), (u_int) off, getpid()));

	  if( softc->vrtpage == NULL )
	  {
	    softc->vrtpage = (int *) tcx_getpages(1) ;
	    if( softc->vrtpage != NULL )
	    {
	      softc->vrtflag |= TCXVRTCTR ;
	      *softc->vrtpage = 0 ;
	      tcx_int_enable(softc);
	    }
	  }
	  if( softc->vrtpage != NULL )
	    rval = hat_getkpfnum((caddr_t) softc->vrtpage) ;
	}


	/* we don't want a mutex here, since this function needs to
	 * be fast.  On the other hand, we need to have a mutex if
	 * we're going to call ddi_map_regs.  This is done in tcx_kmap()
	 */

	else		/* normal mapping */
	{
	  DEBUGF(off ? 3 : 1, ("tcx_mmap(%d, 0x%x), pid=%d\n",
		TCXUNIT(dev), (u_int) off, getpid()));

	  for(i=0; i<N_TCX_MAP; ++i) {
	    idx = tcx_map[i].idx ;
	    if( (diff=off-tcx_map[i].vaddr) >= 0 &&
		diff < softc->maps[idx].size )
	    {
	      /* TODO: only map one page? */
	      GET_MAP(softc,idx,softc->maps[idx].size);
	      if( (rval=softc->maps[idx].pfn) != -1 )
	      {
		rval += diff>>pageshift ;
		softc->last_vaddr = tcx_map[i].vaddr ;
		softc->last_size = softc->maps[idx].size ;
		softc->last_pfn = softc->maps[idx].pfn ;
	      }
	      break ;
	    }
	  }
	}


	DEBUGF(4, ("tcx_mmap returning 0x%x\n", rval));

	return rval;
}


#define	DDI_COPYOUT(s,d,l,f)	ddi_copyout((caddr_t)(s),(caddr_t)(d),(l),(f))
#define	DDI_COPYIN(s,d,l,f)	ddi_copyin((caddr_t)(s),(caddr_t)(d),(l),(f))


	/* Colormap utilities, used by ioctl */

		/* verify that given index & count are valid */
static	int
verify_cmap(index,count,entries)
	int	index, count, entries ;
{
	switch( PIX_ATTRGROUP(index) ) {
	  case 0:
	  case PIXPG_8BIT_COLOR:
	    break ;
	  default:
	    return EINVAL ;
	}

	index &= PIX_ALL_PLANES ;

	if( count < 0 || index + count > entries)
	  return EINVAL;

	return 0 ;
}

	/* wait for an interrupt; must be called from within a mutex */
static
wait_for_interrupt(softc,flag)
	register struct tcx_softc *softc ;
	int	flag ;
{
	int	i;

	DEBUGF(6,("wait_for_interrupt: flag=%x\n",flag));
	tcx_mutex_assert(softc) ;

	/* if vertical sync is disabled, there won't be a vertical
	 * interrupt, so we just return now.
	 */
	if( HCMISC(softc) & TCX_HCMISC_DISABLE_VSYNC )
	  return ;

	i = splbio4x() ;
	softc->vrtflag |= flag ;
	tcx_int_enable(softc);
	/* look, ma! a critical section! right here! */
	tcx_sleep(softc) ;
	splx4x(i) ;
}


		/* read colormap from dacs to buffer */
static	void
read_colormap(softc,cursor_cmap,index,count,red,green,blue)
	register struct tcx_softc *softc ;
	int	cursor_cmap,index,count ;
	u_char	*red,*green,*blue ;
{
	int	i ;
	int	rval ;
register u_char	*rm,*gm,*bm ;
register Tcx_Cmap *cmap = (Tcx_Cmap *) softc->maps[TCX_REG_CMAP].addr ;

	ASRT(cmap != NULL) ;

	if( !cursor_cmap ) {
	  cmap->addr = index<<24 ;
	  for(i=count; --i >= 0;) {
	    *red++ = cmap->cmap >> 24 ;
	    *green++ = cmap->cmap >> 24 ;
	    *blue++ = cmap->cmap >> 24 ;
	  }
	}
	else {
	  cmap->addr = 1<<24 ;
	  red[0] = cmap->omap >> 24 ;
	  green[0] = cmap->omap >> 24 ;
	  blue[0] = cmap->omap >> 24 ;
	  cmap->addr = 3<<24 ;
	  red[1] = cmap->omap >> 24 ;
	  green[1] = cmap->omap >> 24 ;
	  blue[1] = cmap->omap >> 24 ;
	}
	cmap->addr = 0 ;		/* A24 seems to require this */
}


		/* fetch colormap from dacs to buffer, then copy it out */
		/* TODO: 3-color cursor? */
static	int
fetch_colormap(softc,cursor_cmap,index,count,red,green,blue,copyflag)
	register struct tcx_softc *softc ;
	int	cursor_cmap,index,count ;
	u_char	*red,*green,*blue ;
	int	copyflag ;
{
	int	rval ;

	/* See notes on 1142190 at top of file */
	u_char	rtmp[TCX_CMAP_ENTRIES],
		gtmp[TCX_CMAP_ENTRIES],
		btmp[TCX_CMAP_ENTRIES] ;

	tcx_mutex_nassert(softc) ;

	if( count == 0 )
	  return 0 ;

	if( (rval = verify_cmap(index,count,
			cursor_cmap ? 2 : TCX_CMAP_ENTRIES)) )
	  return rval ;

	index &= PIX_ALL_PLANES ;

	tcx_mutex_enter(&softc->mutex) ;

	/* if an update is pending, wait for it */
	if( tcx_update_pending(softc) )
	  wait_for_interrupt(softc,TCX_CMAP_WAIT) ;

	read_colormap(softc,cursor_cmap,index,count,rtmp,gtmp,btmp) ;

	tcx_mutex_exit(&softc->mutex) ;

	if( DDI_COPYOUT(rtmp+index, red, count, copyflag)  ||
	    DDI_COPYOUT(gtmp+index, green, count, copyflag)  ||
	    DDI_COPYOUT(btmp+index, blue, count, copyflag))
	  return EFAULT ;

	return 0 ;
}


		/* copy colormap to buffer, then queue an update */
		/* TODO: 3-color cursor? */
		/* This function is NOT called within a mutex */
static	int
put_colormap(softc,cursor_cmap,index,count,red,green,blue,copyflag)
	register struct tcx_softc *softc ;
	int	cursor_cmap,index,count ;
	u_char	*red,*green,*blue ;
	int	copyflag ;
{
	int	i ;
register u_char	*rm,*gm,*bm ;
	int	rval ;
	u_long	hcmisc ;

	/* See notes on 1142190 at top of file */
	u_char	rtmp[TCX_CMAP_ENTRIES],
		gtmp[TCX_CMAP_ENTRIES],
		btmp[TCX_CMAP_ENTRIES] ;

	if( count == 0 )
	  return 0 ;

	tcx_mutex_nassert(softc) ;

	if( DDI_COPYIN(red, rtmp, count, copyflag)  ||
	    DDI_COPYIN(green, gtmp, count, copyflag)  ||
	    DDI_COPYIN(blue, btmp, count, copyflag))
	  return EFAULT ;

	if( (rval = verify_cmap(index,count,
			cursor_cmap ? 2 : TCX_CMAP_ENTRIES)) )
	  return rval ;

	index &= PIX_ALL_PLANES ;

	i = splbio4x() ;		/* BEGIN CRITICAL SECTION */
	tcx_mutex_enter(&softc->mutex) ;

	if( !cursor_cmap ) {
	  rm = softc->cmap_buffer[1] + index ;
	  gm = softc->cmap_buffer[2] + index ;
	  bm = softc->cmap_buffer[3] + index ;
	}
	else {
	  rm = softc->omap_buffer[1] + index ;
	  gm = softc->omap_buffer[2] + index ;
	  bm = softc->omap_buffer[3] + index ;
	}

	BCOPY(rtmp,rm,count) ;
	BCOPY(gtmp,gm,count) ;
	BCOPY(btmp,bm,count) ;

	/* set flags to indicate update required */
	if( !cursor_cmap ) {
	  rm = softc->cmap_buffer[0] + index ;
	  while(--count >= 0)
	    *rm++ = 1 ;
	}

	softc->vrtflag |= cursor_cmap ? TCXOMAPUD : TCXCMAPUD ;
	/* If we're in the middle of vblank, or vertical sync is turned
	 * off, we can load it now */
	/* TODO: check these numbers */
	hcmisc = HCMISC(softc) ;
	if( hcmisc & TCX_HCMISC_DISABLE_VSYNC  ||
	    hcmisc & TCX_HCMISC_VBLANK && LINECOUNT(softc) < MAX_VRT_LINE )
	{
	  tcx_update_cmaps(softc) ;	/* load it right now */
	}
	else {
	  /* enable interrupt so we can load the hardware colormap */
	  tcx_int_enable(softc);
	}

	(void) splx4x(i);		/* END CRITICAL SECTION */
	tcx_mutex_exit(&softc->mutex) ;

	return 0 ;
}



	/* set up control planes for emulation */

static	void
set_control_plane(softc,value)
	register struct tcx_softc *softc ;
	int	value ;
{
	u_long	*dfb32 ;
	register int len ;
	off_t	size ;

	if( (softc->capabilities & TCX_CAP_C_PLANES) )
	  return ;

	/* temporarily map rdfb32 */

	len = softc->_linebytes * softc->_h ;
	size = len * sizeof(u_long) ;
	if( ddi_map_regs(softc->dip, TCX_REG_RDFB32, (caddr_t *)&dfb32, 0,size)
		!= DDI_SUCCESS )
	{
	  tcx_printf("tcx: unable to map fb at line %d\n", __LINE__) ;
	  return ;
	}

	/* TODO: do this with stipple register instead? */

	{
	  register u_long *ptr = dfb32 ;

	  while( --len >= 0 ) {
	    *ptr = (*ptr & 0xffffff) | value ;
	    ++ptr ;
	  }
	}

	ddi_unmap_regs(softc->dip, TCX_REG_RDFB32, (caddr_t *)&dfb32, 0, size);
}



	/* macro to create variable "name" of type "type" pointing to
	 * ioctl argument.  Under 4.x, the data has already been copied,
	 * so we set "name" = "arg".  Under 5.x, we reserve space to hold
	 * the data and set "name" to point to that space; ARG_COPY* will
	 * move the data as appropriate.
	 *
	 * IOCTL_DATA(type,name)	defines name to be a pointer to type
	 * ARG_COPYIN(name)		brings data into name
	 * ARG_COPYOUT(name)		sends data back out
	 * IMM_COPYIN(dst,type)		copy arg to dst
	 * IMM_COPYOUT(src,type)	copy src to arg
	 */

#ifdef	SYSV
#define	IOCTL_DATA(type,name)	type temp ; register type *name = &temp
#define	E_INVALID	EINVAL
#else
#define	IOCTL_DATA(type,name)	register type *name = (type *)arg
#define	E_INVALID	ENOTTY
#endif	/* SYSV */

/*ARGSUSED*/
STATIC
tcx_ioctl(dev, cmd, arg, mode, credp, result)
	dev_t	dev;
	int	cmd;
	int	arg;
	int	mode;
	cred_t	*credp ;
	int	*result ;
{
	register struct tcx_softc *softc = getsoftc(TCXUNIT(dev));
	int	rval = 0 ;
	int	cursor_cmap;
	int	copyflag ;
	static union {
		u_char cmap[TCX_CMAP_ENTRIES];
		u_long cur[32];
	} iobuf;

	DEBUGF(3, ("tcx_ioctl(%d, 0x%x), pid=%d\n",
		TCXUNIT(dev), cmd, getpid()));

	/* default to updating normal colormap */
	cursor_cmap = 0;

	/* figure out if kernel or user mode call */
#ifdef	SYSV
	copyflag = mode & FKIOCTL ;
#else
	copyflag = fbio_kcopy ? FKIOCTL : 0;
#endif	/* SYSV */

	switch (cmd) {

	case FBIOGATTR: {
	    IOCTL_DATA(struct fbgattr,attr) ;

	    DEBUGF(3, ("FBIOGATTR, emu_type=%d\n", softc->emulation));
	    *attr = tcx_attr;
	    attr->owner = softc->owner == getpid() ? 0 : softc->owner;
	    attr->sattr.emu_type = softc->emulation;
	    attr->sattr.dev_specific[0] = softc->capabilities ;
	    attr->sattr.dev_specific[1] = (int) softc->maps[TCX_REG_DFB8].addr ;
	    attr->fbtype.fb_type = softc->emulation;
	    attr->fbtype.fb_width = softc->_w;
	    attr->fbtype.fb_height = softc->_h;
	    attr->fbtype.fb_depth = softc->em_depth ;
	    /* TODO: emulation-dependent? */
	    attr->fbtype.fb_size = softc->maps[TCX_REG_DFB8].size;
	    if( ARG_COPYOUT(attr) )
	      rval = EFAULT ;
	}
	break ;

	case FBIOGTYPE: {
		IOCTL_DATA(struct fbtype,fb) ;

		*fb = tcx_attr.fbtype;
		fb->fb_type = softc->emulation;
		fb->fb_width = softc->_w;
		fb->fb_height = softc->_h;
		fb->fb_depth = softc->em_depth ;
		/* TODO: emulation-dependent? */
		fb->fb_size = softc->maps[TCX_REG_DFB8].size;
		if( ARG_COPYOUT(fb) )
		  rval = EFAULT ;
	}
	break;

#if NWIN > 0
	case FBIOGPIXRECT: {
		IOCTL_DATA(struct fbpixrect,fbp) ;

		fbp->fbpr_pixrect = &softc->pr;

		DEBUGF(3, ("FBIOGPIXRECT\n"));
		/* initialize pixrect and private data */
		softc->pr.pr_ops = &tcx_pr_ops;
		/* pr_size set in attach */
		softc->pr.pr_depth = 8;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* md_linebytes, md_image set in attach */
		/* md_offset already zero */
		softc->prd.mpr.md_primary = TCXUNIT(dev);
		softc->prd.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
		softc->prd.planes = 255;

		/* enable video */
		tcx_set_video(softc, 1);
	}
	break;
#endif /* NWIN > 0 */


	case FBIOPUTCMAP: {
		IOCTL_DATA(struct fbcmap,cmap) ;

		if( ARG_COPYIN(cmap) ) {
		  rval = EFAULT; break ;
		}

		rval = put_colormap(softc, 0, cmap->index, cmap->count,
		      cmap->red, cmap->green, cmap->blue, copyflag) ;
	}
	break;

	case FBIOGETCMAP: {
		IOCTL_DATA(struct fbcmap,cmap) ;

		if( ARG_COPYIN(cmap) ) {
		  rval = EFAULT; break ;
		}

		rval = fetch_colormap(softc, 0, cmap->index, cmap->count,
			cmap->red, cmap->green, cmap->blue, copyflag) ;
	}
	break ;


	case FBIOSATTR: {
		IOCTL_DATA(struct fbsattr,attr) ;

		DEBUGF(3, ("FBIOSATTR\n"));
		if( ARG_COPYIN(attr) ) {
		  rval = EFAULT; break ;
		}

		tcx_mutex_enter(&softc->mutex) ;
		switch( attr->emu_type ) {
		  case FBTYPE_SUN3COLOR:
		    softc->emulation = attr->emu_type ;
		    softc->em_depth = 8 ;
		    set_control_plane(softc,0) ;
		    break ;
		  case FBTYPE_MEMCOLOR:
		    softc->emulation = attr->emu_type ;
		    softc->em_depth = softc->depth ;
		    set_control_plane(softc,0x1000000) ;
		    break ;
		  case FBTYPE_TCX:
		    softc->emulation = attr->emu_type ;
		    softc->em_depth = 24 ;
		    break ;
		  default:
		    rval = EINVAL ;
		}
		tcx_mutex_exit(&softc->mutex) ;
	}
	break ;


	case FBIOSVIDEO: {
		IOCTL_DATA(int,flag) ;

		if( ARG_COPYIN(flag) ) {
		  rval = EFAULT; break ;
		}

		DEBUGF(3, ("FBIOSVIDEO, flag=%x\n",*flag));

		tcx_mutex_enter(&softc->mutex) ;
		tcx_set_video(softc, (*flag & FBVIDEO_ON) ? 1 : 0) ;
		tcx_mutex_exit(&softc->mutex) ;
	}
	break;

	case FBIOGVIDEO: {
		IOCTL_DATA(int,flag) ;

		tcx_mutex_enter(&softc->mutex) ;
		*flag = tcx_get_video(softc) ? FBVIDEO_ON : FBVIDEO_OFF;
		tcx_mutex_exit(&softc->mutex) ;

		DEBUGF(3, ("FBIOGVIDEO, video=%d\n", *flag));

		if( ARG_COPYOUT(flag) )
		  rval = EFAULT;
	}
	break;

	case FBIOVERTICAL:
		tcx_mutex_enter(&softc->mutex) ;
		wait_for_interrupt(softc, TCXVRTIOCTL) ;
		tcx_mutex_exit(&softc->mutex) ;
		break ;

	/* HW cursor control */
	case FBIOSCURSOR: {
		IOCTL_DATA(struct fbcursor,cp) ;
		u_long image[32], mask[32] ;
		int set ;
		int cbytes;

		if( !(softc->capabilities & TCX_CAP_HW_CURSOR) ) {
		  rval = E_INVALID ;
		  break ;
		}

		if( ARG_COPYIN(cp) ) {
		  rval = EFAULT; break ;
		}

		set = cp->set;

		if (set & FB_CUR_SETSHAPE) {
		    if ((u_int) cp->size.x > 32 ||
			(u_int) cp->size.y > 32)
		    {
			rval = EINVAL;
			break ;
		    } else {
			/* compute cursor bitmap bytes */
			cbytes = cp->size.y *
				sizeof softc->cur.image[0];

			/* copy cursor image from user */
			if( DDI_COPYIN(cp->image, image, cbytes, copyflag))
			  { rval = EFAULT; break; }
			if( DDI_COPYIN(cp->mask, mask, cbytes, copyflag))
			  { rval = EFAULT; break; }
		    }
		}

		tcx_mutex_enter(&softc->mutex) ;
		if (set & FB_CUR_SETCUR)
			softc->cur.enable = cp->enable;

		if (set & FB_CUR_SETPOS)
			softc->cur.pos = cp->pos;

		if (set & FB_CUR_SETHOT)
			softc->cur.hot = cp->hot;

		if (set & FB_CUR_SETSHAPE) {
		    softc->cur.size = cp->size;

		    /* copy cursor image into softc */
		    BZERO(softc->cur.image, sizeof softc->cur.image);
		    BCOPY(image, softc->cur.image, cbytes);
		    BZERO(softc->cur.mask, sizeof softc->cur.mask);
		    BCOPY(mask, softc->cur.mask, cbytes);

		    /* load into hardware */
		    tcx_setcurshape(softc);
		}

		/* update hardware */
		tcx_setcurpos(softc);

		tcx_mutex_exit(&softc->mutex) ;

		/* load colormap */
		if( rval == 0 && (set & FB_CUR_SETCMAP) )
		{
		    rval = put_colormap(softc, 1, cp->cmap.index,
				cp->cmap.count, cp->cmap.red,
				cp->cmap.green, cp->cmap.blue, copyflag) ;
		}
		break;
	}

	case FBIOGCURSOR: {
		IOCTL_DATA(struct fbcursor,cp) ;
		int cbytes,index;

		if( !(softc->capabilities & TCX_CAP_HW_CURSOR) ) {
		  rval = E_INVALID ;
		  break ;
		}

		if( ARG_COPYIN(cp) ) {
		  rval = EFAULT; break ;
		}

		tcx_mutex_enter(&softc->mutex) ;
		cp->set = 0;
		cp->enable = softc->cur.enable;
		cp->pos = softc->cur.pos;
		cp->hot = softc->cur.hot;
		cp->size = softc->cur.size;

		/* compute cursor bitmap bytes */
		/* TODO: is this right?  What is the format for cursor
		   image? */
		cbytes = softc->cur.size.y * sizeof softc->cur.image[0];
		tcx_mutex_exit(&softc->mutex) ;

		if( cp->image &&
		    DDI_COPYOUT(softc->cur.image, cp->image, cbytes,copyflag) )
		{
		  rval = EFAULT ;
		  break ;
		}
		if( cp->mask &&
		    DDI_COPYOUT(softc->cur.mask, cp->mask, cbytes,copyflag) )
		{
		  rval = EFAULT ;
		  break ;
		}

		/* if color pointers are non-null copy colormap */
		if( cp->cmap.red && cp->cmap.green && cp->cmap.blue ) {
		    if( (rval = fetch_colormap( softc,1,
						cp->cmap.index,
						cp->cmap.count,
						cp->cmap.red,
						cp->cmap.green,
						cp->cmap.blue,
						copyflag)) )
			break ;
		}
		/* just trying to find out colormap size */
		/* TODO: 3-color colormap? */
		else {
		    cp->cmap.index = 0;
		    cp->cmap.count = 2;
		}

		if( ARG_COPYOUT(cp) ) {
		  rval = EFAULT; break ;
		}

		break;
	}

	case FBIOSCURPOS: {
		IOCTL_DATA(struct fbcurpos,cp) ;

		if( !(softc->capabilities & TCX_CAP_HW_CURSOR) ) {
		  rval = E_INVALID ;
		  break ;
		}

		if( ARG_COPYIN(cp) ) {
		  rval = EFAULT; break ;
		}
		tcx_mutex_enter(&softc->mutex) ;
		softc->cur.pos = *cp ;
		tcx_setcurpos(softc);
		tcx_mutex_exit(&softc->mutex) ;
		break;
	}

	case FBIOGCURPOS: {
		IOCTL_DATA(struct fbcurpos,cp) ;

		if( !(softc->capabilities & TCX_CAP_HW_CURSOR) ) {
		  rval = E_INVALID ;
		  break ;
		}
		tcx_mutex_enter(&softc->mutex) ;
		*cp = softc->cur.pos ;
		tcx_mutex_exit(&softc->mutex) ;
		if( ARG_COPYOUT(cp) )
		  rval = EFAULT;
		break;
	}

	case FBIOGCURMAX: {
		static struct fbcurpos curmax = { 32, 32 };

		if( !(softc->capabilities & TCX_CAP_HW_CURSOR) ) {
		  rval = E_INVALID ;
		  break ;
		}
		IMM_COPYOUT(curmax, struct fbcurpos) ;
		break;
	}

        /* informational ioctls */

        case FBIOGXINFO: {
		IOCTL_DATA(struct cg6_info,info) ;

		tcx_mutex_enter(&softc->mutex) ;
		info->accessible_width = softc->_w ;
		info->accessible_height = softc->_h ;
		info->line_bytes = softc->_w ;		/* TODO? */
		info->hdb_capable = 0 ;
		info->vmsize = softc->maps[TCX_REG_RDFB32].size ;
		info->boardrev = 0 ;
		info->slot = 0 ;
		tcx_mutex_exit(&softc->mutex) ;
		if( ARG_COPYOUT(info) )
		  rval = EFAULT ;
		break;
	}

        case FBIOMONINFO: {
		IOCTL_DATA(struct mon_info,info) ;
		tcx_mutex_enter(&softc->mutex) ;
		*info = softc->moninfo ;
		tcx_mutex_exit(&softc->mutex) ;
		if( ARG_COPYOUT(info) )
		  rval = EFAULT ;
                break ;
	}

	/* vertical retrace interrupt */

	case FBIOVRTOFFSET: {
		static int vaddr_vrt = TCX_VADDR_VRT ;
		IMM_COPYOUT(vaddr_vrt, int) ;
		break ;
	}

#ifdef	VIS_GETIDENTIFIER
	case VIS_GETIDENTIFIER:
	{
		static struct vis_identifier vis_id = {"SUNWtcx"} ;
		IMM_COPYOUT(vis_id, struct vis_identifier) ;
		break;
	}
#endif

#if TCXDEBUG
#ifdef	POWER_MGT
#ifdef	SYSV
	case 254:
#else
	case _IOW(F, 254, int):
#endif
	{
		IOCTL_DATA(int,level) ;

		if( ARG_COPYIN(level) ) {
		  rval = EFAULT; break ;
		}

		tcx_power(softc->dip, 1, *level) ;
		break ;
	}
#endif	POWER_MGT

#ifdef	SYSV
	case 255:
#else
	case _IOW(F, 255, int):
#endif
		IMM_COPYIN(tcx_debug, int) ;
		if( tcx_debug == -1 )
		  tcx_debug = TCXDEBUG ;
		tcx_mem_debug = (tcx_debug >> 8) & 0xff ;
		tcx_mutex_debug = (tcx_debug >> 16) & 0xff ;
		tcx_debug &= 0xff ;
		tcx_printf("tcx_debug is now %d.%d.%d\n",
			tcx_debug,tcx_mem_debug,tcx_mutex_debug);
		break ;
#endif

	default:
		rval = E_INVALID;
	}

	return rval;
}



#ifndef	SYSV
static
tcx_poll()
{
	register int i, serviced = 0;
	register struct tcx_softc *softc;

	/*
	 * Look for any frame buffers that were expecting an interrupt.
	 */

	DEBUGF(7, ("tcx_poll\n"));
	for (softc = tcx_softc, i = ntcx; --i >= 0; softc++)
		if (tcx_int_pending(softc)) {
			if( softc->vrtflag)
				tcx_intr((caddr_t) softc);
			else
				/* XXX catch stray interrupts? */
				tcx_int_disable(softc);
			serviced++;
		}

	return serviced;
}
#endif	/* !SYSV */

static u_int
tcx_intr(arg)
	caddr_t	arg ;
{
	register struct tcx_softc *softc = (struct tcx_softc *)arg ;
	u_long *in;
	u_long *out;
	u_long tmp;
	void	hat_unload();
	volatile u_long *hcmisc = &HCMISC(softc) ;


	DEBUGF(7, ("tcx_intr(%d), hcmisc=%x, vrtflags=%x\n",
		softc->unit, *hcmisc, softc->vrtflag));

	if( !softc->initialized || !tcx_int_pending(softc) )
	  return DDI_INTR_UNCLAIMED ;

	tcx_mutex_enter(&softc->mutex) ;

	tcx_int_disable(softc);

	DEBUGF(8, ("  %d: hcmisc=%x, vrtflags=%x\n",
		__LINE__, *hcmisc, softc->vrtflag));

	if( softc->vrtflag & TCXCURUPD ) {
	  DEBUGF(8, ("  %d: updating cursor, hcmisc=%x, vrtflags=%x\n",
		  __LINE__, *hcmisc, softc->vrtflag));
	  softc->vrtflag &= ~TCXCURUPD ;
	  tcx_setcurshape(softc);
	  tcx_setcurpos(softc);
	}

	if (softc->vrtflag & (TCXVRTIOCTL|TCX_CMAP_WAIT)) {
	    softc->vrtflag &= ~(TCXVRTIOCTL|TCX_CMAP_WAIT) ;
	    tcx_wakeup(softc) ;
	}

	if( tcx_update_pending(softc) )
	  tcx_update_cmaps(softc) ;

	DEBUGF(8, ("  %d: hcmisc=%x, vrtflags=%x\n",
		__LINE__, *hcmisc, softc->vrtflag));

	if (softc->vrtflag & TCXVRTCTR) {
	    register int *vrtpage = softc->vrtpage ;
	    if( vrtpage == NULL ) {
		softc->vrtflag &= ~TCXVRTCTR;
	    } else
		*vrtpage += 1;
	}


	if (softc->vrtflag)
	    tcx_int_enable(softc);

	DEBUGF(8, ("  %d: hcmisc=%x, vrtflags=%x\n",
		__LINE__, *hcmisc, softc->vrtflag));

	tcx_mutex_exit(&softc->mutex) ;
	return DDI_INTR_CLAIMED ;
}



/*
 * enable/disable/update HW cursor
 */
static
tcx_setcurpos(softc)
	struct tcx_softc *softc;
{
	CURSOR_ADDRESS(softc) = softc->cur.enable ?
		((softc->cur.pos.x - softc->cur.hot.x) << 16) |
			((softc->cur.pos.y - softc->cur.hot.y) & 0xffff) :
		TCX_CURSOR_OFFPOS;
}

/*
 * load HW cursor bitmaps
 * Note: I've liberalized this somewhat; caller is now allowed to
 * put bits in image data where there are none in mask, if that's what
 * they want.
 */
static
tcx_setcurshape(softc)
	struct tcx_softc *softc;
{
	u_long tmp, edge = 0;
	u_long *image, *mask ;
	volatile u_long *hw;
	int i;

	/* compute right edge mask */
	if (softc->cur.size.x)
		edge = (u_long) ~0 << (32 - softc->cur.size.x);

	image = softc->cur.image;
	mask = softc->cur.mask;
	hw = THC(softc,TCX_THC_CURSORA00) ;

	for (i = 0; i < 32; i++) {
		hw[i] = mask[i] & edge ;
		hw[i + 32] = image[i] & edge ;
	}
}

static	void
tcx_reset_reg(softc)
	struct tcx_softc *softc ;
{
	if( softc->maps[TCX_REG_THC].addr == NULL  ||
	    softc->maps[TCX_REG_CMAP].addr == NULL )
	  return ;

	/* turn off interrupts */
	tcx_int_disable(softc) ;

	tcx_set_vidstate(softc, softc->vidstate|VS_SYNC) ;

	/* disable HW cursor */
	if( softc->capabilities & TCX_CAP_HW_CURSOR )
	  CURSOR_ADDRESS(softc) = TCX_CURSOR_OFFPOS;

	/* reprogram DAC to enable HW cursor use */
	{
		Tcx_Cmap *cmap = (Tcx_Cmap *)softc->maps[TCX_REG_CMAP].addr ;

		/* command register */
		cmap->addr = 6 << 24;

		/* turn on CR1:0, overlay enable */
		cmap->ctrl = cmap->ctrl | (0x3 << 24);
		cmap->addr = 0 ;		/* A24 seems to require this */
	}
}


#ifdef	SYSV
static	int
tcx_reset(dip, cmd)
	dev_info_t *dip ;
	ddi_reset_cmd_t cmd ;
{
	int	unit ;
	struct tcx_softc *softc ;

	if( cmd != DDI_RESET_FORCE )
	  return DDI_FAILURE ;

	unit = ddi_get_instance(dip) ;
	softc = getsoftc(unit) ;
	tcx_mutex_enter(&softc->mutex) ;

	tcx_reset_reg(softc) ;

	tcx_mutex_exit(&softc->mutex) ;
	return DDI_SUCCESS ;
}
#endif	/* SYSV */


#if NOHWINIT
tcx_init(softc)
	struct tcx_softc *softc ;
{
	/* Initialize DAC, TODO: use ctrl2? */
	{
		register Tcx_Cmap *cmap ;
		register char *p;

		/* initialize DAC */
		GET_MAP(softc, TCX_REG_DHC, TCX_DHC_SZ);
		cmap = (Tcx_Cmap *)softc->maps[TCX_REG_DHC].addr;

		cmap->addr = 4<<24 ;		/* read mask = 1's */
		cmap->ctrl2 = 0xffffffff ;
		cmap->addr = 5<<24 ;		/* blink maks = 0's */
		cmap->ctrl2 = 0 ;
		cmap->addr = 6<<24 ;		/* cr0 = overlay enabled */
		cmap->ctrl2 = 0x73<<24 ;
		cmap->addr = 7<<24 ;		/* test0 = 0 */
		cmap->ctrl2 = 0 ;
		cmap->addr = 8<<24 ;		/* cr1 = 0  unused */
		cmap->ctrl2 = 0 ;
		cmap->addr = 9<<24 ;		/* cr2 = 0 */
		cmap->ctrl2 = 0 ;
		cmap->addr = 11<<24 ;		/* test1 = 0 */
		cmap->ctrl2 = 0 ;
		if( softc->capabilities & TCX_CAP_24_BIT ) {
		  cmap->addr = 16<<24 ;		/* cr3 = enable true color */
		  cmap->ctrl2 = 0xa0<<24 ;
		  cmap->addr = 17<<24 ;		/* cr4 = 8/8/8 format, 64 pins*/
		  cmap->ctrl2 = 0xb2<<24 ;
		}

		cmap->addr = 0 ;		/* overlay colors */
		cmap->omap = 0xff0000ff ;
		cmap->omap = 0xff0000ff ;
		cmap->omap = 0xff0000ff ;
		cmap->addr = 1<<24 ;
		cmap->omap = 0xff0000ff ;
		cmap->omap = 0xff0000ff ;
		cmap->omap = 0xff0000ff ;
		cmap->addr = 2<<24 ;
		cmap->omap = 0xff0000ff ;
		cmap->omap = 0xff0000ff ;
		cmap->omap = 0xff0000ff ;
		cmap->addr = 3<<24 ;
		cmap->omap = 0 ;
		cmap->omap = 0 ;
		cmap->omap = 0 ;
		cmap->addr = 0 ;		/* A24 seems to require this */
	}

	/* Initialize THC */
	{
		volatile u_long *thc = (u_long *)softc->maps[TCX_REG_THC].addr ;
		volatile u_long *hcmisc = thc + TCX_THC_HCMISC ;
		volatile u_long *strap = thc + TCX_THC_STRAPPING ;
		volatile u_long *delay = thc + TCX_THC_DELAY ;
		int vidon;

		*delay = 0x00011101 ;	/* set video and cas delays=1 (2ns) */

		/* Assume 1152x900@66 */

		/* 1. Reload STRAP */

		*strap = 0x004101d4 ;	/* 0x004101d4 for 30MHz rev0 ASIC */

		/* 2. Reset Video Timing Generator */

		vidon = tcx_get_video(softc) ;
		*hcmisc = 0x0000000f ;	/* disable sync */
		*hcmisc = 0x0000100f ;	/* reset video (?) */
		*hcmisc = 0x0000000f ;	/* then write 0 */

		/* load THC Timing register */

		thc[TCX_TEC_HSS]	= 9 ;
		thc[TCX_TEC_HSE]	= 0x29 ;
		thc[TCX_TEC_HDS]	= 0x5d ;
		thc[TCX_TEC_HSEDVS]	= 0x167 ;
		thc[TCX_TEC_HDE]	= 0x17d ;
		thc[TCX_TEC_VSS]	= 1 ;
		thc[TCX_TEC_VSE]	= 5 ;
		thc[TCX_TEC_VDS]	= 0x24 ;
		thc[TCX_TEC_VDE]	= 0x3a8 ;


		/* 6. start up video timing */

#ifdef	COMMENT
		if (vidon)
#endif	/* COMMENT */
		  *hcmisc = 0x00002486 ;
#ifdef	COMMENT
		else
		  *hcmisc = 0x00002086 ;
#endif	/* COMMENT */

		thc[TCX_THC_CURSOR_ADDRESS] = 0xffe0ffe0 ;  /* -32,-32 */

		if( softc->capabilities & TCX_CAP_24_BIT )
		{
		  volatile u_long *alt ;
		  int i ;
		  GET_MAP(softc, TCX_REG_ALT, TCX_ALT_SZ);
		  alt = (u_long *)softc->maps[TCX_REG_ALT].addr ;

		  set_alt(alt, 0, 2) ;
		  set_alt(alt, 1, 0) ;
		  set_alt(alt, 2, 0) ;
		  set_alt(alt, 3, 2) ;
		  set_alt(alt, 4, 8) ;
		  set_alt(alt, 5, 1) ;
		  set_alt(alt, 6, 0) ;
		  set_alt(alt, 7, 0) ;
		  set_alt(alt, 8, 5) ;
		  set_alt(alt, 9, 0xa) ;
		  set_alt(alt, 10, 0) ;
		  set_alt(alt, 11, 1) ;
		  set_alt(alt, 12, 0) ;
		  set_alt(alt, 13, 0) ;
		  set_alt(alt, 14, 0) ;
		  set_alt(alt, 15, 0) ;
		  for (i=0; i< 32; i++ ) set_alt(alt,13,0);
		}


#ifdef	TODO
		DEBUGF(1, ("TEC rev %d\n",
			(*hcmisc & TCX_HCMISC_CHIPREV) >>
				TCX_HCMISC_CHIPREV_SHIFT));
#endif	/* TODO */
	}
}

set_alt(addr,which,value)
	u_long	*addr ;
	int	which ;
	int	value ;
{
	addr += which ;
	value |= value<<4 ;
	value |= value<<8 ;
	value |= value<<16 ;
	*addr = value ;
}
#endif NOHWINIT




	/* 5.x loadable driver starts here */
#ifdef	SYSV

static	struct cb_ops tcx_cb_ops = {
	  tcx_open,	/* open */
	  tcx_close,	/* close */
	  nodev,	/* strategy */
	  nodev,	/* print */
	  nodev,	/* dump */
	  nulldev,	/* read */
	  nulldev,	/* write */
	  tcx_ioctl,	/* ioctl */
	  nodev,	/* devmap */
	  tcx_mmap,	/* mmap */
	  ddi_segmap,	/* segmap */
	  nochpoll,	/* chpoll */
	  ddi_prop_op,	/* prop_op */
	  NULL,		/* streams info */
	  D_NEW|D_MP,	/* flags, see <sys/conf.h> */
	} ;


static	struct dev_ops tcx_ops = {
	  DEVO_REV,		/* revision */
	  0,			/* ref count */
	  tcx_getinfo,		/* info */
	  tcx_identify,		/* identify */
	  nulldev,		/* probe */
	  tcx_attach,		/* attach */
	  tcx_detach,		/* detach */
	  tcx_reset,		/* reset */
	  &tcx_cb_ops,		/* driver operations */
	  (struct bus_ops *)0,	/* bus operations */
#ifdef	POWER_MGT
	  tcx_power,
#endif	POWER_MGT
	};

extern	struct mod_ops	mod_driverops ;

static	struct modldrv	modldrv = {
	  &mod_driverops,
	  "tcx driver 1.1",
	  &tcx_ops,
	} ;

static	struct modlinkage modlinkage = {
	  MODREV_1,
	  &modldrv,
	  0,
	} ;

int
_init()
{
	register int	e ;
	int		i ;

	if( (e=ddi_soft_state_init(&statep, sizeof(struct tcx_softc), 1)) != 0 )
	  return e ;

	if( (e=mod_install(&modlinkage)) != 0 )
	{
	  ddi_soft_state_fini(&statep) ;
	  return e ;
	}

	/* TODO: other module initialization as needed */
	i = pagesize = ptob(1) ;
	pageoffset = pagesize-1 ;
	for(pageshift=0; (i>>=1) != 0; ++pageshift);

	return e ;
}

int
_fini()
{
	register int	e ;

#ifdef	COMMENT
	if( /* some reason to veto unload */ )
	  return EBUSY ;
#endif	/* COMMENT */

	if( (e=mod_remove(&modlinkage)) != 0 )
	  return e ;

	/* TODO: any driver-specific shutdown.  free resources, etc. */

	ddi_soft_state_fini(&statep) ;

	ASRT(total_memory == 0) ;

	return e ;
}

_info(modinfop)
	struct modinfo *modinfop ;
{
	return mod_info(&modlinkage, modinfop) ;
}

#endif	/* !SYSV */




	/* 4.x loadable driver starts here */
#ifndef	SYSV

struct dev_ops tcx_ops = {
	0,		/* revision */
	tcx_identify,
	tcx_attach,
	tcx_open,
	tcx_close,
	0, 0, 0, 0, 0,
	tcx_ioctl,
	0,
	0,
};


#ifdef	LOADABLE
/* structures for loadable driver */

static	struct cdevsw	tcx_cdev = {
	  tcx_open,	tcx_close,	nulldev,	nulldev,
	  tcx_ioctl,	nulldev,	nulldev,	tcx_mmap,
	  0,		0,
	} ;

#ifdef	sun4m
static	struct vdldrv vd = {
	  VDMAGIC_DRV,	/* Type of module */
	  "tcx",	/* Name of module */
	  &tcx_ops,	/* dev ops vector */
	  NULL,		/* no bdevsw */
	  &tcx_cdev,	/* cdevsw */
	  0,0,		/* major device #.  0 means let system choose */
	  NULL,		/* mb_ctlr structure */
	  NULL,		/* mb_driver structure */
	  NULL,		/* mb_device array */
	  0,		/* number of controllers */
	  1,		/* number of devices */
	} ;
#else
static	struct vdldrv vd = {
	  VDMAGIC_DRV,	/* Type of module */
	  "tcx",	/* Name of module */
#ifdef	sun4c
	  &tcx_ops,	/* dev ops vector */
#else
	  NULL,		/* mb_ctlr structure */
	  NULL,		/* mb_driver structure */
	  NULL,		/* mb_device array */
	  0,		/* number of controllers */
	  1,		/* number of devices */
#endif	/* sun4c */
	  NULL,		/* no bdevsw */
	  &tcx_cdev,	/* cdevsw */
	  0,0,		/* major device #.  0 means let system choose */
	} ;

#endif	/* sun4m */


int
xxxinit(function_code, vdp, vdi, vds)
	u_int		function_code ;
	struct vddrv	*vdp ;
	addr_t		vdi ;
	struct vdstat	vds ;
{
	switch( function_code )
	{
	  case VDLOAD:
	    vdp->vdd_vdtab = (struct vdlinkage *) &vd ;
	    return 0 ;

	  case VDUNLOAD:
	    return tcx_unload() ;

	  case VDSTAT:
	    return 0 ;

	  default:
	    return EIO ;
	}
}

static	int
tcx_unload()
{
	register int i ;
	register struct tcx_softc *softc;

	/* veto if any tcx is open, or interrupt pending */

	DEBUGF(7, ("tcx_unload\n"));
	for( softc = tcx_softc, i = ntcx; --i >= 0; softc++ ) {
	  ASRT((softc->vrtflag & ~(TCXCMAPUD|TCXOMAPUD)) == 0 ) ;
	  if( softc->owner != -1  ||  HCMISC(softc) & TCX_HCMISC_EN_IRQ )
	    return EBUSY ;

	  tcx_unmap_all(softc) ;
	}
	tcx_kmem_free(tcx_softc, sizeof(struct tcx_softc) * ntcx);

	ASRT(total_memory == 0) ;

	ntcx=0 ;
	next_unit=0 ;

	return 0 ;
}

#endif	/* LOADABLE */
#endif	/* !SYSV */
