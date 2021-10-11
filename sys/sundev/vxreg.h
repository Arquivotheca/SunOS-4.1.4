/* @(#)vxreg.h 1.1 94/10/31 Sun Microsystems Visualization Products */

/*
 * Copyright 1990, Sun Microsystems, Inc.
 *
 */

#ifndef	vxreg_DEFINED
#define	vxreg_DEFINED

#include <sys/param.h>

/*
 * VX, Visualization Accelerator system frame buffer, hardware definitions.
 *
 *	Using "cgvx" or "VX" as prefix to many definitions since this data
 *	is the VX equivalent to the "cgsix"/"cg6" definitions found in
 *	"cg6reg.h".
 */

/* Physical frame buffer and color map addresses */
/*
 * The base address is defined in the configuration file, e.g. GENERIC.
 * These constants are the offset from that address.
 */
#define VX_LEGO		0x1500000
#define VX_ADDR_ROM	0x1400000
#define VX_ADDR_FBC	0x1500000
#define	VX_ADDR_TEC	0x1501000
#define VX_ADDR_FHC	0x1580000
#define VX_ADDR_THC	0x1581000
#define VX_ADDR_COLOR	0x1600000
#define VX_ADDR_CMAP	VX_VIDBT
#define	VX_ADDR_VIDCSR	VX_VIDCSR
#define VX_ADDR_VIDCTL	VX_VIDCSR
#define VX_ADDR_VMECTL	VX_VMECTL
#define	VX_ADDR_VIDPAC	VX_VIDPAC
#define	VX_ADDR_VIDMIX	VX_VIDMIX

#define VX_ADDR_FBCTEC	VX_ADDR_FBC
#define VX_ADDR_FHCTHC	VX_ADDR_FHC	

#define VX_CMAP_SZ	(8*1024)	/* 0x002000 */
#define VX_FBCTEC_SZ	(8*1024)	/* 0x002000 */
#define VX_FHCTHC_SZ	(8*1024)	/* 0x002000 */
#define VX_ROM_SZ	(64*1024)	/* 0x010000 */
#define VX_FB_SZ	(2*1024*1024)
#define	VX_VIDPAC_SZ	(8*1024)
#define	VX_VIDMIX_SZ	(8*1024)
#define	VX_VMECTL_SZ	(8*1024)
#define	VX_VIDCSR_SZ	(8*1024)
/*
 * TODO
 * XXX: Currently, VX size is 32MB, MVX size is 32MB, but there is
 *	a 32MB space between them!!
 */
#define	VX_VX_SZ	(96*1024*1024)

/*
 * Offsets of TEC/FHC into page
 */
#define VX_TEC_POFF	0x01000
#define VX_THC_POFF	0x01000

/*
 *	Used by pixrects.
 */ 

/*
 * Virtual (mmap offsets) addresses
 */ 
#define VX_VBASE	0x70000000
#define VX_VADDR(x)	(VX_VBASE + (x) * 8192)

/*
 * VX Virtual object addresses (offsets)
 */
#define VX_VADDR_FBC	(VX_VADDR(0))
#define	VX_VADDR_TEC	(VX_VADDR_FBC   + VX_TEC_POFF)
#define VX_VADDR_CMAP	(VX_VADDR(1))
#define VX_VADDR_FHC	(VX_VADDR(2))
#define	VX_VADDR_THC	(VX_VADDR_FHC   + VX_THC_POFF)
#define VX_VADDR_ROM	(VX_VADDR(3))
#define VX_VADDR_COLOR	(VX_VADDR(3) + VX_ROM_SZ)
#define VX_VADDR_VIDCSR	(VX_VADDR_COLOR + VX_FB_SZ)
#define VX_VADDR_VMECTL	(VX_VADDR_VIDCSR+ VX_VIDCSR_SZ)
#define	VX_VADDR_VIDPAC	(VX_VADDR_VMECTL+ VX_VMECTL_SZ)
#define	VX_VADDR_VIDMIX	(VX_VADDR_VIDPAC+ VX_VIDPAC_SZ)
#define VX_VADDR_VX	0x30000000

/*
 * to map in all of LEGO, use mmapsize below, and offset VX_VBASE
 */
#define VX_MMAPSIZE(dfbsize)	(VX_VADDR_VIDCSR-VX_VBASE+VX_VIDCSR_SZ)

/*
 * convert from address returned by pr_makefromfd (eg. mmap) 
 * to VX register set.
 */
#define VXVA_TO_FBC(base) \
	((struct fbc*)  (((char*)base)+(VX_VADDR_FBC-VX_VBASE)))
