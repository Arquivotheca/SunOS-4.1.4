#ifndef lint
static	char sccsid[] = "@(#)mem_getput.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect pixel get/put
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

/* pointer casting macros */
#define	P8(p)	((u_char *)(p))
#define	P16(p)	((u_short *)(p))
#define	P32(p)	((u_long *)(p))

/* leftmost bit in a byte */
#define	LEFT_BIT	(128)

mem_get(pr, x, y)
	register Pixrect *pr;
	register int x, y;
{
	register struct mpr_data *mprd;
	register caddr_t addr;

	if (x < 0 || x >= pr->pr_size.x || y < 0 || y >= pr->pr_size.y)
		return PIX_ERR;

	mprd = (struct mpr_data *) pr->pr_data;

	x += mprd->md_offset.x;
	y += mprd->md_offset.y;

	addr = (caddr_t) mprd->md_image + pr_product(mprd->md_linebytes, y);

	switch (pr->pr_depth) {
	case 1:
		if (!(mprd->md_flags & MP_REVERSEVIDEO))
			return P8(addr)[x >> 3] >> (~x & 7) & 1;
		else
			return ~(P8(addr)[x >> 3] >> (~x & 7)) & 1;

	case 8:
		return P8(addr)[x];

	case 16:
		return P16(addr)[x];

	case 32:
		return P32(addr)[x];

	default:
		return PIX_ERR;
	}
}

mem_put(pr, x, y, value)
	register Pixrect *pr;
	register int x, y;
	int value;
{
	register struct mprp_data *mprd;
	register caddr_t addr;
	register depth;

	if (x < 0 || x >= pr->pr_size.x || y < 0 || y >= pr->pr_size.y)
		return PIX_ERR;

	mprd = mprp_d(pr);

	x += mprd->mpr.md_offset.x;
	y += mprd->mpr.md_offset.y;

	addr = (caddr_t) mprd->mpr.md_image 
		+ pr_product(mprd->mpr.md_linebytes, y);

	depth = pr->pr_depth;

	if (depth == 1) {
		if (!(mprd->mpr.md_flags & MP_REVERSEVIDEO))
			if (value & 1)
				P8(addr)[x >> 3] |= LEFT_BIT >> (x & 7);
			else
				P8(addr)[x >> 3] &= ~LEFT_BIT >> (x & 7);
		else
			if (!(value & 1))
				P8(addr)[x >> 3] |= LEFT_BIT >> (x & 7);
			else
				P8(addr)[x >> 3] &= ~LEFT_BIT >> (x & 7);
	}
	else if (!(mprd->mpr.md_flags & MP_PLANEMASK)) {
		switch (depth) {
		case 8:  P8(addr)[x]  = value;	break;
		case 16: P16(addr)[x] = value;	break;
		case 32: P32(addr)[x] = value;	break;
		default: return PIX_ERR;
		}
	}
	else {
		/* put with plane mask */

#define	PUTM(type)	{ /* stupid compiler requires casts */	\
		register type d; \
		\
		addr = (caddr_t) & ((type *) addr)[x]; \
		d = * (type *) addr; \
		* (type *) addr = d ^ ((type) value ^ d) & \
			(type) mprd->planes; } \

		switch (depth) {
		case 8:  PUTM(u_char);		break;
		case 16: PUTM(u_short);		break;
		case 32: PUTM(u_long);		break;
		default: return PIX_ERR;
		}
	}

	return 0;
}
