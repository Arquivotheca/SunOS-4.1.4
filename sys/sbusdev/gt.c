#ifndef lint
static char sccsid[] = "@(#)gt.c  1.1 94/10/31 SMI";
#endif


/*
 * Copyright (c) 1990, 1991, 1992 by Sun Microsystems, Inc.
 *
 *  GT graphics accelerator driver
 */
#include "gt.h"
#include "win.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/vmmac.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/session.h>
#include <sys/vnode.h>

#include <sundev/mbvar.h>

#include <sun/fbio.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/enable.h>
#include <machine/eeprom.h>
#include <machine/seg_kmem.h>
#include <machine/intreg.h>

#ifdef sun4m
#include <machine/module.h>
#endif sun4m

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>
#include <pixrect/gtvar.h>

#include <vm/seg_vn.h>
#include <vm/anon.h>
#include <vm/page.h>
#include <vm/vpage.h>
#include <vm/faultcode.h>

#include <vm/hat.h>

#include <vm/as.h>
#include <vm/seg.h>
#include <vm/anon.h>

#include <sbusdev/gtmcb.h>
#include <sbusdev/gtseg.h>
#include <sbusdev/gtreg.h>
#include <sundev/lightpenreg.h>


addr_t	     map_regs();
void	     unmap_regs();
void	     report_dev();
struct proc *pfind();
char	    *strcpy();
u_long	     rmalloc();
u_int	     hat_getkpfnum();
caddr_t	     getpages();


int gt_identify(), gt_attach(), gtpoll(), gtopen();
int gtclose(), gtioctl(), gtsegmap();

static void gtintr();
static void gt_shared_init();
static gt_server_check(), gt_diag_check();
static gt_fbioputcmap(), gt_fbiogetcmap();
static gt_fbiogtype();
#if NWIN > 0
static gt_fbiogpixrect();
#endif NWIN
static gt_fbiogvideo(); 
static gt_fbiosvideo();
static gt_fbiovertical();
static gt_widalloc();
static gt_calc_wid();
static gt_calc_power();
static gt_widfree();
static gt_widread();

static gt_widpost();
static gt_direct_wlut_update();
static gt_clutalloc();
static gt_clutfree();
static gt_clutread();
static gt_clutpost();
static gt_gamma_clutpost(), gt_load_degamma();
static gt_cursor_clutpost();

static gt_post_command();

static gt_fcsalloc();
static gt_fcsfree();
static gt_setserver();
static void gt_server_free();
static gt_setdiagmode();
static void gt_diag_free();
static gt_setwpart();
static gt_getwpart();
static gt_setclutpart();
static gt_getclutpart();
static gt_lightpenenable();
static gt_setlightpenparam();
static gt_getlightpenparam();
static gt_setmonitor();
static gt_getmonitor();
static gt_setgamma();
static gt_getgamma();
static gt_sgversion();
static gt_fbiosattr();
static gt_fbiogattr();
static void gt_set_gattr_fbtype();

static gt_loadkmcb();
static void gt_init_kmcb();

static gt_connect();
static gt_disconnect();
static void gta_exit();

static void gt_freept();
static void gt_freemap();
static gt_vmctl();
static gta_suspend();
static gta_resume();
static gt_vmback();
static gt_vmunback();
static gt_grabhw();
static gt_ungrabhw();

static void gta_reset_accelerator();
static void gt_clean_kmcb();

static void gt_setnocache();

static void gt_link_detach();
static void gt_link_attach();
static struct proc_ll_type *gt_find_proc_link();
static void gt_insert_link();
static void gt_alloc_sr();
static void gt_init_sr();
static void gt_decrement_refcount();

static void gt_aslock();
static void gt_asunlock();
static void gta_unload();

static void gt_cntxsave();
static void gt_cntxrestore();

static struct at_proc *gta_findproc();
static struct at_proc *gta_findas();
static gt_validthread();

static void gt_pagedaemon();
static gt_badfault();
static gt_faultcheck();
static void gt_lvl1fault();
static void gt_pagefault();
static void gta_abortfault();
static void gt_mmuload();
static u_int gta_locksetup();
static struct gt_pte *gta_getmmudata();

static void gt_scheduler();
static void gt_schedkmcb();
static void gta_idle();
static void gta_run();
static void gta_stop();
static void gta_start();
static void gta_kmcbwait();
static void gta_loadroot();

static void gta_init();

static void gta_hash_insert();
static void gta_hash_delete();
static struct gta_lockmap *gta_hash_find();

static void gt_daemon_init();

static void gt_listinsert_next();
static void gt_listinsert_prev();
static void gt_listremove();

static void gt_enable_intr();
static void gt_freeze_atu();
static void gt_thaw_atu();
static void gt_wait_for_thaw();

static void gt_set_flags();
static void gt_clear_flags();

static gt_check_pf_busy();
static void gt_status_dump();

#ifdef sun4m
extern u_int gt_ldsbusreg();
extern void  gt_stsbusreg();
#endif sun4m

#ifdef GT_DEBUG
int gt_debug;

#define DEBUGF(level, args) _STMT(if (gt_debug >= (level)) printf args;)
#else
#define DEBUGF(level, args) /* nothing */
#endif GT_DEBUG

#define DUMP_MIA_REGS	0x1
#define DUMP_RP_REGS	0x2
#define DUMP_GT_REGS	(DUMP_MIA_REGS | DUMP_RP_REGS)

#define GT_PF_BUSY	0x00000040

#define GT_SPIN_LUT	(200 * 1000)	/* lut copy spin: 200 msec	*/
#define	GT_SPIN_PFBUSY	(500 * 1000)	/* pf busy spin: 500 msec	*/


/* FBO defines */
#define FBO_ENABLE_ALL_INTERLEAVE	0x00000050
#define FBO_ENABLE_COPY_REQUEST		0x00000001

/*
 * Window Lookup Table masks
 */
#define HK_WLUT_MASK_IMAGE		0x01
#define HK_WLUT_MASK_OVERLAY		0x02
#define HK_WLUT_MASK_PLANE_GROUP	0x04
#define HK_WLUT_MASK_CLUT		0x08
#define HK_WLUT_MASK_FCS		0x10
#define HKFBO_WLUT_ENTRY_OFFSET		   4	/* offset to next entry */

/*
 * Window Lookup Table entry definitions
*/
#define HKFBO_WLUT_MASK		0x00007FFF
#define HKFBO_WLUT_ALPHA_MODE   0x00006000	/* Alpha mode		  */
#define HKFBO_WLUT_INDEX_STEREO 0x00001000	/* Indexed stereo	  */
#define HKFBO_WLUT_FCS_SELECT   0x00000E00	/* Fast clear set select  */
#define HKFBO_WLUT_FCS_SEL_SHIFT	 9
#define HKFBO_WLUT_CLUT_SELECT  0x000001E0	/* CLUT select		  */
#define HKFBO_WLUT_CLUT_SEL_SHIFT	 5
#define HKFBO_WLUT_OV_BUFFER    0x00000010	/* Overlay buffer (1 = B) */
#define HKFBO_WLUT_IMAGE_BUFFER 0x00000008	/* Image buffer (1 = B)   */
#define HKFBO_WLUT_COLOR_MODE   0x00000007	/* Color mode field	  */
#define HKFBO_WLUT_CM_8OVERLAY  0x00000000	/* Color mode values	  */
#define HKFBO_WLUT_CM_8BLUE     0x00000001
#define HKFBO_WLUT_CM_8GREEN    0x00000002
#define HKFBO_WLUT_CM_8RED      0x00000003
#define HKFBO_WLUT_CM_12LOW     0x00000004
#define HKFBO_WLUT_CM_12HIGH    0x00000005
#define HKFBO_WLUT_CM_24BIT     0x00000006


#define HK_8BIT_RED		0
#define HK_8BIT_GREEN		1
#define HK_8BIT_BLUE		2
#define HK_8BIT_OVERLAY		3
#define HK_12BIT_LOWER		4
#define HK_12BIT_UPPER		5
#define HK_24BIT		6


#define GT_ADDR(a)	(softc->baseaddr + btop(a))


#if defined(sun4c)
#define	GT_KVTOP(x)  (x)

#elif defined(sun4m)
#define GT_KVTOP(x) \
    ((hat_getkpfnum((addr_t)x)<<MMU_PAGESHIFT) | (MMU_PAGEOFFSET&(x)))

#endif	/* sun4c */


/*
 * The GT lists are doubly-linked with heads
 *
 * list operations:
 *
 *   Initialize a list head
 *   Obtain the address of the next/prev node
 *   Insert a newnode after/before node
 *   Remove a node from a list
 */
struct gt_list {
	struct gt_list	*l_next;
	struct gt_list	*l_prev;
};

#define OFFSETOF(type, member)  ((unsigned) &((struct type *) 0)->member)

#define GT_LIST_INIT(node, type, list)					\
    ((struct type *) (node))->list.l_next =				\
	(struct gt_list *) ((struct type *) (node))->list.l_prev =	\
	(struct gt_list *) &(((struct type *) (node))->list)

#define GT_LIST_NEXT(node, type, list)					\
    ((struct type *)							\
	((char *) (((struct type *) (node))->list.l_next) -		\
	OFFSETOF(type, list)))

#define GT_LIST_PREV(node, type, list)					\
    ((struct type *)							\
	((char *) (((struct type *) (node))->list.l_prev) -		\
	OFFSETOF(type, list)))

#define GT_LIST_INSERT_NEXT(node, newnode, type, list)			\
    gt_listinsert_next(							\
	(struct gt_list *) &(((struct type *) (node))->list),		\
	(struct gt_list *) &(((struct type *) (newnode))->list) )

#define GT_LIST_INSERT_PREV(node, newnode, type, list)			\
    gt_listinsert_prev(							\
	(struct gt_list *) &(((struct type *) (node))->list),		\
	(struct gt_list *) &(((struct type *) (newnode))->list) )

#define GT_LIST_REMOVE(node, type, list)				\
    gt_listremove((struct gt_list *) &(((struct type *) (node))->list))



/*
 * gt mapping structure
 */
#define GTM_K	0x0001			/* provide kernel mapping	*/
#define GTM_P	0x0002			/* privileged mapping		*/
#define GTM_KP	(GTM_K | GTM_P)

struct gt_map {
	u_int	vaddr;			/* virtual (mmap offset) address  */
	u_int	poffset;		/* physical offset		  */
	u_int	size;			/* size in bytes		  */
	u_short	flags;			/* superuser only protection flag */
} gt_map[] = {
    {RP_BOOT_PROM_VA,	RP_BOOT_PROM_OF,	RP_BOOT_PROM_SZ,    GTM_P},
#ifndef GT_DEBUG
    {RP_FE_CTRL_VA,	RP_FE_CTRL_OF,		RP_FE_CTRL_SZ,      0},
#else GT_DEBUG
    {RP_FE_CTRL_VA,	RP_FE_CTRL_OF,		RP_FE_CTRL_SZ,      GTM_K},
#endif GT_DEBUG
    {RP_SU_RAM_VA,	RP_SU_RAM_OF,		RP_SU_RAM_SZ,       0},
    {RP_HOST_CTRL_VA,	RP_HOST_CTRL_OF,	RP_HOST_CTRL_SZ,    GTM_K},
    {FBI_PFGO_VA,	FBI_PFGO_OF,		FBI_PFGO_SZ,	    GTM_K},
    {FBI_REG_VA,	FBI_REG_OF,		FBI_REG_SZ,	    GTM_K},
    {FBO_CLUT_CTRL_VA,	FBO_CLUT_CTRL_OF,	FBO_CLUT_CTRL_SZ,   GTM_KP},
    {FBO_CLUT_MAP_VA,	FBO_CLUT_MAP_OF,	FBO_CLUT_MAP_SZ,    GTM_KP},
    {FBO_WLUT_CTRL_VA,	FBO_WLUT_CTRL_OF,	FBO_WLUT_CTRL_SZ,   GTM_KP},
    {FBO_WLUT_MAP_VA,	FBO_WLUT_MAP_OF,	FBO_WLUT_MAP_SZ,    GTM_KP},
    {FBO_SCREEN_CTRL_VA,FBO_SCREEN_CTRL_OF,	FBO_SCREEN_CTRL_SZ, GTM_KP},
    {FBO_RAMDAC_CTRL_VA,FBO_RAMDAC_CTRL_OF,	FBO_RAMDAC_CTRL_SZ, GTM_KP},
    {FE_CTRL_VA,	FE_CTRL_OF,		FE_CTRL_SZ,	    GTM_KP},
    {FE_HFLAG1_VA,	FE_HFLAG1_OF,		FE_HFLAG1_SZ,       GTM_K},
    {FE_HFLAG2_VA,	FE_HFLAG2_OF,		FE_HFLAG2_SZ,       GTM_K},
    {FE_HFLAG3_VA,	FE_HFLAG3_OF,		FE_HFLAG3_SZ,       GTM_KP},
    {FE_I860_VA,	FE_I860_OF,		FE_I860_SZ,	    GTM_K},
    {FB_VA,		FB_OF,			FB_SZ,		    0},
    {FB_CD_VA,		FB_CD_OF,		FB_CD_SZ,	    GTM_K},
    {FB_CE_VA,		FB_CE_OF,		FB_CE_SZ,	    GTM_K},
};


/*
 * kernel mapping indexes
 *
 * Note:  these must be kept in agreement with the gt_map array of structs.
 */
#define MAP_RP_BOOT_PROM	 0
#define MAP_RP_FE_CTRL		 1
#define MAP_RP_SU_RAM		 2
#define	MAP_RP_HOST_CTRL	 3
#define	MAP_FBI_PFGO		 4
#define	MAP_FBI_REG		 5
#define	MAP_FBO_CLUT_CTRL	 6
#define	MAP_FBO_CLUT_MAP	 7
#define	MAP_FBO_WLUT_CTRL	 8
#define	MAP_FBO_WLUT_MAP	 9
#define	MAP_FBO_SCREEN_CTRL	10
#define	MAP_FBO_RAMDAC_CTRL	11
#define	MAP_FE_CTRL		12
#define	MAP_FE_HFLAG1		13
#define	MAP_FE_HFLAG2		14
#define MAP_FE_HFLAG3		15
#define	MAP_FE_I860		16
#define MAP_FB			17
#define	MAP_FB_CD		18
#define	MAP_FB_CE		19

#define	MAP_NMAP (sizeof (gt_map) / sizeof (gt_map[0]))


/*
 * graphics context
 */
typedef struct gt_cntxt {
	unsigned gc_fe_hold;

	unsigned gc_rp_host_csr;
	unsigned gc_rp_host_as;

	unsigned gc_fbi_vwclp_x;
	unsigned gc_fbi_vwclp_y;
	unsigned gc_fbi_fb_width;
	unsigned gc_fbi_buf_sel;
	unsigned gc_fbi_stereo_cl;
	unsigned gc_fbi_cur_wid;
	unsigned gc_fbi_wid_ctrl;
	unsigned gc_fbi_wbc;
	unsigned gc_fbi_con_z;
	unsigned gc_fbi_z_ctrl;
	unsigned gc_fbi_i_wmask;
	unsigned gc_fbi_w_wmask;
	unsigned gc_fbi_b_mode;
	unsigned gc_fbi_b_wmask;
	unsigned gc_fbi_rop;
	unsigned gc_fbi_mpg;
	unsigned gc_fbi_s_mask;
	unsigned gc_fbi_fg_col;
	unsigned gc_fbi_bg_col;
	unsigned gc_fbi_s_trans;
	unsigned gc_fbi_dir_size;
	unsigned gc_fbi_copy_src;
} gt_cntxt;


/*
 * shared resource types
 */
enum res_type {
	RT_CLUT8,			/* 8-bit CLUT		*/
	RT_OWID,			/* overlay WID		*/
	RT_IWID,			/* image WID		*/
	RT_FCS,				/* fast-clear set	*/
};

struct  shar_res_type {
	enum res_type	srt_type;
	int		srt_base;
	u_int		srt_count;
	struct shar_res_type	*srt_next;
};


typedef struct	table_type {
	u_int	tt_flags;
	u_int	tt_refcount;
} table_type;

#define TT_USED	0x1


struct	proc_ll_type {
	short			 plt_pid;
	struct shar_res_type 	*plt_sr;
	struct proc_ll_type	*plt_next;
	struct proc_ll_type	*plt_prev;
};


#define NWID		1024
#define NCLUT		  16
#define NCLUT_8		  14
#define NFCS		   4
#define GT_CMAP_SIZE	 256

#define CLUT_REDMASK	0x000000ff
#define CLUT_GREENMASK	0x0000ff00
#define CLUT_BLUEMASK	0x00ff0000

#define NWIDPOST	16	/* max nbr of WID entries per post	*/

#define MAX_DIAG_PROC    8

#define NWID_BITS	10
int	gt_power_table[NWID_BITS+1] = {1,2,4,8,16,32,64,128,256,512,1024};

/*
 * wait channels
 */
#define WCHAN_VRT	0	/* waiting for vertical retrace		*/
#define WCHAN_CLUT	1	/* waiting for CLUT post		*/
#define WCHAN_WLUT	2	/* waiting for WLUT post		*/
#define WCHAN_VCR	3	/* segfault waiting to access a VCR	*/
#define WCHAN_SCHED	4	/* scheduler sleeps here		*/
#define WCHAN_SUSPEND	5	/* waiting for accelerator to suspend	*/
#define	WCHAN_THAW	6	/* waiting for ATU to thaw		*/
#define	WCHAN_ASLOCK	7	/* waiting for an as_fault to complete	*/

#define TOTAL_WCHANS	8


/*
 * driver per-unit data
 */
static struct gt_softc {
    unsigned	flags;				/* internal flags	      */
    u_int	w, h;				/* resolution		      */
    int		monitor;			/* type of monitor	      */
    u_long	baseaddr;			/* physical base address      */
    struct at_proc	*at_head;		/* accel threads list head    */
    struct at_proc	*at_curr;		/* current accel thread	      */
    struct seg	*uvcrseg;			/* current uvcr seg ptr	      */
    struct seg	*svcrseg;			/* current svcr seg ptr	      */
    addr_t	va[MAP_NMAP];			/* mapped in devices	      */
    fe_ctrl_sp	*fe_ctrl_sp_ptr;		/* pointer to FE ctrl space   */
    Hkvc_kmcb	kmcb;				/* host to GT communication   */
    short	gt_grabber;			/* pid of grabbing process    */
    short	server_proc;			/* server process id	      */
    short	diag_proc;			/* diag process count	      */
    short	diag_proc_table[MAX_DIAG_PROC];	/* diag process id	      */
    u_int	wlut_wid_part;			/* sunview overlay wid index  */
    u_int	wid_ocount;			/* nbr of overlay wids	      */
    u_int	wid_icount;			/* nbr of image wids	      */
    short	clut_part_total;		/* nbr of server cluts	      */
    short	clut_part_used;			/* nbr of server cluts in use */
    struct proc_ll_type *proc_list;		/* process linked list that   */
						/* own a GT resource	      */
    u_int	light_pen_enable;		/* lightpen enable/disable    */
    u_int	light_pen_distance;		/* X+Y delta before returning */
						/* lightpen info	      */
    u_int	light_pen_time;			/* nbr of frames before       */
						/* next light pen info	      */
    table_type	wid_otable[NWID]; 		/* overlay wid table	      */
    table_type	wid_itable[NWID]; 		/* image wid table	      */
    table_type	clut_table[NCLUT_8];		/* CLUT table		      */
    table_type	fcs_table[NFCS];		/* FCS table		      */
    u_int	shadow_owlut[NWID];		/* shadow of overlay WLUT     */
    u_int	shadow_iwlut[NWID];		/* shadow of image WLUT       */
    u_char	shadow_cursor[3][2];		/* shadow of CLUTs	      */
    u_int	*printbuf;			/* gt print buffer	      */
    char	wchan[TOTAL_WCHANS];		/* wait channels	      */
    unsigned	gt_user_colors[GT_CMAP_SIZE];	/* kmcb cmap (user)	      */
    unsigned	gt_sys_colors[GT_CMAP_SIZE];	/* kmcb cmap (kernel)	      */
    u_char	gamma[GT_CMAP_SIZE];		/* gamma table		      */
    u_char	degamma[GT_CMAP_SIZE];		/* degamma table	      */
    float	gammavalue;			/* gamma value		      */
    Gt_clut_update kernel_clut_post;		/* CLUT post info (kernel)    */
    Gt_clut_update user_clut_post;		/* CLUT post info (user)      */
    Gt_kernel_wlut_update wlut_post;		/* WLUT POST info	      */
    Gt_wlut_update gt_wlut_update[NWIDPOST];	/* wlut entries		      */
    gt_cntxt	cntxt;				/* initial gfx context	      */
    struct fbgattr *gattr;			/* bw2 emulation: attributes  */
    struct proc	   *owner;			/*                owner	      */
    struct gt_version gt_version;		/* ucode version numbers      */
#if NWIN > 0
    Pixrect	pr;
    struct gt_data	  gtd;
#endif NWIN > 0

} gt_softc[NGT];

/*
 * softc flags
 */
#define	F_VRT_RETRACE		0x00000001
#define	F_DIRECTOPEN		0x00000002
#define	F_ACCELOPEN		0x00000004
#define F_GAMMALOADED		0x00000010
#define F_DEGAMMALOADED		0x00000020
#define	F_GAMMASET		0x00000040
#define	F_VASCO			0x00000080
#define F_WLUT_PEND		0x00000100
#define F_CLUT_PEND		0x00000200
#define F_CLUT_HOLD		0x00001000
#define F_WLUT_HOLD		0x00002000
#define F_GRAB_HOLD		0x00004000
#define F_FREEZE_LDROOT		0x00010000
#define F_FREEZE_UNLOAD		0x00020000
#define F_FREEZE_TLB		0x00040000
#define F_FREEZE_LVL1		0x00080000
#define F_FREEZE_SIG		0x00100000
#define F_PULSED		0x02000000
#define F_LVL1_MISS		0x04000000
#define F_ATU_FAULTING		0x08000000
#define F_SUSPEND		0x10000000
#define F_THREAD_RUNNING	0x20000000
#define F_UCODE_STOPPED		0x40000000
#define F_UCODE_RUNNING		0x80000000

#define F_DONTSCHEDULE	( F_CLUT_HOLD | \
			  F_WLUT_HOLD | \
			  F_GRAB_HOLD   \
			)

#define F_ATU_FROZEN	( F_FREEZE_LVL1		| \
			  F_FREEZE_TLB		| \
			  F_FREEZE_UNLOAD	| \
			  F_FREEZE_LDROOT	| \
			  F_FREEZE_SIG		  \
			)

#define F_GTOPEN	(F_DIRECTOPEN | F_ACCELOPEN)


#if NWIN > 0
struct pixrectops  ops_v = {
	gt_rop,
	gt_putcolormap,
	gt_putattributes,
#ifdef _PR_IOCTL_KERNEL_DEFINED
	gt_ioctl
#endif
};
#endif NWIN > 0



#define GT_DEFAULT_OWID_COUNT		32	/* 5 bits */
#define	GT_DEFAULT_IWID_COUNT		32	/* 5 bits */
#define GT_DEFAULT_SUNVIEW_CLUT8 	0
#define	GT_DEFAULT_SUNVIEW_OWID_INDEX	1
#define	GT_DEFAULT_SUNVIEW_IWID_INDEX	0
#define GT_DEFAULT_SERVER_CLUT_PART	8

#define GT_QUANTUM	(hz/10)		/* scheduler wakeup frequency	*/
#define GT_KMCBTIMEOUT	(hz*120)	/* kmcb timeout:    2 minutes	*/
#define GT_PULSERATE	(30)		/* frontend pulse: 30 seconds	*/
#define	GT_PROBESIZE	(NBPG)

int  gt_quantum = 0;			/* to allow tunability		*/
long gt_pulse;
u_int gt_sync;				/* used to drain write buffers	*/

/*
 * kmcb request queue
 */
static struct gt_kmcblist {
	unsigned	   q_flags;
	struct gt_list	   q_list;
	struct gt_softc	  *q_softc;
	struct Hkvc_kmcb   q_mcb;
} gt_kmcbhead;

/*
 * flags
 */
#define KQ_POSTED	0x00000001
#define KQ_BLOCK	0x00000002


#define MEG		(1024*1024)
#define GTA_PREMAP	3			/* default premap count	*/

#define GT_HASHTBLSIZE	256
#define GT_HASHMASK	((GT_HASHTBLSIZE-1)*PAGESIZE)
#define GT_HASH(a)	(((unsigned)a & GT_HASHMASK)>>PAGESHIFT)


#define GTDVMASZ	(4*MEG + MEG/2)		/* default gt dvmasize	 */
#define GTLOCKSIZE	(8*MEG)			/* default pagelock size */

#ifdef sun4c
caddr_t		gta_dvmaspace;			/* GT DVMA: base address*/
struct seg	gtdvmaseg;
#endif sun4c
u_int		gta_dvmasize = GTDVMASZ;	/* size			*/
u_int		gta_locksize = GTLOCKSIZE;


struct gta_lockmap {
	struct gt_list		 gtm_list;	/* active/free links	*/
	struct gta_lockmap	*gtm_hashnext;	/* hash link		*/
	struct as		*gtm_as;	/* mapped: addr space	*/
	caddr_t			 gtm_addr;	/* 	   virt addr	*/
	struct page		*gtm_pp;
};



int	gta_lockents;				/* nbr of lockpage entries */
int	gta_premap   = GTA_PREMAP;		/* nbr premapped pages	*/

static struct gta_lockmap  *gta_lockmapbase;	/* ptr to GT lock maps	*/
static struct gta_lockmap   gta_lockfree;	/* the map's free list,	*/
static struct gta_lockmap   gta_lockbusy;	/* the in-use list,	*/
static struct gta_lockmap **gta_lockhash;	/* and the hash table	*/

static struct gt_lvl1	  *gta_dummylvl1;	/* to abort GT faults	*/


/*
 * accelerator thread data
 */
struct at_proc {
    struct gt_list	 ap_list;	/* links			*/
    struct gt_softc	*ap_softc;	/* softc pointer		*/
    struct proc		*ap_proc;	/* proc pointer			*/
    struct as		*ap_as;		/* address space pointer	*/
    struct gt_lvl1	*ap_lvl1;	/* level 1 page table		*/
    struct Hkvc_umcb	*ap_umcb;	/* user mcb pointer		*/
    unsigned		*ap_uvcr;	/* user vcr address		*/
    unsigned		*ap_svcr;	/* user signal vcr address	*/
    unsigned		 ap_flags;	/* thread control		*/
    addr_t		 ap_lastfault;	/* address last faulted by GT	*/
    int			 ap_lockcount;	/* gt aslock reference count	*/
    short		 ap_pid;	/* process's pid		*/
};

/*
 * ap_flags
 */
#define GTA_CONNECT	0x00000001	/* thread is connected		*/
#define GTA_SWLOCK	0x00000002	/* swlock on process		*/
#define GTA_VIRGIN	0x00000010	/* thread hasn't run yet	*/
#define GTA_SUSPEND	0x00000020	/* thread is suspended		*/
#define	GTA_EXIT	0x00000040	/* thread is exiting		*/
#define	GTA_DONTRUN	0x00000080	/* thread is not runnable	*/
#define GTA_GTLOCK	0x00000100	/* thread is GT as_locked	*/
#define GTA_HOSTLOCK	0x00000200	/* thread is HOST as_locked	*/
#define GTA_UVCR	0x00001000	/* thread's user vcr bit	*/
#define GTA_SVCR	0x00002000	/* thread's signal vcr bit	*/
#define GTA_SIGNAL	0x00010000	/* signal the host process	*/
#define GTA_ABORT	0x10000000	/* thread being aborted		*/

#define GTA_BLOCKED	(GTA_VIRGIN	| \
			 GTA_SUSPEND	| \
			 GTA_EXIT	| \
			 GTA_DONTRUN	| \
			 GTA_ABORT	  \
			)



/*
 * Addresses and indexes associated with a GT mapping
 */
struct gta_mmudata {
	struct gt_pte	*gtu_pte;
	struct gt_lvl1	*gtu_lvl1;
	struct gt_lvl2	*gtu_lvl2;
	int		 gtu_index1;
	int		 gtu_index2;
};


enum mia_ops {
	MIA_LOADROOT,			/* load root pointer	*/
	MIA_CLEARROOT,			/* clear root pointer	*/
};


/*
 * Reasons for aborting a page fault
 */
enum abort_types {
	GT_ABORT_LVL1,			/* level 1 unexpected		*/
	GT_ABORT_TLB,			/* tlb miss: exit or unexpected	*/
	GT_ABORT_BADADDR,		/* tlb miss: bad address	*/
	GT_ABORT_FAULTERR,		/* error from as_fault		*/
};


/*
 * Mainbus device data
 */

int ngt = 0;

struct dev_ops gt_ops = {
	0,			/* revision */
	gt_identify,
	gt_attach,
	gtopen,
	gtclose,
	0, 0, 0, 0, 0,
	gtioctl,
	0,
	gtsegmap,
};

addr_t	gt_promrev;		/* OBP rev level	*/


/*
 * Monitor-specific data.  This array must be kept in the order
 * specified by the gtreg.h define constants GT_1280_76, ...
 *
 * 1:    type           h      w  depth cms     size
 * 2:    fbwidth
 */
static struct gt_monitor {
	struct fbtype gtm_fbtype;
	int gtm_fbwidth;
} gt_monitor[] =  {
    { {FBTYPE_SUNGT,  1024,  1280,  8,  256,  0xa00000},	/* 1280_76 */
    GT_MONITOR_TYPE_STANDARD},

    { {FBTYPE_SUNGT,  1024,  1280,  8,  256,  0xa00000},	/* 1280_67 */
    GT_MONITOR_TYPE_STANDARD},

    { {FBTYPE_SUNGT,   900,  1152,  8,  256,  0xa00000},	/* 1152_66 */
    GT_MONITOR_TYPE_STANDARD},

    { {FBTYPE_SUNGT,   680,   960,  8,  256,   0xa0000},	/* stereo  */
    GT_MONITOR_TYPE_STEREO},

    { {FBTYPE_SUNGT,  1035,  1920,  8,  256,  0xf2a000},	/* hdtv    */
    GT_MONITOR_TYPE_HDTV},

    { {FBTYPE_SUNGT,   480,   640,  8,  256,   0xa0000},	/* ntsc    */
    GT_MONITOR_TYPE_STANDARD},

    { {FBTYPE_SUNGT,   575,   765,  8,  256,   0xa0000},	/* pal     */
    GT_MONITOR_TYPE_STANDARD},
};

#define NGT_MONITORS (sizeof (gt_monitor) / sizeof (gt_monitor[0]))

struct gt_fault {
	u_int		 gf_type;	/* common data			*/
	struct gt_softc *gf_softc;
	struct at_proc	*gf_at;
	short		 gf_pid;

	addr_t		 gf_addr;	/* data used for invalid pte	*/

	struct gt_lvl1	*gf_lvl1;	/* data used for invalid ptp	*/
	struct gt_ptp	*gf_ptp;
	int		 gf_index;
} gt_fault;

/*
 * gt_fault flags
 */
#define GF_LVL1_FAULT	0x00000001
#define GF_LVL2_FAULT	0x00000002
#define	GF_SENDSIGNAL	0x00000004
#define	GF_BOGUS	0x00000010


/*
 * The pagein daemon and the scheduler flags are not in softc.
 * These processes serve all GTs.
 */
static u_char	gta_initflag;
static u_char	gt_daemonflag;
static short	gt_pagedaemonpid;

static int	gt_priority;		/* device priority	*/


static int	seggta_create	 (/* seg, argsp			*/);
static int	seggta_dup	 (/* seg, newsegp		*/);
static int	seggta_unmap	 (/* seg, addr, len		*/);
static int	seggta_free	 (/* seg			*/);
static faultcode_t  seggta_fault (/* seg, addr, len, type, rw	*/);
static faultcode_t  seggta_faulta(/* seg, addr			*/);
static int	seggta_hatsync	 (/* seg, addr, ref, mod, flags	*/);
static int	seggta_setprot	 (/* seg, addr, len, prot	*/);
static int	seggta_checkprot (/* seg, addr, len, prot	*/);
static int	seggta_kluster	 (/* seg, addr, delta		*/);
static u_int	seggta_swapout	 (/* seg			*/);
static int	seggta_sync	 (/* seg, addr, len, flags	*/);
static int	seggta_incore	 (/* seg, addr, len, vec	*/);
static int	seggta_lockop	 (/* seg, addr, len, op		*/);
static int	seggta_advise	 (/* seg, addr, len, function	*/);

struct seg_ops seggta_ops = {
	seggta_dup,		/* dup		*/
	seggta_unmap,		/* unmap	*/
	seggta_free,		/* free		*/
	seggta_fault,		/* fault	*/
	seggta_faulta,		/* faulta	*/
	seggta_hatsync,		/* hatsync	*/
	seggta_setprot,		/* setprot	*/
	seggta_checkprot,	/* checkprot	*/
	seggta_kluster,		/* kluster	*/
	seggta_swapout,		/* swapout	*/
	seggta_sync,		/* sync		*/
	seggta_incore,		/* incore	*/
	seggta_lockop,		/* lockop	*/
	seggta_advise,		/* advise	*/
};


static int	seggtx_dup	 (/* seg, newsegp		*/);
static int	seggtx_unmap	 (/* seg, addr, len		*/);
static int	seggtx_free	 (/* seg			*/);
static faultcode_t  seggtx_fault (/* seg, addr, len, type, rw	*/);
static int	seggtx_hatsync	 (/* seg, addr, ref, mod, flags	*/);
static int	seggtx_setprot	 (/* seg, addr, len, prot	*/);
static int	seggtx_checkprot (/* seg, addr, len, prot	*/);
static int	seggtx_incore	 (/* seg, addr, len, vec	*/);

