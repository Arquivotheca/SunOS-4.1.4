/*	@(#)gtreg.h  1.1 94/10/31	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */
#ifndef _gtreg_h
#define _gtreg_h

/*
 * GT graphics accelerator definitions
 */

#define	RP_BOOT_PROM_OF		0x00000000	/* RP boot PROM		     */
#define	RP_FE_CTRL_OF		0x00080000	/* FE RP control space	     */
#define RP_SU_RAM_OF  		0x00084000	/* SU shared RAM	     */
#define RP_HOST_CTRL_OF    	0x00090000	/* RP host control space     */
#define FBI_PFGO_OF		0x00100000	/* FBI PF copy/fill regs     */
#define FBI_REG_OF		0x00200000	/* FBI PP/PF SS 0/1 regs     */
#define FBO_CLUT_CTRL_OF	0x00300000	/* FBO CLUT control	     */
#define FBO_CLUT_MAP_OF		0x00308000	/* FBO CLUT map		     */
#define FBO_WLUT_CTRL_OF	0x00310000	/* FBO WLUT control	     */
#define FBO_WLUT_MAP_OF		0x00318000	/* FBO WLUT map		     */
#define FBO_SCREEN_CTRL_OF	0x00320000	/* FBO programmable regs     */
#define FBO_RAMDAC_CTRL_OF	0x00330000	/* FBO RAMDAC registers	     */
#define FE_CTRL_OF		0x00800000	/* FE control space	     */
#define FE_HFLAG1_OF		0x00810000	/* FE control flags 1	     */
#define FE_HFLAG2_OF		0x00820000	/* FE control flags 2	     */
#define FE_HFLAG3_OF		0x00830000	/* FE control flags 3	     */
#define FE_I860_OF		0x00840000	/* FE i860 space	     */
#define FE_LDM_OF		0x00832000	/* FE LDM		     */
#define	FB_OF			0x01000000	/* frame buffer		     */
#define FB_CD_OF		0x01000000	/* cursor data plane	     */
#define FB_CE_OF		0x01080000	/* cursor enable plane	     */


#define	RP_BOOT_PROM_SZ		   0x80000
#define	RP_FE_CTRL_SZ		    0x2000
#define RP_SU_RAM_SZ  		    0x4000
#define RP_HOST_CTRL_SZ    	    0x2000
#define FBI_PFGO_SZ		    0x2000
#define FBI_REG_SZ		    0x2000
#define FBO_CLUT_CTRL_SZ	    0x1000
#define FBO_CLUT_MAP_SZ		    0x8000
#define FBO_WLUT_CTRL_SZ	    0x1000
#define FBO_WLUT_MAP_SZ		    0x2000
#define FBO_SCREEN_CTRL_SZ	    0x1000
#define FBO_RAMDAC_CTRL_SZ	    0x1000
#define FE_CTRL_SZ		    0x2000
#define FE_HFLAG1_SZ		    0x2000
#define FE_HFLAG2_SZ		    0x1000
#define FE_HFLAG3_SZ		    0x1000
#define FE_I860_SZ		    0x2000
#define FE_LDM_SZ		  0x7CE000
#define	FB_SZ			 0x1000000
#define FB_CD_SZ		   0x42000
#define FB_CE_SZ		   0x42000


#define GT_VOFF_BASE	0x0

#define	RP_BOOT_PROM_VA		GT_VOFF_BASE
#define	RP_FE_CTRL_VA		(RP_BOOT_PROM_VA    + RP_BOOT_PROM_SZ)
#define RP_SU_RAM_VA		(RP_FE_CTRL_VA	    + RP_FE_CTRL_SZ)

#define RP_HOST_CTRL_VA    	(RP_SU_RAM_VA	    + RP_SU_RAM_SZ)
#define FBI_PFGO_VA		(RP_HOST_CTRL_VA    + RP_HOST_CTRL_SZ)
#define FBI_REG_VA		(FBI_PFGO_VA	    + FBI_PFGO_SZ)
#define FE_I860_VA		(FBI_REG_VA	    + FBI_REG_SZ)
#define	FB_VA			(FE_I860_VA	    + FE_I860_SZ)
#define FB_CD_VA		 FB_VA
#define FB_CE_VA		(FB_CD_VA	    + FB_CD_SZ)

