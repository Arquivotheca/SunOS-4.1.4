/* @(#)cg12reg.h 1.1 94/10/31 SMI */

/* Copyright 1989-1990, Sun Microsystems, Inc. */

#ifndef	cg12reg_DEFINED
#define	cg12reg_DEFINED

#define	CG12_WIDTH		1152
#define	CG12_HEIGHT		900
#define	CG12_WIDTH_HR		1280
#define	CG12_HEIGHT_HR		1024
#define	CG12_DEPTH		32
#define	CG12_CMAP_SIZE		256
#define	CG12_OMAP_SIZE		32
#define NWIDS			64

/*	CG12 Flag Bits			*/

#define	CG12_PRIMARY		0x01	/* Mark the PRIMARY Pixrect	*/
#define	CG12_OVERLAY_CMAP	0x02	/* Overlay CMAP to be changed	*/
#define	CG12_24BIT_CMAP		0x04	/* 24 Bit CMAP to be changed	*/
#define	CG12_KERNEL_UPDATE	0x80	/* kernel vs. user ioctl	*/
#define	CG12_DOUBLE_BUF_A	0x10	/* Double Buff Display Mode	*/
#define	CG12_DOUBLE_BUF_B	0x20	/* Double Buff Display Mode	*/
#define	CG12_SLEEPING		0x40	/* Denote if wake_up is necessary */
#define	CG12_COLOR_OVERLAY	0x80	/* view overlay & enable as 2 bits */
#define	CG12_8BIT_CMAP		0x100	/* 8 Bit CMAP to be changed	*/
#define	CG12_WID_UPDATE		0x200	/* WID awaiting vert retrace	*/
#define CG12_ALT_CMAP		0x400	/* Alternate CMAP to be changed */
#define CG12_A12BIT_CMAP	0x800	/* 12 Bit buffer A CMAP to be changed */
#define CG12_B12BIT_CMAP	0x1000	/* 12 Bit buffer B CMAP to be changed */
#define	CG12_4BIT_OVR_CMAP	0x2000	/* 4 Bit overlay CMAP to be changed */

#define	CG12_PADDR_BASE		0x0

#define	CG12_OFF_PROM		0x000000
#define	CG12_OFF_USSC		0x040000
#define	CG12_OFF_DPU		0x040100
#define	CG12_OFF_APU		0x040200
#define	CG12_OFF_DAC		0x040300
#define	CG12_OFF_DAC_ADDR0	0x040300
#define	CG12_OFF_DAC_ADDR1	0x040400
#define	CG12_OFF_DAC_CTRL	0x040500
#define	CG12_OFF_DAC_PRIME	0x040600
#define	CG12_OFF_EIC		0x040700
#define	CG12_OFF_WSC		0x040800
#define	CG12_OFF_WSC_DATA	0x040800
#define	CG12_OFF_WSC_ADDR	0x040900
#define	CG12_OFF_DRAM		0x400000
#define	CG12_OFF_SHMEM		CG12_OFF_DRAM + 0x0E0000
#define	CG12_OFF_DISPLAY	0x600000
#define	CG12_OFF_WID		0x600000
#define	CG12_OFF_OVERLAY0	0x700000
#define	CG12_OFF_OVERLAY1	0x780000
#define	CG12_OFF_INTEN		0x800000
#define	CG12_OFF_DEPTH		0xC00000

#define	CG12_OFF_WID_HR		0x600000
#define	CG12_OFF_OVERLAY0_HR	0xE00000
#define	CG12_OFF_OVERLAY1_HR	0xF00000
#define	CG12_OFF_INTEN_HR	0x800000
#define	CG12_OFF_DEPTH_HR	0x800000

#define	CG12_OFF_CTL		CG12_OFF_USSC	/* 0x040000 */

#define	CG12_PROM_SIZE		0x010000
#define	CG12_USSC_SIZE		0x000100
#define	CG12_DPU_SIZE		0x000100
#define	CG12_APU_SIZE		0x000100
#define	CG12_DAC_SIZE		0x000400
#define	CG12_EIC_SIZE		0x000100
#define	CG12_WSC_SIZE		0x000200
#define	CG12_WSC_ADDR_SIZE	0x000100
#define	CG12_WSC_DATA_SIZE	0x000100
#define	CG12_DRAM_SIZE		0x100000
#define	CG12_COLOR24_SIZE	0x400000
#define	CG12_COLOR8_SIZE	0x100000
#define	CG12_ZBUF_SIZE		0x200000
#define	CG12_WID_SIZE		0x100000
#define	CG12_OVERLAY_SIZE	0x020000
#define	CG12_ENABLE_SIZE	0x020000