static int seggtx_badop();
static int seggtx_create();

struct seg_ops seggtx_ops = {
	seggtx_dup,		/* dup		*/
	seggtx_unmap,		/* unmap	*/
	seggtx_free,		/* free		*/
	seggtx_fault,		/* fault	*/
	seggtx_badop,		/* faulta	*/
	seggtx_hatsync,		/* hatsync	*/
	seggtx_setprot,		/* setprot	*/
	seggtx_checkprot,	/* checkprot	*/
	seggtx_badop,		/* kluster	*/
	(u_int (*)()) NULL,	/* swapout	*/
	seggtx_badop,		/* sync		*/
	seggtx_incore,		/* incore	*/
	seggtx_badop,		/* lockop	*/
	seggtx_badop,		/* advise	*/
};


extern int cpu;			/* CPU type	*/
extern u_int gt_printbuffer[];	/* GT printbuffer  */
extern u_int gt_printbuffer_size;


#ifdef GT_DEBUG
/*
 * GT statistics
 */
struct gtstat {
	u_int	gs_ioctl;		/* ioctl count			*/
	u_int	gs_pollerrs;		/* errors reading hisr		*/

	u_int	gs_intr;		/* interrupt count		*/
	u_int	gs_intr_kvcr;		/* kvcr interrupt count		*/
	u_int	gs_intr_uvcr;		/* uvcr interrupt count		*/
	u_int	gs_intr_lvl1;		/* lvl1 interrupt count		*/
	u_int	gs_intr_tlb;		/* tlb interrupt count		*/
	u_int	gs_intr_lightpen;	/* lightpen interrupts		*/

	u_int	gs_widpost;		/* WID posts			*/
	u_int	gs_clutpost;		/* CLUT posts			*/
	u_int	gs_freemap;		/* dvma freemap operations	*/
	u_int	gs_suspend;		/* suspend count		*/
	u_int	gs_grabhw;		/* grab accelerator count	*/

	u_int	gs_seggt_fault;		/* seggt faults			*/
	u_int	gs_seggtx_fault;	/* seggtx  faults		*/
	u_int	gs_seggta_fault;	/* seggta  faults		*/

	u_int	gs_cntxsave;		/* graphics context saves	*/
	u_int	gs_cntxrestore;		/* graphics context restores	*/

	u_int	gs_mapsteal;		/* dvma map steals		*/
	u_int	gs_scheduler;		/* scheduler wakeups		*/
	u_int	gs_threadswitch;	/* scheduler thread switches	*/
	u_int	gs_kmcb_cmd;		/* kernel kmcb cmd count	*/
	u_int	gs_freeze_atu;		/* freeze atu count		*/
} gtstat;
#endif GT_DEBUG

static struct fbgattr gt_attr;
static struct fbgattr gt_attrdefault = {
	FBTYPE_SUNGT,			/* real type */
	0,				/* owner */
	/* fbtype struct init */
	{
		FBTYPE_SUNGT,
		1024,
		1280,
		8,
		256,
		0xa00000
	},
	/* fbsattr struct init */
	{
		0,			/* flags */
		-1,			/*emu type */
		/* dev_specific */
		{
		  0,
		  0,
		  0,
		  0, 
		  0,
		  0,
	 	  0,
		  0
		}
	},
	/* emu_types */
	{
	  FBTYPE_SUNGT,
	  FBTYPE_SUN2BW,
	  -1, -1
	}
};


/*
 * Determine whether a GT exists at the given address.
 */
gt_identify(name)
	char *name;
{
	if (strcmp(name, "gt") == 0)
		return (++ngt);
	else
		return (0);
}


gt_attach(devi)
	struct dev_info *devi;
{
	struct gt_softc *softc;
	addr_t		 reg, regs;
	int		 gt_bustype;
	static int	 unit;
	addr_t		*vp;
	struct gt_map	*mp;
	struct gt_map   *lastmap  = &gt_map[MAP_NMAP];

	/*
	 * We currently support only one GT on a workstation
	 */
	if (unit > 0)
		return (-1);

#ifdef sun4c
	/*
	 * If the system is a sparcstation 2, set the MICRO_TLB bit
	 * in the system enable register.  This is a workaround for
	 * bug 1045662.
	 */
	if (cpu == CPU_SUN4C_75)
		on_enablereg(1);
#endif

	softc = &gt_softc[unit];

	/*
	 * Grab properties from the GT OpenBoot PROM
	 */
	softc->w = getprop(devi->devi_nodeid, "width",  1280);
	softc->h = getprop(devi->devi_nodeid, "height", 1024);
	gt_promrev = getlongprop(devi->devi_nodeid, "promrev");

	/*
	 * Map in the PROM section (1 page) to calculate the
  	 * baseaddr of the device and unmap it when done.
	 */
	regs = devi->devi_reg[0].reg_addr;
	gt_bustype = devi->devi_reg[0].reg_bustype;

	if (!(reg = map_regs(regs, (u_int) NBPG, gt_bustype)))
		return (-1);

	softc->baseaddr = (u_long) fbgetpage(reg);
	unmap_regs((addr_t) reg, (u_int) NBPG);

	/*
	 * Establish kernel mappings to the various GT sections.
	 * Verify that the hardware responds to being probed.
	 */
	for (mp=gt_map, vp=softc->va; mp < lastmap;  mp++, vp++) {

		if (mp->flags & GTM_K) {

			if (!(*vp = map_regs(regs + mp->poffset,
			    mp->size, gt_bustype))) {
				--vp;
				--mp;
				goto err;
			}
		}
	}

	devi->devi_unit	= unit;
	gt_priority	= ipltospl(devi->devi_intr[0].int_pri);
	addintr(devi->devi_intr[0].int_pri, gtpoll, devi->devi_name, unit);

	/* save back pointer to softc */
	devi->devi_data = (addr_t) softc;

	/* Initialize all of the software data structures */
	softc->fe_ctrl_sp_ptr = (fe_ctrl_sp *)softc->va[MAP_FE_CTRL];
	gt_shared_init(softc, (u_int)0);

	/*
	 * Save context for reuse when window system shuts down.
	 * This is pretty ugly, but we need to turn off the fe_hold
	 * bit in the saved context.
	 */
	gt_cntxsave(softc, &softc->cntxt);
	softc->cntxt.gc_fe_hold = 0;			/* yuck	*/

#ifdef sun4m
	/*
	 * sun4m:
	 *
	 *    Set the IOMMU Bypass bit in the SBus Slot Configuration
	 *    register.  This is more properly done in the device's OpenBoot
	 *    PROM but the GT predated sun4m.  If/when the GT OpenBoot PROM
	 *    is revved, this function should migrate to there.
	 *
	 *    Set 32-byte burst transfer in, of all places, the HSA's
	 *    diagnostic register.  This seems *essential* for healthy
	 *    running on Viking platforms.
	 */
	{
		u_int	slot, regp, val, configbits;
		extern struct modinfo mod_info[];

#define BY	0x00000001	/* IOMMU Bypass				*/
#define CP	0x00008000	/* Cacheable				*/

#define HSA_OF	0x0084f000	/* HSA Diagnostic register		*/
#define HSA_32B	0x30002		/* Enable 32-byte burst read/write	*/

		if ((mod_info[0].mod_type == CPU_VIKING) ||
		    (mod_info[0].mod_type == CPU_VIKING_MXCC)) {
			configbits = CP|BY;
		} else {
			configbits = BY;
		}

		slot = (u_int)(regs) >> 28;
		regp = 0xe0001010 + 4*slot;

		val  = gt_ldsbusreg(regp);
		val |= configbits;
		(void) gt_stsbusreg(regp, val);

		if (!(reg = map_regs(regs + HSA_OF, (u_int)NBPG, gt_bustype))) {
			printf("gt_attach: cannot map HSA diag register\n");
			return (-1);
		}

		*reg |= HSA_32B;

		unmap_regs((addr_t) reg, (u_int) NBPG);
	}
#endif /* sun4m */


	/* prepare for next attach */
	unit++;

	report_dev(devi);

	return (0);

	/*
	 * probe error:  unmap anything already mapped.
	 */
err:
	while (vp >= softc->va) {
		if (vp)
			unmap_regs(*vp, mp->size);
		vp--;
		mp--;
	}

	return (-1);
}


gtopen(dev, flag)
	dev_t	dev;
	int	flag;
{
	struct gt_softc *softc = &gt_softc[minor(dev)>>1];
	dev_t	 device;
	unsigned port;
	int	 rc;

	/*
	 * Make a pseudo-device number for fbopen().
	 * Each physical GT device occupies two
	 * minor-device slots.
	 */
	device = makedev(major(dev), minor(dev)>>1);

	rc = fbopen(device, flag, ngt);

	if (rc == 0) {
		if (minor(dev) & GT_ACCEL_PORT)
			port = (unsigned) F_ACCELOPEN;
		else
			port = (unsigned) F_DIRECTOPEN;

		gt_set_flags(&softc->flags, port);

		if (gta_initflag == (u_char)0) {
			gta_init();
			gta_initflag = 1;
		}
	}

	return (rc);
}


/*ARGSUSED*/
gtclose(dev, flag)
	dev_t	dev;
	int	flag;
{
	struct gt_softc *softc = &gt_softc[minor(dev)>>1];
	unsigned port;

	if (minor(dev) & GT_ACCEL_PORT)
		port = (unsigned) F_ACCELOPEN;
	else
		port = (unsigned) F_DIRECTOPEN;

	gt_clear_flags(&softc->flags, port);

	if (!(softc->flags & F_GTOPEN)) {

		gt_shared_init(softc,
		    (u_int)(F_UCODE_RUNNING | F_VRT_RETRACE | F_GAMMALOADED |
			    F_DEGAMMALOADED | F_GAMMASET    | F_VASCO));

		/*
		 * Restore the original graphics context
		 */
		gt_cntxrestore(softc, &softc->cntxt);

		/*
		 * XXX: Kill the GT daemons
		 * if (gta_initflag) {
		 *	gta_initflag = (u_char)0;
		 * }
		 */
	}
}


/*ARGSUSED*/
gtmmap(dev, off, prot)
	dev_t	dev;
	off_t	off;
	int	prot;
{
	struct gt_softc *softc = &gt_softc[minor(dev)>>1];
	struct gt_map   *mp;

	/*
	 * Mapping requests for a diagnostic process don't
	 * require protection checking or offset translation.
	 */
	if (gt_diag_check(softc))
		return (GT_ADDR(off));

	/*
	 * bwtwo emulation mode?
	 */
	if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW) {
		/*
		 * resolve only for cursor data plane
		 */
		return (GT_ADDR(off+FB_CD_OF));
	}
		
		
	/*
	 * All other requests are validated and translated.
	 */
	for (mp=gt_map; mp < &gt_map[MAP_NMAP]; mp++) {

		if (off >= (off_t)mp->vaddr &&
		    off < (off_t)(mp->vaddr+mp->size)) {

			if ((mp->flags & GTM_P) &&
			    !(suser() || gt_server_check(softc))) {
				return (-1);
			}

			return (GT_ADDR(mp->poffset + (off - mp->vaddr)));
		}
	}

	return (-1);
}


/*ARGSUSED*/
gtioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t	data;
	int	flag;
{
	struct gt_softc *softc = &gt_softc[minor(dev)>>1];

#ifdef GT_DEBUG
	gtstat.gs_ioctl++;
#endif GT_DEBUG

	switch (cmd) {

	case FBIOPUTCMAP:	return (gt_fbioputcmap(softc, data));
	case FBIOGETCMAP:	return (gt_fbiogetcmap(softc, data));

	case FBIOGTYPE:		return (gt_fbiogtype(softc, data));

#if NWIN > 0
	case FBIOGPIXRECT:	return (gt_fbiogpixrect(dev, softc, data));
#endif NWIN > 0

	case FBIOSVIDEO:	return (gt_fbiosvideo(softc, data));
	case FBIOGVIDEO:	return (gt_fbiogvideo(softc, data));

	case FBIOVERTICAL:	return (gt_fbiovertical(softc, data));

	case FBIO_WID_ALLOC:	return (gt_widalloc(softc, data));
	case FBIO_WID_FREE:	return (gt_widfree(softc, data));

	case FBIO_WID_GET:	return (gt_widread(softc, data));
	case FBIO_WID_PUT:	return (gt_widpost(softc, data));

	case FB_CLUTALLOC:	return (gt_clutalloc(softc, data));
	case FB_CLUTFREE:	return (gt_clutfree(softc, data));

	case FB_CLUTREAD:	return (gt_clutread(softc, data));
	case FB_CLUTPOST:	return (gt_clutpost(softc, data));

	case FB_FCSALLOC:	return (gt_fcsalloc(softc, data));
	case FB_FCSFREE:	return (gt_fcsfree(softc, data));

	case FB_SETSERVER:	return (gt_setserver(softc));
	case FB_SETDIAGMODE:	return (gt_setdiagmode(softc));
	case FB_SETWPART:	return (gt_setwpart(softc, data));
	case FB_GETWPART:	return (gt_getwpart(softc, data));

	case FB_SETMONITOR:	return (gt_setmonitor(softc, data));
	case FB_GETMONITOR:	return (gt_getmonitor(softc, data));

	case FB_LOADKMCB:	return (gt_loadkmcb(softc));

	case FB_CONNECT:	return (gt_connect(softc, data, dev));
	case FB_DISCONNECT:	return (gt_disconnect(softc));

	case FB_GRABHW:		return (gt_grabhw(softc, data));
	case FB_UNGRABHW:	return (gt_ungrabhw(softc));

	case FB_VMBACK:		return (gt_vmback(data));
	case FB_VMUNBACK:	return (gt_vmunback(data));

	case FB_SETCLUTPART:	return (gt_setclutpart(softc, data));
	case FB_GETCLUTPART:	return (gt_getclutpart(softc, data));

	case FB_LIGHTPENENABLE:   return (gt_lightpenenable(softc, data));
	case FB_SETLIGHTPENPARAM: return (gt_setlightpenparam(softc, data));
	case FB_GETLIGHTPENPARAM: return (gt_getlightpenparam(softc, data));

	case FB_VMCTL:		return (gt_vmctl(softc, data));

	case FB_SETGAMMA:	return (gt_setgamma(softc, data));
	case FB_GETGAMMA:	return (gt_getgamma(softc, data));

	case FBIOSATTR:		return (gt_fbiosattr(softc, data));
	case FBIOGATTR:		return (gt_fbiogattr(softc, data));

	case FB_GT_SETVERSION:	return (gt_sgversion(softc, data, 0));
	case FB_GT_GETVERSION:	return (gt_sgversion(softc, data, 1));

	default:		return (ENOTTY);

	}
}


/*
 * Called from the level 1 interrupt handler.
 *
 * The stuff about peeking at the HISR rather than simply reading
 * it outright is because the GT may be unresponsive to bus requests
 * and we want to avoid kernel bus timeouts.
 * XXX: Remove the peek kludgery if/when it is outdated.
 */
#define MAXPEEKS	5	/* try peeking at HISR this many times	*/
#define PEEKINTERVAL	1000	/* spin loop 1 msec between peeks	*/

gtpoll()
{
	struct gt_softc *softc;
	fe_ctrl_sp	*fep;
	fe_i860_sp	*fe;
	unsigned	intr_type, lcount;
	int		serviced = 0;

	for (softc = gt_softc; softc < &gt_softc[NGT]; softc++) {
		fep = softc->fe_ctrl_sp_ptr;
		fe  = (fe_i860_sp *)softc->va[MAP_FE_I860];

		if (fep) {

			/*
			 * Is this device interrupting?
			 */
			for (lcount=0;;) {
				if (peekl((long *) &fep->fe_hisr,
				    (long *) &intr_type) == 0)
					break;

#ifdef GT_DEBUG
				gtstat.gs_pollerrs++;
#endif GT_DEBUG

				if (++lcount == MAXPEEKS) {
					panic("gtpoll: can't rd hisr");
				}

				DELAY(PEEKINTERVAL);
			}

			intr_type = intr_type & HISR_INTMASK;

			if (intr_type || (fe->hoint_stat & FE_LBTO)) {
				gtintr(softc, intr_type);
				serviced++;
			}
		}
	}

	return (serviced);
}


static void
gtintr(softc, intr_type)
	struct gt_softc *softc;
	u_int intr_type;
{
	fe_ctrl_sp	*fep = softc->fe_ctrl_sp_ptr;
	fe_i860_sp	*fe  = (fe_i860_sp *)softc->va[MAP_FE_I860];
	fe_page_1	*fp1 = (fe_page_1 *) softc->va[MAP_FE_HFLAG1];
	struct gt_fault	*gfp = &gt_fault;

#ifdef GT_DEBUG
	gtstat.gs_intr++;
#endif GT_DEBUG

	/*
	 * Bus-related error?
	 */
	if (intr_type & (HISR_MSTO|HISR_BERR|HISR_SIZE)) {

		if (intr_type & HISR_MSTO) {		/* XXX: needs work */
			gt_status_dump(softc, DUMP_GT_REGS);
			printf("gtintr: master timeout, par = 0x%X\n",
			    fep->fe_par);
		}

		if (intr_type & HISR_BERR) {		/* XXX: needs work */
			printf("gtintr: master bus error, par = 0x%X\n",
			    fep->fe_par);
			gt_status_dump(softc, DUMP_GT_REGS);
			panic("gt master bus error");
		}

		if (intr_type & HISR_SIZE) {		/* XXX: needs work */
			printf("gt interrupt: slave size error\n");
		}

		fep->fe_hisr &= ~(HISR_MSTO|HISR_BERR|HISR_SIZE);
		gt_sync = fep->fe_hisr;
	}

	/*
	 * How about a local-bus error?
	 * XXX: find a better way to handle this
	 */
	if (fe->hoint_stat & FE_LBTO) {
		printf("gtintr: local bus timeout error, lb_err_addr: 0x%X\n",
		    fe->lb_err_addr);

		gt_status_dump(softc, DUMP_GT_REGS);

		panic("gtintr: local bus timeout error");
	}
	

	/*
	 * ATU interrupt
	 */
	if (intr_type & HISR_ATU) {

		if (intr_type & HISR_ROOT_INV) {

			gt_status_dump(softc, DUMP_MIA_REGS);
			panic("gtintr invalid root ptr");

		} else if (intr_type & HISR_LVL1_INV) {
			gtintr_lvl1(softc);

		} else if (intr_type & HISR_TLB_MISS) {
			gtintr_tlb(softc);
		}
	}

	/*
	 * kernel VCR interrupt:  handle the interrupt and
	 * clear the GT's kernel VCR and interrupt condition.
	 */
	if (intr_type & HISR_KVCR) {
		u_int status;

		if (fep->fe_dflag_0 == 0) {
#ifdef GT_DEBUG
			printf("gtintr: spurious kvcr interrupt\n");
#endif GT_DEBUG
			fep->fe_hisr &= ~HISR_KVCR;
			gt_sync = fep->fe_hisr;
		} else {
#ifdef GT_DEBUG
			gtstat.gs_intr_kvcr++;
#endif GT_DEBUG

			gtintr_kmcb(softc);

			status = softc->kmcb.status;

			fep->fe_dflag_0  =  0;
			gt_sync          = fep->fe_dflag_0;
			fep->fe_hisr    &= ~HISR_KVCR;
			gt_sync          = fep->fe_hisr;

			if (status & HKKVS_DONE_WITH_REQUEST)
				wakeup(&softc->wchan[WCHAN_SCHED]);
		}
	}

	/*
	 * User VCR interrupt:  Clear GT's user VCR and the interrupt
	 * condition, and ask the pagein daemon to signal the user.
	 */
	if (intr_type & HISR_UVCR) {
		if (fp1->fe_dflag_1 == 0) {
#ifdef GT_DEBUG
			printf("gtintr: spurious uvcr interrupt\n");
#endif GT_DEBUG
			fep->fe_hisr    &= ~HISR_UVCR;
			gt_sync          = fep->fe_hisr;
		} else if (softc->at_curr != softc->at_head) {
#ifdef GT_DEBUG
			gtstat.gs_intr_uvcr++;
#endif GT_DEBUG
			gfp->gf_type   |= GF_SENDSIGNAL;
			gfp->gf_softc	= softc;
			gfp->gf_at      = softc->at_curr;
			gfp->gf_pid     = softc->at_curr->ap_proc->p_pid;

			softc->at_curr->ap_flags |= GTA_SIGNAL;

			gt_freeze_atu(softc, F_FREEZE_SIG);

			fp1->fe_dflag_1  =  0;
			gt_sync          = fp1->fe_dflag_1;
			fep->fe_hisr    &= ~HISR_UVCR;
			gt_sync          = fep->fe_hisr;

			wakeup((caddr_t) &gt_daemonflag);
		} else {
			fp1->fe_dflag_1  =  0;
			gt_sync          = fp1->fe_dflag_1;
			fep->fe_hisr    &= ~HISR_UVCR;
			gt_sync          = fep->fe_hisr;
		}
	}
}


/*
 * A level 1 page table invalid condition can only legitimately
 * occur if the appropriate level 2 page table hasn't yet been
 * been allocated.
 */
static
gtintr_lvl1(softc)
	struct gt_softc *softc;
{
	fe_ctrl_sp	*fep = softc->fe_ctrl_sp_ptr;
	fe_page_1	*fp1 = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	struct gt_fault	*gfp = &gt_fault;

#ifdef GT_DEBUG
	gtstat.gs_intr_lvl1++;
#endif GT_DEBUG

	/*
	 * Freeze the GT's ATU
	 */
	gt_freeze_atu(softc, F_FREEZE_LVL1);

	/*
	 * Form the index into the level 1 page table and the
	 * pointers to the PTP and its virtual address.
	 */
	if (!(softc->flags & F_THREAD_RUNNING)) {
		gfp->gf_type |= (GF_LVL1_FAULT | GF_BOGUS);
	} else {
		gfp->gf_type  |= GF_LVL1_FAULT;
		gfp->gf_softc  = softc;
		gfp->gf_at     = softc->at_curr;
		gfp->gf_pid    = softc->at_curr->ap_proc->p_pid;

		gfp->gf_lvl1   = (struct gt_lvl1 *)softc->at_curr->ap_lvl1;
		gfp->gf_index  = (fp1->fe_rootindex & INDX1_MASK) >> 2;
		gfp->gf_ptp    = &(gfp->gf_lvl1->gt_ptp[gfp->gf_index]);
	}

	/*
	 * Clear the interrupt condition and wake the pagein daemon.
	 */
	softc->flags |= F_LVL1_MISS;
	fep->fe_hisr &= ~(HISR_ATU|HISR_LVL1_INV|HISR_TLB_MISS);
	gt_sync = fep->fe_hisr;

	wakeup((caddr_t) &gt_daemonflag);
}


static
gtintr_tlb(softc)
	struct gt_softc *softc;
{
	fe_ctrl_sp	*fep = softc->fe_ctrl_sp_ptr;
	fe_page_1	*fp1 = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	struct gt_fault	*gfp = &gt_fault;
	addr_t		 vaddr;

#ifdef GT_DEBUG
	gtstat.gs_intr_tlb++;
#endif GT_DEBUG

	/*
	 * Freeze the GT's ATU.  The thaw will be done by the pagein daemon.
	 */
	gt_freeze_atu(softc, F_FREEZE_TLB);

	/*
	 * Reconstruct the virtual address.
	 */
	vaddr = (addr_t)
	    ((fp1->fe_rootindex & INDX1_MASK) << INDX1_SHIFT |
	     (fep->fe_lvl1index & INDX2_MASK) << INDX2_SHIFT |
	     (fep->fe_par & PGOFF_MASK));


	if (!(softc->flags & F_THREAD_RUNNING)) {
		gfp->gf_type |= (GF_LVL2_FAULT | GF_BOGUS);
	} else {
		gfp->gf_type  |= GF_LVL2_FAULT;
		gfp->gf_softc  = softc;
		gfp->gf_at     = softc->at_curr;
		gfp->gf_pid    = softc->at_curr->ap_proc->p_pid;

		gfp->gf_addr   = vaddr;
	}

	/*
	 * Clear the interrupt condition and wake the pagein daemon.
	 * XXX:  Here we could use the access type, currently unavailable
	 */
	softc->flags |= F_ATU_FAULTING;
	fep->fe_hisr &= ~(HISR_ATU|HISR_TLB_MISS);
	gt_sync = fep->fe_hisr;

	wakeup((caddr_t) &gt_daemonflag);
}


u_int	prev_button_state = 0xffffffff;
int 	prev_x_loc     = -1;
int	prev_y_loc     = -1;

static
gtintr_kmcb(softc)
	struct gt_softc *softc;
{
	Hkvc_kmcb *kmcb = &softc->kmcb;

	if (kmcb->status & HKKVS_INSTRUCTION_TRAP) {	/* XXX: */
		printf("gtintr: unexpected trap to kernel\n");
	}

	if (kmcb->status & HKKVS_ERROR_CONDITION) {	/* XXX:	*/
		printf("gtintr: gt error %d\n", kmcb->errorcode);
	}


	if (kmcb->status & HKKVS_PRINT_STRING) {
		static int  partial_line = 0;
		char       *printbuf     = (char *)softc->printbuf;

		if (partial_line == 0)
			printf("gt%d: ", softc-gt_softc);

		partial_line = (printbuf[strlen(printbuf)-1] == '\n') ? 0:1;

		printf("%s", softc->printbuf);
	}


	if (kmcb->status &
	    (HKKVS_LIGHT_PEN_NO_HIT		|
	     HKKVS_LIGHT_PEN_BUTTON_UP		|
	     HKKVS_LIGHT_PEN_BUTTON_DOWN	|
	     HKKVS_LIGHT_PEN_HIT
	    )) {
		u_int	lightpen_value = 0;
		u_int	current_button_state = prev_button_state;
		int 	current_x_loc	     = prev_x_loc;
		int	current_y_loc	     = prev_y_loc;
#ifdef GT_DEBUG
		gtstat.gs_intr_lightpen++;
#endif GT_DEBUG
		if (kmcb->status & HKKVS_LIGHT_PEN_BUTTON_DOWN)
			current_button_state = LIGHTPEN_BUTTON_DOWN;

		if (kmcb->status & HKKVS_LIGHT_PEN_BUTTON_UP)
			current_button_state = LIGHTPEN_BUTTON_UP;

		if (kmcb->status & HKKVS_LIGHT_PEN_HIT) {
			current_x_loc = kmcb->light_pen_hit_x;
			current_y_loc = kmcb->light_pen_hit_y;
		}
		if (current_button_state  == prev_button_state) {
			/* Button State is same */
			if (current_button_state == LIGHTPEN_BUTTON_UP){
				if ((prev_x_loc != current_x_loc) ||
				    (prev_y_loc != current_y_loc)) {
					
					lightpen_value = (LIGHTPEN_BUTTON_UP <<
					    LIGHTPEN_SHIFT_BUTTON) |
					    (current_y_loc << 
					    LIGHTPEN_SHIFT_Y) |
					    current_x_loc;

					(void) store_lightpen_events(
					    LIGHTPEN_MOVE, lightpen_value);
				} 
			} else {	
				if ((prev_x_loc != current_x_loc) ||
					(prev_y_loc != current_y_loc)) {
					
					lightpen_value = (LIGHTPEN_BUTTON_DOWN <<
					    LIGHTPEN_SHIFT_BUTTON) |
					    (current_y_loc << 
					    LIGHTPEN_SHIFT_Y) |
					    current_x_loc;

					(void) store_lightpen_events(
					    LIGHTPEN_DRAG, lightpen_value);
				} 
			}
		} else {
			/* Button State is not equal */
			(void) store_lightpen_events(LIGHTPEN_BUTTON,
			    current_button_state);

			if ((prev_x_loc != current_x_loc) ||
			    (prev_y_loc != current_y_loc)) {
				if (current_button_state == LIGHTPEN_BUTTON_UP){
					lightpen_value = (LIGHTPEN_BUTTON_UP <<
					    LIGHTPEN_SHIFT_BUTTON) |
					    (current_y_loc << 
					    LIGHTPEN_SHIFT_Y) |
					    current_x_loc;

					(void) store_lightpen_events(
					    LIGHTPEN_MOVE, lightpen_value);
				} else {	
					lightpen_value = (LIGHTPEN_BUTTON_DOWN
					 << LIGHTPEN_SHIFT_BUTTON) |
					    (current_y_loc << 
					    LIGHTPEN_SHIFT_Y) |
					    current_x_loc;

					(void) store_lightpen_events(
					    LIGHTPEN_DRAG, lightpen_value);
				} 
			}
		}

		prev_button_state = current_button_state;
		prev_x_loc = current_x_loc;
		prev_y_loc = current_y_loc;

		/* Check for lightpen NO HIT for a few frames */
		if (kmcb->status & HKKVS_LIGHT_PEN_NO_HIT)
			(void) store_lightpen_events(LIGHTPEN_UNDETECT, 0);
	}

	if (kmcb->status & HKKVS_VERTICAL_RETRACE) {
		if (softc->flags & F_VRT_RETRACE) {
			softc->flags &= ~F_VRT_RETRACE;
			wakeup(&softc->wchan[WCHAN_VRT]);
		}
	}
}


/*
 * Initialize the global data structures that
 * are associated with shared resource management
 */
static void
gt_shared_init(softc, keepmask)
	struct gt_softc *softc;
	u_int keepmask;
{
	struct proc_ll_type	*rptr;
	struct shar_res_type	*tsr_ptr;
	u_int i, j;
	u_char *printbuf_start, *printbuf_end, *cp;

	/* Initialize the WID tables */
	for (i=0; i<NWID; i++) {
		softc->wid_otable[i].tt_flags	 = 0;
		softc->wid_otable[i].tt_refcount = 0;
		softc->wid_itable[i].tt_flags	 = 0;
		softc->wid_itable[i].tt_refcount = 0;
	}

	/* Initialize the 8 Bit CLUT table */
	for (i=0; i<NCLUT_8; i++) {
		softc->clut_table[i].tt_flags	 = 0;
		softc->clut_table[i].tt_refcount = 0;
	}

	/* Initialize the FCS CLUT table */
	for (i=0; i<NFCS; i++) {
		softc->fcs_table[i].tt_flags	= 0;
		softc->fcs_table[i].tt_refcount = 0;
	}

	/* Free up any proc list elements left*/

	if (softc->proc_list != NULL) {
		while (softc->proc_list->plt_next != softc->proc_list) {
			rptr = softc->proc_list->plt_next;
			gt_link_detach(softc->proc_list->plt_next);

			/*
			 * Now deallocate if any shared resource struct
			 * is attached to this proc struct
			 */
			 while (rptr->plt_sr != NULL) {
				tsr_ptr = rptr->plt_sr;
				rptr->plt_sr = tsr_ptr->srt_next;
				kmem_free((caddr_t)tsr_ptr, sizeof (*tsr_ptr));
			}
			kmem_free((caddr_t)rptr, sizeof (*rptr));
		}

		/*
		 * Now deallocate if any shared resource struct
		 * is attached to this proc struct
		 */
		while (softc->proc_list->plt_sr != NULL) {
			tsr_ptr = softc->proc_list->plt_sr;
			softc->proc_list->plt_sr = tsr_ptr->srt_next;
			kmem_free((caddr_t)tsr_ptr, sizeof (*tsr_ptr));
		}

		/* Now deallocate the first element also */
		kmem_free((caddr_t)softc->proc_list,
		    sizeof (struct proc_ll_type));
		softc->proc_list = NULL;
	}

	/*
	 * Set up all the default values in the softc struct
	 */
	softc->flags	   &= keepmask;
	softc->server_proc  = (short) 0;
	softc->diag_proc    = (short) 0;

	for (i=0; i < MAX_DIAG_PROC; i++)
		softc->diag_proc_table[i] = 0;

	softc->wid_ocount  = GT_DEFAULT_OWID_COUNT;
	softc->wid_icount  = GT_DEFAULT_IWID_COUNT;

	softc->clut_part_total = GT_DEFAULT_SERVER_CLUT_PART;
	softc->clut_part_used  = 0;

	softc->light_pen_enable =  0;
	softc->light_pen_distance = 0;
	softc->light_pen_time =  0;

	/* Initialize all the shadow tables */
	for (i=0; i < NWID; i++) {
		softc->shadow_owlut[i] = 0;
		softc->shadow_iwlut[i] = 0;
	}

	for (i=0; i < 2; i++) {
		for (j=0; j < 3; j++)
			softc->shadow_cursor[i][j] = 0;
	}

	/*
	 * initialize the kmcb queue and the accelerator-thread list
	 */
	GT_LIST_INIT(&gt_kmcbhead, gt_kmcblist, q_list);

	if (softc->at_head == (struct at_proc *)NULL) {
		softc->at_head = (struct at_proc *)
		    new_kmem_zalloc(sizeof (struct at_proc), KMEM_SLEEP);

		GT_LIST_INIT(softc->at_head, at_proc, ap_list);

		softc->at_curr = softc->at_head;
		softc->at_head->ap_flags |= GTA_VIRGIN;
	}

	/*
	 * Initialize the kmcb printbuf pointer.  This is only done
	 * when keepmask is zero, indicating that we are starting
	 * up the system, rather than cleaning up after close.  This
	 * distinction is necessary because at close time the process
	 * in which this code is being run may have already lost its
	 * address space and the gt_setnocache() call would then panic
	 * the kernel.
	 */
	if (keepmask == (u_int)0) {
		softc->printbuf = gt_printbuffer;

		/*
		 * Disable caching for the GT print buffer
		 */
		printbuf_start = (u_char *)softc->printbuf;
		printbuf_end   = (u_char *)
		    (printbuf_start + gt_printbuffer_size);

		for (cp=printbuf_start; cp<printbuf_end; cp+=MMU_PAGESIZE)
			gt_setnocache((addr_t)cp);
	}

	/* Initialize the default attribute */
	gt_set_gattr_fbtype(softc);
	softc->owner = NULL;
}


