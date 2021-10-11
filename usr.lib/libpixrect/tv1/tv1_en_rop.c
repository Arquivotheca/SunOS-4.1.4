
#ifndef lint
static char Sccsid[] = "@(#)tv1_en_rop.c	1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */


/* This file contains the "super" (as opposed to "sub") pixrect code. It is
 * a hack to deal with the flamingo enable plane. Basically a "super" pixrect
 * can be addressed in a space larger than it actually is, thus an existing
 * pixrect may appear as a sub pixrect to a larger virtual pixtrect. 
 * Accesses outside of the sub region have no effect. Ropping from enable
 * areas to enable areas will always clear the destination area.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/tv1var.h>


#define sup_data tv1_enable_data


#define SUP_XLATE_X(x_o, pr_o) \
   ( ((x_o) + (((struct sup_data *) ((pr_o)->pr_data)))->region_start.x) - \
    ((struct sup_data *)((pr_o)->pr_data))->offset->x)
#define SUP_XLATE_Y(y_o, pr_o) \
   ( ((y_o) + (((struct sup_data *) ((pr_o)->pr_data)))->region_start.y) - \
    ((struct sup_data *)((pr_o)->pr_data))->offset->y)


#define SUB_PIXRECT(o_pr)  (((struct sup_data *)(o_pr->pr_data))->sub_pixrect)
#define SUB_HEIGHT(o_pr) \
	(((struct sup_data *)(o_pr->pr_data))->sub_pixrect->pr_height)
#define SUB_WIDTH(o_pr) \
	(((struct sup_data *)(o_pr->pr_data))->sub_pixrect->pr_height)

#define PT_CLIP(x_o, min, max) \
   ( (x_o) < min ? min : ( (x_o) > max ? max : (x_o)))

extern struct pixrectops tv1_en_ops;

#ifndef KERNEL
extern char *malloc();
#endif

struct pixrect *
sup_create(pr, pos, w, h)
    struct pixrect *pr;	    /* pointer to "sub" pixrect */
    struct pr_pos  *pos;    /* pointer to its postion */
    int w, h;		    /* size of "Super" pixrect */
{
    static struct pixrect *npr;
#ifdef KERNEL
    static struct pixrect new_pixrect;
    static struct sup_data new_data;
/* Note that the kernel can only support one */
    npr = &new_pixrect;
    npr->pr_data = (caddr_t) &new_data;
#else
    if (!(npr = (struct pixrect *)malloc(sizeof(struct pixrect)))) {
	return(npr);
    }
    if (!(npr->pr_data = (caddr_t)malloc(sizeof(struct sup_data)))) {
	free((char *)npr);
	return((struct pixrect *)0);
    }
#endif
    npr->pr_ops = &tv1_en_ops;
    npr->pr_size.x = w;
    npr->pr_size.y = h;
    npr->pr_depth = pr->pr_depth;
    ((struct sup_data *)npr->pr_data)->sub_pixrect = pr;
    ((struct sup_data *)npr->pr_data)->offset = pos;
    ((struct sup_data *)npr->pr_data)->region_start.x = 0;
    ((struct sup_data *)npr->pr_data)->region_start.y = 0;
    return(npr);
}

