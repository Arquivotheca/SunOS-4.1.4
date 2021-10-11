/* @(#)tcxreg.h 1.1 94/10/31 SMI */

/*
 * Copyright 1993, Sun Microsystems, Inc.
 */

#ifndef	tcxreg_DEFINED
#define	tcxreg_DEFINED

#define	FBTYPE_TCX	21	/* should really be in <sys/fbio.h>, but... */

/*
 * TCX frame buffer hardware definitions.
 */


#define	TCX_ROM_SZ		(128*1024)	/* 2M allocated */
#define	TCX_CMAP_SZ		4096		/* 256k allocated */
#define	TCX_DHC_SZ		4096		/* 256k allocated */
#define	TCX_ALT_SZ		4096		/* 512k allocated */
#define	TCX_THC_SZ		4096		/* 512k allocated */
#define	TCX_TEC_SZ		4096		/* 512k allocated */

#define	TCX_DFB8_SZ		(1024*1024)	/* 1 meg, 8M allocated */
#define	TCX_DFB24_SZ		(4*1024*1024)	/* 4 meg, 16M allocated */
#define	TCX_STIP_SZ		(8*1024*1024)	/* 8 meg, 32M allocated */
#define	TCX_BLIT_SZ		(8*1024*1024)	/* 8 meg, 32M allocated */
#define	TCX_RDFB32_SZ		(4*1024*1024)	/* 4 meg, 16M allocated */
#define	TCX_RSTIP_SZ		(8*1024*1024)	/* 8 meg, 32M allocated */
#define	TCX_RBLIT_SZ		(8*1024*1024)	/* 8 meg, 32M allocated */

#define	TCX_VRT_SZ		8192


/*
 * Virtual (mmap offsets) addresses.
 */ 

	/* the following constants assume there will never be a tcx larger
	 * than 4Kx4K.  It would be better to get these from the device
	 * driver via an IOCTL, but for now we just hard-wire them.
	 */
#define	TCX_VADDR_DFB8		0		/* up to 16 meg */
#define	TCX_VADDR_DFB24		0x01000000	/* up to 64 meg */
#define	TCX_VADDR_STIP		0x10000000	/* up to 128 meg */
#define	TCX_VADDR_BLIT		0x20000000	/* up to 128 meg */
#define	TCX_VADDR_RDFB32	0x28000000	/* up to 64 meg */
#define	TCX_VADDR_RSTIP		0x30000000	/* up to 128 meg */
#define	TCX_VADDR_RBLIT		0x38000000	/* up to 128 meg */
#define	TCX_VADDR_TEC		0x70000000
#define	TCX_VADDR_CMAP		0x70002000
#define	TCX_VADDR_THC		0x70004000
#define	TCX_VADDR_DHC		0x70008000
#define	TCX_VADDR_ALT		0x7000a000
#define	TCX_VADDR_UART		0x7000c000
#define	TCX_VADDR_VRT		0x7000e000
#define	TCX_VADDR_ROM		0x70010000


/* number of colormap entries */
#define	TCX_CMAP_ENTRIES	256


/*
 * TCX capabilities.  Info on hardware capability is stored
 * in the struct fbsattr.dev_specific[0], returned by FBIOGATTR.
 */

#define	TCX_CAP_STIP_ALIGN	0xf	/* stipple alignment constraint: */
#define	TCX_CAP_STIP_ALIGN_SH	0	/*  0 to 5 (1,2,4,8,16,32) */
#define	TCX_CAP_C_PLANES	0xf0	/* # control planes, 0-8 */
#define	TCX_CAP_C_PLANES_SH	4
#define	TCX_CAP_BLIT_WIDTH	0xf00	/* max blit width, 2^n (4096 max) */
#define	TCX_CAP_BLIT_WIDTH_SH	8
#define	TCX_CAP_BLIT_HEIGHT	0xf000	/* max blit height, 2^n (4096 max) */
#define	TCX_CAP_BLIT_HEIGHT_SH	12
#define	TCX_CAP_STIP_ROP	0x10000	/* stipple-with-rop supported */
#define	TCX_CAP_BLIT_ROP	0x20000	/* blit-with-rop supported */
#define	TCX_CAP_24_BIT		0x40000	/* 24-bit support */
#define	TCX_CAP_HW_CURSOR	0x80000	/* hardware cursor */
#define	TCX_CAP_PLANE_MASK	0x100000 /* plane mask for 8-bit stipple */