/*
 * Check for server privilege.
 */
static
gt_server_check(softc)
	struct gt_softc *softc;
{
	if (u.u_procp->p_pid == softc->server_proc)
		return (1);
	else	return (0);
}


/*
 * Check for diagnostic privilege.
 */
static
gt_diag_check(softc)
	struct gt_softc *softc;
{
	int i;

	if (softc->diag_proc) {
		for (i = 0; i < MAX_DIAG_PROC; i++) {
			if (softc->diag_proc_table[i] == u.u_procp->p_pid)
				return (1);
		}
		return (0);
	}

	return (0);
}


/*
 * Copy the user data into a fb_clut structure and
 * call gt_clutpost() to do the real work.
 *
 * Provided for backwards compatibility.
 */
static
gt_fbioputcmap(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fbcmap *cmap = (struct fbcmap *) data;
	struct fb_clut mcmap;

	mcmap.index  = 0;		/* For backward compatibility */
	mcmap.offset = cmap->index;
	mcmap.count  = cmap->count;
	mcmap.red    = cmap->red;
	mcmap.green  = cmap->green;
	mcmap.blue   = cmap->blue;

	return (gt_clutpost(softc, (caddr_t) &mcmap));
}


/*
 * Copy the user data into a  fb_clut structure and
 * call gt_clutread() to do the job.
 *
 * Provided for backwards compatibility.
 */
static
gt_fbiogetcmap(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fbcmap *cmap = (struct fbcmap *) data;
	struct fb_clut mcmap;

	mcmap.index  = 0;		/* For backward compatibility */
	mcmap.offset = cmap->index;
	mcmap.count  = cmap->count;
	mcmap.red    = cmap->red;
	mcmap.green  = cmap->green;
	mcmap.blue   = cmap->blue;

	return (gt_clutread(softc, (caddr_t) &mcmap));
}


static
gt_fbiogtype(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fbtype *fb = (struct fbtype *) data;

	/* set owner if not owned or if prev, owner is dead */
	if (softc->owner == NULL || softc->owner->p_stat == NULL ||
	    (int) softc->owner->p_pid != softc->gattr->owner) {
		gt_set_gattr_fbtype(softc);
	}

	if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW){
		fb->fb_type = FBTYPE_SUN2BW;
		fb->fb_height = softc->h;
		fb->fb_width = 2048;
		fb->fb_depth = 1;
		fb->fb_size = (softc->h * 2048) / 8;
		fb->fb_cmsize = 2;
	} else 
		*fb = gt_monitor[softc->monitor].gtm_fbtype;
	return (0);
}


#if NWIN > 0
static
gt_fbiogpixrect(dev, softc, data)
	dev_t	dev;
	struct gt_softc *softc;
	caddr_t data;
{
	struct  fbpixrect  *fbpix = (struct fbpixrect *) data;
	struct  gt_fb	   *fbp;
	Pixrect		   *kerpr;
	static int	    i;
	static u_int	    pmask;
	static u_char	    red[2], green[2], blue[2];
	struct fb_clut	    fbclut;

	static int group[] = {
		PIXPG_24BIT_COLOR,
		PIXPG_WID,
		PIXPG_CURSOR,
		PIXPG_CURSOR_ENABLE,
		PIXPG_8BIT_COLOR,
		PIXPG_ZBUF
	};

	static int fdepth[] =  {
		GT_DEPTH_24,
		GT_DEPTH_WID,
		GT_DEPTH_1,
		GT_DEPTH_1,
		GT_DEPTH_8,
		GT_DEPTH_24
	};

	/*
	 * Populate the pixrect
	 */
	fbpix->fbpr_pixrect = &softc->pr;
	softc->pr.pr_size.x = softc->w;
	softc->pr.pr_size.y = softc->h;
	softc->pr.pr_ops = &ops_v;

	softc->pr.pr_data = (caddr_t) & softc->gtd;

	for (fbp = softc->gtd.fb, i = 0; i < GT_NFBS; i++, fbp++) {
		fbp->group = group[i];
		fbp->depth = fdepth[i];
		fbp->mprp.mpr.md_linebytes = mpr_linebytes (2048, fdepth[i]);
		fbp->mprp.mpr.md_offset.x = 0;
		fbp->mprp.mpr.md_offset.y = 0;
		fbp->mprp.mpr.md_primary = 0;

		if (fdepth[i] == GT_DEPTH_24)
			fbp->mprp.planes = 0x00ffffff;
		else
			fbp->mprp.planes = (1 << fbp->depth) - 1;

		fbp->mprp.mpr.md_flags = fbp->mprp.planes != 0 ?
		    MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	}

	softc->gtd.fd	      = minor(dev) >> 1;
	softc->gtd.flag       = 0;
	softc->gtd.gt_rp      = (H_rp*)softc->va[MAP_RP_HOST_CTRL];
	softc->gtd.gt_fbi     = (H_fbi *)softc->va[MAP_FBI_REG];
	softc->gtd.gt_fbi_go  = (H_fbi_go *)softc->va[MAP_FBI_PFGO];
	softc->gtd.gt_fe_h    = (H_fe_hold *)softc->va[MAP_FE_I860];

	/*
	 * NULL all pointers to fb except cursor planes .RIGHT??? ROP??
	 */
	kerpr = &softc->pr;

	/*
	 * XXX - Even though this is a *pixrect* call, it seems to be
	 * invoked by OpenWindows, hence this test.
	 *
	 * Set the WID partition if we aren't running the OW server.
	 */
	if (softc->server_proc == (short)0) {
		pmask = 5;		/* set the wid partition to 5-5 */

		if (gt_setwpart(softc, (caddr_t) &pmask))
			printf("gt_fbiogpixrect: Error in set wid partition\n");
	}

	gt_d(kerpr)->wid_part = 5;

	/*
	 * Set up the cursor colormap - only done once
	 */
	red[0] = 255; green[0] = 255; blue[0] = 255;
	red[1] =   0; green[1] =   0; blue[1] =   0;

	fbclut.flags |= FB_KERNEL;
	fbclut.count  = 2;
	fbclut.offset = 0;
	fbclut.red    = red;
	fbclut.green  = green;
	fbclut.blue   = blue;
	fbclut.index  =  GT_CLUT_INDEX_CURSOR;

	if (gt_clutpost(softc, (caddr_t) &fbclut))
		printf("gt_fbiogpixrect: Error in cursor clut post\n");

	/*
	 * Set up frame buffer pointers
	 */
	pmask = PIX_GROUP(PIXPG_24BIT_COLOR) | 0x00ffffff;
	(void) gt_putattributes(kerpr, &pmask);
	gt_d(kerpr)->fb[0].mprp.mpr.md_image = (short *)0;

	pmask = PIX_GROUP(PIXPG_WID);
	(void) gt_putattributes(kerpr, &pmask);
	gt_d(kerpr)->fb[1].mprp.mpr.md_image = (short *)0;

	pmask = PIX_GROUP(PIXPG_8BIT_COLOR);
	(void) gt_putattributes(kerpr, &pmask);
	gt_d(kerpr)->fb[4].mprp.mpr.md_image = (short *)0;

	pmask = PIX_GROUP(PIXPG_ZBUF) | 0x00ffffff;
	(void) gt_putattributes(kerpr, &pmask);
	gt_d(kerpr)->fb[5].mprp.mpr.md_image = (short *)0;

	pmask = PIX_GROUP(PIXPG_CURSOR_ENABLE);
	(void) gt_putattributes(kerpr, &pmask);
	gt_d(kerpr)->fb[3].mprp.mpr.md_image =
	    gt_d(kerpr)->mprp.mpr.md_image = (short *)softc->va[MAP_FB_CE];

	if (softc->gattr->sattr.emu_type != FBTYPE_SUN2BW){
		(void) gt_rop(kerpr, 0, 0, softc->pr.pr_size.x, 
			softc->pr.pr_size.y,
	    		PIX_CLR, (Pixrect *)0, 0, 0);
	}

	pmask = PIX_GROUP(PIXPG_CURSOR);
	(void) gt_putattributes(kerpr, &pmask);
	gt_d(kerpr)->fb[2].mprp.mpr.md_image =
	    gt_d(kerpr)->mprp.mpr.md_image = (short *)softc->va[MAP_FB_CD];

	if (softc->gattr->sattr.emu_type != FBTYPE_SUN2BW){
		(void) gt_rop(kerpr, 0, 0 ,softc->pr.pr_size.x, 
		softc->pr.pr_size.y,
	    	PIX_CLR, (Pixrect *)0, 0, 0);
	}
	return (0);
}
#endif NWIN > 0


static
gt_fbiogvideo(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	u_int *user_ptr;
	fbo_screen_ctrl *fbo_regs;
	int rc;

	user_ptr  = (u_int *) data;
	fbo_regs  = (fbo_screen_ctrl *) softc->va[MAP_FBO_SCREEN_CTRL];

#ifdef	MULTIPROCESSOR
	xc_attention();		/* Prevent users from busying the PF	*/
#endif	MULTIPROCESSOR

	if (!gt_check_pf_busy(softc)) {
		if (fbo_regs->fbo_videomode_0 & VIDEO_ENABLE)
			*user_ptr = FBVIDEO_ON;
		else
			*user_ptr = FBVIDEO_OFF;
		rc = 0;
	} else
		rc = EFAULT;

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR

	return (rc);
}


static
gt_fbiosvideo(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	fbo_screen_ctrl *fbo_regs = (fbo_screen_ctrl *)
	    softc->va[MAP_FBO_SCREEN_CTRL];
	int rc = 0;

#ifdef	MULTIPROCESSOR
	xc_attention();		/* Prevent users from busying the PF	*/
#endif	MULTIPROCESSOR

	if (!gt_check_pf_busy(softc)) {
		switch (*(u_int *) data) {

		case FBVIDEO_ON:
			fbo_regs->fbo_videomode_0 |= VIDEO_ENABLE;
			break;

		case FBVIDEO_OFF:
			fbo_regs->fbo_videomode_0 &= ~VIDEO_ENABLE;
			break;
		default:
			rc = EINVAL;
			break;
		}
	} else
		rc = EFAULT;

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR

	return (rc);
}


/*
 * If this is a sleep-until request, check to see that vertical
 * retrace interrupts are enabled, enabling them if necessary,
 * and go to sleep.  Otherwise, disable vertical retrace interrupts.
 */
static
gt_fbiovertical(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	u_int cmd;

	if (*(int *)data == 1) {

		if ((softc->flags&F_VRT_RETRACE) == 0) {
			gt_set_flags(&softc->flags, F_VRT_RETRACE);
			cmd = HKKVC_ENABLE_VRT_INTERRUPT;

			if (gt_post_command(softc, cmd, (Hkvc_kmcb *)0))
				return (EINVAL);
		}

		(void) sleep(&softc->wchan[WCHAN_VRT], PZERO+1);

		return (0);

	} else if (*(int *)data == 0) {
		gt_clear_flags(&softc->flags, F_VRT_RETRACE);
		cmd = HKKVC_DISABLE_VRT_INTERRUPT;

		if (gt_post_command(softc, cmd, (Hkvc_kmcb *)0))
			return (EINVAL);
		else
			return (0);
	} else
		return (EINVAL);
}


/*
 * allocate wids
 */
static
gt_widalloc(softc, data)
	struct gt_softc *softc;
	caddr_t data;

{
	struct fb_wid_alloc *wptr = (struct fb_wid_alloc *) data;
	u_int	type  = wptr->wa_type;
	u_int	count = wptr->wa_count;
	int	base, i, j;

	switch (type) {

	case FB_WID_SHARED_8:
		base = GT_DEFAULT_SUNVIEW_OWID_INDEX;
		softc->wid_otable[base].tt_flags |= TT_USED;
		softc->wid_otable[base].tt_refcount++;
		gt_alloc_sr(softc, u.u_procp->p_pid, RT_OWID, base, 1);
		break;

	case FB_WID_SHARED_24:
		base = GT_DEFAULT_SUNVIEW_IWID_INDEX;
		softc->wid_itable[base].tt_flags |= TT_USED;
		softc->wid_itable[base].tt_refcount++;
		gt_alloc_sr(softc, u.u_procp->p_pid, RT_IWID, base, 1);
		break;

	case FB_WID_DBL_8:
		base = gt_calc_wid(softc, FB_WID_DBL_8, count);
		if (base == -1)
			return (EINVAL);

		for (j=0,i=base; j<count; i++,j++) {
			softc->wid_otable[i].tt_flags |= TT_USED;
			softc->wid_otable[i].tt_refcount++;
		}

		gt_alloc_sr(softc, u.u_procp->p_pid, RT_OWID, base, count);
		break;

	case FB_WID_DBL_24:
		base = gt_calc_wid(softc, FB_WID_DBL_24, count);
		if (base == -1)
			return (EINVAL);

		for (j=0,i=base; j<count; i++,j++) {
			softc->wid_itable[i].tt_flags |= TT_USED;
			softc->wid_itable[i].tt_refcount++;
		}

		gt_alloc_sr(softc, u.u_procp->p_pid, RT_IWID, base, count);
		break;

	default:
		return (EINVAL);
	}

	wptr->wa_index = base;
	return (0);
}


static
gt_calc_wid(softc, type, count)
	struct	gt_softc *softc;
	u_int	type;
	u_int	count;
{
	int i, j, k, found, power_val;

	switch (type) {

	case FB_WID_DBL_8:
		if (count > softc->wid_ocount)
			return (-1);

		if ((power_val = gt_calc_power(count)) == -1) {
			return (-1);
		} else {
			for (k=1; k < softc->wid_ocount; k += power_val) {
				found = 1;

				for (j=0,i=k; j<count; j++,i++) {
					if (softc->wid_otable[i].tt_flags &
					    TT_USED) {
						found = 0;
						break;
					}
				}
				if (found) {
					return (k);
				}
			}
		}
		return (-1);

	case FB_WID_DBL_24:
		if (count > softc->wid_icount)
			return (-1);

		if ((power_val = gt_calc_power(count)) == -1) {
			return (-1);
		} else {
			for (k=0; k < softc->wid_icount; k += power_val) {
				found = 1;
				for (j=0,i=k; j<count; j++,i++) {
					if (softc->wid_itable[i].tt_flags &
					    TT_USED) {
						found = 0;
						break;
					}
				}
				if (found) {
					return (k);
				}
			}
		}
		return (-1);

	default:
		return (-1);
	}
}


static
gt_calc_power(count)
	u_int	count;
{
	int i;

	for (i=0; i<NWID_BITS; i++)
		if (gt_power_table[i] >= count)
			return (gt_power_table[i]);
	return (-1);
}


/*
 * Free specified wids
 */
static
gt_widfree(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_wid_alloc	*wlutp = (struct fb_wid_alloc *) data;
	int			index  = wlutp->wa_index;
	u_int			type   = wlutp->wa_type;
	u_int			count  = wlutp->wa_count;
	u_int			sr_found = 0;
	struct shar_res_type	*srptr, *prev;
	struct proc_ll_type	*found;
	struct proc_ll_type	*gt_find_proc_link();
	enum res_type		rtype;

	switch (type) {

	case FB_WID_SHARED_8:
	case FB_WID_DBL_8:
		if (index < 0 || index > (int)softc->wid_ocount)
			return (EINVAL);

		rtype = RT_OWID;
		break;

	case FB_WID_SHARED_24:
	case FB_WID_DBL_24:
		if (index < 0 || index > (int)softc->wid_icount)
			return (EINVAL);

		rtype = RT_IWID;
		break;

	default:
		return (EINVAL);
	}

	found = gt_find_proc_link(softc->proc_list, u.u_procp->p_pid);

	if (found == NULL)
		return (EINVAL);

	prev = srptr = found->plt_sr;

	while (srptr != NULL && !(sr_found)) {
		if (srptr->srt_type == rtype &&
		    srptr->srt_base == index)
			sr_found = 1;
		else {
			prev = srptr;
			srptr = srptr->srt_next;
		}
	}

	if (!sr_found)
		return (EINVAL);

	if (count == srptr->srt_count) {
		gt_decrement_refcount(softc, rtype, index, count);

		if (srptr == found->plt_sr)
			found->plt_sr = srptr->srt_next;
		else
			prev->srt_next = srptr->srt_next;
		kmem_free((caddr_t)srptr, sizeof (*srptr));
	} else {
		/* XXX: Handle this case later
		 * For now we should not come here
		 * We cannot delete the sr struct now because there
		 * is more in that struct
		 */
		printf("Inside gt_widfree wrong number of counts \n");
	}

	return (0);
}


/*
 * Read a wid
 */
/*ARGSUSED*/
static
gt_widread(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	/*
	 * GT Front-end Microcode does not provide a mechanism to read
	 * the WLUT's, therefore this ioctl is not implemented. So far
	 * all the application programs that need this ioctl have a
	 * workaround for this ioctl.
	 *
	 * XXX: When the Front-end Microcode provides the mechanism then
 	 * this ioctl has to be implemented.
	 */
	return (ENOTTY);
}


/*
 * Wake processes sleeping on one of the GT wchans
 */
gt_timeout(wchan)
	caddr_t wchan;
{
	wakeup(wchan);
}


/*
 * A kmcb post hasn't completed.  We interpret this as indicating
 * that the GT frontend has stopped responding.  We'll dequeue all
 * outstanding kmcb requests and reset the frontend.
 */
gt_kmcbtimeout(arg)
	caddr_t arg;
{
	struct gt_softc *softc = (struct gt_softc *)arg;
	int s;

	printf("the GT's accelerator port is unresponsive\n");
	printf("and is being shut down.\n");

	s = splr(gt_priority);

	softc->fe_ctrl_sp_ptr->fe_hcr  =  FE_RESET;
	softc->fe_ctrl_sp_ptr->fe_hcr &= ~FE_RESET;
	gt_sync = softc->fe_ctrl_sp_ptr->fe_hcr;

	gt_set_flags(&softc->flags, F_UCODE_STOPPED);
	gt_clear_flags(&softc->flags, F_ATU_FROZEN);

	wakeup(&softc->wchan[WCHAN_SCHED]);
	wakeup(&softc->wchan[WCHAN_THAW]);

	(void) splx(s);
}


/*
 * Posts wids
 */