#define	CG12_SHMEM_SIZE		0x020000
#define	CG12_FBCTL_SIZE		0x842000
#define	CG12_PMCTL_SIZE		0x041000

#define	CG12_COLOR24_SIZE_HR	0x600000
#define	CG12_COLOR8_SIZE_HR	0x180000
#define	CG12_ZBUF_SIZE_HR	0x300000
#define	CG12_WID_SIZE_HR	0x180000
#define	CG12_OVERLAY_SIZE_HR	0x030000
#define	CG12_ENABLE_SIZE_HR	0x030000
#define	CG12_FBCTL_SIZE_HR	0xc62000

/* Virtual (mmap offsets) addresses */

#define	CG12_VOFF_BASE		0x000000

#define	CG12_VOFF_OVERLAY	CG12_VOFF_BASE
#define	CG12_VOFF_ENABLE	CG12_VOFF_OVERLAY+CG12_OVERLAY_SIZE
#define	CG12_VOFF_COLOR24	CG12_VOFF_ENABLE+CG12_ENABLE_SIZE
#define	CG12_VOFF_COLOR8	CG12_VOFF_COLOR24+CG12_COLOR24_SIZE
#define	CG12_VOFF_WID		CG12_VOFF_COLOR8+CG12_COLOR8_SIZE
#define	CG12_VOFF_ZBUF		CG12_VOFF_WID+CG12_WID_SIZE

#define	CG12_VOFF_CTL		CG12_VOFF_ZBUF+CG12_ZBUF_SIZE
#define	CG12_VOFF_SHMEM		CG12_VOFF_CTL+0x2000
#define	CG12_VOFF_DRAM		CG12_VOFF_SHMEM+CG12_SHMEM_SIZE
#define	CG12_VOFF_PROM		CG12_VOFF_DRAM+CG12_DRAM_SIZE

#define	CG12_VOFF_USSC		CG12_VOFF_CTL
#define	CG12_VOFF_DPU		CG12_VOFF_CTL+CG12_USSC_SIZE
#define	CG12_VOFF_APU		CG12_VOFF_DPU+CG12_DPU_SIZE
#define	CG12_VOFF_DAC		CG12_VOFF_APU+CG12_APU_SIZE
#define	CG12_VOFF_EIC		CG12_VOFF_DAC+CG12_DAC_SIZE
#define	CG12_VOFF_WSC		CG12_VOFF_EIC+CG12_EIC_SIZE
#define	CG12_VOFF_WSC_DATA	CG12_VOFF_EIC+CG12_EIC_SIZE
#define	CG12_VOFF_WSC_ADDR	CG12_VOFF_WSC_ADDR+CG12_WSC_DATA_SIZE

#define	CG12_HR
#define	CG12_VOFF_OVERLAY_HR	0x00000000
#define	CG12_VOFF_ENABLE_HR	0x00030000
#define	CG12_VOFF_COLOR24_HR	0x00060000
#define	CG12_VOFF_COLOR8_HR	0x00660000
#define	CG12_VOFF_WID_HR	0x007e0000
#define	CG12_VOFF_ZBUF_HR	0x00960000
#define	CG12_VOFF_CTL_HR	0x00c60000
#define	CG12_VOFF_SHMEM_HR	0x00c62000
#define	CG12_VOFF_DRAM_HR	CG12_VOFF_SHMEM_HR+CG12_SHMEM_SIZE
#define	CG12_VOFF_PROM_HR	CG12_VOFF_DRAM_HR+CG12_DRAM_SIZE

/* Font storage locations */

#define	CG12_OFF_FONT		(CG12_OFF_INTEN + 1152 * 900)
#define	CG12_VOFF_FONT		(CG12_VOFF_COLOR24 + 1152 * 900)

/* Vertical retrace counter */

#define	CG12_VOFF_VERTCOUNT	(CG12_VOFF_APU + 0xC)
#define	CG12_VOFF_VERTCOUNT_HR	(CG12_VOFF_CTL_HR + 0x20C)

#define CG12_STATIC_BLOCKS	16
/*
 * The "direct port access" register constants.
 * All HACCESSS values include noHSTXY, noHCLIP, and SWAP.
 */

#define	CG12_HPAGE_OVERLAY	0x00000700	/* overlay page		*/
#define	CG12_HACCESS_OVERLAY	0x00000020	/* 1bit/pixel		*/
#define	CG12_PLN_SL_OVERLAY	0x00000017	/* plane 23		*/
#define	CG12_PLN_WR_OVERLAY	0x00800000	/* write mask		*/
#define	CG12_PLN_RD_OVERLAY	0xffffffff	/* read mask		*/

