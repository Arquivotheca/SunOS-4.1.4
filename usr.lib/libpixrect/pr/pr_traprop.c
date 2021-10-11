#ifndef lint
static char sccsid[] = "@(#)pr_traprop.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Pixrect trapezon-clipped rasterop.
 *
 * PIX_DONTCLIP is ignored.
 *
 * Memory depth-1 operations not involving src are performed directly.
 * Other cases are handled by rendering the traprop into a temporary
 * depth-1 memory pixrect and then calling pr_stencil.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>
#include <pixrect/traprop.h>

#define max2(a,b) ((a) > (b) ? (a) : (b))
#define max3(a,b,c) ((a) > (b) ? max2(a, c) : max2(b, c))
#define min2(a,b) ((a) < (b) ? (a) : (b))
#define min3(a,b,c) ((a) < (b) ? min2(a, c) : min2(b, c))

/* tables to expedite counting 0's in a chain */
static u_char
	nybble0[16] = {	/* nybble0[i] = # 0's in nybble i in binary */
		4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0
	},
	byte0[256] = {	/* as for nybble0 but for byte i */
		8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
		4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0
	},
	boff0[4][16] = { /* boff0[i][j] = position of i-th 0 in nybble j */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 9,
		2, 2, 2, 2, 3, 3, 4, 9, 3, 3, 4, 9, 4, 9, 9, 9,
		3, 3, 4, 9, 4, 9, 9, 9, 4, 9, 9, 9, 9, 9, 9, 9
	};

struct curvestate {
	struct pr_chain	*chain;
	int rem0, *badr, bits, remb, dir, left, endx, clip, lclip, rclip;
	short x, relx;
	u_short	msk, *adr;
};

