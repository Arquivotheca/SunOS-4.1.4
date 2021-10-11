#ifndef lint
static char sccsid[] = "@(#)gtconfig.c  1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <nlist.h>
#include <math.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntlcom.h>
#include <fcntl.h>
#include <gtreg.h>
#include <fbio.h>
#include <gtconfig.h>

#include <filehdr.h>
#include <n10aouth.h>
#include <scnhdr.h>
#include <reloc.h>
#include <linenum.h>
#include <storclass.h>

#define	TRUE	1
#define FALSE	0

#define MMAP_SIZE	0x2000000	/* map in the all sections	*/
#define NWID_BITS	10		/* total number of WID planes	*/


#define GT_RESET_TIME		 1000		/*  1 millisecond   */
#define GT_RESET_HOLDOFF	10000		/* 10 milliseconds  */
#define TEN_MSEC		10000		/* 10 milliseconds  */
#define FORTY_MSEC		40000		/* 40 milliseconds  */

#define GT_PF_BUSY      0x00000040


#define GAMMA(i,g) ((int)rint(255.0 * pow((double) (i) / 255.0, 1.0 / (g))))


#define VID_CSR0_ADDR 	   0x30320020
#define SENSEBITS_MASK 	   0x00000f00 /* Mask for Sense Bits */
#define SENSEBITS_0_SUN_21 0x00000300 /* MIDS = 0 MON_ID_L = 011 */
#define SENSEBITS_1_SUN_21 0x00000d00 /* MIDS = 1 MON_ID_L = 101 */
#define SENSEBITS_0_SUN_19 0x00000600 /* MIDS = 0 MON_ID_L = 110 */
#define SENSEBITS_1_SUN_19 0x00000b00 /* MIDS = 1 MON_ID_L = 011 */
#define SENSEBITS_0_SUN_16 0x00000200 /* MIDS = 0 MON_ID_L = 010 */
#define SENSEBITS_1_SUN_16 0x00000900 /* MIDS = 1 MON_ID_L = 001 */



/*
 * Globals
 */
short verbose_flag;
short have_mappings;


/*
 * include SG_1152
 */
fbinit_type sg_1152_66_table[] = {
    {0x30090000, 3},		/* AS = 3		*/
    {0x30090004, 0},		/* state set = 0	*/
    /*
     * load the mode regs
     */
    {0x30320020, 0},		/* init VID mode 0	*/
    {0x30320024, 1},		/* init VID mode 1	*/

    {0, 100},			/* sleep for 100 ms.	*/

    /*
     * load VAG registers
     */
    {0x30320000, 0},		/* init SSA 0		*/
    {0x30320004, 0},		/* init SSA 1		*/
    {0x30320008, 25},		/* init LIN_OS for FB 3	*/

    /*
     * load the horizontal program regs
     */
    {0x30320080, 0x7063},	/* {END_PVH, 'd99}	*/
    {0x30320084, 0xc0d8},	/* {END_CS_BR, 'd216}	*/
    {0x30320088, 0x00dc},	/* {BEG_PHB, 'd220}	*/
    {0x3032008c, 0x40e8},	/* {BEG_HS, 'd236}	*/
    {0x30320090, 0x60f5},	/* {BEG_PV_H, 'd250}	*/
    {0x30320094, 0x5105},	/* {END_HS, 'd261}	*/
    {0x30320098, 0x1122},	/* {END_PHB, 'd290}	*/
    {0x3032009c, 0},		/* unused register	*/

    /*
     * load the vertical program regs
     */
    {0x303200c0, 0x0383},	/* {BEG_VB, 'd899}	*/
    {0x303200c4, 0x2385},	/* {BEG_VS, 'd901}	*/
    {0x303200c8, 0x3389},	/* {END_VS, 'd905}	*/
    {0x303200cc, 0x43a7},	/* {BEG_PVV, 'd935}	*/
    {0x303200d0, 0x13a8},	/* {END_VB, 'd936}	*/
    {0x303200d4, 0},		/* unused register	*/
    {0x303200d8, 0},		/* unused register	*/

    /*
     * start the program
     */
    {0x30320020, 0x1b},	/* VID mode 0: SETUP,S_O_V,SYNC_ON,SYNC_RUN */
};


/*
 * 1280_76
 *
 * Initialize video format for 1280 x 1024 @ 76 Hz
 */
fbinit_type sg_1280_76_table[] = {
    {0x30090000, 3},		/* AS = 3		*/
    {0x30090004, 0},		/* state set = 0	*/
    /*
     * load the mode regs
     */
    {0x30320020, 0},		/* init VID mode 0	*/
    {0x30320024, 0},		/* init VID mode 1	*/

    {0, 100},			/* sleep for 100 ms.	*/

    /*
     * load VAG registers
     */
    {0x30320000, 0},		/* init SSA 0		*/
    {0x30320004, 0},		/* init SSA 1		*/
    {0x30320008, 0},		/* init LIN_OS		*/

    /*
     * load the horizontal program regs
     */
    {0x30320080, 0x707f},	/* {END_PVH,	 12'd640/5 -1}		*/
    {0x30320084, 0x00f5},	/* {BEG_PHB,	 12'd1280/5 -1 -10}	*/
    {0x30320088, 0xc0fa},	/* {END_CS_BR,	 12'd1255/5 -1}		*/
    {0x3032008c, 0x4105},	/* {BEG_HS, 	 12'd1310/5 -1}		*/
    {0x30320090, 0x5112},	/* {END_HS,       12'd1375/5 -1}	*/
    {0x30320094, 0x6117},	/* {BEG_PVH,     12'd1400/5 -1}		*/
    {0x30320098, 0x1142},	/* {END_PHB,	 12'd1665/5 -1 -10}	*/
    {0x3032009c, 0},		/* unused register			*/

    /*
     * load the vertical program regs
     */
    {0x303200c0, 0x03ff},	/* {BEG_VB,	 12'd1023}	*/
    {0x303200c4, 0x2401},	/* {BEG_VS,	 12'd1025}	*/
    {0x303200c8, 0x3409},	/* {END_VS,	 12'd1033}	*/
    {0x303200cc, 0x4428},	/* {BEG_PVV,	 12'd1064}	*/
    {0x303200d0, 0x1429},	/* {END_VB,	 12'd1065}	*/
    {0x303200d4, 0},		/* unused register		*/
    {0x303200d8, 0},		/* unused register		*/

    /*
     * start the program
     */
    {0x30320020, 0x0b},	/* VID mode 0: S_O_V,SYNC_ON,SYNC_RUN */
};

/*
 * 1280_67
 *
 * Initialize video format to 1280 x 1024 @ 67 Hz
 */
fbinit_type sg_1280_67_table[] = {
    {0x30090000, 3},		/* AS = 3		*/
    {0x30090004, 0},		/* state set = 0	*/
    /*
     * load the mode regs
     */
    {0x30320020, 0},		/* init VID mode 0			*/
    {0x30320024, 1},		/* init VID mode 1 for alternate	*/
				/* 117 MHz oscillator			*/

    {0, 100},			/* sleep for 100 ms.	*/

    /*
     * load VAG registers
     */
    {0x30320000, 0},		/* init SSA 0		*/
    {0x30320004, 0},		/* init SSA 1		*/
    {0x30320008, 0},		/* init LIN_OS		*/

    /*
     * load the horizontal program regs
     */
    {0x30320080, 0x7063},	/* {END_PVH,	 12'd500/5 -1}		*/
    {0x30320084, 0xc0ec},	/* {END_CS_BR,	 12'd1200/5 -1}		*/
    {0x30320088, 0x00f5},	/* {BEG_PHB,	 12'd1280/5 -1 -10}	*/
    {0x3032008c, 0x4102},	/* {BEG_HS, 	 12'd1295/5 -1}		*/
    {0x30320090, 0x6110},	/* {BEG_PVH,       12'd1365/5 -1}	*/
    {0x30320094, 0x5118},	/* {END_HS,     12'd1405/5 -1}		*/
    {0x30320098, 0x113b},	/* {END_PHB,	 12'd1630/5 -1 -10}	*/
    {0x3032009c, 0},		/* unused register			*/

    /*
     * load the vertical program regs
     */
    {0x303200c0, 0x03ff},	/* {BEG_VB,	 12'd1023}	*/
    {0x303200c4, 0x2405},	/* {BEG_VS,	 12'd1025}	*/
    {0x303200c8, 0x340d},	/* {END_VS,	 12'd1033}	*/
    {0x303200cc, 0x4431},	/* {BEG_PVV,	 12'd1065}	*/
    {0x303200d0, 0x1432},	/* {END_VB,	 12'd1066}	*/
    {0x303200d4, 0},		/* unused register		*/
    {0x303200d8, 0},		/* unused register		*/

    /*
     * start the program
     */
    {0x30320020, 0x0b},	/* VID mode 0: S_O_V,SYNC_ON,SYNC_RUN */
};