static
gt_widpost(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_wid_list	*pwl = (struct fb_wid_list *) data;
	struct Gt_kernel_wlut_update *wlut_post = &(softc->wlut_post);
	struct Gt_wlut_update	*pwu;
	struct fb_wid_item	*pwi, *tmp;
	struct Hkvc_kmcb	 proto_kmcb;
	u_int			 nbytes;
	int			 i, count, ret_code;
	unsigned		 widtype;
	u_int			 cmd;
#ifdef sun4m
	u_int			 xc_attflag = 0;
#endif sun4m

#ifdef GT_DEBUG
	gtstat.gs_widpost++;
#endif GT_DEBUG

	/*
	 * Validate the request.  Kernel WLUT updates are not supported.
	 */
	if (pwl->wl_count==0 || !pwl->wl_list || pwl->wl_count>NWIDPOST ||
	    (pwl->wl_flags&FB_KERNEL))
		return (EINVAL);

	nbytes   = pwl->wl_count * sizeof (struct fb_wid_item);
	ret_code = 0;

	/*
	 * Block if there is a WLUT post in progress
	 */
	while (softc->flags & F_WLUT_PEND)
		(void) sleep(&softc->wchan[WCHAN_WLUT], PZERO+1);

	gt_set_flags(&softc->flags, F_WLUT_PEND);


	tmp = (struct fb_wid_item *) new_kmem_zalloc(nbytes, KMEM_SLEEP);

	if (copyin((caddr_t) pwl->wl_list, (caddr_t) tmp, nbytes)) {
		kmem_free((caddr_t)tmp, nbytes);
		ret_code = EFAULT;
		goto out;
	}

	/*
	 * Now construct a table
	 */
	widtype = tmp->wi_type;
	pwu	= softc->gt_wlut_update;
	pwi	= tmp;

	for (count = 0; count < pwl->wl_count; count++) {

		if (widtype != pwi->wi_type) {
			kmem_free((caddr_t)tmp, nbytes);
			ret_code = EINVAL;
			goto out;
		}

		pwu->entry	    = (int)		pwi->wi_index;
		pwu->mask	    = (int)		pwi->wi_attrs;

		pwu->image_buffer   = (Gt_buffer)	pwi->wi_values[0];
		pwu->overlay_buffer = (Gt_buffer)	pwi->wi_values[1];
		pwu->plane_group    = (Gt_plane_group)	pwi->wi_values[2];
		pwu->clut	    = (int)		pwi->wi_values[3];
		pwu->fast_clear_set = (int)		pwi->wi_values[4];
	}

	/* Done with the temporary buffer */
	kmem_free((caddr_t)tmp, nbytes);


	if (widtype==FB_WID_SHARED_8 || widtype==FB_WID_DBL_8)
		wlut_post->table = 1;
	else
		wlut_post->table = 0;

	wlut_post->num_entries = pwl->wl_count;
	wlut_post->upd = (Gt_wlut_update *)GT_KVTOP((int)softc->gt_wlut_update);

	/*
	 * Use the RP to post WLUT entries if we can.  Otherwise
	 * we'll have to post the entries directly into the FB,
	 * taking a performance hit along the way.
	 */
	if (softc->flags & F_UCODE_RUNNING) {

		cmd = HKKVC_UPDATE_WLUT | HKKVC_PAUSE_WITH_INTERRUPT;

		proto_kmcb.wlut_info =
		    (Gt_kernel_wlut_update *)GT_KVTOP((int) wlut_post);

		if (gt_post_command(softc, cmd, &proto_kmcb)) {
			ret_code = EINVAL;
			goto out;
		}

		/*
		 * Does the caller want to block until the post completes?
		 * We can synchronize by flushing the RP, via gta_idle(),
		 * and when we get the interrupt indicating completion of
		 * the RP flush, we can poll for the COPY_REQUEST bit to
		 * be reset.
		 */
		if (pwl->wl_flags & FB_BLOCK) {

			fbo_wlut_ctrl *fbo_csr = (fbo_wlut_ctrl *)
			    softc->va[MAP_FBO_WLUT_CTRL];


			/*
			 * Idle the accelerator
			 */
			gta_idle(softc, F_WLUT_HOLD);

#ifdef	MULTIPROCESSOR
			xc_attention();		/* Keeps users from the PF */
			xc_attflag = 1;
#endif	MULTIPROCESSOR

			/*
			 * Check for PF not busy
			 */
			if (gt_check_pf_busy(softc)) {
				ret_code = EFAULT;
				goto out;
			}

			/* wait for the previous wlut post to finish */

			CDELAY(!((fbo_csr->wlut_csr0 &
			    FBO_ENABLE_COPY_REQUEST) ||
			    (fbo_csr->wlut_csr1 & FBO_ENABLE_COPY_REQUEST)),
			    GT_SPIN_LUT);

			if ((fbo_csr->wlut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
			    (fbo_csr->wlut_csr1 & FBO_ENABLE_COPY_REQUEST)) {
				printf("WLUT_CSR is BUSY reset hardware\n");
				ret_code = EFAULT;
				goto out;
			}

			gt_clear_flags(&softc->flags, F_WLUT_HOLD);
			wakeup(&softc->wchan[WCHAN_SCHED]);
		}

	} else {
		/*
		 * Can't use the RP for the post.  Do it directly.
		 */
		struct  Gt_wlut_update *wu;
		fbo_wlut_ctrl *fbo_csr = (fbo_wlut_ctrl *)
		    softc->va[MAP_FBO_WLUT_CTRL];

#ifdef	MULTIPROCESSOR
		/* Prevent user processes from busying the PF. */
		xc_attention();		/* Keeps users from the PF */
		xc_attflag = 1;
#endif	MULTIPROCESSOR

		/*
		 * Check for PF not busy
		 */
		if (gt_check_pf_busy(softc)) {
			ret_code = EFAULT;
			goto out;
		}

		/* wait for the previous wlut post to finish */

		CDELAY(!((fbo_csr->wlut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
		    (fbo_csr->wlut_csr1 & FBO_ENABLE_COPY_REQUEST)),
		    GT_SPIN_LUT);

		if ((fbo_csr->wlut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
		    (fbo_csr->wlut_csr1 & FBO_ENABLE_COPY_REQUEST)) {
			printf("WLUT_CSR is BUSY reset hardware\n");
			ret_code = EINVAL;
			goto out;
		}

		/* First copy the entries to shadow */
		wu = softc->gt_wlut_update;
		for (i=0; i<pwl->wl_count; i++, wu++) {
			ret_code =
			    gt_direct_wlut_update(softc, wu, wlut_post->table);
			if (ret_code) {
				goto out;
			}
		}

		/* Now post it to hardware */
		fbo_csr->wlut_csr0 |= FBO_ENABLE_COPY_REQUEST |
		    FBO_ENABLE_ALL_INTERLEAVE;
		gt_sync = fbo_csr->wlut_csr0;

		if (pwl->wl_flags & FB_BLOCK) {
			/* process wants to sleep until this happens */

			/* wait for the previous wlut post to finish */
			DELAY(100);
			/*
			 * Check for PF not busy
			 */
			if (gt_check_pf_busy(softc)) {
				ret_code = EFAULT;
				goto out;
			}

			CDELAY(!((fbo_csr->wlut_csr0 &
			    FBO_ENABLE_COPY_REQUEST) ||
			    (fbo_csr->wlut_csr1 & FBO_ENABLE_COPY_REQUEST)),
			    GT_SPIN_LUT);

			if ((fbo_csr->wlut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
		   	 (fbo_csr->wlut_csr1 & FBO_ENABLE_COPY_REQUEST)) {
				printf("WLUT_CSR is BUSY reset hardware\n");
				ret_code = EINVAL;
			}
		}
	}

out:
#ifdef MULTIPROCESSOR
	if (xc_attflag) {
		xc_dismissed();
	}
#endif MULTIPROCESSOR
	/*
	 * Wakeup processes waiting for posting WLUTs
	 */
	gta_run(softc, F_WLUT_HOLD);

	gt_clear_flags(&softc->flags, F_WLUT_PEND);
	wakeup(&softc->wchan[WCHAN_WLUT]);


	return (ret_code);
}


/*
 * Post a WLUT entry directly to the FB.
 * This is only used if the GT FE has died.
 */
static
gt_direct_wlut_update(softc, wu, flag)
	struct gt_softc *softc;
	Gt_wlut_update  *wu;
	int	flag;
{
	u_int table_entry, *wlut_addr, i;
	u_char	*temp_wlut_addr;

	if (!(wu->mask & (HK_WLUT_MASK_IMAGE | HK_WLUT_MASK_OVERLAY |
	    HK_WLUT_MASK_PLANE_GROUP | HK_WLUT_MASK_CLUT |
	    HK_WLUT_MASK_FCS))) {

		gt_clear_flags(&softc->flags, F_WLUT_PEND);
		wakeup(&softc->wchan[WCHAN_WLUT]);

		return (EINVAL);
	}

	if (flag)
		table_entry = softc->shadow_owlut[wu->entry];
	else
		table_entry = softc->shadow_iwlut[wu->entry];

	if (wu->mask & HK_WLUT_MASK_IMAGE) {
		table_entry &= ~HKFBO_WLUT_IMAGE_BUFFER;
		if (wu->image_buffer !=0)
			table_entry |= HKFBO_WLUT_IMAGE_BUFFER;
	}

	if (wu->mask & HK_WLUT_MASK_OVERLAY) {
		table_entry &= ~HKFBO_WLUT_OV_BUFFER;
		if (wu->overlay_buffer != 0)
			table_entry |= HKFBO_WLUT_OV_BUFFER;
	}

	if (wu->mask & HK_WLUT_MASK_PLANE_GROUP) {
		table_entry &= ~HKFBO_WLUT_COLOR_MODE;

		switch (wu->plane_group) {

		case HK_8BIT_RED:
			table_entry |= HKFBO_WLUT_CM_8RED;
			break;

		case HK_8BIT_GREEN:
			table_entry |= HKFBO_WLUT_CM_8GREEN;
			break;

		case HK_8BIT_BLUE:
			table_entry |= HKFBO_WLUT_CM_8BLUE;
			break;

		case HK_8BIT_OVERLAY:
			table_entry |= HKFBO_WLUT_CM_8OVERLAY;
			break;

		case HK_12BIT_LOWER:
			table_entry |= HKFBO_WLUT_CM_12LOW;
			break;

		case HK_12BIT_UPPER:
			table_entry |= HKFBO_WLUT_CM_12HIGH;
			break;

		case HK_24BIT:
			table_entry |= HKFBO_WLUT_CM_24BIT;
			break;

		default:
			break;
		}
	}

	if (wu->mask & HK_WLUT_MASK_CLUT) {
		table_entry &= ~HKFBO_WLUT_CLUT_SELECT;
		table_entry |= (wu->clut << HKFBO_WLUT_CLUT_SEL_SHIFT)
		    & HKFBO_WLUT_CLUT_SELECT;
    	}

	if (wu->mask & HK_WLUT_MASK_FCS) {
		table_entry &= ~HKFBO_WLUT_FCS_SELECT;
		table_entry |= (wu->fast_clear_set <<
		    HKFBO_WLUT_FCS_SEL_SHIFT) & HKFBO_WLUT_FCS_SELECT;
	}

	if (flag) {
		/* overlay */
		/* Note: No HDTV alternate WLUT yet XXX */
		softc->shadow_owlut[wu->entry] = table_entry;
		temp_wlut_addr = (u_char *) softc->va[MAP_FBO_WLUT_MAP] +
		    (wu->entry * HKFBO_WLUT_ENTRY_OFFSET * softc->wid_icount);
		wlut_addr = (u_int *) temp_wlut_addr;

		for (i=0; i<softc->wid_icount; i++, wlut_addr++)
			*wlut_addr = table_entry;
	} else {
		/* image */
		softc->shadow_iwlut[wu->entry] = table_entry;
		temp_wlut_addr = (u_char *) softc->va[MAP_FBO_WLUT_MAP] +
		    (wu->entry * HKFBO_WLUT_ENTRY_OFFSET);
		wlut_addr  = (u_int *)temp_wlut_addr;
		*wlut_addr = table_entry;
	}

	gt_sync = (u_int) *wlut_addr;

	return (0);
}


/*
 * Allocate CLUTs
 */
static
gt_clutalloc(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_clutalloc *clutp = (struct fb_clutalloc *) data;
	int	flags     = clutp->flags;
	int	clutindex = clutp->clutindex;
	int	index	  = -1;
	int	i;

	switch (flags) {

	case CLUT_8BIT:
		if (gt_server_check(softc)) {
			if (softc->clut_part_used == softc->clut_part_total)
				return (ENOMEM);

			softc->clut_part_used++;
			i = GT_DEFAULT_SUNVIEW_CLUT8;
		} else
			i = GT_DEFAULT_SUNVIEW_CLUT8 + 1;

		for (; i< NCLUT_8; i++) {
			if (softc->clut_table[i].tt_flags & TT_USED)
				continue;
			else {
				index = i;
				break;
			}
		}

		if (index == -1)
			return (ENOMEM);

		/*
		 * Found a CLUT that we can use.  Mark it used
		 * and return its index to the user.
		 */
		softc->clut_table[index].tt_flags |= TT_USED;
		softc->clut_table[index].tt_refcount++;
		gt_alloc_sr(softc, u.u_procp->p_pid, RT_CLUT8, index, 1);

		clutp->index = index;
		break;

	case CLUT_SHARED:
		if (clutindex < 0 || clutindex > NCLUT_8 -1)
			return (EINVAL);

		if (softc->clut_table[clutindex].tt_flags & TT_USED) {
			softc->clut_table[clutindex].tt_refcount++;

			gt_alloc_sr(softc, u.u_procp->p_pid, RT_CLUT8,
			    clutindex, 1);
		} else {
			softc->clut_table[clutindex].tt_flags |= TT_USED;
			softc->clut_table[clutindex].tt_refcount++;

			gt_alloc_sr(softc, u.u_procp->p_pid, RT_CLUT8,
			    clutindex, 1);
		}

		/*
		 * return the index to the user
		 */
		clutp->index = clutindex;
		break;

	default:
		return (EINVAL);
	}

	return (0);
}


/*
 * Free CLUTs
 */
static
gt_clutfree(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_clutalloc	*clutp   = (struct fb_clutalloc *) data;
	int			index    = (int) clutp->index;
	u_int			sr_found = 0;
	struct shar_res_type	*srptr, *prev;
	struct proc_ll_type	*found;
	struct proc_ll_type	*gt_find_proc_link();

	found = gt_find_proc_link(softc->proc_list, u.u_procp->p_pid);

	if (found == NULL)
		return (EINVAL);

	prev = srptr = found->plt_sr;

	while (srptr != NULL && !(sr_found)) {
		if (srptr->srt_type == RT_CLUT8 && srptr->srt_base == index) {
			sr_found = 1;
		} else {
			prev = srptr;
			srptr = srptr->srt_next;
		}
	}

	if (!sr_found)
		return (EINVAL);

	gt_decrement_refcount(softc, RT_CLUT8, index, 1);

	if (gt_server_check(softc))
		softc->clut_part_used--;

	if (srptr == found->plt_sr)
		found->plt_sr = srptr->srt_next;
	else
		prev->srt_next = srptr->srt_next;

	kmem_free((caddr_t)srptr, sizeof (*srptr));

	return (0);
}


/*
 * Read CLUTs
 */
static
gt_clutread(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	fbo_clut_ctrl  *fbo_csr = (fbo_clut_ctrl *)softc->va[MAP_FBO_CLUT_CTRL];
	fbo_clut_map   *fbo_map = (fbo_clut_map *)softc->va[MAP_FBO_CLUT_MAP];
	struct fb_clut *clutp   = (struct fb_clut *) data;
	u_int	count	 = (u_int)clutp->count;
	u_int	offset	 = (u_int)clutp->offset;
	u_int	flags	 = (u_int)clutp->flags;
	int	clut_id  = (u_int)clutp->index;
	u_int	regamma  = 0;
	u_int	ret_code = 0;
	u_char	red, green, blue;
	u_int	i, limit, color, *colorp;
	u_char	tmp[3][GT_CMAP_SIZE];


	if (!clutp->red || !clutp->green || !clutp->blue)
		return (EINVAL);

	if (clut_id < 0 || clut_id > NCLUT)
		return (EINVAL);

	limit = count + offset;
	if (clut_id <= GT_CLUT_INDEX_24BIT && (limit==0) || (limit>CLUT24_SIZE))
		return (EINVAL);


	if (softc->flags & F_UCODE_RUNNING) {
		/*
		 * Idle the accelerator.
		 * Flush the RP then read the shadow CLUT directly
		 */
		gta_idle(softc, F_CLUT_HOLD);
	}

#ifdef	MULTIPROCESSOR
	xc_attention();		/* Prevent users from busying the PF	*/
#endif	MULTIPROCESSOR

	/*
	 * Check for PF not busy
	 */
	if (gt_check_pf_busy(softc)) {
		ret_code = EFAULT;
		goto out;
	}

	/* wait for the previous clut post to finish */

	CDELAY(!((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
	    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)), GT_SPIN_LUT);

	if ((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
	    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)) {
		printf("CLUT_CSR is BUSY reset hardware\n");
		ret_code = EFAULT;
		goto out;
	}


	/*
	 * Copy the current info from the shadow CLUTs.  8-bit indexed
	 * colormaps are conditionally regammified, depending upon the
	 * requestor's preference or the global state.
	 */
	if (clut_id <= GT_CLUT_INDEX_24BIT) {

		if (clut_id == GT_CLUT_INDEX_24BIT)
			colorp = &fbo_map->fbo_clut24.clut24_map[offset];
		else {
			colorp = &fbo_map->fbo_clut8[clut_id].clut8_map[offset];

			if (softc->flags&F_DEGAMMALOADED) {

				if (flags & FB_DEGAMMA)
					regamma = 1;

				else if (flags & FB_NO_DEGAMMA)
					regamma = 0;

				else
					regamma = (softc->flags&F_VASCO) ? 1:0;
			} else
				regamma = 0;
		}

		for (i=0; i<count; i++) {
			color = *colorp++;

			red   = (color & CLUT_REDMASK);
			green = (color & CLUT_GREENMASK) >>  8;
			blue  = (color & CLUT_BLUEMASK)  >> 16;

			if (regamma) {
				red   = softc->gamma[red];
				green = softc->gamma[green];
				blue  = softc->gamma[blue];
			}

			tmp[0][i] = red;
			tmp[1][i] = green;
			tmp[2][i] = blue;
		}
	} else {
		/*
		 * XXX: If any user wants read back Cursor or Gamma
		 * lookup tables, that code has to go here
		 */
		ret_code = EINVAL;
		goto out;
	}

	/* Now copy it to the user space */
	if (copyout((caddr_t)tmp[0], (caddr_t)clutp->red,   count) ||
	    copyout((caddr_t)tmp[1], (caddr_t)clutp->green, count) ||
	    copyout((caddr_t)tmp[2], (caddr_t)clutp->blue,  count)) {
		ret_code = EFAULT;
		goto out;
	}
out:

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR

	gta_run(softc, F_CLUT_HOLD);
	return (ret_code);
}


/*
 * Post CLUTs
 */
static
gt_clutpost(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_clut *clutp = (struct fb_clut *) data;
	u_int	count	= (u_int)clutp->count;
	u_int	offset	= (u_int)clutp->offset;
	u_int	flags	= (u_int)clutp->flags;
	int	clut_id	= (u_int)clutp->index;
	struct Gt_clut_update *clut_post;
	Hkvc_kmcb proto_kmcb;
	unsigned *cmap, *color;
	u_int	  cmd, limit, i, j;
	int	  degamma    = 0;
	int	  ret_code   = 0;
#ifdef sun4m
	int	  xc_attflag = 0;
#endif sun4m
	u_char	tmp[3][GT_CMAP_SIZE];

#ifdef GT_DEBUG
	gtstat.gs_clutpost++;
#endif GT_DEBUG

	/*
	 * if kernel is posting a CLUT then we don't have to check for lock.
	 */
	switch (clut_id) {
		case GT_CLUT_INDEX_FCS:		/* disallow FCS   CLUT posts */
		case GT_CLUT_INDEX_12BIT:	/* disallow 12bit CLUT posts */
			return (EINVAL);

		case GT_CLUT_INDEX_GAMMA:
			return (gt_gamma_clutpost(softc, data));

		case GT_CLUT_INDEX_CURSOR:
			return (gt_cursor_clutpost(softc, data));

		case GT_CLUT_INDEX_DEGAMMA:
			return (gt_load_degamma(softc, data));

		default:			/* 8-bit or truecolor CLUT    */
			break;
	}

	if (!clutp->red || !clutp->green || !clutp->blue || clut_id < 0 ||
	    clut_id > NCLUT)
		return (EINVAL);

	/*
	 * By the time we reach here, the clut post is to one of the
	 * 256-entry CLUTs.  Time for another sanity check.
	 */
	limit = offset + count;
	if (limit == 0 || limit > CLUT8_SIZE)
		return (EINVAL);


	/*
	 * Block until there are no in-progress CLUT posts
	 */
	if (!(flags & FB_KERNEL)) {
		while (softc->flags & F_CLUT_PEND)
			(void) sleep(&softc->wchan[WCHAN_CLUT], PZERO+1);

		gt_set_flags(&softc->flags, F_CLUT_PEND);
	}

	/*
	 * Copy the cmap from userspace or kernelspace to local buffer,
	 * and then to the appropriate softc color map table.
	 */
	if (flags & FB_KERNEL) {
		cmap	  =  softc->gt_sys_colors;
		clut_post = &softc->kernel_clut_post;

		bcopy((caddr_t) clutp->red,   (caddr_t) tmp[0], count);
		bcopy((caddr_t) clutp->green, (caddr_t) tmp[1], count);
		bcopy((caddr_t) clutp->blue,  (caddr_t) tmp[2], count);

	} else {
		cmap	  =  softc->gt_user_colors;
		clut_post = &softc->user_clut_post;

		if (copyin((caddr_t) clutp->red,   (caddr_t) tmp[0], count) ||
		    copyin((caddr_t) clutp->green, (caddr_t) tmp[1], count) ||
		    copyin((caddr_t) clutp->blue,  (caddr_t) tmp[2], count)) {
			ret_code = EFAULT;
			goto out;
		}
	}


	/*
	 * Is this an 8-bit indexed color map requesting degammifcation?
	 */
	if ((clut_id<GT_CLUT_INDEX_24BIT) && (softc->flags&F_DEGAMMALOADED)) {

		if (flags & FB_DEGAMMA)
			degamma = 1;

		else if (flags & FB_NO_DEGAMMA)
			degamma = 0;
		else
			degamma = (softc->flags&F_VASCO) ? 1:0;
	} else
		degamma = 0;


	color = cmap;
	for (i=0; i<count; i++) {
		if (degamma == 0) {
			*color++ = tmp[2][i] << 16 |
				   tmp[1][i] <<  8 |
				   tmp[0][i];
		} else {
			*color++ = softc->degamma[tmp[2][i]] << 16 |
				   softc->degamma[tmp[1][i]] <<  8 |
				   softc->degamma[tmp[0][i]];
		}
	}


	if (softc->flags & F_UCODE_RUNNING) {

		clut_post->start_entry	= offset;
		clut_post->num_entries	= count;
		clut_post->colors	= (u_int *) GT_KVTOP((int) cmap);
		clut_post->table	= clut_id;

		if (flags & FB_KERNEL)
			cmd = HKKVC_UPDATE_CLUT;
		else
			cmd = HKKVC_UPDATE_CLUT | HKKVC_PAUSE_WITH_INTERRUPT;

		proto_kmcb.clut_info =
		    (Gt_clut_update *)GT_KVTOP((int) clut_post);

		if (gt_post_command(softc, cmd, &proto_kmcb)) {
			ret_code = EINVAL;
			goto out;
		}

		if ((!(flags & FB_KERNEL)) && (flags & FB_BLOCK)) {

			fbo_clut_ctrl *fbo_csr =
			    (fbo_clut_ctrl *)softc->va[MAP_FBO_CLUT_CTRL];

			/*
			 * The process wants to block until the update
			 * completes.
			 */
			gta_idle(softc, F_CLUT_HOLD);

#ifdef	MULTIPROCESSOR
			xc_attention();		/* Keeps users from the PF */
			xc_attflag = 1;
#endif	MULTIPROCESSOR
			/*
			 * Check for PF not busy
			 */
			if (gt_check_pf_busy(softc)) {
				ret_code = EFAULT;
				goto out;
			}

			/* wait for the previous clut post to finish */
			CDELAY(!((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST)
				 || (fbo_csr->clut_csr1 &
				FBO_ENABLE_COPY_REQUEST)), GT_SPIN_LUT);

			if ((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
			    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)) {

				printf("CLUT_CSR is BUSY reset hardware\n");
				ret_code = EFAULT;
				goto out;
			}

			gt_clear_flags(&softc->flags, F_CLUT_HOLD);
			wakeup(&softc->wchan[WCHAN_SCHED]);

		}
	} else {
		/*
		 * FE microcode is not running so
		 * kernel has to post this CLUT
		 */
		fbo_clut_ctrl *fbo_csr =
		    (fbo_clut_ctrl *)softc->va[MAP_FBO_CLUT_CTRL];

		fbo_clut_map *fbo_map =
		    (fbo_clut_map *)softc->va[MAP_FBO_CLUT_MAP];

#ifdef	MULTIPROCESSOR
		xc_attention();		/* Prevent users from busying the PF */
		xc_attflag = 1;
#endif	MULTIPROCESSOR

		/*
		 * Check for PF not busy
		 */
		if (gt_check_pf_busy(softc)) {
			ret_code = EFAULT;
			goto out;
		}

		/* wait for the previous clut post to finish */

		CDELAY(!((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
		    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)),
		    GT_SPIN_LUT);

		if ((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
		    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)) {
			printf("CLUT_CSR is BUSY reset hardware\n");
			ret_code = EINVAL;
			goto out;
		}

		/* copy all the info to the shadow table */
		if (clut_id < 14) {

			/* overlay clut */
			for (i=0,j=offset; i< count; i++,j++)
				if (flags & FB_KERNEL)
				    fbo_map->fbo_clut8[clut_id].clut8_map[j] =
					softc->gt_sys_colors[i];
				else
				    fbo_map->fbo_clut8[clut_id].clut8_map[j] =
					softc->gt_user_colors[i];
		} else {
			for (i=0,j=offset; i < count; i++,j++)
				if (flags & FB_KERNEL)
					fbo_map->fbo_clut24.clut24_map[j] =
					   softc->gt_sys_colors[i];
				else
					fbo_map->fbo_clut24.clut24_map[j] =
					    softc->gt_user_colors[i];
		}
		fbo_csr->clut_csr0 |=
		    (FBO_ENABLE_COPY_REQUEST | FBO_ENABLE_ALL_INTERLEAVE);
		gt_sync = fbo_csr->clut_csr0;


		if (flags & FB_BLOCK) {

			/* wait for the previous clut post to finish */
			DELAY(100);

			/*
			 * Check for PF not busy
			 */
			if (gt_check_pf_busy(softc)) {
				ret_code = EFAULT;
				goto out;
			}

			CDELAY(!((fbo_csr->clut_csr0 &
			    FBO_ENABLE_COPY_REQUEST) ||
			    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)),
			    GT_SPIN_LUT);

			if ((fbo_csr->clut_csr0 & FBO_ENABLE_COPY_REQUEST) ||
			    (fbo_csr->clut_csr1 & FBO_ENABLE_COPY_REQUEST)) {
				printf("CLUT_CSR is BUSY reset hardware\n");
				ret_code = EINVAL;
				goto out;
			}

		}
	}

out:
#ifdef MULTIPROCESSOR
	if (xc_attflag) {
		xc_dismissed();
	}
#endif MULTIPROCESSOR

	/*
	 * Wake processes that are waiting for posting CLUTs
	 */
	if (!(flags & FB_KERNEL)) {
		gta_run(softc, F_CLUT_HOLD);

		gt_clear_flags(&softc->flags, F_CLUT_PEND);
		wakeup(&softc->wchan[WCHAN_CLUT]);
	}

	return (ret_code);
}


/*
 * Post the gamma palette
 */
static
gt_gamma_clutpost(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_clut *clutp = (struct fb_clut *) data;
	u_int	count	= (u_int)clutp->count;
	u_int	offset	= (u_int)clutp->offset;
	u_int	flags	= (u_int)clutp->flags;
	u_char	tmp[3][GT_CMAP_SIZE];
	u_int	i, j;
	fbo_ramdac_map	*ramdac =
	    (fbo_ramdac_map *)softc->va[MAP_FBO_RAMDAC_CTRL];


	if (count < 1 || count > 256)
		return (EINVAL);

	if (flags & FB_KERNEL) {
		bcopy((caddr_t) clutp->red,   (caddr_t) tmp[0], count);
		bcopy((caddr_t) clutp->green, (caddr_t) tmp[1], count);
		bcopy((caddr_t) clutp->blue,  (caddr_t) tmp[2], count);
	} else {
		if (copyin((caddr_t) clutp->red,   (caddr_t) tmp[0], count) ||
		    copyin((caddr_t) clutp->green, (caddr_t) tmp[1], count) ||
		    copyin((caddr_t) clutp->blue,  (caddr_t) tmp[2], count))
			 return (EFAULT);
	}

#ifdef	MULTIPROCESSOR
	xc_attention();		/* Prevent users from busying the PF	*/
#endif	MULTIPROCESSOR

	if (gt_check_pf_busy(softc)) {
#ifdef	MULTIPROCESSOR
		xc_dismissed();
#endif	MULTIPROCESSOR
		return (EINVAL);
	}

	/* Change  addr ptr to the starting  offset */
	ramdac->ramdac_addr_reg = offset;
	for (i=0; i < count; i++) {
		for (j=0; j < 3; j++)
			ramdac->ramdac_gamma_data_reg = tmp[j][i];
	}

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR

	/*
	 * Keep a (sigh) shadow of the gamma table so that we can
	 * regamma degamma'd tables.  Here we just collapse the 
	 * three channels into one, assuming them all to be identical.
	 */
	if ((count == GT_CMAP_SIZE) &&
	    gt_server_check(softc) || gt_diag_check(softc)) {
		bcopy((caddr_t)tmp[0], (caddr_t)softc->gamma,
		    (u_int)GT_CMAP_SIZE);
		gt_set_flags(&softc->flags, F_GAMMALOADED);
	}

	return (0);
}


/*
 * Load a de-gamma table from the red colormap specified.  We just
 * as easily could have taken the green or the blue;  they should 
 * all be identical.
 */
static
gt_load_degamma(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_clut *vasco = (struct fb_clut *) data;

	if (!gt_server_check(softc) && !gt_diag_check(softc))
		return (EACCES);

	if (copyin((caddr_t)vasco->red, (caddr_t)softc->degamma, GT_CMAP_SIZE))
		return (EFAULT);
	else {
		gt_set_flags(&softc->flags, F_DEGAMMALOADED);
		return (0);
	}
}


/*
 * Post the cursor palette
 */
static
gt_cursor_clutpost(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_clut *clutp=(struct fb_clut *) data;
	u_int	count	= (u_int)clutp->count;
	u_int	offset	= (u_int)clutp->offset;
	u_int	flags	= (u_int)clutp->flags;
	u_char	tmp[3][GT_CMAP_SIZE];
	u_int	i, j;
	fbo_ramdac_map	*ramdac =
	    (fbo_ramdac_map *) softc->va[MAP_FBO_RAMDAC_CTRL];


	if (offset != 0 && offset != 1)
		return (EINVAL);

	if (count < 1 || count > 2)
		return (EINVAL);

	if (flags & FB_KERNEL) {
		bcopy((caddr_t) clutp->red,   (caddr_t) tmp[0], count);
		bcopy((caddr_t) clutp->green, (caddr_t) tmp[1], count);
		bcopy((caddr_t) clutp->blue,  (caddr_t) tmp[2], count);
	} else {
		if (copyin((caddr_t) clutp->red,   (caddr_t) tmp[0], count) ||
		    copyin((caddr_t) clutp->green, (caddr_t) tmp[1], count) ||
		    copyin((caddr_t) clutp->blue,  (caddr_t) tmp[2], count))
			 return (EFAULT);
	}

	/* copy into shadow */

	if (count == 2) {
		/* Posting both bg/fg */
		for (i=0; i < 2; i++) {
			for (j=0; j < 3; j++)
				softc->shadow_cursor[j][i] = tmp[j][i];
			}
	} else {
		if (offset) {
			/* Only fg color */
			for (j=0; j < 3; j++)
				softc->shadow_cursor[j][1] = tmp[j][0];
		} else {
			/* Only bg color */
			for (j=0; j < 3; j++)
				softc->shadow_cursor[j][0] = tmp[j][0];
		}
	}

#ifdef	MULTIPROCESSOR
	xc_attention();		/* Prevent users from busying the PF	*/
#endif	MULTIPROCESSOR

	if (gt_check_pf_busy(softc))
		return (EINVAL);

	/*
	 * zero the addr ptr offset. always write all of the
	 * cursor palette registers
	 */
	ramdac->ramdac_addr_reg = 0;

	/* zero out the transparent regs */
	for (i=0; i < 6; i++)
		ramdac->ramdac_cursor_data_reg = 0;

	/* Load the cursor bg/fg color */
	for (i=0; i < 2; i++) {
		for (j=0; j < 3; j++) {
			ramdac->ramdac_cursor_data_reg =
			    softc->shadow_cursor[j][i];
		}
	}

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR

	return (0);
}


/*
 * Post commands to the GT
 */
static
gt_post_command(softc, cmd, proto)
	struct gt_softc *softc;
	u_int		 cmd;
	Hkvc_kmcb	*proto;
{
	struct gt_kmcblist *qptr;
	struct Hkvc_kmcb   *kptr;

	/*
	 * Allocate a kmcb request element
	 */
	qptr = (struct gt_kmcblist *)new_kmem_zalloc(sizeof (*qptr),
	    KMEM_SLEEP);

	kptr = &qptr->q_mcb;

	/*
	 * Transfer data from the prototype kmcb to the request element.
	 */
	kptr->command = cmd;

	if (cmd & (HKKVC_SET_IMAGE_WLUT_BITS	|
	    HKKVC_UPDATE_WLUT			|
	    HKKVC_UPDATE_CLUT			|
	    HKKVC_LOAD_USER_MCB			|
	    HKKVC_SET_FB_SIZE			|
	    HKKVC_SET_LIGHT_PEN_PARAMETERS	|
	    HKKVC_LOAD_CONTEXT)) {

		if (proto == (Hkvc_kmcb *) NULL) {
			printf("gt post kmcb:  missing data\n");
			kmem_free((caddr_t)qptr, sizeof (*qptr));
			return (1);
		}

		kptr->wlut_image_bits	 = proto->wlut_image_bits;
		kptr->wlut_info		 = proto->wlut_info;
		kptr->clut_info		 = proto->clut_info;
		kptr->fb_width		 = proto->fb_width;
		kptr->screen_width	 = proto->screen_width;
		kptr->screen_height	 = proto->screen_height;
		kptr->light_pen_enable	 = proto->light_pen_enable;
		kptr->light_pen_distance = proto->light_pen_distance;
		kptr->light_pen_time	 = proto->light_pen_time;
	}

	/*
	 * Enqueue the new request at the *end* of the queue
	 * and wakeup the GT command daemon.
	 */
	GT_LIST_INSERT_PREV(&gt_kmcbhead, qptr, gt_kmcblist, q_list);

	wakeup(&softc->wchan[WCHAN_SCHED]);

	if (cmd & HKKVC_PAUSE_WITH_INTERRUPT) {
		qptr->q_flags |= KQ_BLOCK;
		(void) sleep((caddr_t)qptr, PZERO-1);
	}

	return (0);
}


/*
 * Allocate the FCS
 */
static
gt_fcsalloc(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_fcsalloc *fcsptr = (struct fb_fcsalloc *) data;
	struct table_type  *tp     = softc->fcs_table;
	int index;

	for (index=0; index<NFCS; index++, tp++) {
		if (!(tp->tt_flags & TT_USED))
			break;
	}

	if (index == NFCS)
		return (ENOMEM);

	tp->tt_flags |= TT_USED;
	tp->tt_refcount++;

	gt_alloc_sr(softc, u.u_procp->p_pid, RT_FCS, index, 1);

	fcsptr->index = index;

	return (0);
}


/*
 * Deallocate the FCS
 */
static
gt_fcsfree(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_fcsalloc   *fcsptr   = (struct fb_fcsalloc *) data;
	int		      index    = (int) fcsptr->index;
	u_int		      sr_found = 0;
	struct 	proc_ll_type *found;
	struct shar_res_type *srptr, *prev;
	struct proc_ll_type  *gt_find_proc_link();

	found = gt_find_proc_link(softc->proc_list, u.u_procp->p_pid);
	if (found == NULL)
		return (EINVAL);

	prev = srptr = found->plt_sr;
	while (srptr != NULL && !(sr_found)) {
		if ((srptr->srt_type==RT_FCS) && (srptr->srt_base==index)) {
			sr_found = 1;
		} else {
			prev  = srptr;
			srptr = srptr->srt_next;
		}
	}

	if (!sr_found)
		return (EINVAL);

	gt_decrement_refcount(softc, RT_FCS, index, 1);

	if (srptr == found->plt_sr)
		found->plt_sr = srptr->srt_next;
	else
		prev->srt_next = srptr->srt_next;

	kmem_free((caddr_t)srptr, sizeof (*srptr));

	return (0);
}


/*
 * Give server-process privilege to the current process
 *
 * XXX: see security hole comment with gt_setdiagmode.
 */
static
gt_setserver(softc)
	struct gt_softc *softc;
{
	if (softc->server_proc != (short) 0)
		return (EINVAL);

	softc->server_proc = u.u_procp->p_pid;

	return (0);
}


static void
gt_server_free(softc)
	struct gt_softc *softc;
{
	softc->server_proc = (short) 0;
}


/*
 * Give the current process the GT diagnostic privilege.
 *
 * XXX: There is a bit of a security hole here:  we identify the current
 * process as having diagnostic privilege by entering its pid in a table.
 * We clear that table entry when any address space segments that have been
 * set up by the GT segment driver are torn down.  If the current process
 * exits without ever having set up any GT memory type 1 segments, its
 * pid won't be cleared from the table.  The next process that gets assigned
 * that pid gets diagnostic privilege without even asking.
 *
 * The same type of hole exists for server privilege.
 */
static
gt_setdiagmode(softc)
	struct gt_softc *softc;
{
	int   got_avail = 0;
	int   i, avail_index;
	short pid;

	for (i=0; i < MAX_DIAG_PROC; i++) {
		pid = softc->diag_proc_table[i];

		if (pid == (short)0) {
			got_avail   = 1;
			avail_index = i;

		} else if (pid == u.u_procp->p_pid) {
			return (0);
		}
	}


	if (got_avail) {

			softc->diag_proc_table[avail_index] = u.u_procp->p_pid;
			softc->diag_proc++;
			return (0);
	} else
		return (EINVAL);
}


static void
gt_diag_free(softc)
	struct gt_softc *softc;
{
	int i;

	for (i=0; i < MAX_DIAG_PROC; i++) {
		if (softc->diag_proc_table[i] == u.u_procp->p_pid) {
			softc->diag_proc_table[i] = (short)0;
			softc->diag_proc--;
		}
	}
}


/*
 * Initialize the WID partition register
 */
static
gt_setwpart(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	u_int		no_of_bits = *(u_int *)data;
	Hkvc_kmcb	proto_kmcb;
	u_int		cmd;

	if (softc->server_proc == (short) 0 || gt_server_check(softc)) {
		if (no_of_bits > NWID_BITS)
			return (EINVAL);

		softc->wlut_wid_part = no_of_bits;
		softc->wid_ocount = gt_power_table[NWID_BITS - no_of_bits];
		softc->wid_icount = gt_power_table[no_of_bits];

		/*
		 * Tell the FE about the new WID partition
		 */
		if (softc->flags & F_UCODE_RUNNING) {
			cmd = HKKVC_SET_IMAGE_WLUT_BITS |
			      HKKVC_PAUSE_WITH_INTERRUPT;

			proto_kmcb.wlut_image_bits = no_of_bits;

			if (gt_post_command(softc, cmd, &proto_kmcb))
				return (EINVAL);
		}
	} else {
		return (EINVAL);
	}

	return (0);
}


/*
 * Returns the value in the WID partition register
 */
static
gt_getwpart(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	*(u_int *)data = softc->wlut_wid_part;

	return (0);
}


/*
 * Initialize the overlay clut partition
 */
static
gt_setclutpart(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	u_int no_of_cluts = *(u_int *)data;

	if (!(gt_diag_check(softc)))
		return (EACCES);

	if (no_of_cluts>NCLUT_8)
		return (EINVAL);

	softc->clut_part_total = no_of_cluts;

	return (0);
}


/*
 * Read back the overlay clut partition
 */
static
gt_getclutpart(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	if (!(gt_server_check(softc)))
		return (EACCES);

	*(u_int *)data = softc->clut_part_total;

	return (0);
}


/*
 * Enable/Disable lightpen
 */
static
gt_lightpenenable(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	int	  enable = *(int *) data;
	u_int	  cmd;
	Hkvc_kmcb proto_kmcb;

	if (enable)
		softc->light_pen_enable = 1;
	else
		softc->light_pen_enable = 0;

	proto_kmcb.light_pen_enable   = softc->light_pen_enable;
	proto_kmcb.light_pen_distance = softc->light_pen_distance;
	proto_kmcb.light_pen_time     = softc->light_pen_time;

	cmd = HKKVC_SET_LIGHT_PEN_PARAMETERS | HKKVC_PAUSE_WITH_INTERRUPT;

	if (gt_post_command(softc, cmd, &proto_kmcb))
		return (EFAULT);

	return (0);
}


/*
 * Initialize the lightpen parameters
 */
static
gt_setlightpenparam(softc, data)
	struct gt_softc *softc;
	caddr_t		data;
{
	struct fb_lightpenparam	*param = (struct fb_lightpenparam *) data;
	Hkvc_kmcb proto_kmcb;
	u_int	  cmd;

	if (param->pen_distance)
		softc->light_pen_distance = param->pen_distance;

	if (param->pen_time)
		softc->light_pen_time = param->pen_time;

	proto_kmcb.light_pen_enable = softc->light_pen_enable;
	proto_kmcb.light_pen_distance = softc->light_pen_distance;
	proto_kmcb.light_pen_time     = softc->light_pen_time;

	cmd = HKKVC_SET_LIGHT_PEN_PARAMETERS | HKKVC_PAUSE_WITH_INTERRUPT;

	if (gt_post_command(softc, cmd, &proto_kmcb))
		return (EFAULT);

	return (0);
}


/*
 * Readback the lightpen parameters
 */
static
gt_getlightpenparam(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_lightpenparam	*param = (struct fb_lightpenparam *) data;

	param->pen_distance = softc->light_pen_distance;
	param->pen_time	    = softc->light_pen_time;

	return (0);
}


/*
 * Register the monitor-specific width, height and frame-buffer
 * width register value with the GT frontend.
 */
static
gt_setmonitor(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{

	fbi_reg	 *fbi_regs	= (fbi_reg *)softc->va[MAP_FBI_REG];
	rp_host  *rp_host_ptr	= (rp_host *)softc->va[MAP_RP_HOST_CTRL];
	int       type		= (int) *(int *)data;
	u_int	  cmd;
	Hkvc_kmcb proto_kmcb;

	if (!gt_diag_check(softc))
		return (EACCES);

	if (type >= NGT_MONITORS)
		return (EINVAL);

	/*
	 * ioctl overrides OpenBoot PROM's values
	 */
	if (type != -1) {
		struct gt_monitor *mp = &gt_monitor[type];
		u_int host_stateset;

  		softc->w = mp->gtm_fbtype.fb_width;
		softc->h = mp->gtm_fbtype.fb_height;
		softc->monitor = type;

		/*
		 * Store override value into the FB width register
		 */
		host_stateset = rp_host_ptr->rp_host_csr_reg & 0x1;
		rp_host_ptr->rp_host_csr_reg |= 1;

#ifdef	MULTIPROCESSOR
		xc_attention();		/* Prevent users from busying the PF */
#endif	MULTIPROCESSOR

		if (gt_check_pf_busy(softc)) {
#ifdef	MULTIPROCESSOR
			xc_dismissed();
#endif	MULTIPROCESSOR
			return (EFAULT);
		}

		fbi_regs->fbi_reg_fb_width = mp->gtm_fbwidth;

#ifdef	MULTIPROCESSOR
		xc_dismissed();
#endif	MULTIPROCESSOR

		if (host_stateset == 0)
			rp_host_ptr->rp_host_csr_reg &= ~0x1;
	}

	proto_kmcb.screen_width  = softc->w;
	proto_kmcb.screen_height = softc->h;
	proto_kmcb.fb_width      = gt_monitor[softc->monitor].gtm_fbwidth;

	/*
	 * Now inform the FE microcode.
	 */
	cmd = HKKVC_SET_FB_SIZE | HKKVC_PAUSE_WITH_INTERRUPT;
	if (gt_post_command(softc, cmd, &proto_kmcb)) {
		return (EINVAL);
	}

	return (0);
}


/*
 * Fetch the monitor-type from the FB_WIDTH register in PP
 */
static
gt_getmonitor(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	if (softc->monitor >= NGT_MONITORS)
		return (EINVAL);

	*(u_int *) data = gt_monitor[softc->monitor].gtm_fbwidth;

	return (0);
}


/*
 * Fetch or store the gamma value
 */
static
gt_setgamma(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_setgamma *fbs = (struct fb_setgamma *) data;

	if (gt_server_check(softc) || gt_diag_check(softc)) {

		softc->gammavalue = fbs->fbs_gamma;
		gt_set_flags(&softc->flags, F_GAMMASET);

		if (fbs->fbs_flag & FB_DEGAMMA)
			gt_set_flags(&softc->flags, F_VASCO);
		else
			gt_clear_flags(&softc->flags, F_VASCO);

		return (0);
	} else
		return (EACCES);
}


static
gt_getgamma(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fb_setgamma *fbs = (struct fb_setgamma *) data;

	if (softc->flags & F_GAMMASET) {
		fbs->fbs_gamma = softc->gammavalue;
		fbs->fbs_flag  = (softc->flags&F_VASCO) ?
		    FB_DEGAMMA:FB_NO_DEGAMMA;

		return (0);
	} else {
		fbs->fbs_gamma = (float) 0.0;
		fbs->fbs_flag  = 0;

		return (EINVAL);
	}
}


static
gt_sgversion(softc, data, flag)
	struct gt_softc *softc;
	caddr_t data;
	int flag;
{
	struct gt_version *gtv = (struct gt_version *) data;

	if (flag == 0) {
		if (gt_diag_check(softc) == 0)
			return (EINVAL);
		else
			softc->gt_version = *gtv;
	} else {
		*gtv = softc->gt_version;
	}

	return (0);
}


/*
 * Set up bwtwo mode
 */
static
gt_fbiosattr(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fbsattr	*sattr = (struct fbsattr *) data;

	softc->gattr->sattr.emu_type = sattr->emu_type;
	softc->gattr->sattr.flags = sattr->flags;

	if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW) {
		softc->gattr->fbtype.fb_type   = FBTYPE_SUN2BW;
		softc->gattr->fbtype.fb_height = softc->h;
		softc->gattr->fbtype.fb_width  = 2048;
		softc->gattr->fbtype.fb_depth  = 1;
		softc->gattr->fbtype.fb_cmsize = 2;
		softc->gattr->fbtype.fb_size = (softc->h * 2048) / 8;
	} else {
		gt_set_gattr_fbtype(softc);
	}
	
	if (sattr->flags & FB_ATTR_DEVSPECIFIC)
		bcopy((char *) sattr->dev_specific,
		     (char *) softc->gattr->sattr.dev_specific,
		     sizeof(sattr->dev_specific));

	/* under new ownership */
	softc->owner = u.u_procp;
	softc->gattr->owner = (int) u.u_procp->p_pid;

	return (0);
}


/*
 * Return the frame buffer mode
 */
static
gt_fbiogattr(softc, data)
	struct gt_softc *softc;
	caddr_t data;
{
	struct fbgattr *gattr = (struct fbgattr *) data;
	
	/*
	 * set owner if not owned or if previous owner is dead
	 */
	if (softc->owner == NULL || softc->owner->p_stat == NULL ||
	    (int) softc->owner->p_pid != softc->gattr->owner) {
		softc->owner = u.u_procp;
		softc->gattr->owner = (int) u.u_procp->p_pid;
		gt_set_gattr_fbtype(softc);
	}

	*gattr = *(softc->gattr);
	
	return (0);
}


static void
gt_set_gattr_fbtype(softc)
	struct gt_softc *softc;
{
	gt_attr = gt_attrdefault;
	gt_attr.fbtype = gt_monitor[softc->monitor].gtm_fbtype;
	softc->gattr = &gt_attr;
}


/*
 * Load the KMCB pointer into FE memory and initialize the FE.
 * Expected to be used only by gtconfig.
 */
static
gt_loadkmcb(softc)
	struct gt_softc *softc;
{
	fe_ctrl_sp	*fep = softc->fe_ctrl_sp_ptr;
	Hkvc_kmcb	*kmcbp = (Hkvc_kmcb *) &softc->kmcb;
	fe_i860_sp	*fe_i860_regs = (fe_i860_sp *) softc->va[MAP_FE_I860];

	/*
	 * restrict this to a diagnostic-mode process
	 */
	if (gt_diag_check(softc) == 0)
		return (EINVAL);

	/*
	 * We may be reloading code on a running system, so we need
	 * to clean up accelerator resources here.
	 */
	gta_reset_accelerator(softc);

	gt_init_kmcb(softc);	/* XXX: move this to general initialization */

	/*
	 * Store the kmcb`s physical address into the test register,
	 * release the FE hold, and assert the FE go bit to start
	 * the frontend executing.
	 */
	fep->fe_test_reg	 =  (u_int)GT_KVTOP((int)kmcbp);
	fe_i860_regs->fe_hold	&= ~FE_HOLD;

	if (fe_i860_regs->fe_hold & FE_HLDA) {
		printf("gt_loadkmcb error: ~FE_HOLD not acknowledged\n");
		return (EINVAL);
	}

	fep->fe_hcr |= FE_GO;

	/*
	 * Enable interrupts
	 */
	gt_enable_intr(softc, HISR_ENABMASK);


	/*
	 * Solicit an interrupt from the frontend microcode.  Having
	 * gotten it, we shall assume that the frontend is initialized
	 * and running.
	 */
	gt_set_flags(&softc->flags, F_UCODE_RUNNING);
	if (gt_post_command(softc, (u_int)HKKVC_PAUSE_WITH_INTERRUPT,
	    (Hkvc_kmcb *)0)) {
		gt_clear_flags(&softc->flags, F_UCODE_RUNNING);
		return (EINVAL);
	}

	return (0);
}


/*
 * Initialize the kernel MCB structure.
 * Make the softc struct non-cacheable.
 * XXX: separate these two functions
 */
static void
gt_init_kmcb(softc)
	struct gt_softc *softc;
{
	Hkvc_kmcb *kmcb = (Hkvc_kmcb *) &softc->kmcb;
	u_char    *cp, *lp;

	cp = (u_char *) ((u_int)softc & MMU_PAGEMASK);
	lp = (u_char *) softc + sizeof (struct gt_softc);

	for (; cp < lp; cp += MMU_PAGESIZE)
		gt_setnocache((addr_t)cp);


	cp = (u_char *) ((u_int)gt_printbuffer & MMU_PAGEMASK);
	lp = (u_char *) gt_printbuffer + gt_printbuffer_size;

	for (; cp < lp; cp += MMU_PAGESIZE)
		gt_setnocache((addr_t)cp);

	kmcb->magic   = HK_KMCB_MAGIC;
	kmcb->version = HK_KMCB_VERSION;
	kmcb->print_buffer = (u_int *) GT_KVTOP((int) softc->printbuf);
}


/*
 * Register this process as an accelerator user.
 *
 * Create an at_proc struct for this process.  Record the address
 * of the process's umcb and return the addresses of his uvcr and svcr.
 */
static
gt_connect(softc, data, dev)
	struct gt_softc *softc;
	caddr_t data;
	dev_t   dev;
{
	struct gt_connect  *iop = (struct gt_connect *) data;
	struct at_proc     *ap;
	dev_t  device;
	u_int  vaddr;

	/*
	 * Make a portless device number
	 */
	device = makedev(major(dev), minor(dev)>>1);

	if (!(softc->flags & F_UCODE_RUNNING))
		return (EINVAL);

	/*
	 * Ensure that this process hasn't already registered
	 * itself as an accelerator user.
	 */
	if (gta_findproc(softc) != (struct at_proc *)NULL)
		return (EINVAL);

	/*
	 * Create the at_proc struct and hang it on the list
	 * of accelerator processes.
	 */
	ap = (struct at_proc *) new_kmem_zalloc(sizeof (*ap), KMEM_SLEEP);

	ap->ap_softc = softc;
	ap->ap_proc  = u.u_procp;
	ap->ap_pid   = u.u_procp->p_pid;
	ap->ap_as    = u.u_procp->p_as;
	ap->ap_umcb  = iop->gt_pmsgblk;

	GT_LIST_INIT(ap, at_proc, ap_list);

	ap->ap_lvl1  =
	    (struct gt_lvl1 *) getpages(btopr(sizeof (struct gt_lvl1)), 0);

	gt_setnocache((addr_t)ap->ap_lvl1);
	bzero((caddr_t)ap->ap_lvl1, sizeof (struct gt_lvl1));

	/*
	 * A little paranoia here.  Make sure the user's mcb
	 * shows the GT as user-stopped.
	 */
	(void) suword((caddr_t) &ap->ap_umcb->gt_stopped, HK_USER_STOPPED);


	/*
	 * Give the caller the virtual addresses of his user and signal
	 * VCRs.  Make sure that he doesn't have loaded translations to
	 * them so that we can detect when he references them.
	 */
	if (gtsegmap(device,
	    gt_map[MAP_FE_HFLAG2].vaddr,
	    u.u_procp->p_as,
	    (addr_t *) &vaddr,
	    sizeof (struct fe_page_2),
	    (u_int) 0, (u_int) 0, (u_int) MAP_PRIVATE,
	    (struct ucred *) NULL) == -1) {
		return (EINVAL);
	}
	iop->gt_puvcr = ap->ap_uvcr = &((struct fe_page_2 *)vaddr)->fe_hflag_2;

	if (gtsegmap(device,
	    gt_map[MAP_FE_HFLAG1].vaddr,
	    u.u_procp->p_as,
	    (addr_t *) &vaddr,
	    sizeof (struct fe_page_1),
	    (u_int) 0, (u_int) 0, (u_int) MAP_PRIVATE,
	    (struct ucred *) NULL) == -1) {
		return (EINVAL);
	}
	iop->gt_psvcr = ap->ap_svcr = &((struct fe_page_1 *)vaddr)->fe_hflag_1;

	gt_set_flags(&ap->ap_flags, (GTA_CONNECT | GTA_VIRGIN));
	GT_LIST_INSERT_NEXT(softc->at_head, ap, at_proc, ap_list);

	return (0);
}


/*
 * Unregister this process as an accelerator user
 */
static
gt_disconnect(softc)
	struct gt_softc *softc;
{
	struct at_proc *ap;

	ap = gta_findproc(softc);

	if (ap != (struct at_proc *)NULL)
		gta_exit(ap);

	return (0);
}


/*
 * Terminate the accelerator thread:
 *
 *	- remove the at_proc struct from the accelerator process list.
 *	- unmap the user VCR pages.
 *	- free the page tables.
 *	- free the GT lockmap elements
 *	- free the at_struct element.
 */
static void
gta_exit(ap)
	struct at_proc *ap;
{
	struct gt_softc    *softc;
	struct seg	   *seg;
	struct gta_lockmap *map, *maphead, *nxtmap;

	softc = ap->ap_softc;

	if (ap->ap_flags & GTA_EXIT)
		return;

	gt_set_flags(&ap->ap_flags, GTA_EXIT);

	/*
	 * Ensure that the accelerator isn't running
	 * display lists for this process.
	 */
	while (softc->at_curr == ap) {
		wakeup(&softc->wchan[WCHAN_SCHED]);
		(void) sleep(&softc->wchan[WCHAN_SUSPEND], PZERO+1);
	}

	GT_LIST_REMOVE(ap, at_proc, ap_list);

	seg = as_segat(ap->ap_as, (addr_t)ap->ap_svcr);
	if (seg != (struct seg *)NULL) {
		if (seg == softc->svcrseg)
			softc->svcrseg = (struct seg *)NULL;

		if (gt_validthread(softc, ap) &&
		    (!(ap->ap_proc->p_flag & SWEXIT)))
			(void) seggt_unmap(seg, seg->s_base, seg->s_size);
	}

	seg = as_segat(ap->ap_as, (addr_t)ap->ap_uvcr);
	if (seg != (struct seg *)NULL) {
		if (seg == softc->uvcrseg)
			softc->uvcrseg = (struct seg *)NULL;

		if (gt_validthread(softc, ap) &&
		    (!(ap->ap_proc->p_flag & SWEXIT)))
			(void) seggt_unmap(seg, seg->s_base, seg->s_size);
	}

	gt_freept(ap);

	maphead = &gta_lockbusy;
	nxtmap  = GT_LIST_NEXT(maphead, gta_lockmap, gtm_list);

	while (nxtmap != maphead) {
		map = nxtmap;
		nxtmap = GT_LIST_NEXT(nxtmap, gta_lockmap, gtm_list);

		if (map->gtm_as == ap->ap_as)
			gt_freemap(map);
	}


	if (ap->ap_flags & GTA_SWLOCK)
		ap->ap_proc->p_swlocks--;

	ap->ap_as->a_hatcallback = 0;

	kmem_free((caddr_t)ap, sizeof (*ap));
}


/*
 * Free page tables
 */
static void
gt_freept(ap)
	struct at_proc *ap;
{
	struct gt_lvl1 *lvl1;
	struct gt_lvl2 **lvl2;
	int i;

	lvl1 = ap->ap_lvl1;
	lvl2 = lvl1->gt_lvl2;

	/*
	 * Return any level 2 tables
	 */
	for (i=0; i<GT_ENTRIES_PER_LVL1; i++, lvl2++) {
		if (*lvl2 != (struct gt_lvl2 *)NULL) {
			freepages((caddr_t) *lvl2,
			    btopr(sizeof (struct gt_lvl2)));
		}
	}

	/*
	 * And the level 1 table
	 */
	freepages((caddr_t) lvl1, btopr(sizeof (struct gt_lvl1)));
}


/*
 * Remove a GT lockmap element from the active lists.
 */
static void
gt_freemap(map)
	struct gta_lockmap *map;
{
	int		  s;
	struct page	 *pp;
#ifdef sun4c
	struct seg	 *seg;
	caddr_t		  addr;
	struct pte	  pte;
#endif sun4c
	extern struct seg kseg;

#ifdef GT_DEBUG
	gtstat.gs_freemap++;
#endif GT_DEBUG

	/*
	 * Remove it from the hash table and from the busy list.
	 */
	gta_hash_delete(map);
	GT_LIST_REMOVE(map, gta_lockmap, gtm_list);

	pp = map->gtm_pp;

	s = splvm();

	/*
	 * Release the mapping
	 */
#ifdef sun4c
	addr  = gta_dvmaspace + ((int)(map-gta_lockmapbase) << MMU_PAGESHIFT);
	seg   = &kseg;

	mmu_getpte(addr, &pte);
	pp = page_numtookpp(pte.pg_pfnum);

 	hat_unlock(seg, addr);
 	hat_unload(seg, addr, MMU_PAGESIZE);
#endif sun4c


	page_pp_unlock(pp, 0);

	(void) splx(s);

	/*
	 * Add the GT lockmap element to the free list.
	 */
	GT_LIST_INSERT_PREV(&gta_lockfree, map, gta_lockmap, gtm_list);
}


static
gt_vmctl(softc, data)
	struct gt_softc *softc;
	caddr_t		 data;
{
	struct fb_vmctl *fbv = (struct fb_vmctl *)data;
	int rc;

	switch (fbv->vc_function) {

	case FBV_SUSPEND:
		rc = gta_suspend(softc);
		break;

	case FBV_RESUME:
		rc = gta_resume(softc);
		break;

	default:
		rc = EINVAL;
		break;
	}

	return (rc);
}


static
gta_suspend(softc)
	struct gt_softc *softc;
{
	struct at_proc *ap;

#ifdef GT_DEBUG
	gtstat.gs_suspend++;
#endif GT_DEBUG

	ap = gta_findproc(softc);
	if (ap == (struct at_proc *)NULL)
		return (EINVAL);

	gt_set_flags(&ap->ap_flags,   GTA_SUSPEND);

	/*
	 * If this thread is currently running, we need to deschedule it.
	 */
	while (ap == (struct at_proc *)softc->at_curr) {
		gt_set_flags(&softc->flags, F_SUSPEND);

		wakeup(&softc->wchan[WCHAN_SCHED]);
		(void) sleep(&softc->wchan[WCHAN_SUSPEND], PZERO+1);
	}

	return (0);
}

static
gta_resume(softc)
	struct gt_softc *softc;
{
	struct at_proc *ap;

	ap = gta_findproc(softc);
	if (ap == (struct at_proc *)NULL)
		return (EINVAL);

	gt_clear_flags(&ap->ap_flags, GTA_SUSPEND);

	return (0);
}

/*
 * Explicitly instantiate/uninstantiate GT virtual memory
 */
static
gt_vmback(data)
	caddr_t		 data;
{
	struct fb_vmback   *fbv = (struct fb_vmback *)data;
	struct segvn_data  *svd;
	struct segvn_crargs crargs_gta;
	struct as  *as;
	struct seg *seg;
	caddr_t addr, eaddr, seglimit;
	int	len, rc;
	u_int	nsize;

	addr = fbv->vm_addr;
	len  = fbv->vm_len;

	/*
	 * Normalize addr and len to be page-aligned
	 */
	addr  = (caddr_t) ((u_int)addr & PAGEMASK);
	len   = (len + PAGESIZE-1) & PAGEMASK;
	eaddr = addr + len;

	if (len <= 0)
		return (EINVAL);

	as = u.u_procp->p_as;

	gt_aslock(as);

	/*
	 * Convert seggtx segments into seggta segments
	 */
	while (len > 0) {

		seg = as_segat(as, addr);
		if (seg == (struct seg *)NULL) {
			gt_asunlock(as);
			return (EINVAL);
		}

		if (seg->s_ops != &seggtx_ops) {
			addr = seg->s_base + seg->s_size;
			len  = eaddr - addr;
			continue;
		}

		/*
		 * Got a seggtx segment to instantiate.  If we
		 * are doing just a part of it, break the seggtx
		 * into multiple segments.
		 */
		svd = (struct segvn_data *)seg->s_data;
		crargs_gta.vp      = (struct vnode *)NULL;
		crargs_gta.offset  = svd->offset + addr - seg->s_base;
		crargs_gta.cred    = svd->cred;
		crargs_gta.type    = svd->type;
		crargs_gta.maxprot = svd->maxprot;
		crargs_gta.prot    = svd->prot;
		crargs_gta.amp     = (struct anon_map *)NULL;

		seglimit = seg->s_base + seg->s_size;
		nsize    = (seglimit >= eaddr) ? eaddr-addr : seglimit-addr;
		len      = eaddr - (addr + nsize);

		(void) seggtx_unmap(seg, addr, nsize);

		rc = as_map(as, addr, nsize, seggta_create,
		    (caddr_t)&crargs_gta);
		if (rc) {
			gt_asunlock(as);
			return (rc);
		}

		seg = as_segat(as, addr);
		if (seg == (struct seg *)NULL) {
			gt_asunlock(as);
			return (EINVAL);
		}

		rc = (seggta_ops.fault)(seg, addr, len, F_INVAL, S_WRITE);
		if (rc) {
			gt_asunlock(as);
			return (rc);
		}
	}

	gt_asunlock(as);
	return (0);
}


static
gt_vmunback(data)
	caddr_t		 data;
{
	struct fb_vmback  *fbv = (struct fb_vmback *)data;
	struct segvn_data *svd;
	struct segvn_crargs crargs_gta;
	struct as  *as;
	struct seg *seg;
	caddr_t addr, eaddr, seglimit;
	int len, rc;
	u_int nsize;

	addr = fbv->vm_addr;
	len  = fbv->vm_len;

	/*
	 * Normalize addr and len in case the user hasn't already
	 * done so.
	 */
	addr  = (caddr_t) ((u_int)addr & PAGEMASK);
	len   = (len + PAGESIZE-1) & PAGEMASK;
	eaddr = addr + len;

	if (len <= 0)
		return (EINVAL);

	as = u.u_procp->p_as;

	gt_aslock(as);

	/*
	 * Convert seggta segments into seggtx segments
	 */
	while (len > 0) {
		seg = as_segat(as, addr);
		if (seg == (struct seg *)NULL) {
			gt_asunlock(as);
			return (EINVAL);
		}

		if (seg->s_ops != &seggta_ops) {
			addr = seg->s_base + seg->s_size;
			len  = eaddr - addr;
			continue;
		}

		/*
		 * Got a seggta segment to uninstantiate.
		 */
		svd = (struct segvn_data *)seg->s_data;
		crargs_gta.offset = svd->offset + addr - seg->s_base;
		crargs_gta.cred   = svd->cred;
		crargs_gta.type   = svd->type;
		crargs_gta.maxprot = svd->maxprot;

		seglimit = seg->s_base + seg->s_size;
		nsize    = (seglimit >= eaddr) ? eaddr-addr : seglimit-addr;
		len      = eaddr - (addr + nsize);

		(void) seggta_unmap(seg, addr, nsize);

		rc = as_map(as, addr, nsize, seggtx_create,
		    (caddr_t)&crargs_gta);
		if (rc) {
			gt_asunlock(as);
			return (EINVAL);
		}
	}

	gt_asunlock(as);
	return (0);
}


/*
 * Allow processes to grab the accelerator hardware.
 *
 * Inform the scheduler not schedule any accelerator threads until
 * the lock is released and stop the current accelerator thread.
 *
 * gt_grabtype specifies variations on the grab/ungrab theme
 *
 * gt_grabtype		action
 *
 *	0		business as usual
 *	1		lazy accelerator scheduling following ungrab
 */

int gt_grabtype = 0;

static
gt_grabhw(softc, data)
	struct gt_softc *softc;
	caddr_t		  data;
{

#ifdef GT_DEBUG
	gtstat.gs_grabhw++;
#endif GT_DEBUG

	if (softc->flags & F_UCODE_RUNNING) {

		softc->gt_grabber = u.u_procp->p_pid;

		gt_set_flags(&softc->flags, F_GRAB_HOLD);

		/*
		 * Deschedule the current thread
		 */
		while (softc->flags & F_THREAD_RUNNING) {
			gt_set_flags(&softc->flags, F_SUSPEND);

			wakeup(&softc->wchan[WCHAN_SCHED]);
			(void) sleep(&softc->wchan[WCHAN_SUSPEND], PZERO+1);
		}

		*(u_int *)data = 1;

	} else {
		*(u_int *)data = 0;
	}

	return (0);
}


/*
 * Allow processes to release the accelerator hardware.
 */
static
gt_ungrabhw(softc)
	struct gt_softc *softc;
{
	if (gt_grabtype == 1) {
		gt_clear_flags(&softc->flags, F_GRAB_HOLD);
	} else {
		gta_run(softc, F_GRAB_HOLD);
	}
 
	softc->gt_grabber = (short)0;

	return (0);
}


static void
gta_reset_accelerator(softc)
	struct gt_softc *softc;
{
	struct at_proc *ap, *ap_head, *ap_next;
	int s;

	s = splr(gt_priority);

	/*
	 * Signal any processes using the accelerator
	 */
	ap_head = softc->at_head;
	ap = GT_LIST_NEXT(ap_head, at_proc, ap_list);

	while (ap != ap_head) {
		ap_next = GT_LIST_NEXT(ap, at_proc, ap_list);

		if (gt_validthread(softc, ap)) {
			if (!(ap->ap_proc->p_flag & SWEXIT))
				psignal(ap->ap_proc, SIGTERM);

			gta_exit(ap);
		}

		ap = ap_next;
	}

	/*
	 * Clean up the kmcb queue
	 */
	gt_clean_kmcb();

	/*
	 * Reset most of the softc flags
	 */
	softc->flags &= (F_VRT_RETRACE | F_GAMMALOADED |
	    F_DEGAMMALOADED | F_GAMMASET | F_VASCO);

	/*
	 * Wakeup processes that may be waiting for accelerator events
	 */
	wakeup(&softc->wchan[WCHAN_VRT]);
	wakeup(&softc->wchan[WCHAN_VCR]);
	wakeup(&softc->wchan[WCHAN_SUSPEND]);
	wakeup(&softc->wchan[WCHAN_THAW]);

	(void) splx(s);
}


static void
gt_clean_kmcb()
{
	struct gt_kmcblist *kptr, *next;

	next = GT_LIST_NEXT(&gt_kmcbhead, gt_kmcblist, q_list);

	while (next != &gt_kmcbhead) {
		kptr = next;
		next = GT_LIST_NEXT(next, gt_kmcblist, q_list);

		GT_LIST_REMOVE(kptr, gt_kmcblist, q_list);

		if (kptr->q_flags & KQ_BLOCK)
			wakeup((caddr_t)kptr);

		kmem_free((caddr_t)kptr, sizeof (*kptr));
	}
}


/*
 * Make a page non-cached.
 */
static void
gt_setnocache(base)
	addr_t base;
{
	int s;
	struct pte pte;

#ifdef sun4m
	/*
	 * Viking?  Great!!!
	 */
	extern struct modinfo mod_info[];

	if ((mod_info[0].mod_type == CPU_VIKING) ||
	    (mod_info[0].mod_type == CPU_VIKING_MXCC)) {
		return;
	} else {
		panic("gt_setnocache: sun4m/ross not supported");
	}
#endif sun4m

	base = (addr_t)((int)base & MMU_PAGEMASK);

	s = splvm();

	mmu_getpte(base, &pte);

	if (pte_valid(&pte)) {
#ifdef sun4c
		pte.pg_nc = 1;
		mmu_setpte(base, pte);
#endif sun4c
	}

	(void) splx(s);
}


/*
 * Shared Resource Management
 */

/*
 * Detach a proc list element
 */
static void
gt_link_detach(ptr)
	struct proc_ll_type *ptr;
{
	ptr->plt_prev->plt_next = ptr->plt_next;
	ptr->plt_next->plt_prev = ptr->plt_prev;
}


/*
 * Attach a proc list element between ptr1 and ptr2
 */
static void
gt_link_attach(ptr1, ptr2, newptr)
	struct proc_ll_type *ptr1, *ptr2, *newptr;
{
	newptr->plt_next = ptr1->plt_next;
	ptr1->plt_next   = newptr;
	newptr->plt_prev = ptr2->plt_prev;
	ptr2->plt_prev   = newptr;
}


/*
 * Find a proc list element for the specified pid.
 */
static struct proc_ll_type *
gt_find_proc_link(startptr, pid)
	struct proc_ll_type *startptr;
	short	pid;
{
	struct proc_ll_type *tptr      = startptr->plt_next;
	struct proc_ll_type *found_ptr = NULL;
	u_int	found = 0;

	if (startptr->plt_pid == pid)
		return (startptr);

	while (!found && tptr != startptr) {
		if (tptr->plt_pid == pid) {
			found_ptr = tptr;
			found = 1;
		} else {
			tptr = tptr->plt_next;
		}
	}

	if (found)
		return (found_ptr);
	else
		return (NULL);
}


/*
 * Insert a proc list element into the list, in ascending order.
 */
static void
gt_insert_link(softc, startptr, cptr)
	struct gt_softc	*softc;
	struct proc_ll_type	*startptr;
	struct proc_ll_type	*cptr;
{
	struct proc_ll_type *tptr = startptr;

	if (startptr->plt_pid <= cptr->plt_pid) {
		tptr = startptr->plt_next;

		while ((tptr != startptr) && (cptr->plt_pid > tptr->plt_pid))
			tptr = tptr->plt_next;

		gt_link_attach(tptr->plt_prev, tptr, cptr);
	} else {
		/* Insert in the front */
		/* Therefore change startptr */
		gt_link_attach(tptr->plt_prev, tptr, cptr);
		softc->proc_list = cptr;
	}

}


static void
gt_alloc_sr(softc, pid, type, base, count)
	struct gt_softc *softc;
	short		  pid;
	enum res_type	  type;
	int		  base;
	u_int		  count;
{
	struct proc_ll_type *found = NULL;
	struct proc_ll_type *gt_find_proc_link();

	if (softc->proc_list != NULL)
		found = gt_find_proc_link(softc->proc_list, pid);

	if (found == NULL) {
		found = (struct proc_ll_type *)
		     new_kmem_zalloc(sizeof (*found), KMEM_SLEEP);
		found->plt_pid = pid;
		found->plt_sr = NULL;
		if (softc->proc_list != NULL)
			gt_insert_link(softc, softc->proc_list, found);
		else {
			softc->proc_list = found;
			found->plt_next  = found;
			found->plt_prev  = found;
		}
	}
	gt_init_sr(found, type, base, count);
}


static void
gt_init_sr(found, type, base, count)
	struct proc_ll_type *found;
	enum res_type	type;
	int		base;
	u_int		count;
{
	struct	shar_res_type *srptr, *newptr, *prevptr;

	newptr = (struct shar_res_type *) new_kmem_zalloc(sizeof (*newptr),
	    KMEM_SLEEP);
	newptr->srt_next = NULL;
	newptr->srt_type = type;
	newptr->srt_base = base;
	newptr->srt_count = count;

	if (found->plt_sr != NULL) {
		srptr=found->plt_sr;
		prevptr = srptr;
		while (srptr != NULL) {
			prevptr = srptr;
			srptr   = srptr->srt_next;
		}
		prevptr->srt_next = newptr;
	} else {
		found->plt_sr = newptr;
	}
}


/*
 * Decrement the ref count on different types of shared resource tables
 */
static void
gt_decrement_refcount(softc, type, base, count)
	struct gt_softc *softc;
	enum res_type	 type;
	int		 base;
	u_int		 count;
{
	u_int i;

	switch (type) {

	case RT_CLUT8:
		for (i=0; i<count; i++,base++) {
			softc->clut_table[base].tt_refcount--;
			if (softc->clut_table[base].tt_refcount == 0)
				softc->clut_table[base].tt_flags &= ~TT_USED;
		}
		break;

	case RT_OWID:
		for (i=0; i<count; i++,base++) {
			softc->wid_otable[base].tt_refcount--;
			if (softc->wid_otable[base].tt_refcount == 0)
				softc->wid_otable[base].tt_flags &= ~TT_USED;
		}
		break;

	case RT_IWID:
		for (i=0; i<count; i++,base++) {
			softc->wid_itable[base].tt_refcount--;
			if (softc->wid_itable[base].tt_refcount == 0)
				softc->wid_itable[base].tt_flags &= ~TT_USED;
		}
		break;

	case RT_FCS:
		for (i=0; i<count; i++,base++) {
			softc->fcs_table[base].tt_refcount--;
			if (softc->fcs_table[base].tt_refcount == 0)
				softc->fcs_table[base].tt_flags &= ~TT_USED;
		}
		break;

	default:
		panic("gt bad data type");
	}
}


/*
 *	SEGMENT DRIVER ROUTINES
 *
 * This implements a graphical context switch that is transparent to the user.
 *
 * This virtualizes the register file by associating a register
 * save area with each mapping of the device (each  segment).
 *
 * Only one of these mappings is valid at any time; a page fault
 * on one of the invalid mappings:
 *
 *	- saves the current context,
 *	- invalidates the current valid mapping,
 *	- restores the former register contents appropriate
 *	  to the faulting mapping,
 * 	- validates the faulting mapping
 */

struct seggt_data *shared_list;
struct gt_cntxt    shared_context;



/*
 * This routine is called via the cdevsw[] table to handle mmap() calls
 */
gtsegmap(dev, offset, as, addr, len, prot, maxprot, flags, cred)
	dev_t		 dev;
	u_int	 	 offset;
	struct as	*as;
	addr_t		*addr;
	u_int	 	 len;
	u_int		 prot;
	u_int		 maxprot;
	u_int		 flags;
	struct ucred	*cred;
{
	struct seggt_crargs	crargs;
	struct segvn_crargs	crargs_gtx;
	int			(*crfp)();
	caddr_t			argsp;
	int			rc;
	int			i;
	struct gt_softc		*softc = &gt_softc[minor(dev)>>1];

	/*
	 * If bwtwo emulation mode, change len to map in
	 * the entire frame buffer
	 */
	if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW)
		len = 0x2000000;

	/*
	 * If this is a direct-port map ensure that the entire range
	 * is legal and we are not trying to map more than the device
	 * will let us.
	 */
	if ((minor(dev) & GT_ACCEL_PORT) == 0) {
		for (i=0; i<len; i+=PAGESIZE) {
			if (gtmmap(dev, (off_t)offset+i, (int)maxprot) == -1)
				return (ENXIO);
		}
	}

	gt_aslock(as);

	/*
	 * If MAP_FIXED isn't specified, pick an address w/o worrying
	 * about any vac alignment constraints.  Otherwise, we have a
	 * user-specified address;  blow away any previous mappings.
	 */
	if ((flags & MAP_FIXED) == 0) {
		map_addr(addr, len, (off_t)0, 0);
		if (*addr == NULL) {
			gt_asunlock(as);
			return (ENOMEM);
		}
	} else {
		 i = (int)as_unmap(as, *addr, len);
	}

	/*
	 * Create the args and create-function pointer
	 * for the port being mapped.
	 */
	if ((minor(dev) & GT_ACCEL_PORT) == 0) {
		crfp  = seggt_create;
		argsp = (caddr_t) &crargs;

		crargs.dev    = dev;
		crargs.offset = offset;

		/*
		 * If this is a shared mapping, link it onto the
		 * shared list;  otherwise, create a private context.
		*/
		if (flags & MAP_SHARED) {
			struct seggt_crargs *a;

			a = shared_list;
			shared_list   = &crargs;
			crargs.link   = a;
			crargs.flag   = SEG_SHARED;
			crargs.cntxtp = &shared_context;

		} else if (offset == gt_map[MAP_FE_HFLAG1].vaddr) {
			crargs.link   = NULL;
			crargs.flag   = SEG_PRIVATE | SEG_SVCR;
			crargs.cntxtp = NULL;

		} else if (offset == gt_map[MAP_FE_HFLAG2].vaddr) {
			crargs.link   = NULL;
			crargs.flag   = SEG_PRIVATE | SEG_UVCR;
			crargs.cntxtp = NULL;

		} else {
			crargs.link   = NULL;
			crargs.flag   = SEG_PRIVATE;
			crargs.cntxtp = (struct gt_cntxt *) new_kmem_zalloc(
			    sizeof (struct gt_cntxt), KMEM_SLEEP);
		}

		rc = as_map(as, *addr, len, crfp, argsp);

	} else {
		/*
		 * Make a GT segment to nail down the address space.
		 */
		crfp  = seggtx_create;;
		argsp = (caddr_t) &crargs_gtx;

		crargs_gtx.offset  = offset;
		crargs_gtx.cred	   = cred;
		crargs_gtx.type	   = flags & MAP_TYPE;
		crargs_gtx.prot	   = prot;
		crargs_gtx.maxprot = maxprot;

		rc = as_map(as, *addr, len, crfp, argsp);
	}

	gt_asunlock(as);

	return (rc);
}

struct seg *gt_curcntxt = NULL;

static	int seggt_dup(/* seg, newsegp */);
static	int seggt_unmap(/* seg, addr, len */);
static	int seggt_free(/* seg */);
static	faultcode_t seggt_fault(/* seg, addr, len, type, rw */);
static	int seggt_checkprot(/* seg, addr, size, prot */);
static	int seggt_incore(/* seg, addr, len, vec */);
static	int seggt_badop();

struct	seg_ops seggt_ops =  {
	seggt_dup,		/* dup		*/
	seggt_unmap,		/* unmap	*/
	seggt_free,		/* free		*/
	seggt_fault,		/* fault	*/
	seggt_badop,		/* faulta	*/
	seggt_badop,		/* hatsync	*/
	seggt_badop,		/* setprot	*/
	seggt_checkprot,	/* checkprot	*/
	seggt_badop,		/* kluster	*/
	(u_int (*)()) NULL,	/* swapout	*/
	seggt_badop,		/* sync		*/
	seggt_incore,		/* incore	*/
	seggt_badop,		/* lockop	*/
	seggt_badop,		/* advise	*/
};



/*
 * Create a device segment.  The context is initialized by the create args.
 */
int
seggt_create(seg,argsp)
	struct seg *seg;
	caddr_t	   argsp;
{
	struct seggt_data *dp;

	dp = (struct seggt_data *)
		new_kmem_zalloc(sizeof (*dp), KMEM_SLEEP);

	*dp = *((struct seggt_crargs *)argsp);

	seg->s_ops  = &seggt_ops;
	seg->s_data = (char *)dp;

	return (0);
}


/*
 * Duplicate seg and return new segment in newsegp.  Copy original context.
 */
static
seggt_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	struct seggt_data *sdp = (struct seggt_data *)seg->s_data;
	struct seggt_data *ndp;

	(void) seggt_create(newseg, (caddr_t)sdp);

	ndp = (struct seggt_data *)newseg->s_data;

	if (sdp->flag & SEG_SHARED) {
		struct seggt_crargs *a;

		a	    = shared_list;
		shared_list = ndp;
		ndp->link   = a;

	} else if (sdp->flag & SEG_VCR) {
		ndp->cntxtp = NULL;
	} else {
		ndp->cntxtp = (struct gt_cntxt *)
			new_kmem_zalloc(sizeof (struct gt_cntxt), KMEM_SLEEP);
		*ndp->cntxtp = *sdp->cntxtp;
	}
	return (0);
}

static
seggt_unmap(seg, addr, len)
	struct seg *seg;
	addr_t addr;
	u_int len;
{
	struct seggt_data *sdp = (struct seggt_data *)seg->s_data;
	struct seg	  *nseg;
	addr_t		   nbase;
	u_int		   nsize;

	if (seg == (struct seg *)NULL)
		return (-1);

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
 		panic("seggt_unmap seg_alloc");

 	nseg->s_ops = seg->s_ops;

 	/*
 	 * Figure out what to do about the new context
	 */
 	nseg->s_data = (caddr_t)new_kmem_alloc(sizeof (struct seggt_data),
	    KMEM_SLEEP);

	if (sdp->flag & SEG_SHARED) {
		struct seggt_crargs *a;

		a = shared_list;
		shared_list = (struct seggt_data *)nseg->s_data;
		((struct seggt_data *)nseg->s_data)->link = a;
	} else {
		if (sdp->cntxtp == NULL) {
			((struct seggt_data *)nseg->s_data)->cntxtp = NULL;
		} else {
			((struct seggt_data *)nseg->s_data)->cntxtp =
			    (struct gt_cntxt *)
			    new_kmem_zalloc(sizeof (struct gt_cntxt),
			    KMEM_SLEEP);

			*((struct seggt_data *)nseg->s_data)->cntxtp =
			    *sdp->cntxtp;
		}
	}

 	/*
 	 * Now we do something so that all the translations which used
 	 * to be associated with seg but are now associated with nseg.
 	 */
 	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);

	return (0);
}