#define FBO_CLUT_CTRL_VA	(FB_VA		    + FB_SZ)
#define FBO_CLUT_MAP_VA		(FBO_CLUT_CTRL_VA   + FBO_CLUT_CTRL_SZ)
#define FBO_WLUT_CTRL_VA	(FBO_CLUT_MAP_VA    + FBO_CLUT_MAP_SZ)
#define FBO_WLUT_MAP_VA		(FBO_WLUT_CTRL_VA   + FBO_WLUT_CTRL_SZ)
#define FBO_SCREEN_CTRL_VA	(FBO_WLUT_MAP_VA    + FBO_WLUT_MAP_SZ)
#define FBO_RAMDAC_CTRL_VA	(FBO_SCREEN_CTRL_VA + FBO_SCREEN_CTRL_SZ)
#define FE_CTRL_VA		(FBO_RAMDAC_CTRL_VA + FBO_RAMDAC_CTRL_SZ)
#define FE_HFLAG1_VA		(FE_CTRL_VA	    + FE_CTRL_SZ)
#define FE_HFLAG2_VA		(FE_HFLAG1_VA	    + FE_HFLAG1_SZ)
#define FE_HFLAG3_VA		(FE_HFLAG2_VA	    + FE_HFLAG2_SZ)
#define FE_LDM_VA		(FE_HFLAG3_VA	    + FE_HFLAG3_SZ)


/*
 * LDM definitions
 */
#define FE_LDM_START	0xA00000	/* LDM start address		*/
#define FE_LDM_END	0xA3FFFC	/* LDM end address		*/

#define FE_WCS_START	0xE00000	/* WCS start address		*/
#define FE_WCS_END	0xFFFFFC	/* WCS end address		*/

#define FE_LOAD_MASK	0x00ffffff	/* Mask for FE Local address	*/

#define feaddr2vaddr(x,y) ((x) + ((y) & FE_LOAD_MASK))


/*
 * mmap defines
 */
#define	GT_DPORT_VBASE		RP_HOST_CTRL_VA
#define GT_DPORT_MMAPSIZE	((FB_VA+FB_SZ) - GT_DPORT_VBASE)

#define	GT_RP_HOST_VOFFSET	(RP_HOST_CTRL_VA - GT_DPORT_VBASE)
#define GT_FBI_PFGO_VOFFSET	(FBI_PFGO_VA     - GT_DPORT_VBASE)
#define GT_FBI_REG_VOFFSET	(FBI_REG_VA      - GT_DPORT_VBASE)
#define GT_FE_I860_VOFFSET	(FE_I860_VA      - GT_DPORT_VBASE)
#define GT_FB_VOFFSET		(FB_VA           - GT_DPORT_VBASE)
#define GT_FB_CD_VOFFSET	 GT_FB_VOFFSET
#define GT_FB_CE_VOFFSET	(GT_FB_VOFFSET +  0x80000)
#define GT_FB_BACCESS_VOFFSET	(GT_FB_VOFFSET + 0x400000)



/*
 * GT registers
 *
 * RP host page
 */
typedef struct rp_host {
	u_int	rp_host_as_reg;			/* AS reg	*/
	u_int	rp_host_csr_reg;		/* CSR reg	*/
} rp_host;


/*
 * direct port FBI section
 */
typedef struct fbi_pfgo {
	u_int	fbi_pfgo_copy_dest_addr;	/* copy destination addr reg */
	u_int	fbi_pfgo_fill_dest_addr;	/* fill destination addr reg */
} fbi_pfgo;