/*
 * stereo
 *
 * Initialize video format 960 x 680 @ 108 HZ (78 KHz)
 */
fbinit_type sg_stereo_table[] = {
    {0x30090000, 3},		/* AS = 3				*/
    {0x30090004, 0},		/* state set = 0			*/

    /*
     * load the mode registers
     */
    {0x30320020, 0},		/* init VID CSR0			*/
    {0x30320024, 0x2002},	/* init VID CSR1 for stereo and 3rd osc. */

    {0, 100},			/* sleep for 100 ms.			*/

    /*
     * load VAG registers
     */
    {0x30320000, 0},		/* init SSA 0				*/
    {0x30320004, 0x20000},	/* init SSA 1				*/

    {0x30320008, 0},		/* init LIN_OS				*/

    /*
     * load the horizontal program regs
     */
    {0x30320080, 0x705f},	/* {END_PVH,      12'd480/5 -1}		*/
    {0x30320084, 0x00b5},	/* {BEG_PHB,      12'd960/5 -1 -10}	*/
    {0x30320088, 0xc0bf},	/* {END_CS_BR,    12'd960/5 -1}		*/
    {0x3032008c, 0x40cc},	/* {BEG_HS,       12'd1025/5 -1}	*/
    {0x30320090, 0x60d1},	/* {BEG_PVH,      12'd1050/5 -1}	*/
    {0x30320094, 0x50d9},	/* {END_HS,       12'd1090/5 -1}	*/
    {0x30320098, 0x10f5},	/* {END_PHB,      12'd1280/5 -1 -10}	*/
    {0x3032009c, 0},		/* unused register			*/

    /*
     * load the vertical program regs
     */
    {0x303200c0, 0x02a7},	/* {BEG_VB,       12'd679        }	*/
    {0x303200c4, 0x22aa},	/* {BEG_VS,       12'd682        }	*/
    {0x303200c8, 0x32ae},	/* {END_VS,       12'd685        }	*/
    {0x303200cc, 0x42ce},	/* {BEG_PVV,      12'd718        }	*/
    {0x303200d0, 0x12cf},	/* {END_VB,       12'd719        }	*/
    {0x303200d4, 0},		/* unused register			*/
    {0x303200d8, 0},		/* unused register			*/

    /*
     * load ref. counter
     */
    {0x30320060, 42},		/* Hsync off	*/
    {0x30320064, 129},		/* Hsync on	*/

    /*
     * start the program
     */
    {0x30320020, 0x0b},		/*  VID CSR0: S_O_V,SYNC_ON,SG_EN*/

    {0x30090004, 1},		/* state set 1 				*/

    {0x302000c8, 3},		/* stereo 960 x 680			*/
};

/*
 * include HDTV
 */
fbinit_type sg_hdtv_table[] = {
    {0x30090000, 3},		/* AS = 3		*/
    {0x30090004, 0},		/* state set = 0	*/
	/* XXX: fill in the values later */
};

/*
 * PAL
 *
 * Initialize video format to 770 x 576 @ 50.0 Hz
 */
fbinit_type sg_pal_table[] = {
    {0x30090000, 3},		/* AS = 3		*/
    {0x30090004, 0},		/* state set = 0	*/

    /*
     * load the mode registers
     *
     *   VID_CSR0: 0
     *   VID_CSR1: 135/9 MHz, interlace, sync_clk = pix_clk
     */
    {0x30320020, 0},
    {0x30320024, 0x1180},

    /*
     * load VAG registers
     */
    {0x30320000, 00},		/* init SSA 0		*/
    {0x30320004, 256},		/* init SSA 1		*/
    {0x30320008, 358},		/* init LIN_OS		*/

    /*
     * load the horizontal program regs
     */
    {0x30320080, 0x7000},	/* {END_PVH,      12'd000 -1}		*/
    {0x30320084, 0x00c7},	/* {BEG_PHB,      12'd290 -1 -50}	*/
    {0x30320088, 0xc0ec},	/* {END_CS_BR,    12'd255 -1}		*/
    {0x3032008c, 0x410a},	/* {BEG_HS,       12'd325 -1}		*/
    {0x30320090, 0xd133},	/* {END_CS_EQ,    12'd360 -1}		*/
    {0x30320094, 0x6139},	/* {BEG_PVH,      12'd380 -1}		*/
    {0x30320098, 0x5143},	/* {END_HS,       12'd395 -1}		*/
    {0x3032009c, 0x1153},	/* {END_PHB,      12'd480 -1 -50}	*/

    /*
     * load the vertical program regs
     */
    {0x303200c0, 0x01e0},	/* {BEG_VB,       12'd575}	*/
    {0x303200c0, 0x21e6},	/* {BEG_VS,       12'd580}	*/
    {0x303200c0, 0x31ec},	/* {END_VS,       12'd585}	*/
    {0x303200c0, 0x51f2},	/* {END_EQ2,      12'd590}	*/
    {0x303200c0, 0x420a},	/* {BEG_PVV,      12'd622}	*/
    {0x303200c0, 0x120c},	/* {END_VB,       12'd624}	*/
    {0x303200c0, 0},		/* unused register		*/

    /*
     * load ref. counter
     */
    {0x30320060, 511},		/* Hsync off	*/
    {0x30320064, 351},		/* Hsync on	*/

    /*
     * start the program
     *
     *   VID_CSR0: SETUP, S_O_V, SYNC_ON, SG_EN
     */
    {0x30320020, 0x1b},
};

/*
 * NTSC
 *
 * Initialize video format to 640 x 480 @ 59.94 Hz
 */
fbinit_type sg_ntsc_table[] = {
    {0x30090000, 3},		/* AS = 3		*/
    {0x30090004, 0},		/* state set = 0	*/

    /*
     * Load the mode registers
     *
     *   VID_CSR0: 0
     *   VID_CSR1: 135/11 MHz, interlace, sync_clk = pix_clk
     */
    {0x30320020,  0},
    {0x30320024, 0x11a0},

    /*
     * load the VAG registers
     */
    {0x30320000, 0},		/* init SSA 0		*/
    {0x30320004, 256},		/* init SSA 1		*/
    {0x30320008, 384},		/* init LIN_OS		*/

    /*
     * load the horizontal program registers
     */
    {0x30320080, 0x7000},	/* {END_PVH,      12'd000 -1}		*/
    {0x30320084, 0x00c7},	/* {BEG_PHB,      12'd250 -1 -50}	*/
    {0x30320088, 0xc0ec},	/* {END_CS_BR,    12'd237 -1}		*/
    {0x3032008c, 0x410a},	/* {BEG_HS,       12'd266 -1}		*/
    {0x30320090, 0xd133},	/* {END_CS_EQ,    12'd308 -1}		*/
    {0x30320094, 0x6139},	/* {BEG_PVH,      12'd314 -1}		*/
    {0x30320098, 0x5143},	/* {END_HS,       12'd324 -1}		*/
    {0x3032009c, 0x1153},	/* {END_PHB,      12'd390 -1 -50}	*/

    /*
     * load the vertical program registers
     */
    {0x303200c0, 0x01e0},	/* {BEG_VB,       12'd480}		*/
    {0x303200c4, 0x21e6},	/* {BEG_VS,       12'd486}		*/
    {0x303200c8, 0x31ec},	/* {END_VS,       12'd492}		*/
    {0x303200cc, 0x51f2},	/* {END_EQ2,      12'd498}		*/
    {0x303200d0, 0x420a},	/* {BEG_PVV,      12'd522}		*/
    {0x303200d4, 0x120c},	/* {END_VB,       12'd524}		*/
    {0x303200d8, 0},		/* unused register			*/

    /*
     * load ref. counter
     */
    {0x30320060, 511},		/* Hsync off		*/
    {0x30320064, 345},		/* Hsync on		*/

    /*
     * start the program
     *
     *   VID_CSR0: SETUP, S_O_V, SYNC_ON, SG_EN
     */
    {0x30320020, 0x1b},
};

/*
 * include InitGammaDAC
 */