#define VXVA_TO_TEC(base)  \
	((struct tec*)  (((char*)base)+(VX_VADDR_TEC-VX_VBASE)))
#define VXVA_TO_FHC(base)  \
	((u_int*) 	(((char*)base)+(VX_VADDR_FHC-VX_VBASE)))
#define VXVA_TO_THC(base)  \
	((struct thc*)  (((char*)base)+(VX_VADDR_THC-VX_VBASE)))
#define VXVA_TO_DFB(base)  \
	((short*) 	(((char*)base)+(VX_VADDR_COLOR-VX_VBASE)))
#define VXVA_TO_ROM(base)  \
	((u_int*)	(((char*)base)+(VX_VADDR_ROM-VX_VBASE)))

#define VXVA_TO_CMAP(base) \
	((u_int*) (((char*)base)+(VX_VADDR_CMAP-VX_VBASE)))

/* number of colormap entries */
#define VX_CMAP_ENTRIES	256


/* VIDEO RELATED MEMORY AND REGISTERS */

/* base addresses of the functional units */
#define VX_VMECTL    (0x01880000)      /* VME cntl          */
#define VX_VIDCSR    (0x01a00000)      /* Video cntl/status */
#define VX_VIDBT     (0x01a80000)      /* Brooktree DACs    */
#define VX_VIDPAC    (0x01b00000)      /* PAC 1000 registers*/
#define VX_VIDMIX    (0x01b80000)      /* Video mixer asic  */
 
/* Brooktree 461/462 RAMDAC internals 
 *	all addresses relative to VX_VIDBT
 */
#define VX_BTLEGO	(0)	/* Lego palette */
#define VX_BTOVERLAY	(0x100)	/* 0-31 overlay/cursor colors */
#define VX_BTID	(0x200)	/* BT461 ID register */
#define VX_BTCR0	(0x201)	/* BT461 command reg 0   */
#define VX_BTCR1	(0x202)	/* BT461 command reg 1   */
#define VX_BTCR2	(0x203)	/* BT461 command reg 2   */
#define VX_BTRDMSKLO	(0x204)	/* BT461 read mask reg 8 LSBs */
#define VX_BTRDMSKHI	(0x205)	/* BT461 read mask reg MSBs */
#define VX_BTBKMSKLO	(0x206)	/* BT461 blink mask reg 8 LSBs*/
#define VX_BTBKMSKHI	(0x207)	/* BT461 blink mask reg MSBs */
#define VX_BTOVRDMASK (0x208)	/* BT461 overlay read mask */
#define VX_BTOVBKMASK	(0x209)	/* BT461 overlay blink mask */
#define VX_BTTEST	(0x20C)	/* BT461 test register */
#define VX_BTPALETTE0 (0x400)	/* jet palette 0 */
#define VX_BTPALETTE1 (0x500)	/* jet palette 1 */
#define VX_BTPALETTE2 (0x600)	/* jet palette 2 */
#define VX_BTPALETTE3 (0x700)	/* jet palette 3 */

/* registers within the video mixer asic */
#define VX_VIDLM0    (VX_VIDMIX + 0x00)   /* vid match register 0 */
#define VX_VIDLM1    (VX_VIDMIX + 0x04)   /* vid match register 1 */
#define VX_VIDLM2    (VX_VIDMIX + 0x08)   /* vid match register 2 */
#define VX_VIDLM3    (VX_VIDMIX + 0x0c)   /* vid match register 3 */
#define VX_VIDLL0    (VX_VIDMIX + 0x10)   /* vid lookup register 0 */
#define VX_VIDLL1    (VX_VIDMIX + 0x14)   /* vid lookup register 1 */
#define VX_VIDLL2    (VX_VIDMIX + 0x18)   /* vid lookup register 2 */
#define VX_VIDLL3    (VX_VIDMIX + 0x1c)   /* vid lookup register 3 */
#define VX_VIDAMASK  (VX_VIDMIX + 0x20)   /* vid alpha mask value  */
#define VX_VIDMIXCSR (VX_VIDMIX + 0x24)   /* vid mix asic control/status*/

/* alternates for addressing match and lookup */
#define VX_VIDMATCH  (VX_VIDMIX + 0x00)   /* match base */
#define VX_VIDLOOKUP (VX_VIDMIX + 0x10)   /* lookup base */

/*
#define VXDELAY(c, n)    \
{ \
        register int N = n; \
        while (--N > 0) { \
                if (c) \
                        break; \
                usec_delay(1); \
        } \
	if (N==0) printf ("cgvx: conditional delay error\n"); \
}
*/

#define VXDELAY(c, n)    { while (c) ; }

/* video mixer stuff */

