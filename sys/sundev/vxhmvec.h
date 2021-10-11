/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */
/*	@(#)vxhmvec.h 1.1 94/10/31 SMI/VPG; from UCB X.X XX/XX/XX	*/

/* vxhmvec.h is used to define all high memory vectors used by vx */


/* Host Process Call Vector */

#define HPCVEC_OFFSET 	    	    0x1f80


/* Host Segment Request Vector */

#define HSRVEC			0xffffff70
#define HSRVEC_OFFSET	     	    0x1f70

/* flag values for DVMA demand paging */
#define HS_WANTPAGE	0x1		/* indicates node wants a page of host mem */
#define HS_READY	0x2		/* kernel is available for another page req */
#define HS_NOSPACE	0x3		/* kernel can't find the DVMA space for req */

struct hsrvec {	int	hs_flag;
		int	hs_off;
		int	hs_mapaddr;
	      };
