
#ifndef lint
static char Sccsid[] = "@(#)tv1.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <sun/tvio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/tv1var.h>

#define IS_VALID_FBTYPE(type) \
	((unsigned) (type) < FBTYPE_LASTPLUSONE && pr_makefun[(type)] != 0)

static struct pr_devdata *tv1devdata;

extern Pixrect *(*pr_makefun[])();
extern char *malloc();

/* The following structure is used to keep track of the Real */
/* tv1 data structures when we are emulationg what we are bound to */

struct tv1d_node {
    struct tv1_data *data;
    struct pixrect  *pr;
    struct tv1d_node *next;
};

static struct tv1d_node *tv1d_list;

struct tv1_data* get_tv1_data( );

Pixrect        *
tv1_make (fd, size, depth)
    int             fd;
    struct pr_size  size;
    int             depth;
{
    static struct fbdesc {
	short           group;
	short           depth;
	int             allplanes;
	struct pr_size	size;
    }               fbdesc[TV1_NFBS] = {
	{ PIXPG_VIDEO, 32, 0x00ffffff, {FB_WIDTH, FB_HEIGHT} },
	{ PIXPG_VIDEO_ENABLE, 1, 0, {CP_WIDTH, CP_HEIGHT} }
    };
    static struct pixrect pr_video, pr_video_enable;

    Pixrect        *pr;
    register struct tv1_data *tv1d;
    struct pr_devdata *curdd;
    int             i;
    register int    pagesize;	       /* # of bytes per page */
    int             mapsize;	       /* # of bytes mapped in */

    extern void tv1_setEmuType();

    pagesize = getpagesize ();
    mapsize = ROUNDUP(FB_SIZE, pagesize) + ROUNDUP(CP_SIZE, pagesize);
    if (!(pr = pr_makefromfd_2(fd, size, depth, &tv1devdata, &curdd,
			       sizeof (struct tv1_data),
			       mapsize, 0x80000000,
			       sizeof(struct csr), CSR_BASE | 0x80000000))) {
	return (Pixrect *) NULL;
    }

    tv1d = (struct tv1_data *) pr->pr_data;
    /* save a pointer to real data, cause we loose it when non-video
    /*  planegroups are active */
    if (tv1_save_data(tv1d, pr)) {
	pr_unmakefromfd(tv1d->fd, &tv1devdata);
	return (Pixrect *) NULL;
    }

    /* each instance has it's own ops structure since we redirect ops and
    /* data when non video plane groups are active */
    if ( !(pr->pr_ops = (struct pixrectops *)
	    malloc(sizeof(struct pixrectops)))) {
	pr_unmakefromfd (tv1d->fd, &tv1devdata);
	return (Pixrect *) NULL;
    }

    tv1d->planes = 0;
    tv1d->fd = curdd->fd;
    tv1d->flags = TV1_PRIMARY;

    /*
     * Initialize all plane groups, we have two here.  All of them are
     * described by the "fbdesc" table.
     */
    for (i = 0; i < TV1_NFBS; i++) {
	tv1d->fb[i].group = fbdesc[i].group;
	tv1d->fb[i].depth = fbdesc[i].depth;
	tv1d->fb[i].mprp.mpr.md_offset.x = 0;
	tv1d->fb[i].mprp.mpr.md_offset.y = 0;
	tv1d->fb[i].mprp.mpr.md_primary = 0;
	tv1d->fbsize[i] = fbdesc[i].size;
	/*
	 * if it is more than 1 bit deep, it is possible to plane mask it
	 */
	tv1d->fb[i].mprp.mpr.md_flags = fbdesc[i].allplanes != 0
	    ? MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	tv1d->fb[i].mprp.planes = fbdesc[i].allplanes;
	tv1d->fb[i].mprp.mpr.md_linebytes =
	    mpr_linebytes (fbdesc[i].size.x, fbdesc[i].depth);/* WRONG */
	
    }

    /*  
        make the emulated frame buffer pixrect
    */
    {
	struct fbgattr attr;
	struct pr_size fbsize;

	if (ioctl(fd, FBIOGATTR, &attr) == -1) {
		return (NULL);
	}
	/* Get bound frame buffer type */
	if (ioctl(fd, TVIOGBTYPE, &attr.fbtype.fb_type) == -1 ||
	    !IS_VALID_FBTYPE(attr.fbtype.fb_type)) {
		/* should free up stuff here too ! */
		return (NULL);
	}
	fbsize.x = attr.fbtype.fb_width;
	fbsize.y = attr.fbtype.fb_height;

	/* create the pixrect for the bound device */
	if ((tv1d->emufb = (*pr_makefun[attr.fbtype.fb_type])(fd, fbsize,
			attr.fbtype.fb_depth, &attr)) == 0) {
		return (NULL);
	}

	/*  move the attributes and ops over to the main pixrect as
	    the default
	*/
	tv1_setEmuType(pr, tv1d->emufb);

	/*  signal that the active fb is emulated  */
	tv1d->active = -1;
    }

    /*
        set up the pixrects for video and video enable planes
    */
    {
	register u_char *im_ptr;
	struct pr_pos *pos_ptr;
	extern struct pixrect *sup_create();

	im_ptr = (u_char *) curdd->va;
	tv1d->fb[0].mprp.mpr.md_image = (short *) im_ptr;
	tv1d->fb[0].mprp.mpr.md_linebytes = 4096;
	pr_video.pr_width = 640;
	pr_video.pr_height = 512;
	pr_video.pr_depth = 32;
	pr_video.pr_ops = &mem_ops;
	pr_video.pr_data = (caddr_t) &(tv1d->fb[0].mprp.mpr);
	im_ptr += FB_SIZE;
	tv1d->fb[1].mprp.mpr.md_image = (short *) im_ptr;
    	tv1d->fb[1].mprp.mpr.md_linebytes = CP_LINEBYTES;
	pr_video_enable.pr_width = CP_WIDTH;
	pr_video_enable.pr_height = CP_HEIGHT;
	pr_video_enable.pr_depth = CP_DEPTH;
	pr_video_enable.pr_ops = &mem_ops;
	pr_video_enable.pr_data = (caddr_t) &(tv1d->fb[1].mprp.mpr);
	tv1d->pr_video = &pr_video;
	pos_ptr = &((struct tv1_enable_plane *)im_ptr)->tv1_enable_offset;
	tv1d->pr_video_enable = sup_create(&pr_video_enable, pos_ptr,
					   tv1d->emufb->pr_size.x,
					   tv1d->emufb->pr_size.y );
	tv1d->tv1_csr = (struct csr *) curdd->va2;
    }

    return pr;
}