#define HOR1280 0x1000
#define HOR1152 0x0800

struct cgvx_vidmix { /* registers within the video mixer asic */
	int vidlm0    ;   /* vid match register 0 */
	int vidlm1    ;   /* vid match register 1 */
	int vidlm2    ;   /* vid match register 2 */
	int vidlm3    ;   /* vid match register 3 */
	int vidll0    ;   /* vid lookup register 0 */
	int vidll1    ;   /* vid lookup register 1 */
	int vidll2    ;   /* vid lookup register 2 */
	int vidll3    ;   /* vid lookup register 3 */
	int vidamask  ;   /* vid alpha mask value  */
	int vidmixcsr ;   /* vid mix asic control/status*/
};

struct cgvx_vidctl { 	/* registers within the PAC-1000 video controller */
    unsigned int vc_temp;	/* reg00 Just tells pac to start up */
    unsigned int vc_h_syncsize;	/* reg01 Horiz sync size - 4  */
    unsigned int vc_line;	/* reg02 Total line time - sync - 4 */
    unsigned int vc_h_bporch;	/* reg03 Horiz backporch - 4 */
    unsigned int vc_h_active;	/* reg04 Horiz active time - 4 */
    unsigned int vc_h_fporch;	/* reg05 Horiz frontporch - 4 */
    unsigned int vc_csynclo;	/* reg06 CSYNC low during vertical sync - 4 */
    unsigned int vc_v_fporch;	/* reg07 # of lines of vertical frontporch-1 */
    unsigned int vc_v_sync;	/* reg08 # of lines of vertical sync - 1 */
    unsigned int vc_v_bporch;	/* reg09 # of lines of vertical backporch - 1 */
    unsigned int vc_v_active;	/* reg10 # of active lines - 1 */
    unsigned int vc_pad0[11];
    unsigned int vc_displaymode;/* reg22 Display mode */
    unsigned int vc_pad1[2];
    unsigned int vc_casoffset;	/* reg25 Line to line CAS offset */
    unsigned int vc_pad2[4];
    unsigned int vc_ras_tof;	/* reg30 TOF RAS address */
    unsigned int vc_cas_tof;	/* reg31 TOF CAS address * 2 + pan_by_4 */

} ;

struct cgvx_vidctl4 { 	/* registers within the PAC-1000 video controller */
  unsigned int vc_go;	       /* reg00 Just tells pac to start up */
  unsigned int vc_pad0[10];
  unsigned int vc_h_fporch;    /* R11 - Horiz frontporch - 4 */
  unsigned int vc_h_syncsize;  /* R12 - Horiz sync size - 4 */
  unsigned int vc_h_bporch;    /* R13 - Horiz backporch - 4 */
  unsigned int vc_h_active;    /* R14 - Horiz active time - 4 */
  unsigned int vc_csynclo;     /* R15 - Csync low during VSYNC 
				    (after H sync high)-4 or 1/2 H-8 (NTSC) */
  unsigned int vc_h_eqsize;    /* R16 - (NTSC) Horizontal equalization 
				    pulse size - 4*/
  unsigned int vc_v_fporch;    /* R17 - # of lines of vertical frontporch - 1 */
  unsigned int vc_v_sync;      /* R18 - # of lines of vertical sync - 1 */
  unsigned int vc_v_bporch;    /* R19 - # of lines of vertical backporch - 1 */
  unsigned int vc_v_active;    /* R20 - # of active lines - 1 */
  unsigned int vc_ras_tof;     /* R21 - Top of frame #1 RAS 
					address + (2**10 * bank select) */
  unsigned int vc_cas_tof;     /* R22 - Top of frame #1 CAS
					address * 2 + (1 for pan_by_4) */
  unsigned int vc_ras_tof2;    /* R23 - Top of frame #2 RAS address (see R30) */
  unsigned int vc_cas_tof2;    /* R24 - Top of frame #2 CAS address (see R31) */
  unsigned int vc_casoffset;   /* R25 - Line to line CAS offset (= CAS * 2) */
  unsigned int vc_user1;       /* R26 - User flag turn on (high) line number*/
  unsigned int vc_user2;       /* R27 - User flag turn off (low) line number*/
  unsigned int vc_stereo_on;   /* R28 - Stereo output turn on (high)
								line number*/
  unsigned int vc_stereo_off;  /* R29 - Stereo output turn off (low)
								line number*/
  unsigned int vc_status;      /* R30 - Status request code (CC7 = 1)*/
  unsigned int vc_displaymode; /* R31 - Display mode/submode */

} ;

