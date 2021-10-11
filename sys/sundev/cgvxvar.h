/* @(#)cgvxvar.h 1.1 94/10/31 Sun Microsystems Visualization Products */

/*
 * Copyright 1990, Sun Microsystems, Inc.
 *
 */

#ifndef	cgvxvar_DEFINED
#define	cgvxvar_DEFINED

#include <sys/param.h>

/*
 * VX, Visualization Accelerator system frame buffer, hardware definitions.
 *
 *	Using "cgvx" or "CGvx" as prefix to many definitions since this data
 *	is the VX equivalent to the "cgsix"/"cg6" definitions found in
 *	"cg6reg.h".
 */


/* Physical frame buffer and color map addresses */
/*
 * The base address is defined in the configuration file, e.g. GENERIC.
 * These constants are the offset from that address.
 */
#define CGvx_BASE		0x1500000
#define CGvx_ADDR_ROM		0x1400000-CGvx_BASE
#define CGvx_ADDR_FBC		0x1500000-CGvx_BASE
#define	CGvx_ADDR_TEC		0x1501000-CGvx_BASE
#define CGvx_ADDR_FHC		0x1580000-CGvx_BASE
#define CGvx_ADDR_THC		0x1581000-CGvx_BASE
#define CGvx_ADDR_COLOR		0x1600000-CGvx_BASE
#define CGvx_ADDR_CMAP		CGvx_VIDBT
#define CGvx_ADDR_VIDCTL	CGvx_VIDCSR
#define CGvx_ADDR_VMECTL	CGvx_VMECTL

#define CGvx_ADDR_FBCTEC	CGvx_ADDR_FBC
#define CGvx_ADDR_FHCTHC	CGvx_ADDR_FHC	

#define CGvx_CMAP_SZ		(8*1024)	/* 0x002000 */
#define CGvx_FBCTEC_SZ		(8*1024)	/* 0x002000 */
#define CGvx_FHCTHC_SZ		(8*1024)	/* 0x002000 */
#define CGvx_ROM_SZ		(64*1024)	/* 0x010000 */
#define CGvx_FB_SZ		(1280*1024)	/* 0x140000 */
#define CGvx_VIDCTL_SZ		(CGvx_VIDMIX - CGvx_VIDCSR + (8*1024))

/*
 * Offsets of TEC/FHC into page
 */
#define CGvx_TEC_POFF		0x01000
#define CGvx_THC_POFF		0x01000

/*
 *	Used by pixrects.
 */ 

/*
 * Virtual (mmap offsets) addresses
 */ 
#define CGvx_VBASE		0x70000000
#define CGvx_VADDR(x)		(CGvx_VBASE + (x) * 8192)

/*
 * CGvx Virtual object addresses
 */
#define CGvx_VADDR_FBC		CGvx_VBASE
#define CGvx_VADDR_CMAP		(CGvx_VADDR_FBC   + CGvx_FBCTEC_SZ)
#define CGvx_VADDR_FHC		(CGvx_VADDR_CMAP  + CGvx_CMAP_SZ)
#define CGvx_VADDR_ROM		(CGvx_VADDR_FHC   + CGvx_FHCTHC_SZ)
#define CGvx_VADDR_COLOR	(CGvx_VADDR_ROM   + CGvx_ROM_SZ)
#define CGvx_VADDR_VIDCTL	(CGvx_VADDR_COLOR + CGvx_FB_SZ)
#define CGvx_VADDR_VMECTL	(CGvx_VADDR_VIDCTL+ 3*8192)

/*
 * to map in all of LEGO, use mmapsize below, and offset CGvx_VBASE
 */
#define CGvx_MMAPSIZE(dfbsize)	(CGvx_VADDR_VIDCTL-CGvx_VBASE+CGvx_VIDCTL_SZ)

/*
 * convert from address returned by pr_makefromfd (eg. mmap) 
 * to CGvx register set.
 */
#define CGvxVA_TO_FBC(base) \
	((struct fbc*)  (((char*)base)+(CGvx_VADDR_FBC-CGvx_VBASE)))
#define CGvxVA_TO_TEC(base)  \
	((struct tec*)  (((char*)base)+(CGvx_VADDR_TEC-CGvx_VBASE)))
#define CGvxVA_TO_FHC(base)  \
	((u_int*) 	(((char*)base)+(CGvx_VADDR_FHC-CGvx_VBASE)))
#define CGvxVA_TO_THC(base)  \
	((struct thc*)  (((char*)base)+(CGvx_VADDR_THC-CGvx_VBASE)))
#define CGvxVA_TO_DFB(base)  \
	((short*) 	(((char*)base)+(CGvx_VADDR_COLOR-CGvx_VBASE)))
#define CGvxVA_TO_ROM(base)  \
	((u_int*)	(((char*)base)+(CGvx_VADDR_ROM-CGvx_VBASE)))
#define CGvxVA_TO_CMAP(base) \
	((u_int*) (((char*)base)+(CGvx_VADDR_CMAP-CGvx_VBASE)))

/* number of colormap entries */
#define CGvx_CMAP_ENTRIES	256


/* VIDEO RELATED MEMORY AND REGISTERS */