typedef struct fbi_reg {
	u_int	    pad0[35];
	u_int	fbi_reg_vwclp_x;
	u_int	    pad1[1];
	u_int	fbi_reg_vwclp_y;
	u_int	    pad2[12];
	u_int	fbi_reg_fb_width;
	u_int	    pad3[5];
	u_int	fbi_reg_il_test;
	u_int	    pad4[72];
	u_int	fbi_reg_buf_sel;
	u_int	fbi_reg_stereo_cl;
	u_int	    pad5[125];
    	u_int	fbi_reg_cur_wid;
	u_int	fbi_reg_wid_ctrl;
	u_int	fbi_reg_wbc;
	u_int	fbi_reg_con_z;
	u_int	fbi_reg_z_ctrl;
   	u_int	fbi_reg_i_wmask;
    	u_int	fbi_reg_w_wmask;
    	u_int	fbi_reg_b_mode;
    	u_int	fbi_reg_b_wmask;
    	u_int	fbi_reg_rop;
   	u_int	    pad6[3];
	u_int	fbi_reg_mpg;
   	u_int	    pad7[498];
  	u_int	fbi_reg_s_mask;
    	u_int	fbi_reg_fg_col;
    	u_int	fbi_reg_bg_col;
    	u_int	fbi_reg_s_trans;
    	u_int	fbi_reg_dir_size;
   	u_int	fbi_reg_copy_src;
} fbi_reg;


/*
 * direct port FBO section
 */
typedef struct	fbo_clut_ctrl {
	u_int	clut_csr0;
	u_int	clut_csr1;
} fbo_clut_ctrl;


#define CLUT8_SIZE	 0x100
#define CLUT24_SIZE	 0x100
#define CLUT12_SIZE	0x4000

#define WLUT_SIZE	0x1000


typedef struct H_fbo_clut8 {
	u_int	clut8_map[CLUT8_SIZE];
} H_fbo_clut8;


typedef struct	H_fbo_clut24 {
	u_int	clut24_map[CLUT24_SIZE];
} H_fbo_clut24;


typedef struct	H_fbo_clutfc {
	u_int	fcs0;
	u_int	fcs1;
	u_int	fcs2;
	u_int	fcs3;
} H_fbo_clutfc;


typedef struct	H_fbo_clut16 {
	u_char	clut12_map[CLUT12_SIZE];
} H_fbo_clut12;


typedef struct  fbo_clut_map {
	H_fbo_clut8	fbo_clut8[14];
	H_fbo_clut24	fbo_clut24;
	H_fbo_clutfc	fbo_clutfc;
	u_int		    pad0[252];
	H_fbo_clut12	fbo_clut12;
} fbo_clut_map;


typedef struct	fbo_wlut_ctrl {
	u_int	wlut_csr0;
	u_int	wlut_csr1;
} fbo_wlut_ctrl;


typedef struct H_fbo_wlut {
	u_char	wlut_map[WLUT_SIZE];
} H_fbo_wlut;


typedef struct fbo_wlut_map {
	struct H_fbo_wlut fbo_wlut[2];
} fbo_wlut_map;


typedef struct fbo_screen_ctrl {
	u_int	    pad0[8];
	u_int	fbo_videomode_0;
} fbo_screen_ctrl;

#define SYNC_RUN	0x00000001
#define VIDEO_ENABLE	0x00000004


typedef struct fbo_rgb_alpha {
	u_int	fbo_rgb_ramdac_addr;
	u_int	fbo_rgb_ramdac_gamma;
	u_int	fbo_rgb_ramdac_ctrl;
	u_int	fbo_rgb_ramdac_cursor;
	u_int	fbo_alpha_ramdac_addr;
	u_int	fbo_alpha_ramdac_gamma;
	u_int	fbo_alpha_ramdac_ctrl;
	u_int	fbo_alpha_ramdac_alpha;
} fbo_rgb_alpha;


typedef struct fbo_ramdac_map {
	u_int	    pad0[2];
	u_int	ramdac_red;
	u_int	    pad1[3];
	u_int	ramdac_green;
	u_int	    pad2[3];
	u_int	ramdac_blue;
	u_int	    pad3[1];
	u_int	ramdac_addr_reg;
	u_int	ramdac_gamma_data_reg;
	u_int	    pad4[1];
	u_int	ramdac_cursor_data_reg;
} fbo_ramdac_map;


/*
 *  FE registers
 */