/* register set id's.  Not all TCX's have all register sets.  */

#define	TCX_REG_DFB8		0
#define	TCX_REG_DFB24		1
#define	TCX_REG_STIP		2
#define	TCX_REG_BLIT		3
#define	TCX_REG_RDFB32		4
#define	TCX_REG_RSTIP		5
#define	TCX_REG_RBLIT		6
#define	TCX_REG_TEC		7
#define	TCX_REG_CMAP		8
#define	TCX_REG_THC		9
#define	TCX_REG_ROM		10
#define	TCX_REG_DHC		11
#define	TCX_REG_ALT		12

#define	TCX_REG_MAX		13


/* Physical frame buffer and color map addresses
 * This information is not germane to applications that will call mmap(2);
 * see the TCX_VADDR_* addresses instead.
 *
 * The base address is defined in the configuration file, e.g. GENERIC, or
 * defined by the prom monitor.
 * These constants are the offset from that address.
 */

#define	TCX_ADDR_ROM		0		/* boot rom */
#define	TCX_ADDR_CMAP		0x0200000	/* colormap */
#define	TCX_ADDR_DHC		0x0240000	/* control registers */
#define	TCX_ADDR_ALT		0x0280000	/* alternate registers */
#define	TCX_ADDR_THC		0x0301000	/* control registers */
#define	TCX_ADDR_TEC		0x0701000	/* control registers */
#define	TCX_ADDR_DFB8		0x0800000	/* 8-bit fb */
#define	TCX_ADDR_DFB24		0x2000000	/* 24-bit fb */
#define	TCX_ADDR_STIP		0x4000000	/* stipple command */
#define	TCX_ADDR_BLIT		0x6000000	/* blit command */
#define	TCX_ADDR_RDFB32		0xa000000	/* raw 32-bit fb */
#define	TCX_ADDR_RSTIP		0xc000000	/* raw stipple */
#define	TCX_ADDR_RBLIT		0xe000000	/* raw blit */




	/* Register definitions */

	/* These offsets are intended to be added to a
	 * pointer-to-integer whose value is the base address
	 * of the TCX memory mapped register area
	 */

	/* TODO: move these to a seperate file? */
	/* TODO: rename 'tec' to something else? */

	/* Hardware configuration and status */

#define	TCX_TEC_CONFIG		( 0x000 / sizeof(int) )
#define	TCX_TEC_DELAY		( 0x090 / sizeof(int) )
#define	TCX_TEC_STRAPPING	( 0x094 / sizeof(int) )
#define	TCX_TEC_HCMISC		( 0x098 / sizeof(int) )
#define	TCX_TEC_LINECOUNT	( 0x09c / sizeof(int) )

#define	TCX_THC_CONFIG		TCX_TEC_CONFIG
#define	TCX_THC_SENSEBUS	( 0x080 / sizeof(int) )
#define	TCX_THC_DELAY		TCX_TEC_DELAY
#define	TCX_THC_STRAPPING	TCX_TEC_STRAPPING
#define	TCX_THC_HCMISC		( 0x818 / sizeof(int) )
#define	TCX_THC_LINECOUNT	TCX_TEC_LINECOUNT

	/* video timing registers */

#define	TCX_TEC_HSS		( 0x0a0 / sizeof(int) )
#define	TCX_TEC_HSE		( 0x0a4 / sizeof(int) )
#define	TCX_TEC_HDS		( 0x0a8 / sizeof(int) )
#define	TCX_TEC_HSEDVS		( 0x0ac / sizeof(int) )
#define	TCX_TEC_HDE		( 0x0b0 / sizeof(int) )
#define	TCX_TEC_VSS		( 0x0c0 / sizeof(int) )
#define	TCX_TEC_VSE		( 0x0c4 / sizeof(int) )
#define	TCX_TEC_VDS		( 0x0c8 / sizeof(int) )
#define	TCX_TEC_VDE		( 0x0cc / sizeof(int) )

	/* cursor control */

#define	TCX_THC_CURSOR_ADDRESS	( 0x8fc / sizeof(int) )