/* base addresses of the functional units */
#define CGvx_VMECTL    (0x01880000-CGvx_BASE)      /* VME cntl          */
#define CGvx_VIDCSR    (0x01a00000-CGvx_BASE)      /* Video cntl/status */
#define CGvx_VIDBT     (0x01a80000-CGvx_BASE)      /* Brooktree DACs    */
#define CGvx_VIDPAC    (0x01b00000-CGvx_BASE)      /* PAC 1000 registers*/
#define CGvx_VIDMIX    (0x01b80000-CGvx_BASE)      /* Video mixer asic  */
 
/* Brooktree 461/462 RAMDAC internals 
 *	all addresses relative to CGvx_VIDBT
 */
#define CGvx_BTLEGO	(0)	/* Lego palette */
#define CGvx_BTOVERLAY	(0x100)	/* 0-31 overlay/cursor colors */
#define CGvx_BTID	(0x200)	/* BT461 ID register */
#define CGvx_BTCR0	(0x201)	/* BT461 command reg 0   */
#define CGvx_BTCR1	(0x202)	/* BT461 command reg 1   */
#define CGvx_BTCR2	(0x203)	/* BT461 command reg 2   */
#define CGvx_BTRDMSKLO	(0x204)	/* BT461 read mask reg 8 LSBs */
#define CGvx_BTRDMSKHI	(0x205)	/* BT461 read mask reg MSBs */
#define CGvx_BTBKMSKLO	(0x206)	/* BT461 blink mask reg 8 LSBs*/
#define CGvx_BTBKMSKHI	(0x207)	/* BT461 blink mask reg MSBs */
#define CGvx_BTOVRDMASK (0x208)	/* BT461 overlay read mask */
#define CGvx_BTOVBKMASK	(0x209)	/* BT461 overlay blink mask */
#define CGvx_BTTEST	(0x20C)	/* BT461 test register */
#define CGvx_BTPALETTE0 (0x400)	/* jet palette 0 */
#define CGvx_BTPALETTE1 (0x500)	/* jet palette 1 */
#define CGvx_BTPALETTE2 (0x600)	/* jet palette 2 */
#define CGvx_BTPALETTE3 (0x700)	/* jet palette 3 */

/* registers within the video mixer asic */
#define CGvx_VIDLM0    (CGvx_VIDMIX + 0x00)   /* vid match register 0 */
#define CGvx_VIDLM1    (CGvx_VIDMIX + 0x04)   /* vid match register 1 */
#define CGvx_VIDLM2    (CGvx_VIDMIX + 0x08)   /* vid match register 2 */
#define CGvx_VIDLM3    (CGvx_VIDMIX + 0x0c)   /* vid match register 3 */
#define CGvx_VIDLL0    (CGvx_VIDMIX + 0x10)   /* vid lookup register 0 */
#define CGvx_VIDLL1    (CGvx_VIDMIX + 0x14)   /* vid lookup register 1 */
#define CGvx_VIDLL2    (CGvx_VIDMIX + 0x18)   /* vid lookup register 2 */
#define CGvx_VIDLL3    (CGvx_VIDMIX + 0x1c)   /* vid lookup register 3 */
#define CGvx_VIDAMASK  (CGvx_VIDMIX + 0x20)   /* vid alpha mask value  */
#define CGvx_VIDMIXCSR (CGvx_VIDMIX + 0x24)   /* vid mix asic control/status*/

/* alternates for addressing match and lookup */
#define CGvx_VIDMATCH  (CGvx_VIDMIX + 0x00)   /* match base */
#define CGvx_VIDLOOKUP (CGvx_VIDMIX + 0x10)   /* lookup base */



#define MAP_CGvxVIDCSR	5
#define MAP_NMAP	6

#define CGvx_VIDFMT_1280 1
#define CGvx_VIDFMT_1152 2
#define CGvx_VIDFMT_NTSC 3
#define CGvx_VIDFMT_USER 4

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

static int cgvx_pac_ver=0, cgvx_pac_rel=0;
static int cgvx_videofmt=CGvx_VIDFMT_1280;

/*#define ShowIt(A,B)	printf(" B\t%x\n",B); (A)=(B)*/
#define ShowIt(A,B)	(A)=(B)
/* #define ShowIt(A,B)	\
	if (pokel(&(A),(B))) printf("cgvx: poke error @ %x\n", &(A)); */

#define FBRESET _IO(F, 48)
#define FBVIDEOFMT _IOW(F, 49, caddr_t)

/*
 *  This should be in /usr/include/sun/fbio.h  !!!!!
 */
#define FB_ATTR_CG6_TYPE	3

static struct monitor_type {
    int x,    y, hz, scode, fmt;
}  jet_monitor[] = {
    {1280, 1024, 76, 0, CGvx_VIDFMT_1280},
    {1152,  900, 66, 1, CGvx_VIDFMT_1152},  /* SonyP3 also handles 
					1152x900 @76Hz & 1280x1024 @67Hz */
    {1024, 1024, 61, 2, CGvx_VIDFMT_1280},
    {1152,  900, 66, 3, CGvx_VIDFMT_1152},  /* ??SonyP3 */
    {1152,  900, 66, 4, CGvx_VIDFMT_1152},  /* could be a 13w3-4BNC converter */
    {1280, 1024, 76, 5, CGvx_VIDFMT_1280},
    {  -1,   -1,  0, 6, CGvx_VIDFMT_1280},
    {  -1,   -1,  0, 7, CGvx_VIDFMT_1280},
  };



#endif	!cgvxvar_DEFINED