/*
 * free a segment and clean up its context.
 */
static
seggt_free(seg)
	struct seg *seg;
{
	struct seggt_data *sdp = (struct seggt_data *)seg->s_data;

	seggt_freeres(seg);

	if (seg == gt_curcntxt)
		gt_curcntxt = NULL;

	if ((sdp->flag & SEG_SHARED) == 0 && (sdp->cntxtp != NULL))
		kmem_free((caddr_t)sdp->cntxtp, sizeof (struct gt_cntxt));

	kmem_free((caddr_t)sdp, sizeof (*sdp));
}


/*
 * Reclaim driver resources used by this process
 */
seggt_freeres(seg)
	struct seg *seg;
{
	struct seggt_data	*sdp   = (struct seggt_data *)seg->s_data;
	struct gt_softc		*softc = &gt_softc[minor(sdp->dev)>>1];
	struct proc_ll_type	*found = NULL;
	struct shar_res_type	*srptr = NULL;
	struct at_proc		*ap;

	if (u.u_procp->p_flag & SWEXIT) {
		ap = gta_findas(seg->s_as);
		if (ap != (struct at_proc *)NULL)
			gta_exit(ap);

	} else if (sdp->flag & SEG_VCR) {
		if (seg == softc->uvcrseg)
			softc->uvcrseg = (struct seg *)NULL;
		if (seg == softc->svcrseg)
			softc->svcrseg = (struct seg *)NULL;
		return;
	}