fbinit_type fbinit_table[] = {

    /*
     * load all 3 read masks = 1 (enable image/overlay planes)
     */
    {0x30330030, 4},
    {0x30330038, 0xff},

    /*
     * load all 3 blink masks = 00 (no blink)
     */
    {0x30330030, 5},
    {0x30330038, 0},

    /*
     * load all 3 cmd regs = 0xe3 (5:1,transparent overlay,
     * med blink rate, no overlay blink, enable overlay data)
     */
    {0x30330030, 6},
    {0x30330038, 0xe3},

    /*
     * # identify each ramdac as red, green or blue
     */
    {0x30330030, 7},
    {0x30330008, 1},
    {0x30330018, 2},
    {0x30330028, 4},
};

#define NUM_FBINIT_TABLE (sizeof(fbinit_table)     / sizeof(fbinit_type))


/*
 * This table must be kept in the order specified by the define
 * constants in gtreg.h:  GT_1280_76, GT_1280_67, ...
 */
struct monitor_setup {
	struct fbinit_type *ms_setup;
	int ms_count;
} monitor_setup[] = {
    {sg_1280_76_table,	(sizeof(sg_1280_76_table) / sizeof(fbinit_type))},
    {sg_1280_67_table,	(sizeof(sg_1280_67_table) / sizeof(fbinit_type))},
    {sg_1152_66_table,	(sizeof(sg_1152_66_table) / sizeof(fbinit_type))},
    {sg_stereo_table,	(sizeof(sg_stereo_table)  / sizeof(fbinit_type))},
    {sg_hdtv_table,	(sizeof(sg_hdtv_table)    / sizeof(fbinit_type))},
    {sg_ntsc_table,	(sizeof(sg_ntsc_table)    / sizeof(fbinit_type))},
    {sg_pal_table,	(sizeof(sg_pal_table)     / sizeof(fbinit_type))},
};

typedef struct scnhdr  scnhdr_t;

extern int   errno;
extern char *gt_errmsg;
char	*progname;
int	usageflag;

char *usage[] = {
    "[ -d <device filename> ]\n",
    "                [ -f <filename> -s0 <filename> -s1 <filename> ]\n",
    "                [ -I <microcode directory> ]\n",
    "                [ -g|G <gamma value> \n",
    "                [ -degamma8|DEGAMMA8  on|off\n",
    "                [ -i ]\n",
    "                [ -E 0|1 ]\n",
    "                [ -m|M 1280_76 1280_67 stereo]\n",
    "                [ -c <1-14> ]\n",
    "                [ -w <2-9> ]\n",
    "                [ -v ]\n",
    0,
};


char *msg_perror	= "error (%s)";
char *msg_open		= "open error on file %s";
char *msg_read		= "read error on file %s";
char *msg_mmap		= "mmap error";
char *msg_malloc	= "malloc error";
char *msg_diag		= "setdiagmode ioctl error";
char *msg_loadkmcb	= "load kmcb ioctl error";
char *msg_setwpart	= "fb_setwpart ioctl error";
char *msg_setclutpart	= "fb_setclutpart ioctl error";
char *msg_clutpost	= "fb_clutpost ioctl error";
char *msg_setmonitor	= "fb_setmonitor ioctl error";
char *msg_loadtext	= "error loading microcode text segment";
char *msg_loaddata	= "error loading microcode data segment";
char *msg_loadabs	= "error loading microcode absolute segment";
char *msg_cmptext	= "text compare error";
char *msg_cmpdata	= "data compare error";
char *msg_cmpabs	= "abs compare error";
char *msg_invalid_s	= "invalid -s option";
char *msg_invalid_w	= "invalid -w option";
char *msg_invalid_c	= "invalid -c option";
char *msg_badopt	= "invalid option (%s)";
char *msg_fb_freq	= "invalid monitor specification (%s)";
char *msg_dsp		= "invalid DSP specification (%s)";
char *msg_filesize	= "file (%s) too large";
char *msg_degamma	= "warning: cannot store de-gamma table\n";
char *msg_gammaval	= "warning: cannot store gamma value\n";
char *msg_getgamma	= "cannot fetch gamma value\n";
char *msg_forceinit	= "forcing initialization: SYNC_RUN not set\n";
char *msg_pfbusy	= "GT won't go unbusy\n";
char *msg_noversion	= "cannot send microcode version number to kernel\n";

char *vmsg_open		= "opening %s device\n";
char *vmsg_fe_ucode	= "loading frontend microcode = %s\n";
char *vmsg_su0_ucode	= "loading SU 0 microcode = %s\n";
char *vmsg_su1_ucode	= "loading SU 1 microcode = %s\n";
char *vmsg_init_ldm	= "initializing LDM space\n";
char *vmsg_hw_reset	= "resetting hardware\n";
char *vmsg_init_fb	= "initializing framebuffer\n";
char *vmsg_sync_gen	= "initializing sync generator for %s\n";
char *vmsg_gamma	= "initializing gamma table with %5.2f\n";
char *vmsg_enable	= "initializing cursor enable plane to %d\n";
char *vmsg_version	= "version: %d.%d.%d\n";
char *vmsg_master_hs	= "starting master handshake\n";

#ifdef GT_PREFCS
char *vmsg_execver	= "exec version: %1.2f\n";
#endif GT_PREFCS


char *sect;
unsigned off, expected, was;

char fe_ucode_file[MAXPATHLEN];
char su0_ucode_file[MAXPATHLEN];
char su1_ucode_file[MAXPATHLEN];

struct nlist nl[] = {
	{"_wcs_date"},
	{"_wcs_version"},
	{"_exec_version"},
	{"_kernel_mcb_ptr"},
	{NULL}
};

struct gt_version gt_version;

struct	gt_ctrl gt_ctrl;


/*
 * Defaults, may be overridden by command line options
 * and, in pre-fcs systems, environment variables
 */
#define DEFAULT_FBNAME		"/dev/gt0"
#define DEFAULT_PATH		"/usr/lib"
#define DEFAULT_FE_UCODE	"gt.ucode"
#define DEFAULT_SU_UCODE0	"gt.c30.ucode"
#define DEFAULT_SU_UCODE1	"gt.c31.ucode"
#define	DEFAULT_FB_FREQ		"1280_76"
#define	DEFAULT_DSP		"01"
#define	DEFAULT_MONITOR		-1		/* default: set by OpenBoot */
#define DEFAULT_VASCO		TRUE
#define DEFAULT_GAMMA		(double)2.22
#define DEFAULT_MAX_WIDS	5
#define DEFAULT_MAX_CLUTS	8
#define	DEFAULT_CURSOR_ENABLE	0

struct cmd {
	unsigned flags;			/* command line overrides	*/
	char	*device_fbname;		/* device filename		*/
	char	*fe_ucode_fname;	/* frontend microcode file	*/
	char 	*su0_ucode_fname;	/* SU processor #0 file		*/
	char 	*su1_ucode_fname;	/* SU processor #1 file		*/
	char	*directory;		/* microcode directory		*/
	char	*su_directory;		/* SU microcode directory	*/
	char	*fb_freq;		/* sync generator type		*/
	char	*nbr_of_dsp;		/* identity of SU DSPs		*/
	int	 monitor_type;		/* monitor type			*/
	u_int	 vasco;			/* automatic degamma 8-bit CLUTs*/
	double	 gamma_value;		/* gamma correction value	*/
	u_int	 nbr_wid_bits;		/* max WIDs used by OW server	*/
	u_int	 nbr_owserver_cluts;	/* max CLUTS used by OW server	*/
	u_int	 cursor_enable;		/* cursor enable initialization	*/
} cmd = {
	0,
	DEFAULT_FBNAME,
	DEFAULT_FE_UCODE,
	DEFAULT_SU_UCODE0,
	DEFAULT_SU_UCODE1,
	DEFAULT_PATH,
	DEFAULT_PATH,
	DEFAULT_FB_FREQ,
	DEFAULT_DSP,
	DEFAULT_MONITOR,
	DEFAULT_VASCO,
	DEFAULT_GAMMA,
	DEFAULT_MAX_WIDS,
	DEFAULT_MAX_CLUTS,
	DEFAULT_CURSOR_ENABLE,
};

/*
 * flags
 */
