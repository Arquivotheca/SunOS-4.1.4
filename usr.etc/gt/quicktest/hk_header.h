#ifndef _HK_HEADER_

/*
 ***********************************************************************
 *
 * hk_header.h
 *
 *      @(#)hk_header.h  1.1 94/10/31  SMI
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * This file contains an external reference to the static variable
 * containing the version number.  It also contains the Hawk magic
 * number and file types.  To bump the version number of the Hawk
 * simulators, change hk_version.c
 *
 *  9-May-89 Kevin C. Rushforth   Initial version created.
 * 19-Jun-89 Scott R. Nelson	  Added pixel type from FB.
 * 26-Jul-89 Scott Johnston	  Added hasm output .hobj type.
 * 14-Mar-90 Kevin C. Rushforth	  Added new .hdl/.hobj type.
 * 13-Feb-91 Scott R. Nelson	  Added constants for Leo files.
 *
 ***********************************************************************
 */

/*
 * Display list header record.  Contains information about the
 * binary display list file.
 */

#define HK_MAGIC_NUMBER 0xA788		/* All Hawk files begin with this */

/* Processor types for file header */

#define HK_DL_TYPE	0x1		/* OBSOLETE -- Display list (.hdl) */
#define HK_FE_TYPE	0x2		/* Front-end output */
#define HK_SU_TYPE	0x3		/* Set-up processor output */
#define HK_EW_TYPE	0x4		/* Edge Walker output */
#define HK_SI_TYPE	0x5		/* Rendering Engine output */
#define HK_FB_TYPE	0x6		/* Frame Buffer output */
#define HK_PX_TYPE	0x7		/* Pixels from the frame buffer */
#define HK_OB_TYPE	0x8		/* OBSOLETE -- Hasm output (.hobj) */
#define HK_FA_TYPE	0x9		/* Frame Buffer output w/ alpha */
#define HK_HDL_TYPE	0xA		/* Display list (.hobj or .hdl) */
#define HK_SD_TYPE	0xB		/* Diagnostic SU output */

/*
 * NOTE: The Hawk context and MCB structures use magic numbers in
 * the range: 0x100-0x110
 *
 * #define HK_CTX_TYPE	0x102
 * #define HK_UMCB_TYPE	0x103
 * #define HK_KMCB_TYPE	0x104
 */

/*
 * Leo files reserve the range 0x200-0x2ff.
 */

#define LF_UCODE_TYPE	0x201		/* LeoFloat microcode binary */
#define LC_TO_LF_TYPE	0x202		/* LeoCommand to LeoFloat */
#define LF_TO_LD_TYPE	0x203		/* LeoFloat to LeoDraw */
#define LC_TO_LD_TYPE	0x204		/* LeoCommand to LeoDraw */
#define LEO_FB_TYPE	0x205		/* Leo frame buffer contents */
#define LEO_SBUS_TO_LC_TYPE 0x206	/* S-bus to LeoCommand */



/* Header for all Hawk binary files */

typedef struct hk_dl_hd {
    unsigned short magic;	/* Magic number */
    unsigned short type;	/* Output type (which processor it came from) */
    unsigned short version;	/* Code version number */
    unsigned short filler;	/* Fill it out to even 32-bit boundary */
} hk_dl_hd;

extern short hk_version;	/* Current version */
#endif _HK_HEADER_