#define	CG12_HPAGE_ENABLE	0x00000700	/* overlay page		*/
#define	CG12_HACCESS_ENABLE	0x00000020	/* 1bit/pixel		*/
#define	CG12_PLN_SL_ENABLE	0x00000016	/* plane 22		*/
#define	CG12_PLN_WR_ENABLE	0x00400000
#define	CG12_PLN_RD_ENABLE	0xffffffff

#define	CG12_HPAGE_24BIT	0x00000500	/* intensity page	*/
#define	CG12_HACCESS_24BIT	0x00000025	/* 32bits/pixel		*/
#define	CG12_PLN_SL_24BIT	0x00000000	/* all planes		*/
#define	CG12_PLN_WR_24BIT	0x00ffffff
#define	CG12_PLN_RD_24BIT	0x00ffffff

#define	CG12_HPAGE_8BIT		0x00000500	/* intensity page	*/
#define	CG12_HACCESS_8BIT	0x00000023	/* 8bits/pixel		*/
#define	CG12_PLN_SL_8BIT	0x00000000
#define	CG12_PLN_WR_8BIT	0x00ffffff
#define	CG12_PLN_RD_8BIT	0x000000ff

#define	CG12_HPAGE_WID		0x00000700	/* overlay page		*/
#define	CG12_HACCESS_WID	0x00000023	/* 8bits/pixel		*/
#define	CG12_PLN_SL_WID		0x00000010	/* planes 16-23		*/
#define	CG12_PLN_WR_WID		0x003f0000
#define	CG12_PLN_RD_WID		0x003f0000

#define	CG12_HPAGE_ZBUF		0x00000000	/* depth page		*/
#define	CG12_HACCESS_ZBUF	0x00000024	/* 16bits/pixel		*/
#define	CG12_PLN_SL_ZBUF	0x00000060
#define	CG12_PLN_WR_ZBUF	0xffffffff
#define	CG12_PLN_RD_ZBUF	0xffffffff

#define	CG12_HPAGE_OVERLAY_HR	0xe00
#define	CG12_HPAGE_ENABLE_HR	0xe00
#define	CG12_HPAGE_24BIT_HR	0xa00
#define	CG12_HPAGE_8BIT_HR	0xa00
#define	CG12_HPAGE_WID_HR	0xe00
#define	CG12_HPAGE_ZBUF_HR	0x000

struct cg12_dpu
{
    unsigned int	r0;
    unsigned int	r1;
    unsigned int	r2;
    unsigned int	r3;
    unsigned int	r4;
    unsigned int	r5;
    unsigned int	r6;
    unsigned int	r7;
    unsigned int	reload_ctl;
    unsigned int	reload_stb;
    unsigned int	alu_ctl;
    unsigned int	blu_ctl;
    unsigned int	control;
    unsigned int	xleft;
    unsigned int	shift0;
    unsigned int	shift1;
    unsigned int	zoom;
    unsigned int	bsr;
    unsigned int	color0;
    unsigned int	color1;
    unsigned int	compout;
    unsigned int	pln_rd_msk_host;
    unsigned int	pln_wr_msk_host;
    unsigned int	pln_rd_msk_local;
    unsigned int	pln_wr_msk_local;
    unsigned int	scis_ctl;
    unsigned int	csr;
    unsigned int	pln_reg_sl;
    unsigned int	pln_sl_host;
    unsigned int	pln_sl_local0;
    unsigned int	pln_sl_local1;
    unsigned int	broadcast;
};

#define	CG12_APU_EN_VID		0x00000010
#define	CG12_APU_EN_HINT	0x00000400

struct cg12_apu
{
    unsigned int	imsg0;
    unsigned int	msg0;
    unsigned int	imsg1;
    unsigned int	msg1;
    unsigned int	ien0;
    unsigned int	ien1;
    unsigned int	iclear;
    unsigned int	istatus;
    unsigned int	cfcnt;
    unsigned int	cfwptr;
    unsigned int	cfrptr;
    unsigned int	cfilev0;
    unsigned int	cfilev1;
    unsigned int	rfcnt;
    unsigned int	rfwptr;
    unsigned int	rfrptr;
    unsigned int	rfilev0;
    unsigned int	rfilev1;
    unsigned int	size;
    unsigned int	res0;
    unsigned int	res1;
    unsigned int	res2;
    unsigned int	haccess;
    unsigned int	hpage;
    unsigned int	laccess;
    unsigned int	lpage;
    unsigned int	maccess;
    unsigned int	ppage;
    unsigned int	dwg_ctl;
    unsigned int	sam;
    unsigned int	sgn;
    unsigned int	length;
    unsigned int	dwg_r0;
    unsigned int	dwg_r1;
    unsigned int	dwg_r2;
    unsigned int	dwg_r3;
    unsigned int	dwg_r4;
    unsigned int	dwg_r5;
    unsigned int	dwg_r6;
    unsigned int	dwg_r7;
    unsigned int	reload_ctl;
    unsigned int	reload_stb;
    unsigned int	c_xleft;
    unsigned int	c_ytop;
    unsigned int	c_xright;
    unsigned int	c_ybot;
    unsigned int	f_xleft;
    unsigned int	f_xright;
    unsigned int	x_dst;
    unsigned int	y_dst;
    unsigned int	dst_ctl;
    unsigned int	morigin;
    unsigned int	vsg_ctl;
    unsigned int	h_sync;
    unsigned int	hblank;
    unsigned int	v_sync;
    unsigned int	vblank;
    unsigned int	vdpyint;
    unsigned int	vssyncs;
    unsigned int	hdelays;
    unsigned int	stdaddr;
    unsigned int	hpitches;
    unsigned int	zoom;
    unsigned int	test;
};