tv1_destroy (pr)
    Pixrect        *pr;
{
    struct tv1d_node **lpp, *tmp;
    int found;

    if (pr) {
	struct tv1_data *tv1d;

	if (tv1d = get_tv1_data(pr)) {
	    /* remove data pointer from list */
	    lpp = &tv1d_list;
	    found = 0;
	    while ((!found) && *lpp) {
		if ((*lpp)->pr == pr) {
		    found = 1;
		    tmp = *lpp;
		    *lpp = (*lpp)->next; /* unlink */
		    free((char *)tmp);
		} else {
		    lpp = &((*lpp)->next);
		}
	    }
	    if (!found) {
		 fprintf(stderr, "tv1_destroy: data not in list\n");
	    }
	    pr_destroy(tv1d->emufb);
	    pr_destroy(tv1d->pr_video_enable); 
	    if (tv1d->flags & TV1_PRIMARY) {
		pr_unmakefromfd (tv1d->fd, &tv1devdata);
	    } else {
		free ((char *)tv1d);
	    }
	    free((char *)pr->pr_ops);
	} else {
	    fprintf(stderr,"tv1_destroy: got non video pixrect\n");
	}
	free((char *)pr);
    }
    return 0;
}

/* insert a tv1_data structure pointer onto the list */
tv1_save_data(data, pr)
    struct tv1_data *data;
    struct pixrect *pr;
{
    struct tv1d_node *new;
    if ( !(new = (struct tv1d_node *)malloc(sizeof(struct tv1d_node)))) {
	fprintf(stderr, "tv1_save_data: malloc failure. \n");
	return (-1);
    }
    /* insert node at front of list */
    new->next = tv1d_list;
    new->data = data;
    new->pr = pr;
    tv1d_list = new;
    return(0);

}

struct tv1_data *
get_tv1_data(pr)
    struct pixrect *pr;
{
    struct tv1d_node **lpp;
    int found;

    lpp = &tv1d_list;
    found = 0;

    while ((!found) && *lpp) {
	if ((*lpp)->pr == pr) {
	    return((*lpp)->data);
        } else {
	    lpp = &((*lpp)->next);
	}
    }
    fprintf(stderr, "get_tv1_data: data not in list\n");
    return (struct tv1_data *)0;
}

