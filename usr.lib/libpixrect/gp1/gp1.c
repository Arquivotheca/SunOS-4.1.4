#ifndef lint
static char sccsid[] = "@(#)gp1.c	1.1  94/10/31  SMI";
#endif

/*
 * Copyright 1985, 1989 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sun/fbio.h>
#include <sun/gpio.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <pixrect/cg2var.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1var.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/cg9var.h>
#include <pixrect/pr_planegroups.h>

static struct pr_devdata *gp1devdata;
extern struct pr_size	cg9size;
extern Pixrect     *cg12_make();

Pixrect *
gp1_make(fd, size, depth, attr)
	int fd;
	struct pr_size size;
	int depth;
	struct fbgattr *attr;
{
    struct fbinfo   fbinfo;
    int             gbufflag;
    register Pixrect *pr = NULL;
    struct pr_devdata *dd;
    register struct gp1pr *gppr;
    int             pagesize = getpagesize ();
    int             cgtype,
                    cgsize,
                    cgoffset,
                    cgextra;
    int             (*cg_prd_init) ();

    /* perform mystical ioctls, create pixrect, map device */
    if (ioctl (fd, FBIOGINFO, &fbinfo) < 0 ||
	ioctl (fd, GP1IO_GET_GBUFFER_STATE, &gbufflag) < 0)
	return pr;
    cgtype = attr->sattr.dev_specific[CG_TYPE];
    switch (cgtype) {
    case FBTYPE_SUN2COLOR:
	{
	    int             _cg2_prd_init();

	    cgoffset = CG2_MAPPED_OFFSET;
	    cgsize = CG2_MAPPED_SIZE;
	    cgextra = pagesize;
	    cg_prd_init = _cg2_prd_init;
	    break;
	}
	break;
    case FBTYPE_SUNROP_COLOR:
	{
	    int				_cg9_prd_init();

	    cg9size = size;	/* set up for cg9_prd_init */

	    cgoffset = 0;
	    cgsize = attr->sattr.dev_specific[CG_SIZE];
	    cgextra = 0;
	    cg_prd_init = _cg9_prd_init;
	    break;
	}
	break;
    case FBTYPE_SUNGP3:		/* cg12 is not a separate fb from gp */
	return cg12_make(fd, size, depth, attr);
    default:
	return 0;
    }

    if (!(pr = pr_makefromfd_2 (fd, size, depth, &gp1devdata, &dd,
				sizeof (struct gp1pr),
				cgsize + cgextra,
				fbinfo.fb_addrdelta + cgoffset - cgextra,
				attr->sattr.dev_specific[GP_SHMEMSIZE],
				GP1_SHMEM_OFFSET)))
	return pr;

    /* initialize pixrect data */
    gppr = (struct gp1pr *) (pr->pr_data);
    gppr->fbtype = cgtype;

    pr->pr_ops = &gp1_ops;
    (*cg_prd_init) ( &(gppr->cgpr), dd->fd, dd->va + cgextra, attr);

    if (cgtype == FBTYPE_SUN2COLOR) {
	GP1_COPY_FROM_CG2(&(gppr->cgpr.cg2pr), gppr);
    }
    else {  /* cg9 */
	/*
	 * default is the monochrome overlay plane.  Note that neither
	 * overlay nor enable planes are initialized.
	 */
	int             video_on = FBVIDEO_ON;

	(void) pr_set_planes(pr, PIXPG_OVERLAY, PIX_ALL_PLANES);
	(void) ioctl (gppr->cgpr.cg9pr.fd, FBIOSVIDEO, &video_on);
	gppr->cgpr.cg9pr.flags &= ~CG9_OVERLAY_CMAP;
	gppr->flags = gppr->cgpr.cg9pr.flags;
	gppr->cgpr_fd = gppr->cgpr.cg9pr.fd;	
    }

    gppr->flags |= CG2D_GP;

    if (gbufflag)
	gppr->flags |= CG2D_GB;

    gppr->gp_shmem = dd->va2;
    gppr->cg2_index = fbinfo.fb_unit;
    gppr->gbufflag = gbufflag;

    if (ioctl (gppr->ioctl_fd = dd->fd,
	       GP1IO_GET_TRUMINORDEV, &gppr->minordev) < 0) {
	(void) gp1_destroy(pr);
	return 0;
    }

    (void) gp1_get_cmdver (pr);

    if (cgtype == FBTYPE_SUN2COLOR) {
	GP1_COPY_TO_CG2(gppr, &(gppr->cgpr.cg2pr));
    }

    pr_set_plane_group(pr, PIXPG_OVERLAY);	/* nop for gp2/cg5 */
    (void) pr_ioctl(pr, GP1IO_SATTR, attr);
    return pr;
}

