/* @(#)cg9reg.h	1.1 94/10/31 SMI */

/* Copyright 1988, 1989 Sun Microsystems, Inc. */

#ifndef	_sundev_cg9reg_h
#define	_sundev_cg9reg_h

#include <sundev/ramdac.h>

#define		CG9_ID			0x00000002	/* from ID Reg. D0-D7  */
#define		CG9_WIDTH		1152
#define		CG9_HEIGHT		900
#define		CG9_DEPTH		32
#define		CG9_CMAP_SIZE		256
#define		CG9_OMAP_SIZE		4

/* 	CG9  Flag Bits  		*/
#define CG9_PRIMARY		0x01   /* Mark the PRIMARY Pixrect  */
#define CG9_OVERLAY_CMAP	0x02   /* Overlay CMAP to be changed */
#define CG9_24BIT_CMAP		0x04   /* 24 Bit CMAP to be changed  */
#define CG9_KERNEL_UPDATE	0x80   /* kernel vs. user ioctl */
#define CG9_DOUBLE_BUF_A	0x10   /* Double Buff Display Mode */
#define CG9_DOUBLE_BUF_B	0x20   /* Double Buff Display Mode */
#define CG9_SLEEPING		0x40   /* Denote if wake_up is necessary */
#define CG9_COLOR_OVERLAY	0x80   /* view overlay & enable as 2 bits */

/*
 * these are the real physical offsets which are not seen by user's codes
 */
#define		CG9_CTRL_BASE		0x000000	/* CG9_BASE  */
#define		CG9_OVERLAY_BASE	0x100000	/* (CG9_BASE + 0x100000)  */
#define		CG9_ENABLE_BASE		0x200000	/* (CG9_BASE + 0x200000)  */
#define		CG9_DB_BIT25_BASE	0x300000	/* (CG9_BASE + 0x300000)  */
#define		CG9_COLOR_FB_BASE	0x400000	/* (CG9_BASE + 0x400000)  */

/*
 * These are what mmap shows the world.  The framebuffer begins at 0 and
 * the control registers begin at 6MB.
 */
#define CG9_ADDR_OVERLAY	0
#define CG9_ADDR_CTRL		0x600000
#define CG9_ADDR_TABLE_SIZE	4      /* 4 pieces of CG9 memory */

#define		CG9_BPROM	131072
#define		CG9_BPROM_SIZE	0x40000

/*	control register bit values 	*/
#define		CG9_REG_VIDEO_ON	0x00010000
#define		CG9_REG_VB_INTR_SET	0x00020000
#define		CG9_REG_VB_INTR_ENAB	0x00040000
#define		CG9_REG_GP2_INTR_SET	0x00080000
#define		CG9_REG_GP2_INTR_ENAB	0x00100000
#define		CG9_REG_RESET		0x00200000
#define		CG9_REG_STEREO		0x00400000
#define		CG9_REG_DAC_DIAG	0x00800000
#define		CG9_REG_RES		0x0f000000
#define		CG9_REG_P2_ENABLE	0x10000000
#define		CG9_REG_CMAP_MODE	0x20000000
#define		CG9_REG_DAC_SYNC	0x40000000
#define		CG9_REG_USER		0x80000000

struct ctrl_reg {
    u_int           user:1;
    u_int           dac_sync:1;
    u_int           cmap_mode:1;
    u_int           p2_enable:1;
    u_int           resolution:4;
    u_int           dac_diag:1;
    u_int           stereo:1;
    u_int           reset:1;
    u_int           gp2_intr_en:1;
    u_int           gp2_intr_set:1;
    u_int           intr_en:1;
    u_int           intr_set:1;
    u_int           video:1;
                    u_int:16;
};
/*	double buffer r/w register bit values	 */
#define		CG9_ENB_12	0x01000000
#define		CG9_DIAG_DMM	0x02000000
#define		CG9_NO_WRITE_A	0x10000000
#define		CG9_NO_WRITE_B	0x20000000
#define		CG9_READ_B	0x40000000

struct dbl_reg {
    u_int           :1;
    u_int           read_b:1;
    u_int           no_write_b:1;
    u_int           no_write_a:1;
                    u_int:2;
    u_int           diag:1;
    u_int           en_12:1;
                    u_int:24;
};

#define		CG9_DISPLAY_B	0x80000000

struct dbl_display {
    u_int           display_b:1;
                    u_int:31;
};
/*      bit blit register bit values            */
#define         CG9_GO          0x20000000
#define         CG9_BACKWARD    0x80000000

struct bitblt_reg {
    u_int           backward:1;
                    u_int:1;
    u_int           go:1;
                    u_int:29;
};


struct cg9_control_sp {
    u_int           ident;	       /* board id */
    u_int           ctrl;	       /* control register */
    u_int           FILL1;
    u_int           cg9_intr;	       /* interrupt vector */
    u_int           gp2_intr;	       /* gp interrupt vector */
    u_int           dbl_reg;	       /* double buffering control */
    u_int           dbl_disp;	       /* double buffer display */
    u_int           bitblt;	       /* bitblt control */
    u_int           src;	       /* bitblt source address */
    u_int           dest;	       /* bitblt destination address */
    u_int           pixel_count;       /* width of bitblting */
    u_int           FILL2;
    u_int           plane_mask;
    u_int           FILL3[51];
    struct ramdac   ramdac_reg;	       /* lut registers */
};

#endif				       /* _sundev_cg9reg_h */