sup_rop(dst, x, y, w, h, op, src, sx, sy)
    struct pixrect *dst;
    int x, y, w, h;
    int op;
    struct pixrect *src;
    int sx, sy;
{
    int sub_w, sub_h, x1, y1;

    /* now clip */
    if ((dst) && (dst->pr_ops->pro_rop == sup_rop)) {
	/* Do region clipping before translation */
	x1 = x + w;
	y1 = y + h;
	if (!(op & PIX_DONTCLIP)) {
	    if (x < 0) {
		sx -= x;
		x = 0;
	    } else if (x >= dst->pr_width ) {
		return(0);
	    }
	    if (y < 0) {
		sy -= y;
		y = 0;
	    } else if (y >= dst->pr_height) {
		return(0);
	    }
	    x1 = PT_CLIP(x1, 0, dst->pr_width);
	    y1 = PT_CLIP(y1, 0, dst->pr_height);
	}
	
	sub_w = ((struct sup_data *)(dst->pr_data))->sub_pixrect->pr_width;
	sub_h = ((struct sup_data *)(dst->pr_data))->sub_pixrect->pr_height;
	x = SUP_XLATE_X(x, dst);
	y = SUP_XLATE_Y(y, dst);
	x1 = SUP_XLATE_X(x1, dst);
	y1 = SUP_XLATE_Y(y1, dst);
	/* Now clip and adjust sx, sy */
	if (x < 0) {
	    sx -= x;
	    x = 0;
	}
	if (y < 0) {
	    sy -= y;
	    y = 0;
	}
	if (x1 > sub_w) {
	    x1 = sub_w;
	}
	if (y1 > sub_h) {
	    y1 = sub_h;
	}
	/* recompute width and get out of here if everthing is clipped */
	if ( ((w = x1 - x) <= 0) || ((h = y1 - y) <= 0)) {
	    return(0);
	}
	/* reassign dst to the real pixrect */
	dst = ((struct sup_data *)(dst->pr_data))->sub_pixrect;
	/* if src is also a super pixrect, just clear the dst */
	if ((src) && (src->pr_ops->pro_rop == sup_rop)) {
	    op = PIX_CLR;
	    src = (struct pixrect *) 0;
	    sx = sy =0;
	}
    } else if ((src) && (src->pr_ops->pro_rop == sup_rop)) {
	/* source is a SuperPixrect */
	x1 = sx + w;
	y1 = sy + h;
	if (!(op & PIX_DONTCLIP)) {
	    if (sx < 0) {
		x -= sx;
		sx = 0;
	    } else if (sx >= src->pr_width) {
		return(0);
	    }
	    if (sy < 0) {
		y -= sy;
		y = 0;
	    } else if (sy >= src->pr_height) {
		return(0);
	    }
	    x1 = PT_CLIP(x1, 0, src->pr_width);
	    y1 = PT_CLIP(y1, 0, src->pr_height);
	}
	sub_w = ((struct sup_data *)(src->pr_data))->sub_pixrect->pr_width;
	sub_h = ((struct sup_data *)(src->pr_data))->sub_pixrect->pr_height;
	
	sx = SUP_XLATE_X(sx, src);
	sy = SUP_XLATE_Y(sy, src);
	x1 = SUP_XLATE_X(x1, src);
	y1 = SUP_XLATE_Y(y1, src);
	if (sx < 0) {
	    x -= sx;	/* adjust destination */
	    sx = 0;
	}
	if (sy < 0) {
	    y -= sy;
	    sy = 0;
	}
	if (x1 > sub_w) {
	    x1 = sub_w;
	}
	if (y1 > sub_h) {
	    y1 = sub_h;
	}
	/* recompute width and get out of here if everthing is clipped */
	if ( ((w = x1 - sx) <= 0) || ((h = y1 - sy) <= 0)) {
	    return(0);
	}
	src = ((struct sup_data *)(src->pr_data))->sub_pixrect;
    }
    /* we should now be all clipped and translated do the ROP */
    if (( x+w > dst->pr_width) || (y+h > dst->pr_height) ||
	  (x < 0) || (y < 0)) {
	printf("sup_rop: should have clipped\n");
	return(PIX_ERR);
    }
    return(pr_rop(dst, x, y, w, h, op, src, sx, sy));
}

sup_nop()
{
    return(0);
}

#ifndef KERNEL
sup_vector(pr, x0, y0, x1, y1, op, value)
    struct pixrect *pr;
    int x0, y0, x1, y1, op, value;
{
    int xmax, ymax;
    /* This will only work with Horizontal and vertical lines !! */
    x0 = SUP_XLATE_X(x0, pr);
    y0 = SUP_XLATE_Y(y0, pr);
    x1 = SUP_XLATE_X(x1, pr);
    y1 = SUP_XLATE_Y(y1, pr);
    /* Note: no region clipping yet ! */
    xmax = ((struct sup_data *)(pr->pr_data))->sub_pixrect->pr_width - 1;
    ymax = ((struct sup_data *)(pr->pr_data))->sub_pixrect->pr_height - 1;
    x0 = PT_CLIP(x0, 0, xmax);
    y0 = PT_CLIP(y0, 0, ymax);
    x1 = PT_CLIP(x1, 0, xmax);
    y1 = PT_CLIP(y1, 0, ymax);
    return
	(
	    pr_vector(((struct sup_data *)(pr->pr_data))->sub_pixrect,
	    x0, y0, x1, y1, op, value)
	);
    
}


struct pixrect *
sup_region(pr, x, y, w, h)
    struct pixrect *pr;
    int x, y, w, h;
{
    struct pixrect *npr;
    if (!(npr = (struct pixrect *)malloc(sizeof(struct pixrect)))) {
	return(npr);
    }
    if (!(npr->pr_data = (caddr_t)malloc(sizeof(struct sup_data)))) {
	free((char *)npr);
	return((struct pixrect *)0);
    }
    npr->pr_ops = &tv1_en_ops;
    npr->pr_size.x = w;
    npr->pr_size.y = h;
    npr->pr_depth = pr->pr_depth;
    ((struct sup_data *)npr->pr_data)->offset =
	 ((struct sup_data *)pr->pr_data)->offset; /* copy pointer to offset */
    ((struct sup_data *)npr->pr_data)->region_start.x =
	 x + ((struct sup_data *)pr->pr_data)->region_start.x;
    ((struct sup_data *)npr->pr_data)->region_start.y = y +
	((struct sup_data *)pr->pr_data)->region_start.y;
    npr->pr_depth = pr->pr_depth;
    ((struct sup_data *)npr->pr_data)->sub_pixrect =
	 ((struct sup_data *)pr->pr_data)->sub_pixrect;
    ((struct sup_data *)npr->pr_data)->offset =
	((struct sup_data *)pr->pr_data)->offset; 
    return(npr);
}

sup_destroy(pr)
    struct pixrect *pr;
{    
    if (pr) {
	free((char *)pr->pr_data);
	free((char *)pr);
    }
    return(0);
}