struct cg12_ramdac
{
    unsigned int	addr_lo;
    unsigned char	pad1[0x100 - 4];
    unsigned int	addr_hi;
    unsigned char	pad2[0x100 - 4];
    unsigned int	control;
    unsigned char	pad3[0x100 - 4];
    unsigned int	color;
    unsigned char	pad4[0x100 - 4];
};

#define	CG12_EIC_SC_CN		0x80000000
#define	CG12_EIC_READB		0x40000000
#define	CG12_EIC_HSEM		0x20000000
#define	CG12_EIC_WSTATUS	0x20000000
#define	CG12_EIC_RES0		0x10000000
#define	CG12_EIC_ZBUF		0x04000000
#define	CG12_EIC_RES1		0x02000000
#define	CG12_EIC_DBL		0x01000000

#define	CG12_EIC_DSPRST		0x02000000
#define	CG12_EIC_SYSRST		0x01000000

#define	CG12_EIC_EICVER		0x01000000

#define	CG12_EIC_UBUSY		0x80000000
#define	CG12_EIC_UABORT		0x40000000
#define	CG12_EIC_CSEM		0x20000000
#define	CG12_EIC_FPNAN		0x10000000
#define	CG12_EIC_TESTIN		0x40000000
#define	CG12_EIC_TEST1		0x20000000
#define	CG12_EIC_TEST0		0x10000000

#define	CG12_EIC_HINT		0x40000000
#define	CG12_EIC_EICINT		0x20000000
#define	CG12_EIC_INT		0x60000000
#define	CG12_EIC_EN_HINT	0x02000000
#define	CG12_EIC_EN_EICINT	0x01000000

struct cg12_eic
{
    unsigned int	host_control;
    unsigned int	eic_control;
    unsigned int	c30_control;
    unsigned int	interrupt;
    unsigned int	timeout_reg;
    unsigned int	pad0;
    unsigned int	pad1;
    unsigned int	pad2;
    unsigned int	pad3;
    unsigned int	pad4;
    unsigned int	pad5;
    unsigned int	pad6;
    unsigned int	pad7;
    unsigned int	pad8;
    unsigned int	pad9;
    unsigned int	reset;
};

#define	CG12_WID_8_BIT		0	/* indexed color		*/
#define	CG12_WID_24_BIT		1	/* true color			*/
#define	CG12_WID_ENABLE_2	2	/* overlay/cursor enable has 2 colors */
#define	CG12_WID_ENABLE_3	3	/* overlay/cursor enable has 3 colors */
#define	CG12_WID_ALT_CMAP	4	/* use alternate colormap	*/
#define	CG12_WID_DBL_BUF_DISP_A	5	/* double buffering display A	*/
#define	CG12_WID_DBL_BUF_DISP_B	6	/* double buffering display A	*/
#define	CG12_WID_ATTRS		7	/* total no of attributes	*/

struct cg12_wsc
{
    unsigned char	pad0;
    unsigned char	data;
    unsigned char	pad1[0x100 - 2];
    unsigned char	pad2;
    unsigned char	addr;
    unsigned char	pad3[0x100 - 2];
};

struct cg12_ussc
{
    unsigned short	ussc_mem[48];
};

struct cg12_ctl_sp
{
    struct cg12_ussc	ussc;
    unsigned char	pad1[CG12_OFF_DPU-CG12_OFF_USSC-sizeof(struct cg12_ussc)];
    struct cg12_dpu	dpu;
    unsigned char	pad2[CG12_OFF_APU-CG12_OFF_DPU-sizeof(struct cg12_dpu)];
    struct cg12_apu	apu;
    struct cg12_ramdac	ramdac;
    struct cg12_eic	eic;
    unsigned char	pad4[CG12_OFF_WSC-CG12_OFF_EIC-sizeof(struct cg12_eic)];
    struct cg12_wsc	wsc;
};

#endif	/* !cg12reg_DEFINED */