/* cursor data registers, plane A */
#define	TCX_THC_CURSORA00	( 0x900 / sizeof(int) )
#define	TCX_THC_CURSORA01	( 0x904 / sizeof(int) )
#define	TCX_THC_CURSORA02	( 0x908 / sizeof(int) )
#define	TCX_THC_CURSORA03	( 0x90c / sizeof(int) )
#define	TCX_THC_CURSORA04	( 0x910 / sizeof(int) )
#define	TCX_THC_CURSORA05	( 0x914 / sizeof(int) )
#define	TCX_THC_CURSORA06	( 0x918 / sizeof(int) )
#define	TCX_THC_CURSORA07	( 0x91c / sizeof(int) )
#define	TCX_THC_CURSORA08	( 0x920 / sizeof(int) )
#define	TCX_THC_CURSORA09	( 0x924 / sizeof(int) )
#define	TCX_THC_CURSORA10	( 0x928 / sizeof(int) )
#define	TCX_THC_CURSORA11	( 0x92c / sizeof(int) )
#define	TCX_THC_CURSORA12	( 0x930 / sizeof(int) )
#define	TCX_THC_CURSORA13	( 0x934 / sizeof(int) )
#define	TCX_THC_CURSORA14	( 0x938 / sizeof(int) )
#define	TCX_THC_CURSORA15	( 0x93c / sizeof(int) )
#define	TCX_THC_CURSORA16	( 0x940 / sizeof(int) )
#define	TCX_THC_CURSORA17	( 0x944 / sizeof(int) )
#define	TCX_THC_CURSORA18	( 0x948 / sizeof(int) )
#define	TCX_THC_CURSORA19	( 0x94c / sizeof(int) )
#define	TCX_THC_CURSORA20	( 0x950 / sizeof(int) )
#define	TCX_THC_CURSORA21	( 0x954 / sizeof(int) )
#define	TCX_THC_CURSORA22	( 0x958 / sizeof(int) )
#define	TCX_THC_CURSORA23	( 0x95c / sizeof(int) )
#define	TCX_THC_CURSORA24	( 0x960 / sizeof(int) )
#define	TCX_THC_CURSORA25	( 0x964 / sizeof(int) )
#define	TCX_THC_CURSORA26	( 0x968 / sizeof(int) )
#define	TCX_THC_CURSORA27	( 0x96c / sizeof(int) )
#define	TCX_THC_CURSORA28	( 0x970 / sizeof(int) )
#define	TCX_THC_CURSORA29	( 0x974 / sizeof(int) )
#define	TCX_THC_CURSORA30	( 0x978 / sizeof(int) )
#define	TCX_THC_CURSORA31	( 0x97c / sizeof(int) )

/* cursor data registers, plane B */
#define	TCX_THC_CURSORB00	( 0x980 / sizeof(int) )
#define	TCX_THC_CURSORB01	( 0x984 / sizeof(int) )
#define	TCX_THC_CURSORB02	( 0x988 / sizeof(int) )
#define	TCX_THC_CURSORB03	( 0x98c / sizeof(int) )
#define	TCX_THC_CURSORB04	( 0x990 / sizeof(int) )
#define	TCX_THC_CURSORB05	( 0x994 / sizeof(int) )
#define	TCX_THC_CURSORB06	( 0x998 / sizeof(int) )
#define	TCX_THC_CURSORB07	( 0x99c / sizeof(int) )
#define	TCX_THC_CURSORB08	( 0x9a0 / sizeof(int) )
#define	TCX_THC_CURSORB09	( 0x9a4 / sizeof(int) )
#define	TCX_THC_CURSORB10	( 0x9a8 / sizeof(int) )
#define	TCX_THC_CURSORB11	( 0x9ac / sizeof(int) )
#define	TCX_THC_CURSORB12	( 0x9b0 / sizeof(int) )
#define	TCX_THC_CURSORB13	( 0x9b4 / sizeof(int) )
#define	TCX_THC_CURSORB14	( 0x9b8 / sizeof(int) )
#define	TCX_THC_CURSORB15	( 0x9bc / sizeof(int) )
#define	TCX_THC_CURSORB16	( 0x9c0 / sizeof(int) )
#define	TCX_THC_CURSORB17	( 0x9c4 / sizeof(int) )
#define	TCX_THC_CURSORB18	( 0x9c8 / sizeof(int) )
#define	TCX_THC_CURSORB19	( 0x9cc / sizeof(int) )
#define	TCX_THC_CURSORB20	( 0x9d0 / sizeof(int) )
#define	TCX_THC_CURSORB21	( 0x9d4 / sizeof(int) )
#define	TCX_THC_CURSORB22	( 0x9d8 / sizeof(int) )
#define	TCX_THC_CURSORB23	( 0x9dc / sizeof(int) )
#define	TCX_THC_CURSORB24	( 0x9e0 / sizeof(int) )
#define	TCX_THC_CURSORB25	( 0x9e4 / sizeof(int) )
#define	TCX_THC_CURSORB26	( 0x9e8 / sizeof(int) )
#define	TCX_THC_CURSORB27	( 0x9ec / sizeof(int) )
#define	TCX_THC_CURSORB28	( 0x9f0 / sizeof(int) )
#define	TCX_THC_CURSORB29	( 0x9f4 / sizeof(int) )
#define	TCX_THC_CURSORB30	( 0x9f8 / sizeof(int) )
#define	TCX_THC_CURSORB31	( 0x9fc / sizeof(int) )


	/* CONFIG register definitions */