static struct cgvx_vidctl4 cgvx_vidfmt4[]={
/* 1280 76Hz */{
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,  0x00,  0x04,  0x20,  0x9c,  0x92,
	        0,  0x01,  0x07,  0x1f, 0x3ff,  0x00,  0x00,     0,
		0, 0x140,0xffac,0xfffe,     0,     0,     0,     0,
	       },
/* 1152 66Hz */{
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,  0x00,  0x0c,  0x14,  0x8c,  0x96,
	        0,  0x01,  0x03,  0x1e, 0x383,  0x00,  0x00,     0,
		0, 0x120,0xffb6,0xfffe,     0,     0,     0,     0,
	       },
/* 1280 67Hz */{
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,  0x00,  0x0c,  0x18,  0x9c,  0x80,
	        0,  0x05,  0x07,  0x24, 0x3ff,  0x00,  0x00,     0,
		0, 0x140,0xffac,0xfffe,     0,     0,     0,     0,
	       },
/* NTSC      */{0,     0,     0,     0,     0,     0,     0,     0,   
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,     0,     0,     0,     0,     0,   
	       },
/* USER      */{0,     0,     0,     0,     0,     0,     0,     0,   
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,     0,     0,     0,     0,     0,
		0,     0,     0,     0,     0,     0,     0,     0,   
	       },
};


struct cgvx_viddata {
        short   HORZ, VFP, VSYN, VBP, VVIS, HSEDVS;
        long    hchs, hchsdvs, hchd;
};

static struct cgvx_viddata cgvx_vid_data[] = {
        /*       HORZ    VFP VSYN VBP  VVIS HSEDVS  hchs  hchsdvs    hchd */
/* 1280 @ 76Hz */{HOR1280, 2,   9,  32, 1024, 42, 0x0002, 0x290000, 0x0b0033},
/* 1152 @ 76Hz */{HOR1152, 2,   5,  31,  900, 44, 0x0004, 0x2b0000, 0x0a002e},
/* 1280 @ 67Hz */{HOR1280, 6,   9,  37, 1024, 00, 0x0004, 0x240000, 0x0b0033},
/* NTSC        */{      0, 0,   0,   0,    0, 00,      0,        0,        0},
/* USER        */{      0, 0,   0,   0,    0, 00,      0,        0,        0},
};


/*
 *  This should be in /usr/include/sun/fbio.h  !!!!!
 */
#define FBRESET _IO(F, 48)
#define FBVIDEOFMT _IOW(F, 49, caddr_t)
#define FB_ATTR_CG6_TYPE	3

/* commands for the FBIOVERTICAL ioctl, these should also be someplace else */

#define	VX_VRTWAIT	1	/* sleep until VRT hits */
#define	VX_VRTSTART	2	/* start sending VRT signals to user */
#define	VX_VRTSTOP	4	/* stop sending VRT signals to user */

/* Video formats supported by the VX
 */
#define VX_VIDFMT_unknown   -1
#define VX_VIDFMT_1280_76Hz 0
#define VX_VIDFMT_1152_66Hz 1
#define VX_VIDFMT_1280_67Hz 2
#define VX_VIDFMT_NTSC 3
#define VX_VIDFMT_USER 4
static struct videoformat {
    int x, y, hz, clk;
}  videofmt[] = {
		{1280, 1024, 76, 0},
		{1152,  900, 66, 1},  
		{1280, 1024, 67, 3},
		{  -1,   -1,  0, 0},
		{  -1,   -1,  0, 0},
};

/* Mapping of monitor type to video format used. */
static int
    vx_monitor[] = {VX_VIDFMT_1280_76Hz,	/* 0; no monitor connected */
		    VX_VIDFMT_1152_66Hz,	/* 1; ?? Sony `P3` 16" ?? */
		    VX_VIDFMT_unknown  ,	/* 2; */
		    VX_VIDFMT_1152_66Hz,	/* 3; Sony `P3` 19" */
		    VX_VIDFMT_1152_66Hz,	/* 4; Hitachi 19" & 13W3-4BNC */
		    VX_VIDFMT_1280_76Hz,	/* 5; Toshiba 21" */
		    VX_VIDFMT_unknown  ,	/* 6; */
		    VX_VIDFMT_unknown  ,	/* 7; */
};
/* The Sony 'P3' is a multi-sync monitor that handles video formats
 *	1152x900 @ 76Hz, 1152x900 @ 66Hz, & 1280x1024 @ 67Hz
 * The 13W3-4BNC is an adaptor for older monitors.
 */

static int cgvx_pac_ver=0, cgvx_pac_rel=0;


#define VX_ADDR(a) (PGT_VME_D32 | btop(softc->baseaddr + (a)))