gp1_destroy(pr)
    Pixrect *pr;
{

    if (pr) {
	register struct gp1pr *gp1pr;


	if (gp1pr = gp1_d(pr)) {
	    if (gp1pr->cgpr_fd != -1)
		(void) pr_unmakefromfd(gp1pr->cgpr_fd, &gp1devdata);
	    free((char *) gp1pr);
	}
	free((char *) pr);
    }

    return 0;
}


gp1_ucode_version(pr, version)
Pixrect *pr;
register struct gp1_version *version;
{
	register struct gp1_shmem *shmem = 
		(struct gp1_shmem *) gp1_d(pr)->gp_shmem;

	/* assume 3.0 microcode */
	version->majrel = 3;
	version->minrel = 0;
	version->serialnum = 1;
	version->flags = 0x7;

	switch (shmem->ver_flag) {
	case 0:		/* Releases 2.2, 2.3, and 3.0 */
		break;

	case 128:	/* Releases 3.2Beta2, 3.2Pilot, etc. */
		version->majrel = 3;
		version->minrel = 2;
		version->serialnum = 0;
		version->flags = 0x3;
		break;

	case 1:		/* Release 3.2FCS and later */
		version->majrel = shmem->ver_release >> 8;
		version->minrel = shmem->ver_release;
		version->serialnum = shmem->ver_serial >> 8;
		version->flags = shmem->ver_serial;
		break;

	default:	/* unknown version */
		return PIX_ERR;
	}
	
	return 0;
}


/*
Command Versions by Release Number

Command			2.2,2.3,3.0	3.2FCS	3.4BETA	 4.0 FCS
-------			-----------	------	-------	 -------

 0 GP1_EOCL		      1		   1		
 1 GP1_USE_CONTEXT	      1		   1
 2 GP1_PR_VEC		      1		   1
 3 GP1_PR_ROP_NF	      1		   1
 4 GP1_PR_ROP_FF	      1		   1

 5 GP1_PR_PGON_SOL	      1		   1
 6 GP1_SET_ZBUF		      1		   1
 7 GP1_SET_HIDDEN_SURF	      1		   1
 8 GP1_SET_MAT_NUM	      1		   1
 9 GP1_MUL_POINT_FLT_2D	      1		   1

10 GP1_MUL_POINT_FLT_3D	      1		   1
11 GP1_XF_PGON_FLT_3D	      1		   2		 3
12 GP1_XF_PGON_FLT_2D	      1		   2		 3
13 GP1_CORENDCVEC_3D	      1		   1
14 GP1_CGIVEC		      1		   1

15 GP1_SET_CLIP_LIST	      1		   1
16 GP1_SET_FB_NUM	      1		   1
17 GP1_SET_VWP_3D	      1		   1
18 GP1_SET_VWP_2D	      1		   1
19 GP1_SET_ROP		      1		   1

20 GP1_SET_CLIP_PLANES	      1		   1
21 GP1_MUL_POINT_INT_2D	      0		   1
22 GP1_MUL_POINT_INT_3D	      0		   1
23 GP1_SET_FB_PLANES	      1		   1
24 GP1_SET_MAT_3D	      1		   1

25 GP1_XFVEC_3D		      1		   1
26 UNUSED		      0		   0
27 GP1_XFVEC_2D		      1		   1
28 GP1_SET_COLOR	      1		   1
29 GP1_SET_MAT_2D	      1		   1

30 GP1_XF_PGON_INT_3D	      1		   2		 3
31 UNUSED		      0		   0
32 GP1_MUL_MAT_2D	      1		   1
33 GP1_MUL_MAT_3D	      1		   1
34 GP1_GET_MAT_2D	      1		   1

35 GP1_GET_MAT_3D	      1		   1
36 GP1_PROC_LINE_FLT_3D	      1		   1	   2	 3
37 GP1_PROC_PGON_FLT_3D	      1		   1		 
38 GP1_PROC_PGON_FLT_2D	      0		   1		 
39 UNUSED		      0		   0

40 GP1_PR_LINE		      0		   1
41 GP1_PR_POLYLINE	      0		   1
42 GP1_SET_LINE_TEX	      0		   1
43 GP1_SET_LINE_WIDTH	      0		   1
44 GP1_CGI_LINE		      0		   1

45 GP1_XF_LINE_FLT_2D	      0		   1		 
46 GP1_XF_LINE_FLT_3D	      0		   1		 2
47 GP1_XF_LINE_INT_3D	      0		   1		 2
48 GP1_PR_PGON_TEX	      0		   1		 2
49 UNUSED		      0		   0

50 GP1_PR_ROP_TEX	      0		   1		 2
51 GP1_SET_PGON_TEX_BLK	      0		   1
52 GP1_SET_PGON_TEX	      0		   1
53 GP1_SET_PGON_TEX_ORG_SCR   0		   1
54 GP1_SET_PGON_TEX_ORG_XF_2D 0		   1

55 GP1_SET_PGON_TEX_ORG_XF_3D 0		   1
56 UNUSED		      0		   0
57 GP1_XF_LINE_INT_2D	      0		   1
58 GP1_XF_PGON_INT_2D	      0		   1		 2
59 GP1_PROC_PGON_INT_2D	      0		   1

60 GP1_PROC_LINE_FLT_2D	      0		   1	   2	 3
61 GP1_PROC_LINE_INT_2D	      0		   1	   2	 3
62 GP1_PROC_LINE_INT_3D	      0		   1	   2	 3
63 GP1_PROC_PGON_INT_3D	      0		   1

			3.4FCS
			------
64 GP1_SET_PICK_ID	  1
65 GP1_SET_PICK_WINDOW	  1
66 GP1_GET_PICK		  1

*/