#define CMD_FILENAME	0x00000001
#define	CMD_S0		0x00000002
#define CMD_S1		0x00000004
#define CMD_PATH	0x00000008
#define CMD_SUPATH	0x00000010
#define CMD_GAMMA	0x00000020
#define CMD_GAMMAONLY	0x00000040
#define CMD_INITIALIZE	0x00000080
#define CMD_MONITOR	0x00000100
#define CMD_MONITORONLY	0x00000200
#define CMD_CLUTQUOTA	0x00000400
#define CMD_WIDQUOTA	0x00000800
#define CMD_DSP		0x00001000
#define	CMD_CE_INIT	0x00002000
#define	CMD_CE_ONLY	0x00004000
#define CMD_VASCO	0x00008000
#define CMD_VASCO_ONLY	0x00010000
#define CMD_USE_DSP0	0x40000000
#define CMD_USE_DSP1	0x80000000

#define CMD_USE_GAMMA (CMD_GAMMA | CMD_GAMMAONLY)
#define CMD_USE_VASCO (CMD_VASCO | CMD_VASCO_ONLY)

#define SURAM_BASE  0x00084000              /* SU shared ram */
#define SURAM_SIZE  0x0800                  /* Size (in words) of ram */


main(argc, argv)
	int    argc;
	char **argv;
{
	fbo_screen_ctrl *fbo;

	/*
	 * Save program name, cook command line options and,
	 * in pre-fcs versions, environment variables.
	 */
	if (rindex(*argv, '/'))
		progname = rindex(*argv, '/') + (char *)1;
	else
		progname = *argv;

	usageflag++;
	gt_options(argc, argv);

#ifdef GT_PREFCS
	gt_getenv();
#endif GT_PREFCS

	gt_cookoptions();
	usageflag = 0;

	/*
	 * Open the GT and set up diagnostic mode and addressing.
	 */
	gt_open();

	/*
	 * If backend initialization hasn't been specified, we
	 * verify that SYNC_RUN is set, and force backend
	 * initialization if it isn't set.
	 */
	if (!(cmd.flags & CMD_INITIALIZE)) {
		if (!(gt_ctrl.fbo_screen_ctrl_regs->fbo_videomode_0 &
		    SYNC_RUN)) {
			gt_fprintf(stderr, msg_forceinit);
			cmd.flags |= CMD_INITIALIZE;
		}
	}

	if (!(cmd.flags & CMD_INITIALIZE)) {
		/*
		 * If we are only changing the gamma table, the monitor
		 * type or initializing the cursor enable plane or mucking
		 * with the gamma tables, do it now and exit.
		 */
		if (cmd.flags & CMD_GAMMAONLY) {
			init_gamma_table();
			exit(0);
		}
		
		if (cmd.flags & CMD_MONITORONLY) {
			gt_setmonitor(1);

			gt_ctrl.fbo_screen_ctrl_regs->fbo_videomode_0 |=
			    VIDEO_ENABLE;

			exit(0);
		}

		if (cmd.flags & CMD_VASCO_ONLY) {
			(void) gt_setgamma();
			exit(0);
		}

		if (cmd.flags & CMD_CE_ONLY) {
			wait_for_pf();
			init_cursor_enable(0x400, cmd.cursor_enable);
			wait_for_pf();
			exit(0);
		}
	}


	/*
	 * Initialize the GT backend if requested.  Otherwise, initialize
	 * the frontend only.
	 */
	if (cmd.flags & CMD_INITIALIZE) {
		gt_init_hw();
	} else {
		gt_ctrl.fe_ctrl_regs->fe_hcr &= ~FE_GO;
		gt_ctrl.fe_ctrl_regs->fe_hcr |=  FE_RESET;
		gt_ctrl.fe_ctrl_regs->fe_hcr &= ~FE_RESET;
	}

	/*
	 * Initialize the frame buffer
	 */
	gt_init_framebuffer();


	/*
	 * Reset the SU section and download the SU microcode
	 */
	gt_su_reset();
	gt_download_su_code();
	

	/*
	 * Download the FE microcode
	 */
	gt_download_fe();

	gt_get_version();

	/*
	 * Finish FE initialization
	 */
	gt_initialize_fe();

	exit(0);
}


/*
 * process the command line options
 */
static
gt_options(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*arg;

	errno = 0;

	while (--argc > 0 && *++argv != NULL) {

		arg = *argv;
		if (*arg == '-') {
			switch (*++arg) {

			case 'f':
				cmd.fe_ucode_fname = *++argv;
				cmd.flags |= CMD_FILENAME;
				break;

			case 's':
				if (*++arg == '0') {
					cmd.su0_ucode_fname = *++argv;
					cmd.flags |= CMD_S0;
				} else if (*arg == '1') {
					cmd.su1_ucode_fname = *++argv;
					cmd.flags |= CMD_S1;
				} else
					gt_perror(msg_invalid_s);
				break;

			case 'E':
				cmd.flags |= CMD_CE_ONLY;
				cmd.flags |= CMD_CE_INIT;

				if (*(argv+1) != NULL &&
				    (**(argv+1) == '0' || **(argv+1) == '1'))
					cmd.cursor_enable = atoi(*++argv);

				break;

			case 'G':
				cmd.flags |= CMD_GAMMAONLY;
			case 'g':
				cmd.gamma_value = (double) atof(*++argv);
				cmd.flags |= CMD_GAMMA;
				break;

			case 'd':
			case 'D':
				if (strcmp(arg, "d") == 0) {
					cmd.device_fbname = *++argv;
					break;

				} else if (strcmp(arg, "D") == 0) {
					cmd.nbr_of_dsp = *++argv;
					cmd.flags |= CMD_DSP;
					break;

				} else if (strcmp(arg, "degamma8") == 0)
					cmd.flags |= CMD_VASCO;

				else if (strcmp(arg, "DEGAMMA8") == 0)
					cmd.flags |= CMD_VASCO_ONLY;


				if (cmd.flags & (CMD_VASCO|CMD_VASCO_ONLY)) {
					if (*(argv+1) == NULL)
						break;

					if (strcmp(*(++argv), "on") == 0)
						cmd.vasco = TRUE;

					else if (strcmp(*(argv), "off") == 0)
						cmd.vasco = FALSE;
				}

				break;

			case 'I':
				cmd.directory    = *++argv;
				cmd.su_directory = cmd.directory;
				cmd.flags |= CMD_PATH;
				break;

			case 'M':
				cmd.flags  |= CMD_MONITORONLY;
			case 'm':
				cmd.fb_freq = *++argv;
				cmd.flags  |= CMD_MONITOR;
				break;

			case 'i':
				cmd.flags |= CMD_INITIALIZE;
				break;

			case 'w':	
				cmd.nbr_wid_bits = atoi(*++argv);
				cmd.flags |= CMD_WIDQUOTA;
				break;

			case 'c':	
				cmd.nbr_owserver_cluts = atoi(*++argv);
				cmd.flags |= CMD_CLUTQUOTA;
				break;

			case 'v':
				verbose_flag = 1;
				break;

			default:
				gt_perror(msg_badopt, arg);

			}
		} else {
			gt_perror(msg_badopt, arg);
		}
	}
}


#ifdef GT_PREFCS
/*
 * Fetch any relevant environment variables
 */
static
gt_getenv()
{
	char *ptr;
	extern char *getenv();

	if (!(cmd.flags & CMD_PATH))
		if ((ptr=getenv("HFE_UCODE_PATH")) != NULL)
			cmd.directory = ptr;

	if (!(cmd.flags & CMD_FILENAME))
		if ((ptr=getenv("HFE_UCODE")) != NULL)
			cmd.fe_ucode_fname = ptr;

	if (!(cmd.flags & CMD_S0))
		if ((ptr=getenv("HSU_UCODE0")) != NULL)
			cmd.su0_ucode_fname = ptr;

	if (!(cmd.flags & CMD_S1))
		if ((ptr=getenv("HSU_UCODE1")) != NULL)
			cmd.su1_ucode_fname = ptr;

	if (!(cmd.flags & CMD_GAMMA))
		if ((ptr=getenv("HK_GAMMA")) != NULL)
			cmd.gamma_value = (double)atof(ptr);

	if (!(cmd.flags & CMD_MONITOR))
		if ((ptr=getenv("FB_FREQ")) != NULL)
			cmd.fb_freq = ptr;

	if (!(cmd.flags & CMD_DSP))
		if ((ptr=getenv("DSP")) != NULL)
			cmd.nbr_of_dsp = ptr;

	if ((ptr=getenv("HSU_UCODE_PATH")) != NULL)
		cmd.su_directory = ptr;
}
#endif GT_PREFCS


/*
 * After having processed the command line and any relevant
 * environment variables, cook the results into a coherent
 * gtconfig specification.
 */