typedef struct	fe_ctrl_sp {
	u_int	    pad0;
	u_int	fe_hcr;		/* host communications register		*/
	u_int	fe_imr;		/* interface mode register		*/
	u_int	fe_hisr;	/* host interrupt status register	*/
	u_int	fe_fisr;	/* frontend interrupt status register	*/
	u_int	fe_atu_syncr;	/* ATU synchronizing register		*/
	u_int	fe_mdb_syncr;	/* MDB synchronizing register		*/
	u_int	fe_hflag_0;	/* host flag 0:  kernel VCR		*/
	u_int	fe_dflag_0;	/* gt   flag 0:  kernel VCR		*/
	u_int	    pad1[2];	/* 					*/
	u_int	fe_test_reg;	/* test register			*/
	u_int	    pad2[1076];	/* 					*/
	u_int	fe_par;		/* physical address register		*/
	u_int	    pad3[127];	/* 					*/
	u_int	fe_lvl1index;	/* level 1 PTP + index 2		*/
	u_int	fe_lvl1;	/* level 1 PTP				*/
} fe_ctrl_sp;


/*
 * Host Communications Register definitions
 */
#define FE_RESET	0x00000001	/* reset the FEP only		*/
#define FE_GO		0x00000002	/* allow the GT FEP to run	*/
#define FE_ZERO		0x00000020	/* force sequencer to address 0	*/
#define HK_RESET	0x00000040	/* reset to all of the GT	*/
#define FREEZE_ACK	0x00000200	/* ATU freeze acknowledge	*/


/*
 * Host Interrupt Status Register definitions
 */
#define HISR_MSTO	0x80000000	/* master timeout		*/
#define HISR_BERR	0x40000000	/* bus error			*/
#define HISR_SIZE	0x20000000	/* size error			*/
#define HISR_ATU	0x08000000	/* ATU timeout			*/
#define HISR_ROOT_INV	0x04000000	/* root pointer not valid	*/
#define HISR_LVL1_INV	0x02000000	/* level 1 pointer not valid	*/
#define HISR_TLB_MISS	0x01000000	/* persistent TLB miss		*/
#define HISR_UVCR	0x00020000	/* user VCR			*/
#define HISR_KVCR	0x00010000	/* kernel VCR			*/

#define HISR_INTMASK	( HISR_MSTO	| \
			  HISR_BERR	| \
			  HISR_SIZE	| \
			  HISR_ATU	| \
			  HISR_ROOT_INV	| \
			  HISR_LVL1_INV	| \
			  HISR_TLB_MISS	| \
			  HISR_UVCR	| \
			  HISR_KVCR	  \
			)


#define HISR_EN_MSMERR	0x00008000	/* enable: intr on MSTO/BERR	*/
#define HISR_EN_MSSERR	0x00002000	/*	intr on SIZE error	*/
#define	HISR_EN_ATU	0x00000800	/*	intr on ATU error	*/
#define HISR_EN_UVCR	0x00000002	/*	intr on user VCR	*/
#define HISR_EN_KVCR	0x00000001	/*	intr on kernel VCR	*/

#define HISR_ENABMASK	( HISR_EN_MSMERR  | \
			  HISR_EN_ATU     | \
			  HISR_EN_UVCR    | \
			  HISR_EN_KVCR      \
			)

/*
 * ATU Synchronizing register definitions
 */
#define ATU_FREEZE	0x00000002	/* freeze any new MSM cycles	*/
#define ATU_CLEAR	0x00000001	/* clear the ATU's TLB		*/

/*
 * Masks and shifts for forming virtual addresses from the components
 * in the fe_par, fe_rootindex, and fe_lvl1index registers.
 */
#define INDX1_MASK	0x00000ffc	/* lvl1 index:  fe_rootplus[11:2] */
#define INDX2_MASK	0x00000ffc	/* lvl2 index:  fe_l1plus[11:2]	  */
#define PGOFF_MASK	0x00000fff	/* offset:      fe_par[11:0]	  */
#define INDX1_SHIFT	20		/* lvl1 index left-shifts 20 bits */
#define INDX2_SHIFT	10		/* lvl2 index left-shifts 10 bits */
#define GT_PAGESHIFT	12		/* bytes to gt pages		  */