#define	TCX_CONFIG_FBID_SHIFT	28
#define	TCX_CONFIG_FBID		0xf0000000
#define	TCX_CONFIG_SENSE_SHIFT	24
#define	TCX_CONFIG_SENSE	0x7000000
#define	TCX_CONFIG_REV_SHIFT	20
#define	TCX_CONFIG_REV		0xf00000
#define	TCX_CONFIG_RESET_SHIFT	15
#define	TCX_CONFIG_RESET	0x8000

	struct tcx_config {
	  unsigned int	fbid	: 4 ;
	  unsigned		: 1 ;
	  unsigned int	sense	: 3 ;
	  unsigned int	rev	: 4 ;
	  unsigned		: 4 ;
	  unsigned int	reset	: 1 ;
	  unsigned		: 15 ;
	} ;




	/* HCMISC register definitions */

#define	TCX_HCMISC_OPEN_FLAG_SHIFT	31	/* device is open */
#define	TCX_HCMISC_OPEN_FLAG		0x80000000
#define	TCX_HCMISC_EN_SW_ERR_IRQ_SHIFT	29	/* enable SW error interrupt */
#define	TCX_HCMISC_EN_SW_ERR_IRQ	0x20000000
#define	TCX_HCMISC_SW_ERR_SHIFT		28	/* SW error ocurred */
#define	TCX_HCMISC_SW_ERR		0x10000000
#define	TCX_HCMISC_VSYNC_LEVEL_SHIFT	27	/* vsync level when disabled */
#define	TCX_HCMISC_VSYNC_LEVEL		0x8000000
#define	TCX_HCMISC_HSYNC_LEVEL_SHIFT	26	/* hsync level when disabled */
#define	TCX_HCMISC_HSYNC_LEVEL		0x4000000
#define	TCX_HCMISC_DISABLE_VSYNC_SHIFT	25	/* disable v sync */
#define	TCX_HCMISC_DISABLE_VSYNC	0x2000000
#define	TCX_HCMISC_DISABLE_HSYNC_SHIFT	24	/* disable h sync */
#define	TCX_HCMISC_DISABLE_HSYNC	0x1000000
#define	TCX_HCMISC_RESET_SHIFT		12	/* reset */
#define	TCX_HCMISC_RESET		0x1000
#define	TCX_HCMISC_ENABLE_VIDEO_SHIFT	10	/* enable video */
#define	TCX_HCMISC_ENABLE_VIDEO		0x400
#define	TCX_HCMISC_SYNC_SHIFT		9	/* composite sync (ro) */
#define	TCX_HCMISC_SYNC			0x200
#define	TCX_HCMISC_VSYNC_SHIFT		8	/* vertical sync (ro) */
#define	TCX_HCMISC_VSYNC		0x100
#define	TCX_HCMISC_ENABLE_SYNC_SHIFT	7	/* enable sync & video */
#define	TCX_HCMISC_ENABLE_SYNC		0x80
#define	TCX_HCMISC_VBLANK_SHIFT		6	/* vblank in progress (ro) */
#define	TCX_HCMISC_VBLANK		0x40
#define	TCX_HCMISC_EN_IRQ_SHIFT		5	/* enable vert. interrupt */
#define	TCX_HCMISC_EN_IRQ		0x20
#define	TCX_HCMISC_IRQ_SHIFT		4	/* vert. interrupt ocurred */
#define	TCX_HCMISC_IRQ			0x10
#define	TCX_CYCLES_DACWAIT_SHIFT	0	/* mclk between DAC accesses*/
#define	TCX_CYCLES_DACWAIT		0xf

	struct tcx_hcmisc {
	  unsigned int sw_err_int_en : 1 ;
	  unsigned int sw_err	: 1 ;
	  unsigned int vsync_level : 1 ;
	  unsigned int hsync_level : 1 ;
	  unsigned int dis_vsync : 1 ;
	  unsigned int dis_hsync : 1 ;
	  unsigned		: 13 ;
	  unsigned int enable_video : 1 ;
	  unsigned int sync	: 1 ;
	  unsigned int vsync	: 1 ;
	  unsigned int enable_sync : 1 ;
	  unsigned int vblank	: 1 ;
	  unsigned int int_en	: 1 ;
	  unsigned int intr	: 1 ;
	  unsigned int cycles_dacwait : 4 ;
	} ;



	/* DELAY register definitions */