#define MAXVER	4

gp1_get_cmdver(pr)
	Pixrect *pr;
{
	struct gp1pr *gppr = gp1_d(pr);
	struct gp1_version uversion;
	register int vernum;
	register u_char *ver;
	static u_char *verptr[MAXVER];
	char *calloc();

	if (gp1_ucode_version(pr, &uversion) == PIX_ERR) 
		(void) fprintf(stderr,
		    "Warning: You are running unknown GP microcode!\n");

	gppr->ncmd = GP1_NCMDS;
	
	/* assume 2.2, 2.3, 3.0 */
	vernum = 0;

	if (uversion.majrel == 3) {
		/* 3.2FCS */
		if (uversion.minrel == 2 && uversion.serialnum != 0) 
			vernum = 1;
		/* 3.4BETA or later */
		if (uversion.minrel > 2)
			vernum = 2;
	}
	/* 4.0 */
	else if (uversion.majrel >= 4)
		vernum =3;

	/* maybe we've seen this version before */
	if (gppr->cmdver = ver = verptr[vernum])
		return 0;

	/* new version, alloc & zero cmdver array */
	if (!(ver = (u_char *) calloc(GP1_NCMDS, 1)))
		return PIX_ERR;

	verptr[vernum] = ver;
	gppr->cmdver = ver;

	/* initialize cmdver array to 2.2, 2.3, 3.0 functionality */
	incver(ver, 0, 37);
	ver[21] = 0;
	ver[22] = 0;
	ver[26] = 0;
	ver[31] = 0;

	if (--vernum < 0)
		return 0;

	/* apply changes for 3.2 functionality */
	ver[11] = 2;
	ver[12] = 2;
	ver[21] = 1;
	ver[22] = 1;
	ver[30] = 2;
	incver(ver, 38, 63);
	ver[39] = 0;
	ver[49] = 0;
	ver[56] = 0;
	
	if (--vernum < 0)
		return 0;

	/* apply changes for 3.4 functionality */
	ver[36] = 2;
	ver[60] = 2;
	ver[61] = 2;
	ver[62] = 2;

	if (--vernum < 0)
		return 0;

	/* apply 4.0 changes */
	ver[11] = 3;
	ver[12] = 3;
	ver[30] = 3;
	ver[36] = 3;
	ver[46] = 2;
	ver[47] = 2;
	ver[48] = 2;
	ver[50] = 2;
	ver[58] = 2;
	ver[60] = 3;
	ver[61] = 3;
	ver[62] = 3;

	return 0;
}

static
incver(p, lo, hi)
	register u_char *p;
	register int lo, hi;
{
	p += lo;
	hi -= lo;

	while (hi-- >= 0) {
		(*p)++;
		p++;
	}
}