static
gt_cookoptions()
{
	char *ptr;

	/*
	 * Construct full pathnames for the microcode files
	 */
	gt_makepath(fe_ucode_file,  cmd.fe_ucode_fname,  cmd.directory);
	gt_makepath(su0_ucode_file, cmd.su0_ucode_fname, cmd.su_directory);
	gt_makepath(su1_ucode_file, cmd.su1_ucode_fname, cmd.su_directory);
 
	/*
	 * Set flags to use the desired DSPs
	 */
	ptr = cmd.nbr_of_dsp;

	while (*ptr != '\0') {
		switch (*ptr) {

		case '0':
			cmd.flags |= CMD_USE_DSP0;
			break;

		case '1':
			cmd.flags |= CMD_USE_DSP1;
			break;

		default:
			gt_perror(msg_dsp, cmd.nbr_of_dsp);
		}

		ptr++;
	}

	/*
	 * Process the monitor type
	 */
#ifdef GT_PREFCS
	if ((strcmp(cmd.fb_freq, "1152_66") == 0) ||
	    (strcmp(cmd.fb_freq, "1152") == 0))
		cmd.monitor_type = GT_1152_66;
	else
#endif GT_PREFCS

	if (strcmp(cmd.fb_freq, "1280_76") == 0)
		cmd.monitor_type = GT_1280_76;

	else if (strcmp(cmd.fb_freq, "1280_67") == 0)
		cmd.monitor_type = GT_1280_67;

	else if (strcmp(cmd.fb_freq, "stereo") == 0)
		cmd.monitor_type = GT_STEREO;

	else if (strcmp(cmd.fb_freq, "ntsc") == 0)
		cmd.monitor_type = GT_NTSC;

	else if (strcmp(cmd.fb_freq, "pal") == 0)
		cmd.monitor_type = GT_PAL;

	else if (strcmp(cmd.fb_freq, "hdtv") == 0)
		cmd.monitor_type = GT_HDTV;

   	else
		gt_perror(msg_fb_freq, cmd.fb_freq);

	/*
	 * A couple of sanity checks
	 */
	if (cmd.nbr_wid_bits < 2 || cmd.nbr_wid_bits > 9)
		gt_perror(msg_invalid_w);

	if (cmd.nbr_owserver_cluts < 1 || cmd.nbr_owserver_cluts > 14)
		gt_perror(msg_invalid_c);
}


/*
 * Construct absolute pathnames from components
 */
gt_makepath(path, file, directory)
	char *path;
	char *file;
	char *directory;
{
	if (*file == '/')
		strcpy(path, file);
	else {
		strcpy(path, directory);
		strcat(path, "/");
		strcat(path, file);
	}
}


/*
 * Open the frame buffer, set diagnostic mode,
 * and establish addressing to the GT.
 */
gt_open()
{
	unsigned data;
	u_char *vaddr;

	verbose(vmsg_open, cmd.device_fbname);

	if ((gt_ctrl.fd = open(cmd.device_fbname, O_RDWR, 0)) < 0) {
		if (verbose_flag) {
			gt_perror(msg_open, cmd.device_fbname);
		} else
			exit(1);
	}


	if (ioctl(gt_ctrl.fd, FB_SETDIAGMODE, (caddr_t) &data) < 0)
		gt_perror(msg_diag);

	vaddr = (u_char *)mmap((caddr_t)0, MMAP_SIZE, PROT_READ|PROT_WRITE,
	    MAP_PRIVATE, gt_ctrl.fd, 0);

	if (vaddr == (u_char *) -1)
		gt_perror(msg_mmap);

	/*
	 * Initialize pointers in the global control structure
	 */
	gt_ctrl.baseaddr = vaddr;
	gt_ctrl.rp_host_regs	  = (rp_host *)    (vaddr + RP_HOST_CTRL_OF);
	gt_ctrl.fbi_regs	  = (fbi_reg *)    (vaddr + FBI_REG_OF);
	gt_ctrl.fbi_pfgo_regs	  = (fbi_pfgo *)   (vaddr + FBI_PFGO_OF);
	gt_ctrl.fe_ctrl_regs	  = (fe_ctrl_sp *) (vaddr + FE_CTRL_OF);
	gt_ctrl.fe_i860_ctrl_regs = (fe_i860_sp *) (vaddr + FE_I860_OF);
	gt_ctrl.fbo_screen_ctrl_regs =
	    (fbo_screen_ctrl *) (vaddr + FBO_SCREEN_CTRL_OF);

	have_mappings = 1;
}


/*
 * Download the GT frontend microcode
 */
gt_download_fe()
{
	FILE *fp;
	
	verbose(vmsg_fe_ucode, fe_ucode_file);

	/*
	 * Get the nlist symbol entries
	 * Open the microcode file
	 * Initialize the GT LDM
	 */
	nlist(fe_ucode_file, nl);

	if ((fp = fopen(fe_ucode_file, "r")) == NULL)
		gt_perror(msg_open, fe_ucode_file);

	gt_init_ldm();
	errno = 0;

	/*
	 * Download the text, the data, and the abs sections.
	 */
	if (gt_loadtext(fp, 0) < 0)
		gt_perror(msg_loadtext);

	if (gt_loaddata(fp, 0) < 0)
		gt_perror(msg_loaddata);

	if (gt_loadabs(fp, 0) < 0)
		gt_perror(msg_loadabs);

	/*
	 * Verify that the text, data, and abs sections have been
	 * correctly downloaded.
	 */
	if (gt_loadtext(fp, 1) < 0)
		gt_perror(msg_cmptext);

	if (gt_loaddata(fp, 1) < 0)
		gt_perror(msg_cmpdata);

	if (gt_loadabs(fp, 1) < 0)
		gt_perror(msg_cmpabs);
}


/*
 * FE initialization that must be performed after the FE
 * microcode has been downloaded.
 */
gt_initialize_fe()
{
	unsigned data;

	/*
	 * Load the kernel MCB address into the GT.  After GT has
	 * the kmcb address initialize the WID partition and the
	 * server process CLUT quota.
	 */
	errno = 0;

	verbose(vmsg_master_hs);
	if (ioctl(gt_ctrl.fd, FB_LOADKMCB, (caddr_t) &data) < 0)
		gt_perror(msg_loadkmcb);

	if (ioctl(gt_ctrl.fd, FB_GT_SETVERSION, (caddr_t) &gt_version) < 0)
		gt_fprintf(stderr, msg_noversion);

	data = NWID_BITS - cmd.nbr_wid_bits;
	if (ioctl(gt_ctrl.fd, FB_SETWPART, (caddr_t) &data) < 0)
		gt_perror(msg_setwpart);

	data = cmd.nbr_owserver_cluts;
	if (ioctl(gt_ctrl.fd, FB_SETCLUTPART, (caddr_t) &data) < 0)
		gt_perror(msg_setclutpart);

	gt_setmonitor(0);
}


/*
 * Send the monitor type to the GT
 */
gt_setmonitor(flag)
	int flag;
{
	if (flag)
		init_sync_generator();
	else
		cmd.monitor_type == -1;

	if (ioctl(gt_ctrl.fd, FB_SETMONITOR, (caddr_t) &cmd.monitor_type) > 0)
		gt_perror(msg_setmonitor);

}


gt_loadtext(fp, flag)
	FILE *fp;
	int flag;
{
	unsigned *p, *q, *r, l;
	int s, i;

	if (((r = (unsigned *)gt_gettext(fp, &s, &l)) == NULL) && s)
		return (-1);

	p = r;
	q = (unsigned *)feaddr2vaddr(gt_ctrl.baseaddr, l);

	for (i=0; i<s/4; i++,q++,p++) {
		if (flag) {
			if (*q != *p) {
				gt_compare_err("text", q, *p, *q);
				return (NULL);
			}
		} else {
			*q = *p;
		}
	}

	free(r);
	return (s);
}


gt_loaddata(fp, flag)
	FILE *fp;
	int flag;
{
	unsigned *p, *q, *r, l;
	int ds, bs, i;

	if (((r = (unsigned *)gt_getdata(fp, &ds, &l)) == NULL) && ds)
		return (-1);

	/*
	 * XXX: must be able to deal with unaligned data
	 */
	p = r;
	q = (unsigned *)feaddr2vaddr(gt_ctrl.baseaddr, l);

	for (i=0; i<ds/4; i++,q++, p++) {
		if (flag) {
			if (*q != *p) {
				gt_compare_err("data", q, *p, *q);
				return (NULL);
			}
		} else {
			*q = *p;
		}
	}

	bs = gt_getbss(fp);
	for (i=0; i<bs/4; i++)
		*q++ = 0;

	free(r);

	return (ds+bs);
}