	/*
	 * Free the shared resources that this process was using.
	 */
	if ((softc->proc_list != NULL) && (!(sdp->flag & SEG_LOCK) ||
	    (u.u_procp->p_pid != softc->server_proc))) {

		found = gt_find_proc_link(softc->proc_list, u.u_procp->p_pid);

		if (found != NULL) {

			/*
			 * This process was using some shared resources
			 */
			while (found->plt_sr != NULL) {
				srptr = found->plt_sr;
				found->plt_sr = srptr->srt_next;
				gt_decrement_refcount(softc,
				    srptr->srt_type,
				    srptr->srt_base,
				    srptr->srt_count);
				kmem_free((caddr_t)srptr, sizeof (*srptr));
			}

			gt_link_detach(found);

			if (found == softc->proc_list) {

				/* first element is being deleted */
				if (found == found->plt_next &&
				    found == found->plt_prev) {
					softc->proc_list = NULL;
				} else {
					softc->proc_list = found->plt_next;
				}
			}
			kmem_free((caddr_t)found, sizeof (*found));
		}
	}

	/*
	 * Release diagnostic and/or server privilege if appropriate.
	 */
	if (gt_diag_check(softc))
		gt_diag_free(softc);

	if (gt_server_check(softc)) {
		if (!(sdp->flag & SEG_LOCK))
			gt_server_free(softc);

		softc->clut_part_used = 0;
	}
}



#define GTMAP(sp, addr, page)			\
	hat_devload(				\
		sp,				\
		addr,				\
		fbgetpage((caddr_t)page),	\
		PROT_READ|PROT_WRITE|PROT_USER,	\
		0)

#define GTUNMAP(segp)				\
	hat_unload(				\
		segp,				\
		(segp)->s_base,			\
		(segp)->s_size);


/*
 * Handle a fault on a segment.  The only tricky part is that only one
 * valid mapping at a time is allowed.  Whenever a new mapping is called
 * for, the current values of the state set 0 registers in the old context
 * are saved away, and the old values in the new context are restored.
 */
/*ARGSUSED*/
static faultcode_t
seggt_fault(seg, addr, len, type, rw)
	struct seg *seg;
	addr_t	addr;
	u_int	len;
	enum	fault_type type;
	enum	seg_rw	   rw;
{
	struct seggt_data *old;
	struct seggt_data *current = (struct seggt_data *)seg->s_data;
	struct gt_softc   *softc   = &gt_softc[minor(current->dev)];
	addr_t	adr;
	int	pf, s;

#ifdef GT_DEBUG
	gtstat.gs_seggt_fault++;
#endif GT_DEBUG

	if (type != F_INVAL)
		return (FC_MAKE_ERR(EFAULT));

	/*
	 * VCR access?
	 */
	if (current->flag & SEG_VCR) {
		struct at_proc *ap;
		caddr_t wchan = (caddr_t) &softc->wchan[WCHAN_VCR];

		ap = gta_findproc(softc);

		if (ap == (struct at_proc *)NULL)
			return (FC_MAKE_ERR(EFAULT));

		gt_clear_flags(&ap->ap_flags, GTA_VIRGIN);

		while ((softc->at_curr != ap) && gt_validthread(softc, ap))
			(void) sleep(wchan, PZERO-1);

		if (!gt_validthread(softc, ap))
			return (FC_MAKE_ERR(EFAULT));

		/*
		 * Either this process has the accelerator or no
		 * one does.  Record and grant the translation.
		 */
		if (current->flag & SEG_UVCR)
			softc->uvcrseg = seg;
		else
			softc->svcrseg = seg;

#ifndef MULTIPROCESSOR
		s = splvm();
#else
		s = splvm();
		xc_attention();
#endif MULTIPROCESSOR

	} else {

#ifndef MULTIPROCESSOR
		s = splvm();
#else
		s = splvm();
		xc_attention();
#endif MULTIPROCESSOR

		/*
		 * Need to switch contexts?
		 */
		if (current->cntxtp && gt_curcntxt != seg) {

			if (gt_curcntxt != NULL) {
				old = (struct seggt_data *)gt_curcntxt->s_data;

				/*
				 * wipe out old valid mapping.
				 *
				 * TODO: Figure out the PF busy is cleared &
				 *       if rp_stalled then put the current
				 * 	 process to sleep with timeout
				 */

				/*
				 * Check for PF not busy
				 */
				if (gt_check_pf_busy(softc)) {
#ifdef MULTIPROCESSOR
					xc_dismissed();
#endif MULTIPROCESSOR
					(void) splx(s);
					return (FC_HWERR);
				}

				hat_unload(gt_curcntxt, gt_curcntxt->s_base,
				   gt_curcntxt->s_size);

				/*
				 * switch hardware contexts
				 *
				 * If the old and new contexts are shared
				 * then no need to save and restore contexts
				 */
				if (!(current->flag == SEG_SHARED &&
				    old->flag == SEG_SHARED)) {

					gt_cntxsave(softc, old->cntxtp);
					gt_cntxrestore(softc, current->cntxtp);
				}
			} else {
				if (current->flag & SEG_PRIVATE) {
					/*
					 * Check for PF not busy
					 */
					if (gt_check_pf_busy(softc)) {
#ifdef MULTIPROCESSOR
						xc_dismissed();
#endif MULTIPROCESSOR
						(void) splx(s);
						return (FC_HWERR);
					}
					gt_cntxrestore(softc, current->cntxtp);
				}
			}

			/*
			 * switch software context
			 */
			gt_curcntxt = seg;
		}
	}


	for (adr=addr; adr < addr+len; adr += PAGESIZE) {
		pf = gtmmap(current->dev,
		    (off_t)current->offset+(adr-seg->s_base),
		    PROT_READ|PROT_WRITE);

		if (pf == -1) {
#ifdef MULTIPROCESSOR
			xc_dismissed();
#endif MULTIPROCESSOR
			(void) splx(s);
			return (FC_MAKE_ERR(EFAULT));
		}
		if (softc->gattr->sattr.emu_type == FBTYPE_SUN2BW) {
			if (softc->gattr->sattr.flags & FB_ATTR_AUTOINIT) {
				if (gt_setup_bwtwo(softc)) {
#ifdef MULTIPROCESSOR
					xc_dismissed();
#endif MULTIPROCESSOR
					(void) splx(s);
                               		 return (FC_MAKE_ERR(EFAULT));
				}
				softc->gattr->sattr.flags &=
			 	    ~FB_ATTR_AUTOINIT;
			}
                }

		hat_devload(seg, adr, pf, PROT_READ|PROT_WRITE|PROT_USER, 0);
	}

#ifdef MULTIPROCESSOR
	xc_dismissed();
#endif MULTIPROCESSOR
	(void) splx(s);

	return (0);
}


/*ARGSUSED*/
static
seggt_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t	addr;
	u_int	len;
	u_int	prot;
{
	return (PROT_READ|PROT_WRITE);
}


/*
 * segdev pages are always "in core"
 */
/*ARGSUSED*/
static
seggt_incore(seg, addr, len, vec)
	struct seg *seg;
	addr_t addr;
	u_int len;
	char *vec;
{
	u_int v = 0;

	for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE,
	    v += PAGESIZE)

	*vec++ = 1;

	return (v);
}



static
seggt_badop()
{
	return (0);		/* silently fail	*/
}


/*
 * Create a GT segment
 */
static int
seggtx_create(seg, argsp)
	struct seg *seg;
	caddr_t     argsp;
{
	struct segvn_data   *svd;
	struct segvn_crargs *a = (struct segvn_crargs *)argsp;

	svd = (struct segvn_data *) new_kmem_zalloc(sizeof (*svd), KMEM_SLEEP);

	seg->s_ops  = &seggtx_ops;
	seg->s_data = (char *)svd;

	svd->offset  = a->offset;
	svd->cred    = a->cred;
	svd->type    = a->type;
	svd->maxprot = a->maxprot;
	svd->prot    = a->prot;

	return (0);
}


static int
seggtx_dup(seg, newseg)
	struct seg *seg;
	struct seg *newseg;
{
	struct segvn_data *svd;
	struct segvn_data *ndp;
	struct as	  *as;

	as = seg->s_as;
	gt_aslock(as);

	svd = (struct segvn_data *) seg->s_data;
	ndp = (struct segvn_data *) new_kmem_zalloc(sizeof (*ndp), KMEM_SLEEP);

	newseg->s_ops  = &seggtx_ops;
	newseg->s_data = (char *)ndp;

	ndp->offset  = svd->offset;
	ndp->cred    = svd->cred;
	ndp->type    = svd->type;
	ndp->maxprot = svd->maxprot;
	ndp->prot    = svd->prot;

	gt_asunlock(as);

	return (0);
}


static int
seggtx_unmap(seg, addr, len)
	struct seg *seg;
	addr_t      addr;
	u_int       len;
{
	struct segvn_data *svd;
	struct segvn_data *nsvd;
	struct seg *nseg;
	struct as  *as;
	addr_t      nbase;
	u_int       nsize;

	if (seg == (struct seg *)NULL)
		return (-1);

	/*
	 * Check for bad sizes
	 */
	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
	    (len & PAGEOFFSET) || ((u_int)addr & PAGEOFFSET))
		panic("seggtx_unmap");

	as = seg->s_as;

	gt_aslock(as);
	svd = (struct segvn_data *)seg->s_data;

	/*
	 * We never load translations for these segments, so there are
	 * none to unload here.
	 *
	 * Check for entire segment.
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		gt_asunlock(as);
		return (0);
	}

	/*
	 * Check for beginning of segment
	 */
	if (addr == seg->s_base) {
		seg->s_base += len;
		seg->s_size -= len;
		svd->offset += len;
		gt_asunlock(as);
		return (0);
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		seg->s_size -= len;
		gt_asunlock(as);
		return (0);
	}

	/*
	 * The section to go is in the middle of the segment, so we've
	 * got to split the segment into two.  nseg is made for the
	 * high end and seg is cut down at the low end.
	 */
	nbase = addr + len;				/* new seg base   */
	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size   */
	seg->s_size = addr - seg->s_base;		/* shrink old seg */
	nseg = seg_alloc(seg->s_as, nbase, nsize);

	if (nseg == NULL)
		panic("seggtx_unmap seg_alloc");

	nsvd = (struct segvn_data *) new_kmem_zalloc(sizeof (*nsvd),
	    KMEM_SLEEP);

	nseg->s_ops  = seg->s_ops;
	nseg->s_data = (char *)nsvd;

	nsvd->offset  = svd->offset + nseg->s_base - seg->s_base;
	nsvd->cred    = svd->cred;
	nsvd->type    = svd->type;
	nsvd->maxprot = svd->maxprot;
	nsvd->prot    = svd->prot;

	gt_asunlock(as);

	return (0);
}


static int
seggtx_free(seg)
	struct seg *seg;
{
	struct at_proc    *ap;
	struct as	  *as;
	struct segvn_data *svd;

	as = seg->s_as;
	gt_aslock(as);

	svd = (struct segvn_data *)seg->s_data;

	ap = gta_findas(as);
	if (ap != (struct at_proc *)NULL) {
		if (ap->ap_proc->p_flag & SWEXIT)
			gta_exit(ap);
	}

	kmem_free((caddr_t)svd, sizeof (*svd));

	gt_asunlock(as);

	return (0);
}


/*
 * Faults in the seggtx segment are resolved by converting the page
 * containing the faulting address to a seggta_segment and resolving
 * the fault there.
 */
static faultcode_t
seggtx_fault(seg, addr, len, type, rw)
	struct seg	*seg;
	addr_t		 addr;
	u_int		 len;
	enum fault_type	 type;
	enum seg_rw	 rw;
{
	struct segvn_data *svd;
	struct segvn_crargs crargs_gta;
	struct as  *as;
	struct seg *nseg;
	addr_t      nbase;
	u_int       nsize;
	int         rc;

#ifdef GT_DEBUG
	gtstat.gs_seggtx_fault++;
#endif GT_DEBUG

	/*
	 * If there is an in-progress fault on a GT portion of the
	 * same address space, block this one until that one completes.
	 */
	as = seg->s_as;
	gt_aslock(as);

	if (gt_faultcheck(as) == -1) {
		gt_asunlock(as);
		return (FC_MAKE_ERR(EFAULT));
	}

	svd = (struct segvn_data *)seg->s_data;

	/*
	 * Make a copy of the segment private data and then unmap
	 * the page containing the faulting address
	 */
	nbase = (addr_t)((u_int)addr & PAGEMASK);
	nsize = (len + PAGESIZE-1) & PAGEMASK;


	crargs_gta.vp      = (struct vnode *)NULL;
	crargs_gta.offset  = svd->offset + nbase - seg->s_base;
	crargs_gta.cred    = svd->cred;
	crargs_gta.type    = svd->type;
	crargs_gta.maxprot = svd->maxprot;
	crargs_gta.prot    = svd->prot;
	crargs_gta.amp     = (struct anon_map *)NULL;

	(void) seggtx_unmap(seg, nbase, nsize);

	rc = as_map(as, nbase, nsize, seggta_create, (caddr_t)&crargs_gta);

	if (rc)
		return (FC_MAKE_ERR(EFAULT));

	nseg = as_segat(as, addr);

	if (nseg == 0)
		return (FC_MAKE_ERR(EFAULT));

	rc = (seggta_ops.fault)(nseg, addr, len, type, rw);

	/*
	 * Wakeup anyone blocked for this fault to complete
	 */
	gt_asunlock(as);

	return (rc);
}


static
gt_faultcheck(as)
	struct as       *as;
{
	struct at_proc *ap;

	if (u.u_procp->p_pid == gt_pagedaemonpid) {

		ap = gta_findas(as);
		if (ap != (struct at_proc *)NULL) {

			if (ap->ap_proc->p_flag & SWEXIT) {

				gta_exit(ap);
				return (-1);
			}
		}
	}

	return (0);
}


/*
 * When a translation is unloaded, we see whether the process
 * is exiting.  I wish there were another way to stop GT from
 * using resources of an exiting process.
 */
/*ARGSUSED*/
static int
seggtx_hatsync(seg, addr, ref, mod, flags)
	struct seg	*seg;
	addr_t		 addr;
	u_int		 ref;
	u_int		 mod;
	u_int		 flags;
{
	struct at_proc *ap;

	if (u.u_procp->p_flag & SWEXIT) {
		ap = gta_findas(seg->s_as);
		if (ap != (struct at_proc *)NULL)
			gta_exit(ap);
	}

	return (0);
}


static int
seggtx_badop()
{
	printf("seggtx_badop\n");
	return (0);			/* silently fail	*/
}


/*ARGSUSED*/
static int
seggtx_setprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t      addr;
	u_int       len;
	u_int       prot;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;

	if ((svd->maxprot & prot) != prot)
		return (-1);

	svd->prot = prot;

	return (0);
}


/*ARGSUSED*/
static int
seggtx_checkprot (seg, addr, len, prot)
	struct seg *seg;
	addr_t      addr;
	u_int       len;
	u_int       prot;
{
	struct segvn_data *svd = (struct segvn_data *)seg->s_data;

	return (((svd->prot & prot) != prot) ? -1 : 0);
}


/*ARGSUSED*/
static int
seggtx_incore(seg, addr, len, vec)
{
	return (0);
}


static int
seggta_create(seg, argsp)
	struct seg *seg;
	caddr_t     argsp;
{
	struct seg *pseg;
	struct seg *nseg;
	struct as  *as;
	addr_t      pbase = 0;
	addr_t      nbase = 0;
	int         rc;

	/*
	 * Check the previous and next segments.  If they are seggta
	 * segments, temporarily convert them to segvn segs to allow
	 * segvn_create the opportunity to coalesce them.  When done,
	 * convert them back to seggta segments if they still exist.
	 */
	as = seg->s_as;
	gt_aslock(as);

	pseg = seg->s_prev;
	nseg = seg->s_next;

	if ((pseg != seg) && (pseg->s_ops == &seggta_ops)) {
		pbase = pseg->s_base;
		pseg->s_ops = &segvn_ops;
	}

	if ((nseg != seg) && (nseg->s_ops == &seggta_ops)) {
		nbase = nseg->s_base;
		nseg->s_ops = &segvn_ops;
	}

	rc = segvn_create(seg, argsp);

	if (rc == 0)
		seg->s_ops = &seggta_ops;

	/*
	 * Revert the previous/next segs, if necessary.
	 */
	pseg = seg->s_prev;
	nseg = seg->s_next;

	if ((pseg != seg) && (pseg->s_base == pbase))
		pseg->s_ops = &seggta_ops;
	
	if ((nseg != seg) && (nseg->s_base == nbase))
		nseg->s_ops = &seggta_ops;
	
	gt_asunlock(as);

	return (rc);
}


static int
seggta_dup(seg, newsegp)
	struct seg *seg, *newsegp;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.dup)(seg, newsegp);
	newsegp->s_ops = &seggta_ops;

	gt_asunlock(as);
	return (rc);
}


static int
seggta_unmap(seg, addr, len)
	struct seg *seg;
	addr_t	    addr;
	u_int	    len;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	gta_unload(as, addr, len);
	rc = (segvn_ops.unmap)(seg, addr, len);

	gt_asunlock(as);
	return (rc);
}


static int
seggta_free(seg)
	struct seg *seg;
{
	struct at_proc	*ap;
	struct as	*as;
	int rc;

	/*
	 * If the process is exiting, shut down the corresponding
	 * GT accelerator thread.  Otherwise, release any mappings
	 * to addresses in the segment being freed.
	 */
	as = seg->s_as;
	gt_aslock(as);

	ap = gta_findas(as);

	if (ap != (struct at_proc *)NULL) {
		if (ap->ap_proc->p_flag & SWEXIT) {
			gta_exit(ap);
		} else {
			gta_unload(as, seg->s_base, seg->s_size);
		}
	}

	rc = (segvn_ops.free)(seg);

	gt_asunlock(as);

	return (rc);
}


static faultcode_t
seggta_fault(seg, addr, len, type, rw)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	enum fault_type	type;
	enum seg_rw	rw;
{
	struct as *as;
	int rc;

#ifdef GT_DEBUG
	gtstat.gs_seggta_fault++;
#endif GT_DEBUG
 
	/*
	 * If there is an in-progress fault on a GT portion of the
	 * same address space, block this one until that one completes.
	 */
	as = seg->s_as;
	gt_aslock(as);

	if (gt_faultcheck(as) == -1) {
		gt_asunlock(as);
		return (FC_MAKE_ERR(EFAULT));
	}

	rc = (segvn_ops.fault)(seg, addr, len, type, rw);

	/*
	 * Wakeup anyone blocked for this fault to complete
	 */
	gt_asunlock(as);

	return (rc);
}


static faultcode_t
seggta_faulta(seg, addr)
	struct seg	*seg;
	addr_t		addr;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	if (gt_faultcheck(as) == -1) {
		gt_asunlock(as);
		return (FC_MAKE_ERR(EFAULT));
	}

	rc = (segvn_ops.faulta)(seg, addr);

	gt_asunlock(as);
	return (rc);
}


static int
seggta_hatsync(seg, addr, ref, mod, flags)
	struct seg	*seg;
	addr_t		 addr;
	u_int		 ref;
	u_int		 mod;
	u_int		 flags;
{
	struct at_proc	*ap;
	struct as	*as;
	int rc;

	if (u.u_procp->p_flag & SWEXIT) {
		as = seg->s_as;
		gt_aslock(as);

		ap = gta_findas(as);
		if (ap != (struct at_proc *)NULL)
			gta_exit(ap);

		gt_asunlock(as);
	}

	rc = (segvn_ops.hatsync)(seg, addr, ref, mod, flags);

	return (rc);
}


static int
seggta_setprot(seg, addr, len, prot)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	u_int		prot;
{
	int rc;

	rc = (segvn_ops.setprot)(seg, addr, len, prot);
	return (rc);
}


static int
seggta_checkprot(seg, addr, len, prot)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	u_int		prot;
{
	int rc;

	rc = (segvn_ops.checkprot)(seg, addr, len, prot);
	return (rc);
}


static int
seggta_kluster(seg, addr, delta)
	struct seg	*seg;
	addr_t		addr;
	int		delta;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.kluster)(seg, addr, delta);

	gt_asunlock(as);

	return (rc);
}


static u_int
seggta_swapout(seg)
	struct seg *seg;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.swapout)(seg);

	gt_asunlock(as);

	return (rc);
}


static int
seggta_sync(seg, addr, len, flags)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	u_int		flags;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.sync)(seg, addr, len, flags);

	gt_asunlock(as);

	return (rc);
}


static int
seggta_incore(seg, addr, len, vec)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	char		*vec;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.incore)(seg, addr, len, vec);

	gt_asunlock(as);

	return (rc);
}


static int
seggta_lockop(seg, addr, len, op)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	int		op;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.lockop)(seg, addr, len, op);

	gt_asunlock(as);

	return (rc);
}


static int
seggta_advise(seg, addr, len, behav)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
	int		behav;
{
	struct as *as;
	int rc;

	as = seg->s_as;
	gt_aslock(as);

	rc = (segvn_ops.advise)(seg, addr, len, behav);

	gt_asunlock(as);

	return (rc);
}


/*
 * Lock the address space against simultaneous GT/host faults
 */
static void
gt_aslock(as)
	struct as *as;
{
	struct gt_softc *softc;
	struct at_proc  *ap;
	unsigned	mylock, otherlock;
	caddr_t		wchan;


	if ((ap=gta_findas(as)) == (struct at_proc *)NULL)
		return;

	softc = ap->ap_softc;
	wchan = &softc->wchan[WCHAN_ASLOCK];

	if (u.u_procp->p_pid == gt_pagedaemonpid) {
		mylock    = GTA_GTLOCK;
		otherlock = GTA_HOSTLOCK;
	} else {
		mylock    = GTA_HOSTLOCK;
		otherlock = GTA_GTLOCK;
	}

	/*
	 * Block until there is no (other) GT as lock on this address space.
	 */
	while (ap->ap_flags & otherlock) {

		(void) sleep(wchan, PZERO-1);

		ap = gta_findas(as);
		if (ap == (struct at_proc *)NULL) {
			wakeup(&softc->wchan[WCHAN_ASLOCK]);
			return;
		}
	}

	ap->ap_lockcount++;
	gt_set_flags(&ap->ap_flags, mylock);
}


/*
 * Remove the GT aslock if it is present, and wake any blocked processes.
 */
static void
gt_asunlock(as)
	struct as *as;
{
	struct at_proc *ap;
	unsigned	mylock;

	if (u.u_procp->p_pid == gt_pagedaemonpid)
		mylock = GTA_GTLOCK; 
	else
		mylock = GTA_HOSTLOCK; 

	ap = gta_findas(as);
	if (ap != (struct at_proc *)NULL) {
		if (--ap->ap_lockcount <= 0) {
			gt_clear_flags(&ap->ap_flags, mylock);
			wakeup(&ap->ap_softc->wchan[WCHAN_ASLOCK]);
		}
	}
}



/*
 * Unload all the GT mappings in the range [addr..addr+len).
 *
 * addr and len must be GT_PAGESIZE aligned.
 */
static void
gta_unload(as, addr, len)
	struct as *as;
	addr_t	   addr;
	u_int	   len;
{
	struct gta_mmudata mmudata;
	struct gt_lvl2  **pp2;
	struct gt_lvl2	 *p2;
	struct gt_ptp	 *ptp;
	struct gt_pte	 *pte = (struct gt_pte *)NULL;
	struct gt_pte	 *ptelast;
	struct at_proc	 *ap;
	addr_t	a, lastaddr;
	int	index2;
	struct gta_lockmap *map;

	ap = gta_findas(as);

	/*
	 * If the user has already issued a gt_disconnect, there
	 * won't be an at_proc struct for this address space.
	 */
	if (ap == (struct at_proc *) NULL)
		return;

	addr = (addr_t) ((u_int)addr & PAGEMASK);
	lastaddr = addr + len;

	(void) gta_getmmudata(ap, addr, &mmudata);

	/*
	 * pp2:  &(level 2 pointer in the level 1 table)
	 * ptp:  &(page table pointer in the level 1 table)
	 */
	pp2	= &((mmudata.gtu_lvl1)->gt_lvl2[mmudata.gtu_index1]);
	ptp	= &((mmudata.gtu_lvl1)->gt_ptp[mmudata.gtu_index1]);
	index2	= mmudata.gtu_index2;

	/*
	 * Before invalidating the GT PTEs, freeze the ATU and clear
	 * its TLB.  After invalidating the PTEs, thaw the ATU.
	 *
	 * XXX: synchronize the ref/mod bits
	 */
	gt_freeze_atu(ap->ap_softc, F_FREEZE_UNLOAD);
	gt_set_flags(&ap->ap_softc->fe_ctrl_sp_ptr->fe_atu_syncr, ATU_CLEAR);

	for (a=addr; a<lastaddr; a+=GT_PAGESIZE) {

		/*
		 * If the level 2 pointer is null, bump the address to
		 * avoid processing all the GT PTEs in the address range.
		 */
		if ((p2 = *pp2) == (struct gt_lvl2 *)NULL) {
			a = (addr_t) ((u_int)a & ~(GT_LVL2SIZE - 1)) +
			    GT_LVL2SIZE - GT_PAGESIZE;

			pte    = (struct gt_pte *)NULL;
			index2 = 0;
			pp2++;
			ptp++;

			continue;
		}

		/*
		 * If the PTE pointer is null, initialize it and
		 * the ptelast pointers to address the level 2 table
		 */
		if (pte == (struct gt_pte *)NULL) {
			pte	= &p2->gt_pte[index2];
			ptelast	= &p2->gt_pte[GT_ENTRIES_PER_LVL2];
		}

		/*
		 * If this is a valid PTE, invalidate it and free
		 * the GT lock map entry.
		 */
		if (pte->pte_v == 1) {
			pte->pte_v = 0;

			map = gta_hash_find(a, as);
			if (map != (struct gta_lockmap *) NULL)
				gt_freemap(map);

			ptp->ptp_cnt--;
			if (ptp->ptp_cnt == 0) {
				ptp->ptp_v    = 0;
				ptp->ptp_base = 0;

				freepages((caddr_t) p2->gt_pte,
				    btopr(sizeof (struct gt_lvl2)));

				*pp2  = (struct gt_lvl2 *)NULL;

				a = (addr_t) ((u_int)a & ~(GT_LVL2SIZE-1)) +
				    GT_LVL2SIZE - GT_PAGESIZE;
 
				pte    = (struct gt_pte *)NULL;
				index2 = 0;
				pp2++;    
				ptp++;
 
				continue;
			}
		}

		/*
		 * If this is the last page in a level 2 table, clear
		 * the pme pointer to cause it to be recalculated.
		 */
		if (++pte == ptelast) {
			pte    = (struct gt_pte *)NULL;
			index2 = 0;
			pp2++;
			ptp++;
		}
	}

	gt_thaw_atu(ap->ap_softc, F_FREEZE_UNLOAD);
}


/*
 * Save the state set 0 registers.
 */
static void
gt_cntxsave(softc, save)
	struct gt_softc *softc;
	struct gt_cntxt *save;
{
	struct rp_host	  *rp;
	struct fbi_reg	  *fb;
	struct fe_i860_sp *fe;

#ifdef GT_DEBUG
	gtstat.gs_cntxsave++;
#endif GT_DEBUG

	rp = (struct rp_host *)softc->va[MAP_RP_HOST_CTRL];
	fb = (struct fbi_reg *)softc->va[MAP_FBI_REG];
	fe = (struct fe_i860_sp *)softc->va[MAP_FE_I860];

#ifdef	MULTIPROCESSOR
	/*
	 * Idle all other processors during this critical section.
	 * In most cases, xc_attention() is already held by the caller
	 * at this point. If so, this call to xc_attention() is very
	 * lightweight, and only increments the nesting count.
	 */
	xc_attention();
#endif	MULTIPROCESSOR

	/*
	 * We need to preserve the notion of host-chokes-frontend
	 * before touching the PF.
	 */
	if (fe->fe_hold & FE_HOLD) {
		while (!(fe->fe_hold & FE_HLDA))
			DELAY(2);
	}
	save->gc_fe_hold	= fe->fe_hold & FE_HOLD;

	/*
	 * First check if current host FB state set is 1. If so then
	 * save it and change the host FB state to 0.
	 */
	save->gc_rp_host_csr	= rp->rp_host_csr_reg;

	if (rp->rp_host_csr_reg & 0x1)
		rp->rp_host_csr_reg &= 0xfffffffe;

	save->gc_rp_host_as	= rp->rp_host_as_reg;

	save->gc_fbi_vwclp_x	= fb->fbi_reg_vwclp_x;
	save->gc_fbi_vwclp_y	= fb->fbi_reg_vwclp_y;
	save->gc_fbi_fb_width	= fb->fbi_reg_fb_width;
	save->gc_fbi_buf_sel	= fb->fbi_reg_buf_sel;
	save->gc_fbi_stereo_cl	= fb->fbi_reg_stereo_cl;
	save->gc_fbi_cur_wid	= fb->fbi_reg_cur_wid;
	save->gc_fbi_wid_ctrl	= fb->fbi_reg_wid_ctrl;
	save->gc_fbi_wbc	= fb->fbi_reg_wbc;
	save->gc_fbi_con_z	= fb->fbi_reg_con_z;
	save->gc_fbi_z_ctrl	= fb->fbi_reg_z_ctrl;
	save->gc_fbi_i_wmask	= fb->fbi_reg_i_wmask;
	save->gc_fbi_w_wmask	= fb->fbi_reg_w_wmask;
	save->gc_fbi_b_mode	= fb->fbi_reg_b_mode;
	save->gc_fbi_b_wmask	= fb->fbi_reg_b_wmask;
	save->gc_fbi_rop	= fb->fbi_reg_rop;
	save->gc_fbi_mpg	= fb->fbi_reg_mpg;
	save->gc_fbi_s_mask	= fb->fbi_reg_s_mask;
	save->gc_fbi_fg_col	= fb->fbi_reg_fg_col;
	save->gc_fbi_bg_col	= fb->fbi_reg_bg_col;
	save->gc_fbi_s_trans	= fb->fbi_reg_s_trans;
	save->gc_fbi_dir_size	= fb->fbi_reg_dir_size;
	save->gc_fbi_copy_src	= fb->fbi_reg_copy_src;

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}


/*
 * Restore the state set 0 registers.
 */
static void
gt_cntxrestore(softc, save)
	struct gt_softc *softc;
	struct gt_cntxt *save;
{
	struct rp_host	  *rp;
	struct fbi_reg	  *fb;
	struct fe_i860_sp *fe;
	unsigned	  dummy;

#ifdef GT_DEBUG
	gtstat.gs_cntxrestore++;
#endif GT_DEBUG

	rp = (struct rp_host *)   softc->va[MAP_RP_HOST_CTRL];
	fb = (struct fbi_reg *)   softc->va[MAP_FBI_REG];
	fe = (struct fe_i860_sp *)softc->va[MAP_FE_I860];

#ifdef	MULTIPROCESSOR
	/*
	 * Idle all other processors during this critical section.
	 * In most cases, xc_attention() is already held by the caller
	 * at this point. If so, this call to xc_attention() is very
	 * lightweight, and only increments the nesting count.
	 */
	xc_attention();
#endif	MULTIPROCESSOR