pr_traprop(dpr, dx, dy, t, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy;
	struct pr_trap t;
	int op;
	Pixrect *spr;
	int sx, sy;
{
	int vcnt, remt, y;
	struct curvestate left, right;
	int dw, dh;

	left.left = 1;
	right.left = 0;

	t.y0 = max3(t.y0, t.left->pos.y, t.right->pos.y);
	t.y0 = max2(t.y0, -dy);
	t.y1 = min2(t.y1, dpr->pr_size.y - dy);

	left.clip = 0;
	right.clip = dpr->pr_size.x;

	if (spr != 0) {
		/* clip to src, even if !PIXOP_NEEDS_SRC(op) */
		t.y0 = max2(t.y0, - sy);
		left.clip = max2(left.clip, dx - sx);
		t.y1 = min2(t.y1, - sy + spr->pr_size.y);
		right.clip = min2(right.clip,
				  dx - sx + spr->pr_size.x);
	}

	init(&left,  dx, dy, t.left, &t.y0, &t.y1);
	init(&right, dx, dy, t.right, &t.y0, &t.y1);

	/* lclip, rclip used only to export from init to size */
	left.clip = max2(left.clip, left.lclip);
	right.clip = min2(right.clip, right.rclip);
	dw = right.clip - left.clip;
	dh = t.y1 - t.y0;
	left.x = max2(left.x, left.clip);
	right.x = min2(right.x, right.clip);

	/* end of clipping */

	remt = t.y1 - t.y0, y = dy + t.y0;

	/* y + remt == dy + t.y1 preserved from here on */
	if (remt <= 0 || dw <= 0)
		return 0;

	/* direct traprop */
	if (!MP_NOTMPR(dpr) && dpr->pr_depth == 1 && 
		(spr == 0 || !PIXOP_NEEDS_SRC(op))) {
		register struct mpr_data *md = mpr_d(dpr);

		{
			register int rop = PIXOP_OP(op);

			if (spr == 0)
				rop = PIXOP_COLOR(op) ? rop >> 2 : rop & 3;

			rop |= rop << 2;

			if (rop == PIX_OPDST)
				return 0;

			if (md->md_flags & MP_REVERSEVIDEO)
				rop = PIX_OP_REVERSEDST(rop);

			op = rop;
		}

		left.adr = (u_short *) mprd_addr(md, left.x, y);
		left.msk = 0xffff >> mprd_skew(md, left.x, y);

		right.adr = (u_short *) mprd_addr(md, right.x, y);
		right.msk = 0xffff >> mprd_skew(md, right.x, y);

		while (remt > 0) {
			if (left.rem0  <= 0)
				next_chain(&left,  y, md);
			if (right.rem0 <= 0)
				next_chain(&right, y, md);
			vcnt = min3(remt, left.rem0, right.rem0);
			remt -= vcnt;
			y += vcnt;
			left.rem0 -= vcnt;
			right.rem0 -= vcnt;
			mem_set(vcnt, &left, &right, md->md_linebytes, op);
		}
	}
	/* too hard, traprop into temporary pixrect and stencil into dst */
	else {
		register Pixrect *tpr;
		int tdx, tdy;
		int errors;

		if (!(tpr = mem_create(dw, dh, 1)))
			return PIX_ERR;

		tdx = dx - left.clip;
		tdy = - t.y0;

		if (!(errors = pr_traprop(tpr, tdx, tdy, t, 
			PIX_SET, (Pixrect *) 0, 0, 0))) {
			sx -= tdx;
			sy -= tdy;

			errors = pr_stencil(dpr, left.clip, y, dw, dh, 
				op | PIX_DONTCLIP, tpr, 0, 0, spr, sx, sy);
		}

		(void) pr_destroy(tpr);

		return errors;
	}

	return 0;
}

/* offsets into f, initializes cs, clips *y1 to bottom of fall */
/*ARGSUSED*/
static
init(cs, x, y, f, y0, y1)
	register struct curvestate *cs;
	int x, y;
	struct pr_fall *f;
	int *y0, *y1;
{
	register struct pr_chain *ch;
	struct pr_chain *ch0;
	register yoff, yy, bbu, b0, boff;
	register u_char *bb;
	int oyoff, xoff;

	yoff = *y0 - f->pos.y;

	/* main goal is to offset into fall by yoff 0's */
	/* 1:  offset by chains */
	cs->x = cs->lclip = cs->rclip = x + f->pos.x;
	for (ch = f->chain; ch && ch->size.y <= yoff; ch = ch->next) {
		cs->x += ch->size.x, yoff -= ch->size.y;
		cs->lclip = min2(cs->lclip, cs->x);
		cs->rclip = max2(cs->rclip, cs->x);
	}
	if (!ch) {
		*y1 = *y0;
		return;
	}

	cs->chain = ch;					/* got pr_chain, ch */
	cs->dir = ch->size.x >= 0;
	cs->rem0 = ch->size.y - yoff;
	cs->endx = cs->x + ch->size.x;
	oyoff = yoff;				/* remember yoff at ch start */

	/* digression to clip *y1 to bottom of chain */
	ch0 = ch, b0 = cs->x;
	for (yy = yoff + *y1 - *y0; ch && yy > 0; ch = ch->next) {
		b0 += ch->size.x, yy -= ch->size.y;
		cs->lclip = min2(cs->lclip, b0);
		cs->rclip = max2(cs->rclip, b0);
	}
	if (yy > 0)
		*y1 -= yy;
	ch = ch0;	/* end of digression, pop back to saved state */

	/* 2:  offset by bytes */
	bb = (u_char *)ch->bits;
	while ((yoff -= byte0[*bb++]) >= 0);		/* actual search */
	yoff += byte0[*--bb];		/* now 0 <= yoff <= byte0[*bb] <= 7 */

	/* 3:  offset by nybble then bit */
	bbu = *bb >> 4, b0 = nybble0[bbu];
	boff = yoff < b0? boff0[yoff][bbu]: 4 + boff0[yoff-b0][*bb&15];

	/* int align bb */
	while ((int)bb & 3) {
		boff += 8;
		bb--;
	}
	if (boff >= 32) {
		boff -= 32;
		bb += 4;
	}

	xoff = ( ((int)bb - (int)ch->bits) << 3 ) + boff - oyoff;
	if (ch->size.x < 0)
		xoff = -xoff;
	cs->x += xoff;
	cs->lclip = min2(cs->lclip, cs->x);
	cs->rclip = max2(cs->rclip, cs->x);
	cs->badr = (int *)bb;
	if (boff)
		cs->bits = *cs->badr++ << (boff-1), cs->remb = 33 - boff;
	else
		cs->remb = 1;
	cs->relx = cs->x - cs->clip;
}

static
next_chain(cs, y, md)
	register struct curvestate *cs;
	int y;
	register struct mpr_data *md;
{
	register struct pr_chain *ch;

	cs->chain = ch = cs->chain->next;

	if (!ch)
		return;

	cs->rem0 = ch->size.y;
	cs->dir = ch->size.x >= 0;
	cs->badr = ch->bits;
	cs->remb = 1;
	cs->x = cs->endx;
	cs->endx += ch->size.x;
	cs->relx = cs->x - cs->clip;
	cs->x = cs->left? max2(cs->x, cs->clip): min2(cs->x, cs->clip);
	cs->adr = (u_short *)mprd_addr(md, cs->x, y);
	cs->msk = 0xffff >> mprd_skew(md, cs->x, y);
}


/* Clipping: left fall clipped to left edge, right fall to right edge, and
 * left fall to right fall.  This makes it redundant to clip left fall to
 * right edge or right fall to left edge; the eq test skips those cases.
 * Since eq is a macro the eq test is performed and optimized at compile time.
 * Side.relx is measured relative to side's clipping edge to simplify test.
 */

#define	IFleft(l,r) l
#define	IFright(l,r) r

#define across(side) \
while ((--side->remb ? \
		side->bits <<= 1 : \
		(side->remb = 32, side->bits = *side->badr++)) < 0) \
	if (!side->dir) { \
		if (CAT(IF,side)(--side->relx >= 0, --side->relx <= 0)) \
			if (side->msk == 0xffff) { \
				side->msk = 1; \
				side->adr--; \
			} \
			else { \
				side->msk <<= 1; \
				side->msk |= 1; \
			} \
	} \
	else { \
		if (CAT(IF,side)(++side->relx >= 0, ++side->relx <= 0)) \
			if (side->msk == 1) { \
				side->msk = ~0; \
				side->adr++; \
			} \
			else { \
				side->msk >>= 1; \
			} \
	}


static
mem_set(vcnt, left, right, h, op)
	register int vcnt;
	register struct curvestate *left, *right;
	int h, op;
{
	register width;
	register u_short *adr;

	while (vcnt-- > 0) {
		across(left);
		across(right);
		adr = left->adr;

		if ((width = right->adr - left->adr - 1) >= -1)
			switch(op) {
			case PIX_OPSET:
				if (width == -1)
					*adr |= left->msk & ~right->msk;
				else {
					*adr++ |= left->msk;
					PR_LOOP(width, *adr++ = ~0);
					if (~right->msk)
						*adr |= ~right->msk;
				}
				break;
			case PIX_OPCLR:
				if (width == -1)
					*adr &= ~left->msk | right->msk;
				else {
					*adr++ &= ~left->msk;
					PR_LOOP(width, *adr++ = 0);
					if (~right->msk)
						*adr &= right->msk;
				}
				break;
			case PIX_OPNOT(PIX_OPDST):
				if (width == -1)
					*adr ^= left->msk & ~right->msk;
				else {
					*adr++ ^= left->msk;
					PR_LOOP(width, *adr++ ^= ~0);
					if (~right->msk)
						*adr ^= ~right->msk;
				}
				break;
			}

		left->adr = (u_short *) ((caddr_t) left->adr + h);
		right->adr = (u_short *) ((caddr_t) right->adr + h);
	}
}