typedef struct fe_i860_sp {
	u_int	fe_hold;
	u_int	    pad_0;
	u_int	feint_stat;
	u_int	    pad_1;
	u_int	feint_msk;
	u_int	    pad_2;
	u_int	lbdes;
	u_int	    pad_3;
	u_int	lbseg;
	u_int	    pad_4;
	u_int	feds;
	u_int	    pad_5;
	u_int	diagdes;
	u_int	    pad_6;
	u_int	misc_flg;
	u_int	    pad_7;
	u_int	hoint_stat;
	u_int	    pad_8;
	u_int	hoint_msk;
	u_int	    pad_9;
	u_int	itmisc;
	u_int	    pad_10;
	u_int	lb_err_addr;
} fe_i860_sp;

/*
 * fe_hold defines
 */
#define FE_HOLD	0x1
#define FE_HLDA	0x2

/*
 * hoint_stat and hoint_msk define
 */
#define FE_LBTO	0x1


typedef struct fe_page_1 {
	u_int	    pad0[9];
	u_int	fe_hflag_1;		/* host flag 1:  signal VCR	*/
	u_int	fe_dflag_1;		/* gt   flag 1:  user   VCR	*/
	u_int	    pad1[1141];
	u_int	fe_rootindex;		/* root PTP + index 1		*/
	u_int	fe_rootptp;		/* root PTP			*/
} fe_page_1;

#define ROOTVALID	0x00000001	/* root ptp valid bit		*/


typedef struct fe_page_2 {
	u_int	    pad[12];
	u_int	fe_hflag_2;		/* host flag 2:  user VCR	*/
	u_int	fe_dflag_2;		/* <unused>			*/
} fe_page_2;


typedef struct fe_page_3 {
	u_int	    pad[14];
	u_int	fe_hflag_3;		/* host flag 3:			*/
	u_int	fe_dflag_3;
} fe_page_3;


/*
 * GT ioctls
 */
struct	fb_clutalloc {
	unsigned int	index;		/* returned CLUT index	*/
	unsigned int	flags;		/* CLUT type and flags	*/
	unsigned int	clutindex;	/* desired clut		*/
};


/*
 * CLUT types
 */
#define CLUT_8BIT	0x1		/* sunview 8 bit CLUT	*/
#define CLUT_SHARED	0x2		/* shared CLUT		*/

struct	fb_clut {
	unsigned int	flags;		/* flags			*/	
	int		index;		/* CLUT id			*/
	int		offset;		/* offset within the CLUT	*/
	int		count;		/* nbr of entries to be posted	*/
	unsigned char	*red;		/* pointer to red table		*/
	unsigned char	*green;		/* pointer to green table	*/
	unsigned char	*blue;		/* pointer to blue table	*/
};

struct fb_fcsalloc {
	int	index;			/* returned FCS			*/
};

struct fb_vmback {
	caddr_t		vm_addr;	/* virtual base address		*/
	int		vm_len;		/* length (bytes)		*/
};


struct fb_lightpenparam {
	int	pen_distance;	/* X+Y delta tolerance			*/
	int	pen_time;	/* nbr of frames for lightpen undetect	*/
};


struct fb_vmctl {
	u_int	vc_function;	/* vm function				*/
	caddr_t	vc_addr;	/* starting address			*/
	caddr_t	vc_len;		/* length (bytes)			*/
};


struct fb_setgamma {
	u_int	fbs_flag;	/* sets default degamma-or-not state	*/
	float	fbs_gamma;	/* the gamma value			*/
};


struct gt_version {
	int	gtv_ucode_major;
	int	gtv_ucode_minor;
	int	gtv_ucode_subminor;
	u_int	gtv_flags;
	int	gtv_reserved[4];
};

#define FBV_SUSPEND	0x00000004
#define FBV_RESUME	0x00000008



/*
 * ioctl flags
 */
#define FB_BLOCK	0x1
#define FB_KERNEL	0x2
#define FB_NO_DEGAMMA	0x4	/* don't degamma 8-bit indexed colors	*/
#define FB_DEGAMMA	0x8	/* degamma 8-bit indexed colors		*/


/*
 * ioctl definitions
 */
#define	FB_CLUTALLOC	_IOWR(t,  1, struct fb_clutalloc)
#define	FB_CLUTFREE	_IOW(t,   2, struct fb_clutalloc)
#define	FB_CLUTREAD	_IOWR(t,  3, struct fb_clut)
#define	FB_CLUTPOST	_IOW(t,   4, struct fb_clut)