	/*
	 * If the new context had placed a hold on the i860 frontend,
	 * we need to restore that hold and block until it is
	 * acknowledged.
	 */
	fe->fe_hold |= save->gc_fe_hold;

	if (fe->fe_hold & FE_HOLD) {
		while (!(fe->fe_hold & FE_HLDA))
			DELAY(2);
	}

	/*
	 * After restoring the RP HOST CSR, force the state set
	 * to 0 before restoring the remaining registers.  After
	 * restoring these, we re-restore the RP HOST CSR to set
	 * the correct state set.
	 */
	rp->rp_host_csr_reg	= save->gc_rp_host_csr & 0xfffffffe;

	/*
	 * To workaround a bug in the PBM, we do a dummy read of
	 * the PP Image Write Mask register.  If we just restored
	 * the Host Stalling RP bit in the RP CSR, the dummy read
	 * will block until the stall completes.
	 */
	dummy = fb->fbi_reg_i_wmask;
	dummy = dummy+1;		/* makes lint happier with dummy */

	rp->rp_host_as_reg	= save->gc_rp_host_as;

	fb->fbi_reg_vwclp_x	= save->gc_fbi_vwclp_x;
	fb->fbi_reg_vwclp_y	= save->gc_fbi_vwclp_y;
	fb->fbi_reg_fb_width	= save->gc_fbi_fb_width;
	fb->fbi_reg_buf_sel	= save->gc_fbi_buf_sel;
	fb->fbi_reg_stereo_cl	= save->gc_fbi_stereo_cl;
	fb->fbi_reg_cur_wid	= save->gc_fbi_cur_wid;
	fb->fbi_reg_wid_ctrl	= save->gc_fbi_wid_ctrl;
	fb->fbi_reg_wbc		= save->gc_fbi_wbc;
	fb->fbi_reg_con_z	= save->gc_fbi_con_z;
	fb->fbi_reg_z_ctrl	= save->gc_fbi_z_ctrl;
	fb->fbi_reg_i_wmask	= save->gc_fbi_i_wmask;
	fb->fbi_reg_w_wmask	= save->gc_fbi_w_wmask;
	fb->fbi_reg_b_mode	= save->gc_fbi_b_mode;
	fb->fbi_reg_b_wmask	= save->gc_fbi_b_wmask;
	fb->fbi_reg_rop		= save->gc_fbi_rop;
	fb->fbi_reg_mpg		= save->gc_fbi_mpg;
	fb->fbi_reg_s_mask	= save->gc_fbi_s_mask;
	fb->fbi_reg_fg_col	= save->gc_fbi_fg_col;
	fb->fbi_reg_bg_col	= save->gc_fbi_bg_col;
	fb->fbi_reg_s_trans	= save->gc_fbi_s_trans;
	fb->fbi_reg_dir_size	= save->gc_fbi_dir_size;
	fb->fbi_reg_copy_src	= save->gc_fbi_copy_src;
	rp->rp_host_csr_reg     = save->gc_rp_host_csr;

#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}


static struct at_proc *
gta_findproc(softc)
	struct gt_softc *softc;
{
	struct proc    *p;
	struct at_proc *ap, *ap_head;

	p = u.u_procp;

	ap_head = softc->at_head;
	ap = GT_LIST_NEXT(ap_head, at_proc, ap_list);

	while (ap != ap_head) {

		if ((ap->ap_proc == p) && (ap->ap_pid == p->p_pid))
			return (ap);

		ap = GT_LIST_NEXT(ap, at_proc, ap_list);
	}

	return ((struct at_proc *) NULL);
}


static struct at_proc *
gta_findas(as)
	struct as *as;
{
	struct gt_softc *softc;
	struct at_proc  *ap, *ap_head;

	for (softc=gt_softc; softc < &gt_softc[NGT]; softc++) {

		ap_head = softc->at_head;
		ap = GT_LIST_NEXT(ap_head, at_proc, ap_list);

		while (ap != ap_head) {

			if ((ap->ap_as == as) &&
			    (ap->ap_proc == pfind(ap->ap_pid)))
				return (ap);

			ap = GT_LIST_NEXT(ap, at_proc, ap_list);
		}
	}

	return ((struct at_proc *) NULL);
}


static
gt_validthread(softc, thread)
	struct gt_softc *softc;
	struct at_proc  *thread;
{
	struct at_proc *ap, *ap_head;

	ap_head = softc->at_head;
	ap = GT_LIST_NEXT(ap_head, at_proc, ap_list);

	while (ap != ap_head) {

		if ((ap == thread) && (ap->ap_proc == pfind(ap->ap_pid)))
			return (1);

		ap = GT_LIST_NEXT(ap, at_proc, ap_list);
	}

	return (0);
}


/*
 * The GT pagein daemon, which runs as a kernel process.
 */
static void
gt_pagedaemon()
{
	struct gt_fault	*gfp = &gt_fault;
	struct gt_softc	*softc;
	fe_ctrl_sp	*fep;
	caddr_t		 wchan;

	gt_daemon_init("gtdaemon");
	gt_pagedaemonpid = u.u_procp->p_pid;

	wchan = (caddr_t) &gt_daemonflag;

	(void) splvm();
	(void) splr(gt_priority);

	/*
	 * Sleep until woken to resolve a page fault or to allocate
	 * a new level one page table.  We do the page table allocation
	 * in this thread so that getpages() can sleep, if necessary,
	 * and return when the page has been rmalloc'ed.
	 *
	 * Until the request is satisfied the GT MMU is kept frozen.
	 */
	for (;;) {
		while (gfp->gf_type == 0)
			(void) sleep(wchan, PZERO+1);

		softc = gfp->gf_softc;
		fep   = softc->fe_ctrl_sp_ptr;

		if (gfp->gf_type & GF_LVL1_FAULT) {
			if (gt_badfault())
				gta_abortfault(gfp->gf_at, GT_ABORT_LVL1);
			else
				gt_lvl1fault();
		}

		if (gfp->gf_type & GF_LVL2_FAULT) {
			if (gt_badfault()) {
				gta_abortfault(gfp->gf_at, GT_ABORT_TLB);
			} else {
				gt_pagefault();
			}
		}

		if (gfp->gf_type & GF_SENDSIGNAL) {
			struct at_proc *ap_head, *ap;

			ap_head = softc->at_head;
			ap = GT_LIST_NEXT(ap_head, at_proc, ap_list);

			while (ap != ap_head) {
				if (ap->ap_flags & GTA_SIGNAL) {
					if (!gt_badfault())
						psignal(ap->ap_proc, SIGXCPU);
					ap->ap_flags &= ~GTA_SIGNAL;
				}

				ap = GT_LIST_NEXT(ap, at_proc, ap_list);
			}
		}

		/*
		 * Unfreeze the ATU and clear the interrupt condition
		 * XXX: Do we really need to clear the TLB?
		 */
		bzero((caddr_t)gfp, sizeof (*gfp));
		softc->flags &= ~(F_ATU_FAULTING | F_LVL1_MISS);

		fep->fe_atu_syncr |=  ATU_CLEAR;
		gt_sync = fep->fe_atu_syncr;
		gt_thaw_atu(softc,
		    (F_FREEZE_TLB | F_FREEZE_LVL1 | F_FREEZE_SIG));
	}
}


static
gt_badfault()
{
	struct proc     *p;
	struct at_proc  *ap;
	struct gt_fault *gfp = &gt_fault;

	if ((ap=gfp->gf_at) == (struct at_proc *)NULL)
		return (1);

	p  = ap->ap_proc;

	if ((gfp->gf_type & GF_BOGUS) ||
	    (p != pfind(gfp->gf_pid)) ||
	    (p->p_flag & SWEXIT)      ||
	    (ap->ap_flags & GTA_EXIT))
		return (1);
	else
		return (0);
}


static void
gt_lvl1fault()
{
	struct gt_fault *gfp = &gt_fault;
	struct gt_lvl2  *lvl2;

	/*
	 * Allocate a lvl 2 page table.  Make the page table uncached,
	 * and construct its entry in the level 1 table.
	 */
	lvl2 = (struct gt_lvl2 *)
	    getpages(btopr(sizeof (struct gt_lvl2)), 0);

	gt_setnocache((addr_t)lvl2);
	bzero((caddr_t)lvl2, sizeof (struct gt_lvl2));
 
	gfp->gf_lvl1->gt_lvl2[gfp->gf_index] = lvl2;
 
	gfp->gf_ptp->ptp_base  = GT_KVTOP((int)lvl2) >> GT_PAGESHIFT;
	gfp->gf_ptp->ptp_v     = 1;
}


static void
gt_pagefault()
{
	struct gt_fault	   *gfp = &gt_fault;
	struct at_proc	   *ap;
	struct as	   *as;
	struct seg	   *seg, *sega;
	struct segvn_data  *svd;
	struct page	   *pp;
	struct anon	  **app;
	struct gta_lockmap *map;
#ifndef sun4m
	struct ctx	   *octx;
#else
	u_int		    octx;
#endif sun4m
	addr_t		     a_addr, x_addr;
	faultcode_t	     fault_err;
	int		     i, page, err;
	u_int		     off, dummy, lookahead, pagecnt, faultsize;
	u_int		     lvl1, lvla;
	int		     prefault;
	int		     s;
	extern struct modinfo mod_info[];

	ap = gfp->gf_at;
	as = ap->ap_as;

	gt_aslock(as);

	octx = mmu_getctx();
	hat_setup(as);

	a_addr = (addr_t) ((int)(gfp->gf_addr) & PAGEMASK);


	/*
	 * We'll consider prefaulting if the currently-faulted page
	 * is one page higher than the previously-faulted page.
	 */
	if (gta_premap) {
		if (a_addr == ap->ap_lastfault+PAGESIZE)
			prefault = 1;
		else
			prefault = 0;
	} else
		prefault = 0;


	seg = as_segat(as, a_addr);

	/*
	 * Transmogrify a vnode segment for use by the GT accelerator?
	 */
	if (seg && (seg->s_ops == &segvn_ops))
		seg->s_ops = &seggta_ops;

	/*
	 * Is this an unresolvable fault?
	 */
	if (seg == NULL ||
	    ((seg->s_ops != &seggta_ops) && (seg->s_ops != &seggtx_ops))) {
		gta_abortfault(ap, GT_ABORT_BADADDR);
		goto out;
	}


	/*
	 * Shall we prefault?  We constrain the prefault to not
	 * extend beyond the mappings of a GT MMU level 2 table.
	 */
	if (prefault) {
		lookahead = gta_premap * PAGESIZE;
		pagecnt   = gta_premap+1;
		lvl1	  = (u_int)a_addr & (INDX1_MASK<<INDX1_SHIFT);

		for (;lookahead; lookahead -= PAGESIZE, pagecnt--) {
			x_addr = a_addr + lookahead;

			sega   = as_segat(as, x_addr);
			lvla   = (u_int)x_addr & (INDX1_MASK<<INDX1_SHIFT);

			if ((sega == seg) && (lvla == lvl1))
				break;
		}
	} else
		pagecnt = 1;


	faultsize = pagecnt * PAGESIZE;
	ap->ap_lastfault = a_addr + faultsize - PAGESIZE;

	/*
	 * Let the vnode segfault handler do its thing.
	 */
	fault_err = as_fault(as, a_addr, faultsize, F_SOFTLOCK, S_OTHER);

	if (fault_err) {
		gta_abortfault(ap, GT_ABORT_FAULTERR);
		goto out;
	}

	x_addr = a_addr;

	for (i=0; i<pagecnt; i++, x_addr += PAGESIZE) {

		seg = as_segat(as, x_addr);

		/*
		 * If this page is already mapped, we just move its
		 * map element to the head of the queue.
		 *
		 * Otherwise, find the pages and make entries in the GT's
		 * page tables.  Its hard to believe that there isn't an
		 * easier way to find the pages that we just  as_faulted in.
		 */
		map = gta_hash_find(x_addr, as);
		if (map != (struct gta_lockmap *)NULL) {
			GT_LIST_REMOVE(map, gta_lockmap, gtm_list);
			GT_LIST_INSERT_NEXT(&gta_lockbusy, map,
			    gta_lockmap, gtm_list);
		} else {
			page = seg_page(seg, x_addr);
			svd  = (struct segvn_data *)seg->s_data;
			off  = svd->offset + ((x_addr-seg->s_base) & PAGEMASK);

			if (svd->amp == NULL) {
				pp = page_find(svd->vp, off);

			} else {
				AMAP_LOCK(svd->amp)
				app = &svd->amp->anon[svd->anon_index + page];

				err = anon_getpage(app, &dummy,
				    (struct page **) NULL,
				    PAGESIZE, seg, x_addr, S_WRITE, svd->cred);

				if (err) {
					printf("gt_pagedaemon:  err = 0x%X\n",
					    FC_MAKE_ERR(err));
					panic("gt_pagedaeon: anon page");
				}

				pp = (*app)->un.an_page;

				AMAP_UNLOCK(svd->amp)
			}
	
			if (pp == (struct page *)NULL)
				panic("gt_pagedaemon: cannot find page");

			(void) page_pp_lock(pp, 0, 0);


			/*
			 * If our system has a VAC, make the page uncached,
			 * but if we have a PAC, leave it cached.
			 */
			s = splvm();
#ifdef sun4c
			gt_setnocache(x_addr);
			vac_pageflush(x_addr);
#elif defined(sun4m)
			if ((mod_info[0].mod_type != CPU_VIKING) &&
			    (mod_info[0].mod_type != CPU_VIKING_MXCC)) {
				panic("gt_pagefault: sun4m/ross not supported");
			}
#endif sun4c
			(void) splx(s);

			/*
			 * Now load the translation into the GT MMU
			 */
			gt_mmuload(ap, x_addr, pp, seg, i);
		}
	}

#ifndef sun4m
	/*
	 * Make sure that the process's context isn't clean
	 */
	{
		struct as   *a;
		struct ctx  *c;

		a = ap->ap_proc->p_as;
		if (a != (struct as *)NULL) {
			c = a->a_hat.hat_ctx;
			if (c != (struct ctx *)NULL) {
				c->c_clean = 0;
			}
		}
	}
#endif sun4m

	/*
	 * If this is the first time a page has been mapped for this 
	 * thread, lock the process against swapping and start the
	 * hat callbacks.  We need the callbacks so that we can tell
	 * if the process is exiting before hitting some nasty
	 * assertion checks at hat_free time.
	 */
	if (!(ap->ap_flags & GTA_SWLOCK)) {

		gt_set_flags(&ap->ap_flags, GTA_SWLOCK);
		ap->ap_proc->p_swlocks++;

		as->a_hatcallback = 1;
	}

	(void) as_fault(as, a_addr, faultsize, F_SOFTUNLOCK, S_OTHER);

out:
	mmu_setctx(octx);
	gt_asunlock(as);
}


static void
gta_abortfault(ap, reason)
	struct at_proc  *ap;
	enum abort_types reason;
{
	struct gt_softc *softc = ap->ap_softc;
	fe_page_1	*fp1   = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	fe_page_3	*fp3   = (fe_page_3 *)softc->va[MAP_FE_HFLAG3];

	switch (reason) {

#ifdef GT_DEBUG
	case GT_ABORT_LVL1:
		printf("gt_abortfault: level 1 miss\n");
		break;

	case GT_ABORT_TLB:
		printf("gt_abortfault: tlb miss\n");
		break;

	case GT_ABORT_FAULTERR:
		printf("gt_abortfault: fault error\n");
		break;
#endif GT_DEBUG

	case GT_ABORT_BADADDR:
		printf("gt_abortfault: invalid address: 0x%X\n",
		    gt_fault.gf_addr);
		break;
	}


	if (gt_validthread(softc, ap)) {

		gt_set_flags(&ap->ap_flags, (GTA_ABORT | GTA_DONTRUN));

		if (!(ap->ap_proc->p_flag & SWEXIT))
			psignal(ap->ap_proc, SIGTERM);

		gta_exit(ap);
	}


	fp1->fe_rootptp = (u_int) gta_dummylvl1 | ROOTVALID;

	gt_clear_flags(&softc->flags, F_THREAD_RUNNING);
	softc->at_curr = softc->at_head;

	fp3->fe_hflag_3 = 1;
	gt_sync = fp3->fe_hflag_3;
}


static void
gt_mmuload(ap, addr, pp, seg, premapflag)
	struct at_proc	*ap;
	addr_t		 addr;
	struct page	*pp;
	struct seg	*seg;
	int		 premapflag;
{
	struct gt_pte	   *pte;
	struct gt_ptp	   *ptp;
	struct gta_mmudata  mmudata;
	u_int		 ppn, gtppn;
	int		 i;

	/*
	 * Sanity checks
	 */
	if ((pp == NULL) || pp->p_free)
		panic("gt mmuload");

	/*
	 * Find the physical page number, relative to the gt's pagesize.
	 */
	ppn    = page_pptonum(pp);
	gtppn  = ppn * GT_PAGE_RATIO;

	/*
	 * If we are running on a sun4c, we must shunt GT VM accesses
	 * through a context zero window in kernel space.  Set up the
	 * context zero translation, and use the virtual address of the
	 * kernel window in the GT page table in lieu of the physical
	 * address of the page.
	 */
	pte = gta_getmmudata(ap, addr, &mmudata);
	ptp = &((mmudata.gtu_lvl1)->gt_ptp[mmudata.gtu_index1]);

	/*
	 * Increment ptp_cnt before calling gta_locksetup() so
	 * that the target level 2 page table won't be freed
	 * if gta_locksetup() unloads the only current
	 * translation in it.
	 */
	ptp->ptp_cnt += GT_PAGE_RATIO;

#ifdef sun4c
	ppn   = gta_locksetup(seg, addr, pp, premapflag);
	gtppn = ppn * GT_PAGE_RATIO;
#elif defined(sun4m)
	(void) gta_locksetup(seg, addr, pp, premapflag);
#endif sun4c

	/*
	 * Load gt translations for all gt pages spanned by the
	 * corresponding host page.
	 */
	for (i=0; i<GT_PAGE_RATIO; i++) {
		pte->pte_ppn = gtppn;

		pte->pte_u   = 1;
		pte->pte_r   = 0;
		pte->pte_m   = 0;
		pte->pte_v   = 1;

		pte++;
		gtppn++;
	}
}


/*
 * Allocate a lockmap element.
 *
 * If we are on a sun4c, set up a translation to the specified
 * page through the GT DVMA window.
 */
static u_int
gta_locksetup(seg, addr, pp, premapflag)
	struct seg	*seg;
	caddr_t		 addr;
	struct page	*pp;
	int		 premapflag;
{
	struct gta_lockmap *map;
	struct as *as;
	caddr_t	xaddr;
	int	i;
#ifdef sun4c
	int	index;
#endif sun4c
	extern struct seg kseg;

	xaddr = (caddr_t) ((u_int)addr & PAGEMASK);

	/*
	 * Allocate a window.  Take one from the free list if possible.
	 * Otherwise reclaim one from the end of the active list to
	 * the free list.
	 */
	for (;;) {
		map = GT_LIST_NEXT(&gta_lockfree, gta_lockmap, gtm_list);

		if (map != &gta_lockfree) {
			break;
		} else {
			map = GT_LIST_PREV(&gta_lockbusy, gta_lockmap,
			    gtm_list);

			for (i = 0; i < (premapflag-1); i++)
				map = GT_LIST_PREV(map, gta_lockmap, gtm_list);

			gta_unload(map->gtm_as, map->gtm_addr, MMU_PAGESIZE);
#ifdef GT_DEBUG
			gtstat.gs_mapsteal++;
#endif GT_DEBUG
		}
	}

	/*
	 * At this time, we are guaranteed that this map element
	 * is on the free list.
	 */
	GT_LIST_REMOVE(map, gta_lockmap, gtm_list);


	/*
	 * Map the chosen window to the specified physical page.
	 */
	as = seg->s_as;
	map->gtm_as   = as;
	map->gtm_addr = xaddr;
	map->gtm_pp   = pp;

	pg_setref(pp, 1);

#ifdef sun4c
	index = (int)(map-gta_lockmapbase);
	xaddr = gta_dvmaspace + (index << MMU_PAGESHIFT);

 	hat_memload(&kseg, xaddr, pp, PROT_READ|PROT_WRITE, 1);

	gt_setnocache(xaddr);
	vac_pageflush(xaddr);
#endif	/* sun4c */

	/*
	 * Insert the element into the hash list and into the
	 * the active list.  If this is a pre-mapped page, we
	 * put it at the end of the active list;  otherwise we
	 * put it at the beginning.
	 */
	gta_hash_insert(map);

	if (premapflag == 0) {
		GT_LIST_INSERT_NEXT(&gta_lockbusy, map, gta_lockmap, gtm_list);
	} else {
		GT_LIST_INSERT_PREV(&gta_lockbusy, map, gta_lockmap, gtm_list);
	}

	return ((u_int)xaddr >> MMU_PAGESHIFT);
}


static struct gt_pte *
gta_getmmudata(ap, addr, gtu)
	struct at_proc		*ap;
	addr_t			 addr;
	struct gta_mmudata	*gtu;
{
	/*
	 * Generate the level 1 and level 2 page table indices
	 * and the level 1 and 2 page table addresses.
	 */
	gtu->gtu_index1 = ((u_int)addr >> (INDX1_SHIFT+2)) & 0x3ff;
	gtu->gtu_index2 = ((u_int)addr >> (INDX2_SHIFT+2)) & 0x3ff;

	gtu->gtu_lvl1 = (struct gt_lvl1 *)ap->ap_lvl1;
	gtu->gtu_lvl2 = gtu->gtu_lvl1->gt_lvl2[gtu->gtu_index1];

	gtu->gtu_pte  = &gtu->gtu_lvl2->gt_pte[gtu->gtu_index2];

	if (((u_int)addr % MMU_PAGESIZE) >= GT_PAGESIZE)
		gtu->gtu_pte--;

	return (gtu->gtu_pte);
}


/*
 * The GT accelerator's scheduler, which runs as a kernel process
 */
static void
gt_scheduler()
{
	struct gt_softc *softc;
	struct at_proc *ap;
	struct at_proc *cp;
	caddr_t		wchan;

	gt_daemon_init("gtscheduler");

	/*
	 * The scheduler uses the first softc's wchan
	 */
	wchan = (caddr_t) &gt_softc[0].wchan[WCHAN_SCHED];

	(void) splr(gt_priority);

	for (;;) {
#ifdef GT_DEBUG
		gtstat.gs_scheduler++;
#endif GT_DEBUG

		for (softc=gt_softc; softc < &gt_softc[NGT]; softc++) {

			/*
			 * If the GT microcode isn't healthy we may as well
			 * go back to sleep.  If it has just stopped, we
			 * need to reset the accelerator and kill any
			 * accelerator threads.
			 */
			if ((softc->flags & F_UCODE_STOPPED) ||
			    !(softc->flags & F_UCODE_RUNNING)) {

				if (softc->flags & F_UCODE_STOPPED) {

					softc->at_curr = softc->at_head;
					gta_reset_accelerator(softc);

					gt_clear_flags(&softc->flags,
					    (F_UCODE_STOPPED|F_UCODE_RUNNING));
				}

				continue;
			}


			/*
			 * Run through the kmcb request queue.  When we
			 * return from gt_schedkmcb() the kmcb channel
			 * may be either active or idle.
			 */
			gt_schedkmcb(softc);


			/*
			 * Suspend the current thread?
			 */
			if ((softc->flags & F_SUSPEND) ||
			    (softc->at_curr->ap_flags & GTA_BLOCKED)) {
				gta_stop(softc);
			}

			if (softc->flags & (F_DONTSCHEDULE|F_ATU_FROZEN))
				continue;


			/*
			 * If the current process is runnable, but
			 * stopped, let it proceed.
			 */
			if (!(softc->at_curr->ap_flags & GTA_BLOCKED) &&
			    (softc->kmcb.gt_stopped & HK_KERNEL_STOPPED)) {

				if (softc->flags & F_THREAD_RUNNING) {

					softc->kmcb.command = 0;
					softc->fe_ctrl_sp_ptr->fe_hflag_0  = 1;
					gt_sync = softc->fe_ctrl_sp_ptr->fe_hflag_0;

				} else
					gta_start(softc, softc->at_curr);

				continue;
			}



			/*
			 * Look for the next roundrobin candidate.  If the
			 * next runnable process is not the current process,
			 * stop the current process and start the next one.
			 * If the next runnable process is the current one,
			 * just let it keep running.
			 */
			cp = softc->at_curr;
			ap = GT_LIST_NEXT(cp, at_proc, ap_list);

			for (; ap!=cp;) {

				if (!(ap->ap_flags & GTA_BLOCKED)) {
#ifdef GT_DEBUG
					gtstat.gs_threadswitch++;
#endif GT_DEBUG
					gta_stop(softc);

					if (gt_validthread(softc, ap)) {
						gta_start(softc, ap);
						break;
					} else
						ap = softc->at_head;
				}

				ap = GT_LIST_NEXT(ap, at_proc, ap_list);
			}
		}


		/*
		 * Until next time...
		 */
		timeout(gt_timeout, wchan, gt_quantum ? gt_quantum:GT_QUANTUM);
		(void) sleep(wchan, PZERO+1);

		/*
		 * The scheduler may have been woken by some other means
		 * than by timeout.  The untimeout ensures that we don't
		 * accrue multiple timeouts for the scheduler.
		 */
		untimeout(gt_timeout, wchan);

	}
}


static void
gt_schedkmcb(softc)
	struct gt_softc *softc;
{
	struct fe_ctrl_sp  *fep   = softc->fe_ctrl_sp_ptr;
	struct Hkvc_kmcb   *kmcb  = &softc->kmcb;
	struct Hkvc_kmcb   *qmcb;
	struct gt_kmcblist *kptr, *next;
	int	setpulse;

	/*
	 * Is it time to check the frontend's pulse?  We do this
	 * if its been GT_PULSERATE seconds beyond the last time
	 * we've conducted a kmcb transaction with it.
	 */
	if ((time.tv_sec >= gt_pulse+GT_PULSERATE) &&
	    !(softc->flags & F_PULSED)) {

		kptr = (struct gt_kmcblist *)
		    new_kmem_zalloc(sizeof (*kptr), KMEM_SLEEP);

		qmcb = &kptr->q_mcb;
		qmcb->command = HKKVC_PAUSE_WITH_INTERRUPT;

		GT_LIST_INSERT_PREV(&gt_kmcbhead, kptr, gt_kmcblist, q_list);

		gt_pulse = time.tv_sec;
		setpulse = 1;
	} else
		setpulse = 0;


	/*
	 * Run through the kmcb request queue
	 */
	next = GT_LIST_NEXT(&gt_kmcbhead, gt_kmcblist, q_list);

	while (next != &gt_kmcbhead) {
		kptr = next;
		next = GT_LIST_NEXT(next, gt_kmcblist, q_list);

		/*
		 * If the kernel-to-hawk VCR is not clear, the GT isn't
		 * finished with the last command, so we try again later.
		 */
		if (fep->fe_hflag_0)
			break;

		/*
		 * Has this kmcb request already been posted?
		 * If so, its already been acknowledged so we
		 * can free the request element.
		 */
		if (kptr->q_flags & KQ_POSTED) {
			untimeout(gt_kmcbtimeout, (caddr_t)softc);
			gt_clear_flags(&softc->flags, F_PULSED);

			GT_LIST_REMOVE(kptr, gt_kmcblist, q_list);

			if (kptr->q_flags & KQ_BLOCK)
				wakeup((caddr_t)kptr);

			kmem_free((caddr_t)kptr, sizeof (*kptr));

			next = GT_LIST_NEXT(&gt_kmcbhead, gt_kmcblist, q_list);
			continue;
		}


		/*
		 * Copy the enqueued request to the kmcb
		 */
		qmcb = &kptr->q_mcb;

		kmcb->command		 = qmcb->command;
		kmcb->wlut_image_bits	 = qmcb->wlut_image_bits;
		kmcb->wlut_info		 = qmcb->wlut_info;
		kmcb->clut_info		 = qmcb->clut_info;
		kmcb->fb_width		 = qmcb->fb_width;
		kmcb->screen_width	 = qmcb->screen_width;
		kmcb->screen_height	 = qmcb->screen_height;
		kmcb->light_pen_enable	 = qmcb->light_pen_enable;
		kmcb->light_pen_distance = qmcb->light_pen_distance;
		kmcb->light_pen_time	 = qmcb->light_pen_time;

		/*
		 * Post the request to the GT
		 */
		if ((softc->flags & F_DONTSCHEDULE) ||
		    !(softc->flags & F_THREAD_RUNNING))
			kmcb->command |= HKKVC_PAUSE_WITH_INTERRUPT;

		kptr->q_flags |= KQ_POSTED;
		fep->fe_hflag_0 = 1;
		gt_sync = fep->fe_hflag_0;

		gt_pulse = time.tv_sec;

#ifdef GT_DEBUG
		gtstat.gs_kmcb_cmd++;
#endif GT_DEBUG
	}

	if (setpulse) {
		timeout(gt_kmcbtimeout, (caddr_t)softc, GT_KMCBTIMEOUT);
		gt_set_flags(&softc->flags, F_PULSED);
	}
}


/*
 * Idle the accelerator, leaving the current thread, if any, intact.
 */
static void
gta_idle(softc, flag)
	struct gt_softc *softc;
	unsigned flag;
{
	u_int cmd;

	if (softc->flags & F_UCODE_RUNNING) {

		gt_set_flags(&softc->flags, flag);

		cmd = HKKVC_FLUSH_RENDERING_PIPE | HKKVC_PAUSE_WITH_INTERRUPT;

		(void) gt_post_command(softc, cmd, (Hkvc_kmcb *)0);
	}
}


/*
 * Let the accelerator continue running.
 */
static void
gta_run(softc, flag)
	struct gt_softc *softc;
	unsigned flag;
{
	gt_clear_flags(&softc->flags, flag);
	wakeup(&softc->wchan[WCHAN_SCHED]);
}


/*
 * Stop the current accelerator thread
 */
static void
gta_stop(softc)
	struct gt_softc *softc;
{
	fe_ctrl_sp	*fep = softc->fe_ctrl_sp_ptr;
	fe_page_1	*fp1 = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	fe_page_2	*fp2 = (fe_page_2 *)softc->va[MAP_FE_HFLAG2];
	struct at_proc	*ap  = softc->at_curr;
	struct seg	*ssp;
	struct seg	*usp;
	int		 exitflag = 0;

	if (softc->flags & F_THREAD_RUNNING) {

		if (gt_validthread(softc, ap)) {

			if ((ap->ap_proc->p_flag & SWEXIT) ||
			    (ap->ap_flags & GTA_EXIT))
				exitflag = 1;
		}

		usp = softc->uvcrseg;
		ssp = softc->uvcrseg;

		if (!exitflag) {
			if (usp)
				hat_unload(usp, usp->s_base, usp->s_size);
			if (ssp)
				hat_unload(ssp, ssp->s_base, ssp->s_size);
		}

		softc->uvcrseg = (struct seg *)NULL;
		softc->svcrseg = (struct seg *) NULL;

		if (!(ap->ap_flags & GTA_ABORT)) {

			/*
			 * Wait for the kmcb channel to be clear
			 */
			gta_kmcbwait(softc);

			/*
			 * Flush the graphics pipeline, the context if
			 * so requested,  and halt.
			 */
			gt_wait_for_thaw(softc);

			softc->kmcb.command = HKKVC_FLUSH_RENDERING_PIPE |
			    HKKVC_PAUSE_WITH_INTERRUPT;

			exitflag = 0;

			if (gt_validthread(softc, ap)) {
				if ((ap->ap_proc->p_flag & SWEXIT) ||
				    (ap->ap_flags & GTA_EXIT))
					exitflag = 1;
			}

			if (!exitflag)
				softc->kmcb.command |=
				    HKKVC_FLUSH_FULL_CONTEXT;

			if (!(softc->flags & F_UCODE_STOPPED))
				fep->fe_hflag_0  = 1;

			gta_kmcbwait(softc);

			/*
			 * Save the user and signal vcr states
			 */
			if (fp2->fe_hflag_2 & 0x1)
				ap->ap_flags |=  GTA_UVCR;
			else
				ap->ap_flags &= ~GTA_UVCR;

			if (fp1->fe_hflag_1 & 0x1)
				ap->ap_flags |=  GTA_SVCR;
			else
				ap->ap_flags &= ~GTA_SVCR;

			gt_wait_for_thaw(softc);


			gta_loadroot(softc, (struct gt_lvl1 *)NULL,
			    MIA_CLEARROOT);

		}

		softc->at_curr =  softc->at_head;
		gt_clear_flags(&softc->flags, (F_THREAD_RUNNING | F_SUSPEND));
		wakeup(&softc->wchan[WCHAN_SUSPEND]);
	}
}


/*
 * Start an accelerator thread
 */
static void
gta_start(softc, ap)
	struct gt_softc *softc;
	struct at_proc *ap;
{
	fe_ctrl_sp *fep = softc->fe_ctrl_sp_ptr;
	fe_page_1  *fp1 = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	fe_page_2  *fp2 = (fe_page_2 *)softc->va[MAP_FE_HFLAG2];

	gta_kmcbwait(softc);
	gt_wait_for_thaw(softc);

	if (softc->flags & F_UCODE_STOPPED)
		return;