sup_get(pr, x, y)
    struct pixrect *pr;
    int x, y;
{
    /* Region clip */
    if ((x < 0) || (x >= pr->pr_width) || (y < 0) ||( y >= pr->pr_height)) {
	return(0);
    }
    x = SUP_XLATE_X(x, pr);
    y = SUP_XLATE_Y(y, pr);
    if ((x < 0) || (x >= SUB_WIDTH(pr)) || (y < 0) || (y >= SUB_HEIGHT(pr) )) {
	return(0);
    }
    return(pr_get(SUB_PIXRECT(pr), x, y));

}

sup_put(pr, x, y, value)
    struct pixrect *pr;
    int x, y, value;
{
    /* Region clip */
    if ((x < 0) || (x >= pr->pr_width) || (y < 0) ||( y >= pr->pr_height)) {
	return(0);
    }
    x = SUP_XLATE_X(x, pr);
    y = SUP_XLATE_Y(y, pr);
    if ((x < 0) || (x >= SUB_WIDTH(pr)) || (y < 0) || (y >= SUB_HEIGHT(pr))) {
	return(0);
    }
    return(pr_put(SUB_PIXRECT(pr), x, y, value));
}
#endif !KERNEL

sup_putcolormap(pr, index, count, red, green, blue)
    struct pixrect *pr;
    int index, count;
    unsigned char red[], green[], blue[];
{
    return(pr_putcolormap(SUB_PIXRECT(pr),index, count, red, green, blue));
}

#ifndef KERNEL
sup_getcolormap(pr, index, count, red, green, blue)
    struct pixrect *pr;
    int index, count;
    unsigned char red[], green[], blue[];
{
    return(pr_getcolormap(SUB_PIXRECT(pr),index, count, red, green, blue));
}
#endif !KERNEL

sup_putattributes(pr, planes)
    struct pixrect *pr;
    unsigned int *planes;
{
    return(pr_putattributes(SUB_PIXRECT(pr), planes));
}

#ifndef KERNEL
sup_getattributes(pr, planes)
    struct pixrect *pr;
    unsigned int *planes;
{
    return(pr_getattributes(SUB_PIXRECT(pr), planes));
}

sup_batchrop(dpr, dx, dy, op, items, n)
    struct pixrect *dpr;
    int dx, dy, op;
    struct pr_prpos items[];
    int n;
{
    int i;
    
    for (i=0; i < n; i++) {
	dx += items[i].pos.x;
	dy += items[i].pos.y;
	pr_rop (dpr, dx, dy, items[i].pr->pr_width, items[i].pr->pr_height, 
		op, items[i].pr, 0, 0);
    }
    return(0);
}

sup_stencil(dpr, dx, dy, dw, dh, op, stpr, stx, sty, spr, sx, sy)
    struct pixrect	*dpr;
    int			dx, dy, dw, dh;
    struct pixrect	*stpr;
    int			stx, sty;
    struct pixrect	*spr;
    int			sx, sy;
{
    struct pixrect *dst, *src, *sten;
    int            result;

    /* This function will be slow because it simply creates memory pixrects */
    /* for each super pixrects, copies to them, and does the stencil
    /* operation with non super pixrects. */
    
    if ((dpr) && (dpr->pr_ops->pro_stencil == sup_stencil)) {
	/* Copy the pixrect to a memory pixrect */
	if (dst = mem_create(dpr->pr_width, dpr->pr_height, dpr->pr_depth)) {
	    pr_rop(dst, 0, 0, dpr->pr_width, dpr->pr_height, PIX_SRC,
	           dpr, 0, 0);
	} else {
	    return(PIX_ERR);
	}
    } else {
	dst = dpr;
    }
    if ((spr) && (spr->pr_ops->pro_stencil == sup_stencil)) {
	if (src = mem_create(spr->pr_width, spr->pr_height, spr->pr_depth)) {
	    pr_rop(src, 0, 0, spr->pr_width, spr->pr_height, PIX_SRC,
	           spr, 0, 0);
	} else {
	    return(PIX_ERR);
	}
    } else {
	src = spr;
    }
    if ((stpr) && (stpr->pr_ops->pro_stencil == sup_stencil)) {
	if (sten = mem_create(stpr->pr_width,stpr->pr_height,stpr->pr_depth)) {
	    pr_rop(sten, 0, 0, stpr->pr_width, stpr->pr_height, PIX_SRC,
		   stpr, 0, 0);
	} else {
	    return(PIX_ERR);
	}
    } else {
	sten = stpr;
    }
    result = pr_stencil(dst, dx, dy, dw, dh, op, sten, stx, sty, src, sx, sy);
    if (dst != dpr) {
	/* copy image back to super pixrect */
	pr_rop(dpr, 0, 0, dpr->pr_width, dpr->pr_height, PIX_SRC, dst, 0, 0);
	pr_destroy(dst);
    }
    if (src != spr) {
	pr_destroy(src);
    }
    if (sten != stpr) {
	pr_destroy(sten);
    }
    return(result);
}

#endif !KERNEL