#define	FB_FCSALLOC	_IOWR(t,  5, struct fb_fcsalloc)
#define	FB_FCSFREE	_IOW(t,   6, struct fb_fcsalloc)

#define	FB_SETSERVER	_IO(t,    7)
#define	FB_SETDIAGMODE	_IOW(t,   8, int)
#define	FB_SETWPART	_IOW(t,   9, int)
#define	FB_GETWPART	_IOR(t,  10, int)

#define	FB_SETMONITOR	_IOW(t,  11, int)
#define	FB_GETMONITOR	_IOR(t,  12, int)

#define FB_DISCONNECT	_IO(t,   16)
#define	FB_LOADKMCB	_IOW(t,  17, int)
#define FB_CONNECT	_IOWR(t, 18, struct gt_connect)

#define	FB_GRABHW	_IOR(t,  19, int)
#define	FB_UNGRABHW	_IO(t,   20)

#define	FB_VMBACK	_IOW(t,  21, struct fb_vmback)
#define	FB_VMUNBACK	_IOW(t,  22, struct fb_vmback)

#define	FB_SETCLUTPART	_IOW(t,  23, int)
#define	FB_GETCLUTPART	_IOR(t,  24, int)

#define	FB_LIGHTPENENABLE   _IOW(t,  25, int)
#define	FB_SETLIGHTPENPARAM _IOW(t,  26, struct fb_lightpenparam)
#define	FB_GETLIGHTPENPARAM _IOR(t,  27, struct fb_lightpenparam)

#define FB_VMCTL	_IOW(t,	30, struct fb_vmctl)
#define FB_SETGAMMA	_IOW(t, 31, struct fb_setgamma)
#define FB_GETGAMMA	_IOR(t, 32, struct fb_setgamma)

#define FB_GT_SETVERSION	_IOW(t, 40, struct gt_version)
#define FB_GT_GETVERSION	_IOR(t, 41, struct gt_version)


/*
 * defines for fb_wid_item attributes list
 */
#define GT_WID_ATTR_MASK_IB		0x00000001
#define GT_WID_ATTR_MASK_OB		0x00000002
#define GT_WID_ATTR_MASK_PG		0x00000004
#define GT_WID_ATTR_MASK_CI		0x00000008
#define GT_WID_ATTR_MASK_FCS		0x00000010

#define GT_WID_ATTR_IMAGE_BUFFER	0
#define GT_WID_ATTR_OVERLAY_BUFFER	1
#define GT_WID_ATTR_PLANE_GROUP		2
#define GT_WID_ATTR_CLUT_INDEX		3
#define GT_WID_ATTR_FCS			4
#define GT_WID_TOTAL_ATTR		5


/*
 * defines for CLUTs
 */
#define GT_CLUT_INDEX_24BIT	14
#define GT_CLUT_INDEX_FCS	15
#define GT_CLUT_INDEX_12BIT	16
#define GT_CLUT_INDEX_GAMMA	17
#define GT_CLUT_INDEX_CURSOR	18
#define GT_CLUT_INDEX_DEGAMMA	19


/*
 * The low-order bit of the minor device number is used to 
 * differentiate the accelerator port from the direct port
 */
#define GT_ACCEL_PORT	0x1


/*
 * GT monitor types
 */
#define GT_MONITOR_TYPE_STANDARD	0x0
#define GT_MONITOR_TYPE_HDTV		0x1
#define GT_MONITOR_TYPE_QUAD		0x2
#define GT_MONITOR_TYPE_STEREO		0x3

/*
 * Various  monitor configurations
 */
#define GT_1280_76	0x0
#define GT_1280_67	0x1
#define GT_1152_66	0x2
#define GT_STEREO	0x3
#define GT_HDTV		0x4			/* reserved	*/
#define GT_NTSC		0x5			/* reserved	*/
#define GT_PAL		0x6			/* reserved	*/


/*
 * Some GT geometry
 */
#define GT_NWID		1024
#define GT_NCLUT	  16
#define GT_NCLUT_8	  14
#define GT_NFCS		   4
#define GT_CMAP_SIZE	 256

#define GT_PREFETCH_SZ	  64

#endif	_gtreg_h