struct mapping {
        u_int   vaddr;          /* virtual (mmap offset) address */
        u_int   paddr;          /* physical (byte) address */
        u_int   size;           /* size in bytes (page multiple) */
        u_char  prot;           /* superuser only ? */
};

/*
 * Indices into address table
 */
#define MAP_CMAP        0
#define MAP_FHCTHC      1
#define MAP_FBCTEC      2
#define MAP_COLOR       3
#define	MAP_VIDCSR	4
#define	MAP_VIDPAC	5
#define	MAP_VIDMIX	6
#define	MAP_VMECTL	7
#define MAP_ROM         8
#define MAP_CG4COLOR    9
#define	MAP_VX		10

#define MAP_NPROBE      8	/* actually mapped in by cgsixprobe */
#define MAP_NMAP        11	/* mmappable by user process */

#ifdef NVX > 0

#define	VX_NKEYS	4

/*
 * driver per-unit data
 */
struct vx_softc {
				/* user virtual to physical mappings */
	struct mapping	addrs[MAP_NMAP];
	short         	found;          /* probe found device */
	short         	videofmt;	/* See videofmt[] above */
	int		      vx_ndmask;
	int                   vx_nopen;
	struct vx_procinfo    vx_procinfo[VX_MAXPROCS];
	struct vx_nodeinfo    vx_nodeinfo[VX_MAXNODES];
	dev_t                 vx_dev;
	struct	proc	      *vx_keyprocs[VX_NKEYS];
        int             w, h, hz;       /* resolution */
        int             clk;       	/* video controller clock select */
        struct proc     *owner;         /* owner of the frame buffer */
        struct fbgattr  gattr;          /* current attributes */
        u_long          *p4reg;         /* pointer to P4 register */
        u_long          baseaddr;       /* physical base address */
        u_int           size;           /* size of frame buffer */
        u_int           bw1size;        /* size of overlay/enable */
        caddr_t         va[MAP_NPROBE];	/* Mapped in devices */
        struct fbc      fbc;            /* saved fbc for PROM */
        struct thc      *thc;

        struct softcur {
                short enable;           /* cursor enable */
                short video_blanking;	/* Video blanking indicator */
                struct fbcurpos pos;    /* cursor position */
                struct fbcurpos hot;    /* cursor hot spot */
                struct fbcurpos size;   /* cursor bitmap size */
                u_long image[32];       /* cursor image bitmap */
                u_long mask[32];        /* cursor mask bitmap */
        } cur;

        union {                         /* shadow overlay color map */
                u_long  omap_long[1];   /* cheating here to save space */
                u_char  omap_char[3][2];
        } omap_image;
#define omap_rgb        omap_image.omap_char[0]
        u_short         omap_update;    /* overlay colormap update flag */

        u_short         cmap_index;     /* colormap update index */
        u_short         cmap_count;     /* colormap update count */
        union {                         /* shadow color map */
                u_long  cmap_long[CG6_CMAP_ENTRIES * 3 / sizeof (u_long)];
                u_char  cmap_char[3][CG6_CMAP_ENTRIES];
        } cmap_image;
#define cmap_red        cmap_image.cmap_char[0]
#define cmap_green      cmap_image.cmap_char[1]
#define cmap_blue       cmap_image.cmap_char[2]
#define cmap_rgb        cmap_image.cmap_char[0]

#if NWIN > 0
        Pixrect    pr;
        struct mprp_data  prd;
#endif NWIN > 0
	int		pri;		/* interrupt priority */
	int		pvec;		/* programmable vector */
	int		vvec;		/* VRT vector */
	int		intr;		/* interrupt flags */
	caddr_t		lockbase;	/* locking */
        struct seg      *curcntxt;      /* context switching */
};
#endif

struct	vx_vmectl {
    unsigned long	pgm_csr;	/* programmable intr register */
    unsigned long	vrt_csr;	/* Vertical ReTrace register */
    unsigned long	master;		/* beats me */
    unsigned long	intr;		/* poke here to cause pgm intr */
};

/*
 * handy macros
 */
#define getsoftc(unit)  (&vx_softc[unit])
#define DEBUGprint(X,A,B,C)     if (cgvxdebug&(X))printf(A,B,C)
#define DEBUGprint1(X,A,B)      if (cgvxdebug&(X))printf(A,B)

/* #define ShowIt(A,B)	printf(" B\t%x\n",B); (A)=(B) */
/* #define ShowIt(A,B)	\
	if (pokel(&(A),(B))) printf("cgvx: poke error @ %x\n", &(A)); */
#define ShowIt(A,B)	(A)=(B)

#endif	!vxreg_DEFINED