gt_loadabs(fp, flag)
	FILE	*fp;
	int	flag;
{
	unsigned *p, *q, l;
	int ds, i, j, cc;
	struct filehdr hc;
	struct scnhdr *section_p;
	N10AOUTHDR aouthdr;
	char *p1;

	rewind(fp);
	fread(&hc, 1, sizeof(struct filehdr), fp);
	fread(&aouthdr, 1, sizeof(N10AOUTHDR), fp);

	section_p = (scnhdr_t *) malloc(hc.f_nscns*sizeof(scnhdr_t));

	fread(section_p, 1, sizeof(scnhdr_t)*hc.f_nscns, fp);

	for (j=0; j<hc.f_nscns; j++) {
		if (section_p[j].s_flags == STYP_ABS) {
			fseek(fp, section_p[j].s_scnptr, 0);
			p1 = (char *)valloc(section_p[j].s_size);

			if ((cc=fread(p1, 1, section_p[j].s_size, fp)) !=
			    section_p[j].s_size)
				return (NULL);

			l = section_p[j].s_paddr;
			ds = section_p[j].s_size;

			/*
			 * XXX: must be able to deal with unaligned data
			 */
			p = (unsigned *) p1;
			q = (unsigned *)feaddr2vaddr(gt_ctrl.baseaddr, l);
			for (i=0; i<ds/4; i++, q++, p++) {
				if (flag) {
					if (*q != *p) {
						gt_compare_err("abs", q,*p,*q);
						return (NULL);
					}
				} else {
					*q = *p;
				}
			}
		}
	}

	return (1);
}


/*
 * Initialize all the necessary hardware registers
 * and zero the WCS area
 */
gt_init_hw()
{
	gt_hw_reset();

	init_cursor_colors();

	init_sync_generator();
	usleep(TEN_MSEC);

	init_cursor_enable(0xc00, 0); 
	wait_for_pf();
}


/*
 * Reset the GT
 */
gt_hw_reset()
{
	verbose(vmsg_hw_reset);

	gt_ctrl.fe_ctrl_regs->fe_hcr  = (u_int) 0;
	usleep(GT_RESET_TIME);

	gt_ctrl.fe_ctrl_regs->fe_hcr |= HK_RESET;
	usleep(GT_RESET_TIME);

	gt_ctrl.fe_ctrl_regs->fe_hcr &= ~HK_RESET;
	usleep(GT_RESET_HOLDOFF);
}


/*
 * Initialize the frame buffer
 */
gt_init_framebuffer()
{
	verbose(vmsg_init_fb, cmd.device_fbname);

	init_gamma_table();

	init_z_buffer();
	wait_for_pf();

	init_lookup_tables(0);
	wait_for_pf();

	init_image_enable(0);
	wait_for_pf();

	init_image_enable(1);
	wait_for_pf();

	init_window_enable();
	wait_for_pf();

	/*
	 * Having gone to all this trouble, make sure that video
	 * enable is asserted.
	 */
	gt_ctrl.fbo_screen_ctrl_regs->fbo_videomode_0 |= VIDEO_ENABLE;
}


gt_su_reset()
{
	/*
 	* In order, we:
 	*
 	*	- reset the SU
	*	- reset the EW
 	*	- reset the SI
 	*	- set the filtering enable bit in the SI
 	*	- stall the PBM
 	*	- enable SI loopback
 	*
	* Then, wait a few milliseconds for things to settle.
	*/

	*(u_int *) (gt_ctrl.baseaddr + SU_CSR_ADDR) =
	    SU_DSP_DISABLE_0 | SU_DSP_DISABLE_1 | SU_DSP_DISABLE_2;

	*(u_int *) (gt_ctrl.baseaddr + EW_CSR_ADDR)	= EW_CSR_RESET;
	*(u_int *) (gt_ctrl.baseaddr + SI_CSR1_ADDR)	= SI_CSR1_RESET;
	*(u_int *) (gt_ctrl.baseaddr + SI_CSR2_ADDR)	= SI_CSR2_F_ENA;
	*(u_int *) (gt_ctrl.baseaddr + PBM_HCSR_ADDR)	= HKPBM_HCS_HSRP;
	*(u_int *) (gt_ctrl.baseaddr + PBM_FECSR_ADDR)	= HKPBM_FECS_SILE;

	usleep(TEN_MSEC);


	/*
	 * Let the PBM transfer data
	 *
	 *	- set the FE address space register to (image | depth)
	 *	- set state set 1
	 *	- set the host address space register to (image | depth)
	 *	- unstall the PBM
	 */
	*(u_int *) (gt_ctrl.baseaddr + PBM_FEFBAS_ADDR)	= HKAS_IMAGE;
	*(u_int *) (gt_ctrl.baseaddr + PBM_FECSR_ADDR)	= HKPBM_FECS_AFBSS;
	*(u_int *) (gt_ctrl.baseaddr + PBM_HFBAS_ADDR)	= HKAS_IMAGE;
	*(u_int *) (gt_ctrl.baseaddr + PBM_HCSR_ADDR)	= 0;
}


/*
 * Initialize and verify LDM space
 */
gt_init_ldm()
{
	u_int	*ptr, i;

	verbose(vmsg_init_ldm);

	ptr = (u_int *) (gt_ctrl.baseaddr + FE_WCS_START);
	for (i=0; i < (FE_WCS_END-FE_WCS_START)/4; i++,ptr++)
		*ptr = 0;

	ptr = (u_int *) (gt_ctrl.baseaddr + FE_WCS_START);
	for (i=0; i < (FE_WCS_END-FE_WCS_START)/4; i++,ptr++) {
		if (*ptr != (u_int) 0) {
			gt_compare_err("LDM", ptr, 0, *ptr);
			exit(1);
		}
	}
}


init_cursor_colors()
{
	struct fb_clut clut;
	u_char  red[256], green[256], blue[256];

	/*
	 * bg: black    fg: white
	 */
	red[0] = 0;	green[0] = 0;	 blue[0] = 0;
	red[1] = 0xff;	green[1] = 0xff; blue[1] = 0xff;

	init_cursor_hw_regs();

	clut.index	= GT_CLUT_INDEX_CURSOR;
	clut.offset	= 0;
	clut.count	= 2;
	clut.red	= &red[0];
	clut.green	= &green[0];
	clut.blue	= &blue[0];
	clut.flags	= 0;

	if (ioctl(gt_ctrl.fd, FB_CLUTPOST, (caddr_t) &clut) < 0)
		gt_perror(msg_clutpost);
}


init_cursor_hw_regs()
{
	fbinit_type	*tbl = fbinit_table; 	/* pointer to the database */
	u_int		num_entries = NUM_FBINIT_TABLE, i;

	for (i=0; i<num_entries; i++,tbl++) {

		if (tbl->offset == 0) {
			usleep(tbl->ppdata * 1000);
		} else {
			*(u_int *) (gt_ctrl.baseaddr +
			    (tbl->offset & MS_OFFSET_MASK)) = tbl->ppdata;
		}
	}
}


init_gamma_table()
{
	struct fb_clut	   clut;
	u_char  red[256], green[256], blue[256];
	u_int	gamma, degamma;
	double   inverse_gamma;
	int	i;

	verbose(vmsg_gamma, cmd.gamma_value);

	/*
	 * Initialize gamma correction table
	 */
	for (i=0; i < 256; i++) {
		gamma = GAMMA(i, cmd.gamma_value);

		red[i]   = gamma;
		green[i] = gamma;
		blue[i]  = gamma;
	}

	/*
	 * Post it to the GT
	 */
	clut.index	= GT_CLUT_INDEX_GAMMA;
	clut.offset	= 0;
	clut.count	= 256;
	clut.red	= red;
	clut.green	= green;
	clut.blue	= blue;
	clut.flags	= 0;

	if (ioctl(gt_ctrl.fd, FB_CLUTPOST, (caddr_t) &clut) < 0)
		gt_perror(msg_clutpost);

	/*
	 * Now build and post a de-gamma table
	 */
	inverse_gamma = 1.0/cmd.gamma_value;

	for (i=0; i < 256; i++) {
		degamma = GAMMA(i, inverse_gamma);

		red[i]   = degamma;
		green[i] = degamma;
		blue[i]  = degamma;
	}
	clut.index = GT_CLUT_INDEX_DEGAMMA;
	clut.offset	= 0;
	clut.count	= 256;
	clut.red	= red;
	clut.green	= green;
	clut.blue	= blue;
	clut.flags	= 0;

	if (ioctl(gt_ctrl.fd, FB_CLUTPOST, (caddr_t) &clut) < 0) {
		gt_fprintf(stderr, msg_degamma);
	} else {
		if (gt_setgamma())
			gt_fprintf(stderr, msg_gammaval);
	}
}