	/*
	 * Make sure that this is still valid.  It may have disconnected
	 * while we were asleep in kmcbwait().
	 */
	if (gt_validthread(softc, ap) && !(ap->ap_flags & GTA_BLOCKED)) {

		/*
		 * Load the root pointer into the GT, and load the
		 * user's mcb pointer and the user's context.
		 */
		gta_loadroot(softc, ap->ap_lvl1, MIA_LOADROOT);

		softc->kmcb.command  = HKKVC_LOAD_USER_MCB | HKKVC_LOAD_CONTEXT;
		softc->kmcb.user_mcb = ap->ap_umcb;
		softc->at_curr       = ap;

		fp2->fe_hflag_2 = (ap->ap_flags & GTA_UVCR) ? 1:0;
		fp1->fe_hflag_1 = (ap->ap_flags & GTA_SVCR) ? 1:0;

		fep->fe_hflag_0  = 1;
		gt_sync = fep->fe_hflag_0;
		gt_set_flags(&softc->flags, F_THREAD_RUNNING);
	}

	/*
	 * A process might be sleeping while processing an address
	 * space fault caused by referencing the user VCRs.
	 */
	wakeup(&softc->wchan[WCHAN_VCR]);
}


/*
 * Wait for the kmcb channel to be clear
 */
static void
gta_kmcbwait(softc)
	struct gt_softc *softc;
{
	fe_ctrl_sp *fep		=  softc->fe_ctrl_sp_ptr;
	caddr_t     wchan	= &softc->wchan[WCHAN_SCHED];
	int	    notfirst	= 0;
	u_int	    pauseflag;

	pauseflag = softc->kmcb.command & HKKVC_PAUSE_WITH_INTERRUPT;

	while (fep->fe_hflag_0) {

		if (!pauseflag || notfirst)
			timeout(gt_timeout, wchan,
			    gt_quantum ? gt_quantum:GT_QUANTUM);

		(void) sleep(wchan, PZERO+1);
		notfirst = 1;
	}
}


/*
 * Freeze the GT's ATU, invalidate its TLBs,
 * load a new root pointer, and release the ATU.
 */
static void
gta_loadroot(softc, root, fcn)
	struct gt_softc *softc;
	struct gt_lvl1	*root;
	enum   mia_ops   fcn;
{
	fe_ctrl_sp  *fep  = softc->fe_ctrl_sp_ptr;
	fe_page_1   *fp1  = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	u_int rootptp;

	gt_freeze_atu(softc, F_FREEZE_LDROOT);

	gt_set_flags(&fep->fe_atu_syncr, ATU_CLEAR);

	switch (fcn) {

	case MIA_LOADROOT:
		rootptp = (u_int)GT_KVTOP((int)root) | ROOTVALID;
		break;

	case MIA_CLEARROOT:
		rootptp = (u_int)NULL;
		break;

	default:
		panic("gta_loadroot: bad argument");
		break;
	}

	fp1->fe_rootptp = rootptp;
	gt_sync = fp1->fe_rootptp;

	gt_thaw_atu(softc, F_FREEZE_LDROOT);
}


/*
 * Initialize GT accelerator structures
 */
static void
gta_init()
{
	struct gta_lockmap  *nextmap;
	struct gt_lvl1      *lvl1;
	struct gt_lvl2      *lvl2;
	caddr_t		     pp;
	u_int   nbytes;
	int	i;
#ifdef sun4c
	u_long	kmx;
#endif	sun4c

	/*
	 * Start the daemons that service the accelerator
	 */
	kern_proc(gt_pagedaemon, 0);
	kern_proc(gt_scheduler, 0);

#ifdef sun4c
	/*
	 * XXX: pretty crufty allocation of kernel address space
	 */
retry:
	kmx = rmalloc(kernelmap, (long)btoc(gta_dvmasize));
	if (kmx == (u_long) 0) {
		gta_dvmasize -= 1*MEG;
		if (gta_dvmasize <= 1*MEG)
			panic("gta_init: cannot allocate space for GT DVMA");
		goto retry;
	}

	gta_dvmaspace = (caddr_t) kmxtob(kmx);

	(void) seg_attach(&kas, gta_dvmaspace, gta_dvmasize, &gtdvmaseg);
	(void) segkmem_create(&gtdvmaseg, (caddr_t)NULL);

	hat_reserve(&gtdvmaseg, gta_dvmaspace, gta_dvmasize);

	gta_lockents  = btoc(gta_dvmasize);
#else
	if (gta_dvmasize > gta_locksize)
		gta_locksize = gta_dvmasize;

	gta_lockents  = btoc(gta_locksize);
#endif	/* sun4c */


	/*
	 * Allocate and initialize the GT DVMA map
	 */
	GT_LIST_INIT(&gta_lockbusy, gta_lockmap, gtm_list);
	GT_LIST_INIT(&gta_lockfree, gta_lockmap, gtm_list);

	nbytes = sizeof (struct gta_lockmap) * gta_lockents;
	gta_lockmapbase =
	    (struct gta_lockmap *) new_kmem_zalloc(nbytes, KMEM_NOSLEEP);

	nextmap = gta_lockmapbase;

	for (i=0; i<gta_lockents; i++) {

		GT_LIST_INSERT_NEXT(&gta_lockfree, nextmap++,
		    gta_lockmap, gtm_list);
	}

	/*
	 * Allocate and initialize the GT DVMA hash table
	 */
	nbytes = sizeof (struct gta_map *) * GT_HASHTBLSIZE;
	gta_lockhash =
	    (struct gta_lockmap **) new_kmem_zalloc(nbytes, KMEM_NOSLEEP);


	/*
	 * Set up a dummy set of page tables in which every address
	 * maps to a dummy page.  These are used when we need to
	 * abort an in-progress GT pagefault.
	 *
	 * These are set up so that every entry in the level 1 table
	 * refers to the same level 2 table, and every entry in the
	 * level 2 table refers to the same page.  The dummy page
	 * itself is uninitialized.
	 *
	 * XXX: can anyone think of a better approach?
	 */
	lvl1 =
	    (struct gt_lvl1 *) getpages(btopr(sizeof (struct gt_lvl1)), 0);

	lvl2 =
	    (struct gt_lvl2 *) getpages(btopr(sizeof (struct gt_lvl2)), 0);

	pp = getpages(1, 0);		/* get one page, ok to sleep	*/

	gt_setnocache((addr_t)lvl1);
	gt_setnocache((addr_t)lvl2);

	gta_dummylvl1 = lvl1;

	for (i = 0; i < GT_ENTRIES_PER_LVL1; i++) {
		lvl1->gt_ptp[i].ptp_base = (unsigned)lvl2>>GT_PAGESHIFT;
		lvl1->gt_ptp[i].ptp_v    = 1;
		lvl1->gt_ptp[i].ptp_cnt  = GT_ENTRIES_PER_LVL2;

	}

	for (i = 0; i < GT_ENTRIES_PER_LVL2; i++) {
		lvl2->gt_pte[i].pte_ppn = (unsigned)pp>>GT_PAGESHIFT;
		lvl2->gt_pte[i].pte_m   = 0;
		lvl2->gt_pte[i].pte_r   = 0;
		lvl2->gt_pte[i].pte_v   = 1;
	}
}


/*
 * GT DVMA hash routines
 */
static void
gta_hash_insert(map)
	struct gta_lockmap *map;
{
	unsigned hashindex;

	hashindex = GT_HASH(map->gtm_addr);

	map->gtm_hashnext = gta_lockhash[hashindex];
	gta_lockhash[hashindex] = map;
}


static void
gta_hash_delete(map)
	struct gta_lockmap *map;
{
	struct gta_lockmap *node, *trailnode;
	unsigned hashindex;

	hashindex = GT_HASH(map->gtm_addr);
	node      = gta_lockhash[hashindex];
	trailnode = (struct gta_lockmap *)NULL;

	while (node) {
		if (node == map) {

			if (trailnode)
				trailnode->gtm_hashnext = node->gtm_hashnext;
			else
				gta_lockhash[hashindex] = node->gtm_hashnext;

			return;
		}
		trailnode = node;
		node = node->gtm_hashnext;
	}
}


static struct gta_lockmap *
gta_hash_find(addr, as)
	addr_t addr;
	struct as *as;
{
	unsigned hashindex;
	struct gta_lockmap *node;

	hashindex = GT_HASH(addr);
	node      = gta_lockhash[hashindex];

	while (node) {
		if ((node->gtm_addr == addr) && (node->gtm_as == as))
			return (node);

		node = node->gtm_hashnext;
	}

	return (NULL);
}


/*
 * Initialize a GT daemon
 */
static void
gt_daemon_init(name)
	char *name;
{
	struct proc *p = u.u_procp;
	int i;
	extern struct vnode *rootdir;

	(void) strcpy(u.u_comm, name);
	relvm(u.u_procp);

	VN_RELE(u.u_cdir);
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);

	if (u.u_rdir)
		VN_RELE(u.u_rdir);
	u.u_rdir = NULL;

	/*
	 * Close all open files
	 */
	for (i=0; i<u.u_lastfile+1; i++) {
		if (u.u_ofile[i] != NULL) {
			closef(u.u_ofile[i]);
			u.u_ofile[i]  = NULL;
			u.u_pofile[i] = 0;
		}
	}

	(void) setsid(SESS_NEW);

	for (i=0; i<NSIG; i++) {
		u.u_signal[i]  = SIG_IGN;
		u.u_sigmask[i] =
		    ~(sigmask(SIGKILL)|sigmask(SIGTERM));
	}
	u.u_signal[SIGKILL] = SIG_DFL;
	u.u_signal[SIGTERM] = SIG_DFL;
	
	p->p_sig    = 0;
	p->p_cursig = 0;
	p->p_sigmask   = ~(sigmask(SIGKILL)|sigmask(SIGTERM));
	p->p_sigignore = ~(sigmask(SIGKILL)|sigmask(SIGTERM));
	p->p_sigcatch  =  0;

	u.u_cred = crcopy(u.u_cred);
	u.u_groups[0] = NOGROUP;

	if (setjmp(&u.u_qsave))
		exit(0);
}


/*
 * Insert a list element to right of node
 */
static void
gt_listinsert_next(node, newnode)
	struct gt_list 	*node;
	struct gt_list	*newnode;
{
	newnode->l_prev		= node;
	newnode->l_next		= node->l_next;
	(node->l_next)->l_prev	= newnode;
	node->l_next		= newnode;
}


/*
 * Insert a list element to left of node
 */
static void
gt_listinsert_prev(node, newnode)
	struct gt_list 	*node;
	struct gt_list	*newnode;
{
	newnode->l_next		= node;
	newnode->l_prev		= node->l_prev;
	(node->l_prev)->l_next	= newnode;
	node->l_prev		= newnode;
}


/*
 * Remove node from its list
 */
static void
gt_listremove(node)
	struct gt_list *node;
{
	(node->l_prev)->l_next = node->l_next;
	(node->l_next)->l_prev = node->l_prev;
}


/*
 * GT hardware manipulations
 *
 *    - enable/disable interrupts
 *    - freeze/thaw the ATU
 *    - wait for RP CSR PFBUSY to be reset
 */

/*
 * Enable GT interrupts specified x.
 */
static void
gt_enable_intr(softc, x)
	struct gt_softc *softc;
	unsigned x;
{
	struct fe_i860_sp *fe = (struct fe_i860_sp *)softc->va[MAP_FE_I860];
	u_int intr_reg;
	int s;

	/*
	 * Enable most of the interrupts
	 */
	s = splr(gt_priority);

	intr_reg = softc->fe_ctrl_sp_ptr->fe_hisr;
	softc->fe_ctrl_sp_ptr->fe_hisr = intr_reg | HISR_INTMASK | (x);

	/*
	 * spooky - This is needed, but we don't know why.
	 *
	 * Without the delay, the localbus timeout enable bit does
	 * not set, and the GT starts stuttering -- sending lots
	 * and lots of partial and repeated bits of messages to the
	 * host.
	 */
	DELAY(1000);

	fe->hoint_msk = FE_LBTO;
	gt_sync = fe->hoint_msk;

	(void) splx(s);
}


/*
 * Freeze the GT ATU
 */
#define FREEZE_DELAY	1000		/* 1 millisecond	*/

static void
gt_freeze_atu(softc, cond)
	struct gt_softc *softc;
	unsigned cond;
{
	fe_ctrl_sp	*fep = softc->fe_ctrl_sp_ptr;
	int		s;

	s = splr(gt_priority);

	if ((softc->flags & F_ATU_FROZEN) == 0) {
		int cnt = FREEZE_DELAY;

		/*
		 * We need to re-assert ATU_FREEZE when we don't see
		 * FREEZE_ACK to avoid a race in the frontend.  If
		 * we don't do so, we might assert ATU_FREEZE whilst
		 * the frontend is searching its TLB for a translation
		 * that it doesn't have.  In this event, FREEZE_ACK
		 * will never get asserted.  Re-asserting ATU_FREEZE
		 * clears the condition.
		 */

#ifdef GT_DEBUG
		gtstat.gs_freeze_atu++;
#endif GT_DEBUG

		while (--cnt > 0) {
			fep->fe_atu_syncr |= ATU_FREEZE;
			if (fep->fe_hcr & FREEZE_ACK)
				break;

			usec_delay(1);
		}

		if (!(fep->fe_hcr & FREEZE_ACK)) {
			gt_status_dump(softc, DUMP_GT_REGS);
			panic("GT ATU won't freeze");
		}


		/*
		 * Having frozen the ATU, clear any pending ATU
		 * interrupts.  The condition can re-assert the
		 * interrupt after we thaw.
		 */
		fep->fe_hisr &=
		    ~(HISR_ATU|HISR_ROOT_INV|HISR_LVL1_INV|HISR_TLB_MISS);
		gt_sync = fep->fe_hisr;
	}

	softc->flags |= cond;

	(void) splx(s);
}


/*
 * Thaw the GT ATU
 */
static void
gt_thaw_atu(softc, cond)
	struct gt_softc *softc;
	unsigned cond;
{
	fe_ctrl_sp *fep = softc->fe_ctrl_sp_ptr;
	int    s;

	s = splr(gt_priority);

	softc->flags &= ~cond;

	if (!(softc->flags&F_ATU_FROZEN)) {

		fep->fe_atu_syncr &= ~(ATU_FREEZE|ATU_CLEAR);
		gt_sync = fep->fe_atu_syncr;
		wakeup(&softc->wchan[WCHAN_THAW]);
	}

	(void) splx(s);
}


static void
gt_wait_for_thaw(softc)
	struct gt_softc *softc;
{

	while (softc->flags & F_ATU_FROZEN)
		(void) sleep(&softc->wchan[WCHAN_THAW], PZERO+1);
}


static void
gt_set_flags(ptr, flags)
	unsigned *ptr;
	unsigned flags;
{
	int s;

	s = splr(gt_priority);
	*ptr |= flags;
	(void) splx(s);
}


static void
gt_clear_flags(ptr, flags)
	unsigned *ptr;
	unsigned flags;
{
	int s;

	s = splr(gt_priority);
	*ptr &= ~flags;
	(void) splx(s);
}


/*
 * Check whether the PF (pixel formatter) is busy.
 * If so, spin loop a short while for nonbusy.
 * Returns 1 if busy, 0 otherwise.
 *
 * NB: In the mp environment, we can check that the PF is not busy,
 * and a moment later a user process running on another processor can
 * busy it. For this reason calls to gt_check_pf_busy() from kernel 
 * mode should make the check and the subsequent actions depending 
 * on the check an exclusive access operation by using the exclusion 
 * primitives xc_attention() and xc_dismissed().
 *
 * XXX - We need to do something better than printing the
 * persistent busy message
 */
static
gt_check_pf_busy(softc)
	struct gt_softc *softc;
{
	rp_host *rp_host_ptr = (rp_host *)softc->va[MAP_RP_HOST_CTRL];

	CDELAY(!(rp_host_ptr->rp_host_csr_reg & GT_PF_BUSY), GT_SPIN_PFBUSY);

	if (rp_host_ptr->rp_host_csr_reg & GT_PF_BUSY) {
		printf("gt%d: PF persistent busy\n", softc-gt_softc);
		gt_status_dump(softc, DUMP_GT_REGS);

		return (1);
	} else
		return (0);
}


/*
 * Print useful registers for the diagnosticians.
 */
/*ARGSUSED*/
static void
gt_status_dump(softc, flags)
	struct gt_softc *softc;
	unsigned flags;
{
#ifdef GT_DEBUG
	fe_ctrl_sp    *fep = softc->fe_ctrl_sp_ptr;
	fe_page_1     *fp1 = (fe_page_1 *)softc->va[MAP_FE_HFLAG1];
	fe_i860_sp    *fe  = (fe_i860_sp *)softc->va[MAP_FE_I860];
	rp_host	      *rh = (rp_host *)softc->va[MAP_RP_HOST_CTRL];
	long  peekval;
	unsigned int   hcr, hisr, atusync, par, rootindex, lvl1index;
	unsigned int   hold, lbdes, lbseg, intstat;
	unsigned int   su_csr, su_idesc, ew_csr, si_csr1;
	unsigned int   si_csr2, fe_asr, fe_csr, host_asr, host_csr;

	struct rp_regs {
		u_int	su_csr,
			    pad0,
			su_input_data,
			    pad1,
			ew_csr,
			si_csr1,
			si_csr2,
			pbm_fe_asr,
			pbm_fe_csr;
	} *rp = (struct rp_regs *)softc->va[MAP_RP_FE_CTRL];

	if (flags & DUMP_MIA_REGS) {

		/*
		 * Peek at the first register to be sure that we can read
		 * GT registers.  If we can, we can just read the others.
		 */
		if (peekl((long *) &fep->fe_hcr, &peekval) != 0) {
			printf("gt_status:  cannot read FE registers\n");
			return;
		}

		rootindex = fp1->fe_rootindex;
		lvl1index = fep->fe_lvl1index;
		par	  = fep->fe_par;

		hcr	= peekval	    & 0xff00075b;
		hisr  	= fep->fe_hisr	    & 0xef0fa80f;
		atusync = fep->fe_atu_syncr & 0x00000003;
		hold	= fe->fe_hold	    & 0x00000003;
		lbdes	= fe->lbdes	    & 0x0000000f;
		lbseg	= fe->lbseg	    & 0x00000030;
		intstat	= fe->hoint_stat    & 0x00000001;

		printf("gt fe hcr:	0x%X\n", hcr);
		printf("      hisr:	0x%X\n", hisr);
		printf("      atu sync:	0x%X\n", atusync);
		printf("      test reg:	0x%X\n", fep->fe_test_reg);
		printf("      par:	0x%X\n", par);
		printf("      root ptp:	0x%X\n", fp1->fe_rootptp);
		printf("      rootindx:	0x%X\n", rootindex);
		printf("      lvl1:	0x%X\n", fep->fe_lvl1);
		printf("      lvl1indx:	0x%X\n", lvl1index);
		printf("      hold:	0x%X\n", hold);
		printf("      lbdes:    0x%X\n", lbdes);
		printf("      lbseg:    0x%X\n", lbseg);
		printf("      intstat:	0x%X\n", intstat);
		printf("      lbto_err:	0x%X\n\n", fe->lb_err_addr);

		printf("partially decoding these, we find:\n");
		if (hcr & 0xaa000000) {
			printf("    host flags:");
			if (hcr & 0x80000000) printf("  0");
			if (hcr & 0x20000000) printf("  1");
			if (hcr & 0x08000000) printf("  2");
			if (hcr & 0x02000000) printf("  3");
			printf("\n");
		}

		if (hcr & 0x55000000) {
			printf("   gt flags:");
			if (hcr & 0x40000000) printf("  0");
			if (hcr & 0x10000000) printf("  1");
			if (hcr & 0x04000000) printf("  2");
			if (hcr & 0x01000000) printf("  3");
			printf("\n");
		}

		if ((hcr & 0x0000025b) || (atusync & 0x00000003)) {
			printf("    atu state:");
			if (hcr & 0x00000002) printf("  go");
			if (hcr & 0x00000008) printf("  ss");
			if (hcr & 0x00000010) printf("  jam");
			if (hcr & 0x00000040) printf("  gt reset");
			if (hcr & 0x00000001) printf("  fe reset");
			if (atusync & 0x00000001) printf("  atu clear");
			if (atusync & 0x00000002) printf("  atu freeze");
			if (hcr & 0x00000200)     printf("  freeze ack");
			printf("\n");
		}

		if (hold & 0x00000003) {
			printf("    i860 hold:");
			if (hold & 0x00000001) printf("  hold");
			if (hold & 0x00000002) printf("  hlda");
			printf("\n");
		}

		if (hisr & 0xe0000000) {
			printf("    bus-related interrupts:");
			if (hisr & 0x80000000) printf("  mst0");
			if (hisr & 0x40000000) printf("  berr");
			if (hisr & 0x20000000) printf("  size");
			printf("\n");
		}

		if (hisr & 0x0e000000) {
			addr_t vaddr;

			vaddr = (addr_t)
			    ((rootindex & INDX1_MASK) << INDX1_SHIFT |
			     (lvl1index & INDX2_MASK) << INDX2_SHIFT |
			     (par & PGOFF_MASK));

			printf("    atu-related interrupts:");
			if (hisr & 0x08000000) printf("  atu");
			if (hisr & 0x04000000) printf("  root invalid");
			if (hisr & 0x02000000) printf("  level 1 invalid");
			if (hisr & 0x01000000) printf("  tlb miss");
			printf("\n    virtual address: 0x%X\n", vaddr);
		}
	}

	if (flags & DUMP_RP_REGS) {
		

		if (peekl((long *) &rp->su_csr, &peekval) != 0) {
			printf("gt_status: cannot read RP registers\n");
			return;
		}

		su_csr		= peekval	      & 0xb0ff7edb;
		su_idesc	= rp->su_input_data   & 0x0fff000f;
		ew_csr		= rp->ew_csr	      & 0x40000fff;
		si_csr1		= rp->si_csr1	      & 0x7fffffef;
		si_csr2		= rp->si_csr2	      & 0x8000ffff;
		fe_asr		= rp->pbm_fe_asr      & 0x00000007;
		fe_csr		= rp->pbm_fe_csr      & 0x0000ffff;
		host_asr	= rh->rp_host_as_reg  & 0x00000007;
		host_csr	= rh->rp_host_csr_reg & 0x0000007b;
		
		printf("gt su csr:	0x%X\n", su_csr);
		printf("      inp desc:	0x%X\n", su_idesc);
		printf("   ew csr:	0x%X\n", ew_csr);
		printf("   si csr 1:	0x%X\n", si_csr1);
		printf("      csr 2:	0x%X\n", si_csr2);
		printf("   fb fe asr:	0x%X\n", fe_asr);
		printf("      fe csr:	0x%X\n", fe_csr);
		printf("      host asr: 0x%X\n", host_asr);
		printf("      host csr: 0x%X\n", host_csr);

		printf("Nestled in these registers, we find:\n");

		if ((su_csr & 0x00004200)  || (ew_csr & 0x40000000) ||
		    (si_csr1 & 0x40000000) || (fe_csr & 0x00000804)) {
			printf("    interrupts enabled:");
			if (su_csr&0x00004000) printf("  su input full");
			if (su_csr&0x00000200)
				printf("  dsp input sync error");
			if (ew_csr&0x40000000) printf("  ew input seq err");
			if (si_csr1&0x40000000) printf("  si input seq err");
			if (fe_csr & 0x00002000)  printf("  rp semaphore");
			if (fe_csr & 0x00000800)  printf("  pick/hit");
			if (fe_csr & 0x00000004)  printf("  vrt");
			printf("\n");
		}

		if (su_csr & 0x0000b800) {
			printf("    su:");
                	if (su_csr & 0x00002000) printf("  input not full");
                	if (su_csr & 0x00000800) printf("  input data rdy");
                	if (su_csr & 0x00001000) printf("  output data rdy");
			printf("\n");
		}

		if (su_csr & 0x80000000) printf("    su p wait\n");

		if (!(su_idesc & 0x08000000))
			printf("    input fifo stage 1 full\n");
		if (!(su_idesc & 0x04000000))
			printf("    input fifo stage 2 empty\n");


		printf("    dsp 0:");
		if (su_csr & 0x00000001) printf("  run");
		if (su_csr & 0x00000008) printf("  disable");
		if (su_csr & 0x10000000) printf("  wait");
		if (su_csr & 0x00000040) printf("  input sync err");

		if (!(su_idesc & 0x00800000))
			printf("  input fifo stage 2 almost full");
		if (!(su_idesc & 0x00400000))
			printf("  input fifo stage 2 empty");

		if (!(su_idesc & 0x00040000))
			printf("  output fifo full");
		else if (!(su_idesc & 0x00010000))
			printf("  output fifo empty");
		else if (!(su_idesc & 0x00020000))
			printf("  output fifo almost empty");
		printf("\n");


		printf("    dsp 1:");
		if (su_csr & 0x00000002) printf("  run");
		if (su_csr & 0x00000010) printf("  disable");
		if (su_csr & 0x20000000) printf("  wait");
		if (su_csr & 0x00000080) printf("  input sync err");

		if (!(su_idesc & 0x02000000))
			printf("  input fifo stage 2 almost full");
		if (!(su_idesc & 0x01000000))
			printf("  input fifo stage 2 empty");

		if (!(su_idesc & 0x00200000))
			printf("  output fifo full");
		else if (!(su_idesc & 0x00100000))
			printf("  output fifo almost empty");
		else if (!(su_idesc & 0x00080000))
			printf("  output fifo empty");
		printf("\n");


		if (ew_csr & 0x0000007f) {
			printf("    ew:");
			if (ew_csr & 0x00000040) printf("  inp seq err");
			if (ew_csr & 0x00000020) printf("  accepting data");
			if (ew_csr & 0x00000010) printf("  output data rdy");
			if (ew_csr & 0x00000008) printf("  held\n");
			if (ew_csr & 0x00000004) printf("  reset\n");
			if (ew_csr & 0x00000002) printf("  semaphore flag");
			if (ew_csr & 0x00000001) printf("  antialiasing");
			printf("\n");
		}

		if (si_csr1 & 0x3000006b) {
			printf("    si:");
			if (si_csr1 & 0x20000000)
				printf("  x msip almost full");
			if (si_csr1 & 0x10000000)
				printf("  z msip almost full");
			if (si_csr1 & 0x00000040) printf("  input seq err");
			if (si_csr1 & 0x00000020) printf("  accepting data");
			if (si_csr1 & 0x00000008) printf("  held");
			if (si_csr1 & 0x00000004) printf("  reset");
			if (si_csr1 & 0x00000001) printf("  semaphore flag");
			printf("\n");
		}


		if (fe_csr & 0x000017fa) {
			printf("    fe pbm csr:");
			if (fe_csr&0x00001000)  printf("  rp semaphore");
			if (fe_csr&0x00000400)  printf("  pick/hit");
			if (fe_csr&0x00000200)  printf("  pick pending");
			if (fe_csr&0x00000100)  printf("  si loopback rdy");
			if (fe_csr&0x00000080)  printf("  si loopback");
			if (fe_csr&0x00000040)  printf("  pf busy");
			if (fe_csr&0x00000020)  printf("  pb not busy");
			if (fe_csr&0x00000010)  printf("  fe stalling rp");
			if (fe_csr&0x00000008)  printf("  rp stalled");
			if (fe_csr&0x00000002)  printf("  vert retrace");
			printf("\n");
		}

		if (fe_csr & 0x00000001)  printf("    fe fb stateset 1\n");
		else			  printf("    fe fb stateset 0\n");

		if (host_csr & 0x0000007a) {
			printf("    host pbm csr:");
			if (host_csr&0x00000040) printf("  pf busy");
			if (host_csr&0x00000020) printf("  pb not busy");
			if (host_csr&0x00000010) printf("  host stalling rp");
			if (host_csr&0x00000008) printf("  rp stalled");
			if (host_csr&0x00000002) printf("  vert retrace");
			printf("\n");
		}
		if (host_csr & 0x00000001) printf("    host stateset 1\n");
		else			   printf("    host stateset 0\n");

		if (host_csr & 0x00000008) {
			printf("    rp stalled by:");
			if (fe_csr   & 0x00000010)  printf("  fe");
			if (host_csr & 0x00000010)  printf("  host");
			if (fe_csr   & 0x00001000)  printf("  rp semaphore");
			if (fe_csr   & 0x00000400)  printf("  pick/hit");
			if (fe_csr   & 0x00000200)  printf("  pick pending");
			if (!(fe_csr   & 0x00000010) &&
			    !(host_csr & 0x00000010) &&
			    !(fe_csr   & 0x00001000) &&
			    !(fe_csr   & 0x00000400) &&
			    !(fe_csr   & 0x00000200))
				printf("  wait for vertical");
			printf("\n");
		}
	}
#endif GT_DEBUG
}


/*
 * lightpen routines
 */
struct	lightpen_events *lightpen_readbuf   = NULL;
struct	lightpen_events *lightpen_writebuf  = NULL;
struct	lightpen_events  lightpen_readbuf_A = {NULL, 0};
struct	lightpen_events  lightpen_readbuf_B = {NULL, 0};
int lightpen_opened = 0;

store_lightpen_events(type, val)
	unsigned type;
	unsigned val;
{
	struct	lightpen_events_data	*datap, *newptr, *pdatap;
	int	s;

	if (!lightpen_opened)
		return (0);

	/*
	 * Figure out which buffer to write to
	 */
	s = splr(gt_priority);

	lightpen_writebuf = &lightpen_readbuf_A;

	/*
	 * switch buffers?
	 */
	if (lightpen_writebuf->flags & LP_READING)
		lightpen_writebuf = &lightpen_readbuf_B;	/* yes */

	(void) splx(s);

	/*
	 * Traverse the list
	 */
	if (lightpen_writebuf->event_list == NULL) {

		/*
		 * found the free pointer
		 *
		 * XXX: check 2nd arg in new_kmem_alloc().  this is called
		 * from an interrupt thread.
		 */
		 newptr = (struct lightpen_events_data *)
		    new_kmem_alloc(sizeof (*newptr), 0);

		if (newptr == NULL) {
			printf(" Error in store_lightpen_events no memory\n");
			return (1);
		} else {
			newptr->next_event  = NULL;
			newptr->event_type  = type;
			newptr->event_value = val;
			lightpen_writebuf->event_list = newptr;
			lightpen_writebuf->event_count++;

			if ((lightpen_readbuf == NULL) ||
			    (!(lightpen_readbuf->flags & LP_READING)))
				lightpen_readbuf = lightpen_writebuf;
			return (0);
		}
	}

	datap  = lightpen_writebuf->event_list;
	pdatap = datap;

	while (datap != NULL) {
		pdatap = datap;
		datap = datap->next_event;
	}

	/*
	 * Found a null pointer
	 */
	newptr = (struct lightpen_events_data *)
	    new_kmem_alloc(sizeof(*newptr), 0);

	if (newptr == NULL) {
		printf(" Error in store_lightpen_events no memory\n");
		return (1);
	} else {
		newptr->next_event  = NULL;
		newptr->event_type  = type;
		newptr->event_value = val;
		pdatap->next_event  = newptr;
		lightpen_writebuf->event_count++;

		if ((lightpen_readbuf == NULL) ||
		    (!(lightpen_readbuf->flags & LP_READING)))
			lightpen_readbuf = lightpen_writebuf;

		return (0);
	}
}


gt_setup_bwtwo(softc)
	struct gt_softc *softc;
{
	struct rp_host	  *rp;
	struct fbi_reg	  *fb;
	struct fbi_pfgo	  *fb_pfgo;

	rp = (struct rp_host *)   softc->va[MAP_RP_HOST_CTRL];
	fb = (struct fbi_reg *)   softc->va[MAP_FBI_REG];
	fb_pfgo = (struct fbi_pfgo *) softc->va[MAP_FBI_PFGO];

#ifdef	MULTIPROCESSOR
	xc_attention();		/* Prevent users from busying the PF	*/
#endif	MULTIPROCESSOR

	if (gt_check_pf_busy(softc)) {
#ifdef	MULTIPROCESSOR
		xc_dismissed();
#endif	MULTIPROCESSOR
		return (1);
	} else {
		
		gt_cntxrestore(softc, &softc->cntxt);

		/*
		 * Use the PF to initialize the cursor planes, as follows:
		 *
		 *    window write mask:  cd, ce only
		 *    foreground color:   cd = black (0), ce = transparent (0)
		 *    direction/size:     LtoR, 1280 x 1024
		 *    fill dest addr:     window, upper left corner of screen
		 */
		fb->fbi_reg_vwclp_x  = softc->w - 1;
		fb->fbi_reg_vwclp_y  = softc->h - 1;
		fb->fbi_reg_fb_width  = 0x0;
		fb->fbi_reg_buf_sel  = 0x1;
		fb->fbi_reg_stereo_cl  = 0x0;
		fb->fbi_reg_cur_wid  = 0x0;
		fb->fbi_reg_wid_ctrl  = 0x0;
		fb->fbi_reg_con_z  = 0x0;
		fb->fbi_reg_z_ctrl  = 0x0;
		fb->fbi_reg_i_wmask  = 0xffffffff;
		fb->fbi_reg_w_wmask  = 0x0;
		fb->fbi_reg_b_mode  = 0x3;
		fb->fbi_reg_b_wmask  = 0xff;
		fb->fbi_reg_rop  = 0x0c;
		fb->fbi_reg_mpg  = 0xfc;

		fb->fbi_reg_w_wmask  = 0xc00;
		fb->fbi_reg_fg_col   = 0xc00;
		fb->fbi_reg_dir_size = 0x00200500;
		fb_pfgo->fbi_pfgo_fill_dest_addr =  0x00800000;

		if (gt_check_pf_busy(softc)) {
#ifdef	MULTIPROCESSOR
			xc_dismissed();
#endif	MULTIPROCESSOR
			return (1);
		}

		rp->rp_host_csr_reg &= 0xfffffffe;
		rp->rp_host_as_reg = 0x3;
		fb->fbi_reg_buf_sel = 0;
		gt_sync = fb->fbi_reg_buf_sel;

#ifdef	MULTIPROCESSOR
		xc_dismissed();
#endif	MULTIPROCESSOR

		return (0);
	}
}