#define	TCX_DELAY_SYNC_SHIFT	8
#define	TCX_DELAY_SYNC		0xf00
#define	TCX_DELAY_WR_F_SHIFT	6
#define	TCX_DELAY_WR_F		0xc0
#define	TCX_DELAY_WR_R_SHIFT	4
#define	TCX_DELAY_WR_R		0x30
#define	TCX_DELAY_SOE_F_SHIFT	2
#define	TCX_DELAY_SOE_F		0xc
#define	TCX_DELAY_SOE_S_SHIFT	0
#define	TCX_DELAY_SOE_S		0x3

	struct tcx_delay {
	  unsigned			: 20 ;
	  unsigned int	sync		: 4 ;
	  unsigned int	wr_falling	: 2 ;
	  unsigned int	wr_rising	: 2 ;
	  unsigned int	soe_falling	: 2 ;
	  unsigned int	soe_sclk	: 2 ;
	} ;


	/* STRAPPING register definitions */

#define	TCX_STRAP_FIFO_LIMIT_SHIFT	20
#define	TCX_STRAP_FIFO_LIMIT		0xf00000
#define	TCX_STRAP_CACHE_EN_SHIFT	16
#define	TCX_STRAP_CACHE_EN		0x10000
#define	TCX_STRAP_ZERO_OFFSET_SHIFT	15
#define	TCX_STRAP_ZERO_OFFSET		0x8000
#define	TCX_STRAP_REFRESH_DIS_SHIFT	14
#define	TCX_STRAP_REFRESH_DIS		0x4000
#define	TCX_STRAP_REF_LOAD_SHIFT	12
#define	TCX_STRAP_REF_LOAD		0x1000
#define	TCX_STRAP_REFRESH_PERIOD_SHIFT	0
#define	TCX_STRAP_REFRESH_PERIOD	0x3ff

	struct tcx_strapping {
	  unsigned			: 8 ;
	  unsigned int	fifo_limit	: 4 ;
	  unsigned			: 3 ;
	  unsigned int	cache_enable	: 1 ;
	  unsigned int	zero_offset	: 1 ;
	  unsigned int	refresh_dis	: 1 ;
	  unsigned			: 1 ;
	  unsigned int	ref_load	: 1 ;
	  unsigned			: 2 ;
	  unsigned int	refresh_period	: 10 ;
	} ;


	/* CURSOR ADDRESS register definitions */

#define	TCX_CURSOR_ADDR_X_SHIFT	16
#define	TCX_CURSOR_ADDR_X	0xffff0000
#define	TCX_CURSOR_ADDR_Y_SHIFT	0
#define	TCX_CURSOR_ADDR_Y	0xffff

	struct tcx_cursor_address {
	  int	cursor_x	: 16 ;
	  int	cursor_y	: 16 ;
	} ;


#endif	/* tcxreg_DEFINED */