gt_setgamma()
{
	struct fb_setgamma fbs;

	ioctl(gt_ctrl.fd, FB_GETGAMMA, (caddr_t) &fbs);

	if (!((cmd.flags & CMD_VASCO_ONLY) && !(cmd.flags & CMD_USE_GAMMA)))
		fbs.fbs_gamma = cmd.gamma_value;

	if (!((cmd.flags & CMD_GAMMAONLY) && !(cmd.flags & CMD_USE_VASCO)))
		fbs.fbs_flag  = cmd.vasco ? FB_DEGAMMA:FB_NO_DEGAMMA;

	return (ioctl(gt_ctrl.fd, FB_SETGAMMA, (caddr_t) &fbs));
}


/*
 * Initialize WLUTs and CLUTs for 24-bit mode for whole screen.
 */
init_lookup_tables(flag)
	int flag;
{
	u_int	*wlut_map_ptr, *wlut_ctrl_ptr;
	u_int	*clut_map_ptr, *clut_ctrl_ptr;
	int	i;

	/*
	 * Calculate WLUT and CLUT addresses
	 */
	wlut_map_ptr  = (u_int *) (gt_ctrl.baseaddr + FBO_WLUT_MAP_OF);
	wlut_ctrl_ptr = (u_int *) (gt_ctrl.baseaddr + FBO_WLUT_CTRL_OF);

	clut_map_ptr  = (u_int *) (gt_ctrl.baseaddr + FBO_CLUT_MAP_OF);
	clut_ctrl_ptr = (u_int *) (gt_ctrl.baseaddr + FBO_CLUT_CTRL_OF);

	/*
	 * Load entire WLUT with  FC_SEL=OFF, CLUT_SEL=0, IM_SEL=24-bit
	 *
	 * First load shadow
	 */
	for (i=0; i<2048; i++,wlut_map_ptr++) {
		if (flag)
			*wlut_map_ptr = 0x80E;		/* buffer A	*/
		else
			*wlut_map_ptr = 0x806;		/* buffer B	*/
	}

	/* Now post it to the real hardware table */
	*wlut_ctrl_ptr = 0x51;
	usleep(TEN_MSEC);

	/* Load CLUT #0 with an identity pattern */
	for (i=0; i<256; i++, clut_map_ptr++)
		*clut_map_ptr = ((i << 16) | (i << 8) | (i));

	/*
	 * Now post it to the real hardware table
	 * Load CLUT #14 with an identity pattern
	 */
	clut_map_ptr = (u_int *) (gt_ctrl.baseaddr +
	    (FBO_CLUT_MAP_OF + (0x400 * 14)));

	for (i=0; i<256; i++, clut_map_ptr++)
		*clut_map_ptr = ((i << 16) | (i << 8) | (i));

	/* Now post it to the real hardware table */
	*clut_ctrl_ptr = 0x51;
	usleep(TEN_MSEC);
}


/*
 * Initialize the sync generator
 */
init_sync_generator()
{
	struct fbinit_type *p;
	int cnt, i;
	
	if (!((cmd.flags & CMD_MONITOR) || 
		(cmd.flags & CMD_MONITORONLY))){
		read_sense_bits();
	}

	verbose(vmsg_sync_gen, cmd.fb_freq);

	p   = monitor_setup[cmd.monitor_type].ms_setup;
	cnt = monitor_setup[cmd.monitor_type].ms_count;

	for (i=0; i<cnt; i++, p++) {
		if (p->offset == 0) {
			usleep(p->ppdata * 1000);
		} else {
			*(u_int *) (gt_ctrl.baseaddr +
			    (p->offset & MS_OFFSET_MASK)) = p->ppdata;
		}
	}
	usleep(FORTY_MSEC);
}


/*
 * Clear the Z-buffer
 */
init_z_buffer()
{
	set_common_regs(0);

	gt_ctrl.fbi_regs->fbi_reg_z_ctrl = 4;

	/*
	 * Use the PF to initialize the Z buffer, as follows:
	 *
	 *    foreground color:
	 *    direction/size:	LtoR, 1280 x 1024
	 *    fill dest addr:	upper left corner of screen
	 */
	gt_ctrl.fbi_regs->fbi_reg_fg_col   = 0x00ffffff;
	gt_ctrl.fbi_regs->fbi_reg_dir_size = 0x00200500;
	gt_ctrl.fbi_pfgo_regs->fbi_pfgo_fill_dest_addr =  0x00400000;
}


/*
 * Initialize the cursor planes
 */
init_cursor_enable(mask, flag)
	u_int	mask, flag;
{
	verbose(vmsg_enable, flag);

	set_common_regs(0);

	/*
	 * Use the PF to initialize the cursor planes, as follows:
	 *
	 *    window write mask:	cd, ce only
	 *    foreground color:		cd = black (0), ce = transparent (0)
	 *    direction/size:		LtoR, 1280 x 1024
	 *    fill dest addr:		window, upper left corner of screen
	 */
	gt_ctrl.fbi_regs->fbi_reg_w_wmask  = mask;
	gt_ctrl.fbi_regs->fbi_reg_fg_col   = flag ? mask : 0; 
	gt_ctrl.fbi_regs->fbi_reg_dir_size = 0x00200500;
	gt_ctrl.fbi_pfgo_regs->fbi_pfgo_fill_dest_addr =  0x00800000;
}


set_common_regs(buffer)
	int buffer;
{
	/*
	 * set up the PBM for state set 0 pixel access
	 */
	gt_ctrl.rp_host_regs->rp_host_as_reg  = HKAS_IMAGE;
	gt_ctrl.rp_host_regs->rp_host_csr_reg = 0x0;

	/*
	 * load the state set 0 PP registers, as follows:
	 *
	 *     view x clip:		no clipping
	 *     view y clip:		no clipping
	 *     global pp fb width:	1280 x 1024
	 *     buffer select:		A: 0x6,  B: 0x1
	 *     stereo control:		no stereo
	 *     current wid:		0
	 *     wid control:		no wid clip or replace
	 *     constant z src:		z = 0
	 *     z control:		0
	 *     image write mask:	enable all planes
	 *     window write mask:	don't write F, C, W
	 *     bytemode channel:	red
	 *     bytemode write mask:	enabled
	 *     ROP blend operation:	DST = SRC
	 *     multiplane group:	disable fastclear, enable other planes
	 */
	gt_ctrl.fbi_regs->fbi_reg_vwclp_x   = 0x000007ff;
	gt_ctrl.fbi_regs->fbi_reg_vwclp_y   = 0x000007ff;
	gt_ctrl.fbi_regs->fbi_reg_fb_width  = 0x0;

	if (buffer)
		gt_ctrl.fbi_regs->fbi_reg_buf_sel =  0x6;	/* buffer a */
	else
		gt_ctrl.fbi_regs->fbi_reg_buf_sel =  0x1;	/* buffer b */

	gt_ctrl.fbi_regs->fbi_reg_stereo_cl = 0x0;
	gt_ctrl.fbi_regs->fbi_reg_cur_wid   = 0x0;
	gt_ctrl.fbi_regs->fbi_reg_wid_ctrl  = 0x0;
	gt_ctrl.fbi_regs->fbi_reg_con_z     = 0x0;
	gt_ctrl.fbi_regs->fbi_reg_z_ctrl    = 0x0;

	gt_ctrl.fbi_regs->fbi_reg_i_wmask   = 0xffffffff;
	gt_ctrl.fbi_regs->fbi_reg_w_wmask   = 0x0;
	gt_ctrl.fbi_regs->fbi_reg_b_mode    = 0x3;		/* red	     */
	gt_ctrl.fbi_regs->fbi_reg_b_wmask   = 0xff;		/* enabled   */

	gt_ctrl.fbi_regs->fbi_reg_rop       = 0x0c;		/* DST = SRC */
	gt_ctrl.fbi_regs->fbi_reg_mpg       = 0xfc;		/* disable   */
}


init_image_enable(buffer)
	int	buffer;
{
	set_common_regs(buffer);

	/*
	 * Use the PF to initialize image planes, as follows:
	 *
	 *    foreground color:
	 *    direction/size:		LtoR, 1280 x 1024
	 *    fill dest addr:		upper left corner of screen
	 */
	gt_ctrl.fbi_regs->fbi_reg_fg_col   = 0x0;
	gt_ctrl.fbi_regs->fbi_reg_dir_size = 0x00200500;
	gt_ctrl.fbi_pfgo_regs->fbi_pfgo_fill_dest_addr =  0x00000000;
}


init_window_enable()
{
	set_common_regs(0);

	/*
	 * Use the PF to initialize image planes, as follows:
	 *
	 *    window write mask:	all WIDs
	 *    foreground color:
	 *    direction/size:		LtoR, 1280 x 1024
	 *    fill dest addr:		window, upper left corner of screen
	 */
	gt_ctrl.fbi_regs->fbi_reg_w_wmask  = 0x3ff;	/* all WID */
	gt_ctrl.fbi_regs->fbi_reg_fg_col   = 0x0;	/* cd=0, ce=0 */
	gt_ctrl.fbi_regs->fbi_reg_dir_size = 0x00200500;
	gt_ctrl.fbi_pfgo_regs->fbi_pfgo_fill_dest_addr =  0x00800000;
}


/*
 * Obtain microcode microcode version
 */
gt_get_version()
{
	u_int	*versp, *exec_versp;
	int 	 version, absversion, exec_version;

	versp = (u_int *) feaddr2vaddr(gt_ctrl.baseaddr,
	    nl[WCS_VERSION].n_value);

	exec_versp = (u_int *) feaddr2vaddr(gt_ctrl.baseaddr,
	    nl[EXEC_VERSION].n_value);

	version  = *versp;

	absversion = (version < 0) ? -version:version;

	gt_version.gtv_ucode_major    = version/100;
	gt_version.gtv_ucode_minor    = (absversion / 10) % 10;
	gt_version.gtv_ucode_subminor = absversion % 10;

	if (verbose_flag) {
		gt_fprintf(stdout, "%s: ", progname);
		gt_fprintf(stdout, vmsg_version,
		    gt_version.gtv_ucode_major,
		    gt_version.gtv_ucode_minor,
		    gt_version.gtv_ucode_subminor);

#ifdef GT_PREFCS
		exec_version = *exec_versp;
		gt_fprintf(stdout, "%s: ", progname);
		gt_fprintf(stdout, vmsg_execver, (float)exec_version/100);
#endif GT_PREFCS

	}
}


gt_download_su_code()
{
	u_int	*ptr;
	u_int	data;

	/*
	 * Unstall the PBM, then
	 *
	 *     - set SI ACCEPT DATA and settle down, then
	 *     - set EW ACCEPT DATA and settle down
	 */
	*(u_int *) (gt_ctrl.baseaddr + PBM_HCSR_ADDR) = 0;

	*(u_int *) (gt_ctrl.baseaddr + SI_CSR1_ADDR)  = SI_CSR1_AD;
	usleep(TEN_MSEC);

	*(u_int *) (gt_ctrl.baseaddr + EW_CSR_ADDR)   = EW_CSR_AD;
	usleep(TEN_MSEC);


	ptr = (u_int *) (gt_ctrl.baseaddr + SU_CSR_ADDR);

	*ptr = SU_DSP_DISABLE_0 |
	       SU_DSP_DISABLE_1 |
	       SU_DSP_DISABLE_2 |
	       SU_DSP_RUN_0     |
	       SU_DSP_RUN_1	|
	       SU_DSP_RUN_2;
	usleep(TEN_MSEC);

	*ptr = SU_DSP_DISABLE_0 | SU_DSP_DISABLE_1 | SU_DSP_DISABLE_2;


	/*
	 * Download the SU code for the designated DSPs.
	 */
	if (cmd.flags & CMD_USE_DSP0) {
		verbose(vmsg_su0_ucode, su0_ucode_file);

		if (load_su(su0_ucode_file))
			exit(1);
		*ptr |= SU_DSP_RUN_0;
		usleep(1000);
	}

	if (cmd.flags & CMD_USE_DSP1) {
		verbose(vmsg_su1_ucode, su1_ucode_file);

		if (load_su(su1_ucode_file))
			exit(1);
		*ptr |= SU_DSP_RUN_1;
		usleep(1000);
	}

	/*
	 * Start the DSPs
	 */
	data = *ptr;
	if (cmd.flags & CMD_USE_DSP0)
		data &= ~SU_DSP_DISABLE_0;

	if (cmd.flags & CMD_USE_DSP1)
		data &= ~SU_DSP_DISABLE_1;

	*ptr = data;

	usleep(TEN_MSEC);
}



int
load_su(filename)
	char	*filename;
{
	int i;
	int nwords, error;
	unsigned ex, re;
	unsigned *mem;
	unsigned *src, *dst;
	struct stat sbuf;
	FILE *fp;

	/*
	 * Read in the input file
 	 */
	errno = 0;

	if (stat(filename, &sbuf) < 0)
		gt_perror(msg_perror, filename);

	nwords = sbuf.st_size / sizeof(unsigned);

	if (nwords > SURAM_SIZE)
		nwords = SURAM_SIZE;

	if ((mem = (unsigned *) malloc(sbuf.st_size)) == NULL)
		gt_perror(msg_malloc);

	if ((fp = fopen(filename, "r")) == NULL)
		gt_perror(msg_open, filename);

	if (fread(mem, sizeof(unsigned), nwords, fp) != nwords)
		gt_perror(msg_read, filename);

	fclose(fp);

	/*
	 * Write the SU, then verify it.
	 */
	src  = mem;
	dst  = (unsigned *) (gt_ctrl.baseaddr + SURAM_BASE);

	for (i=0; i<nwords; i++)
		*dst++ = *src++;

	error = 0;
	src = mem;
	dst = (unsigned *) (gt_ctrl.baseaddr + SURAM_BASE);

	for (i=0; i<nwords; i++) {
		ex = *src++;
		re = *dst++;

		if (ex != re) {
			gt_compare_err("SU RAM", i, ex, re);
			error = -1;
		}
	}

	free(mem);

	return (error);
}

static int gt_dummyvar;
#define BACKOFF_LIMIT 2000

wait_for_pf()
{
	int    i;
	int    cnt = 0;


	while (gt_ctrl.rp_host_regs->rp_host_csr_reg & GT_PF_BUSY) {
		/*
		 * spin a bit, but don't hog the GT bus with
		 * rapidfire local bus accesses.
		 */

		for (i=0; i<cnt; i++)
			gt_dummyvar = i; /* outwit clever compilers */

		if (cnt++ > BACKOFF_LIMIT) {
			/*
			 * We get here if there is a big slowdown in
			 * the GT system.  We slow the loop, and reduce
			 * CPU activity.
			 */
			sleep(1);

			if (cnt > BACKOFF_LIMIT+3) {
				errno = 0;
				gt_perror(msg_pfbusy);
			}
		}
	}
}


read_sense_bits()
{
	u_int	reg_val;

	/*
	 * Read the hw sense bits in the VID CSR 0 register
	 */
	reg_val = *(u_int *) (gt_ctrl.baseaddr +
	    (VID_CSR0_ADDR & MS_OFFSET_MASK));

	switch (reg_val & SENSEBITS_MASK) {

		/*
		 * 1280X1024 @ 76Hz   Toshiba Sun 21" monitor?
		 */
		case SENSEBITS_0_SUN_21:
		case SENSEBITS_1_SUN_21:
			cmd.monitor_type = GT_1280_76;
			strcpy(cmd.fb_freq,
			    "Sun 21\" monitor (1280X1024@76)");
			break;

		/*
		 * 1280X1024 @ 67Hz   Sony P3 Sun 19" monitor?
		 */
		case SENSEBITS_0_SUN_19:
		case SENSEBITS_1_SUN_19:
			cmd.monitor_type = GT_1280_67;
			strcpy(cmd.fb_freq,
			    "Sun 19\" monitor (1280X1024@67)");
			break;

		/*
		 * 1280X1024 @ 67Hz   Sony P3 Sun 16/17" monitor
		 */
		case SENSEBITS_0_SUN_16:
		case SENSEBITS_1_SUN_16:
			cmd.monitor_type = GT_1280_67;
			strcpy(cmd.fb_freq,
			    "Sun 16\" monitor (1280X1024@67)");
			break;

		default:
			cmd.monitor_type = GT_1280_67;
			strcpy(cmd.fb_freq,
			    "Unknown monitor type.  Initializing to Sun 19\" monitor (1280X1024@67)");
	}
}
