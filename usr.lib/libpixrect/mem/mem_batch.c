#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)mem_batch.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986-1989, Sun Microsystems,  Inc.
 */

/*
 * Memory batchrop
 *
 * Warnings:
 *	- not intended to be compiled -O1
 *	- not optimized for 680x0
 *	- adding opcodes may break things
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/mem_rop_impl_ops.h>

#define	INRANGE(x,L,U)	((unsigned) ((x) - (L)) <= (unsigned) ((U) - (L)))

typedef	u_long	ROP8_T;		/* type used for 8 bit dst rops */
typedef	u_short	ROP1_T;		/* type used for 1 bit dst rops */
typedef	u_long	ROP1W_T;	/* two ROP1_Ts (SHORT_ALIGN only) */

/* XXX: 4.0 compiler grok, probably not needed in 4.1 */
#if sparc && SUNOS < 41
#define	ROP1TMP		ROP1W_T	/* prevent superfluous masking */
#else
#define	ROP1TMP		ROP1_T
#endif

#undef	OP_mfgen2
#undef	OP_mfill2
#define	OP_mfgen2(m,c)	((m) & (c))
#define	OP_mfill2(d,m,k) ((d) & ~(k))	

#define	IF11	IFTRUE
#define	IF18	IFFALSE

#define	CASE_BATCH(depth,op) _STMT( \
	if ((op) == 0) /* s */ \
		_CAT(BATCH,depth)(C,IFTRUE); \
	else \
		switch (op) { \
		case 1: /* d | s */ \
			_CAT(BATCH,depth)(E,IFFALSE); break; \
		_CAT(IF1,depth)( \
			case 2: /* ~s */ \
				_CAT(BATCH,depth)(3,IFTRUE); break; \
		,) \
		case 3: /* d & ~s */ \
			_CAT(BATCH,depth)(2,IFFALSE); break; \
		case 4: /* d ^ s */ \
			_CAT(BATCH,depth)(6,IFFALSE); break; \
		})

/*
 * 8 bit macros
 *
 * We pull 4 bits at a time off the right end of the source font word and
 * use them to index into a pre-masked 1-bit to 8-bit expansion table.
 *
 * Special case loops are used for characters which require writing 2 or 3
 * dst words per scan line (all 5-9 pixel wide characters).
 */

#define	BATCH8(op,usemask) _STMT( \
	usemask(ROP8_T m = planes; rm &= m; lm &= m;,) \
	if (w == 0 && lm) { \
		sskew -= PRESKEW; \
		BATCH8X(op,usemask,IFFALSE,IFFALSE); \
	} \
	else if (w == 1) { \
		sskew -= PRESKEW; \
		doffset -= 4; \
		BATCH8X(op,usemask,IFFALSE,IFTRUE); \
	} \
	else \
		BATCH8X(op,usemask,IFTRUE,IFFALSE); \
)

#define BATCH8X(op,usemask,loop,mid) _STMT( \
	PR_LOOPVP(h, \
		sword = (loop(*s++ << PRESKEW, *s++)) >> sskew; \
		WRITE8(op, usemask, d[0], rm, rm & SRC8(sword)); \
		loop(PR_LOOP(w, \
			sword >>= 4; \
			d--; \
			WRITE8(op, usemask, d[0], m, SRC8(sword)); ); \
		if (lm) \
		, ) \
			WRITE8(op, usemask, d[-1], mid(m,lm), \
				mid(,lm &) SRC8(sword >> 4)); \
		mid(WRITE8(op, usemask, d[-2], lm, lm & SRC8(sword >> 8));,) \
		PTR_INCR(ROP8_T *, d, doffset)); \
)

#define	PRESKEW	5

#define	SRC8(s) (* (ROP8_T *) PTR_ADD(stp, (s) & (15 << 2)))

#define	WRITE8(op,usemask,d,m,s) \
	((d) = _CAT(OP_mfill,op)((d), \
		usemask(_CAT(OP_mfmsk,op)(m), @Error), (s)))


/*
 * 1 bit macros
 *
 * There are 3 flavors of inner loop:
 *
 * 1. dst writes fit in 16 bit word
 * 2. dst writes fit in aligned 32 bit word
 * 3. dst writes straddle 32 bit boundary
 *
 * We could use #2 instead of #1, but performance would suffer on frame
 * buffers with an 8 or 16 bit wide data bus.
 *
 * On machines which do not require aligned 32 bit accesses we use #2
 * instead of #3.
 */

#define	BATCH1(op,usemask) _STMT( \
	if (lskew >= 0) { \
		usemask(ROP1TMP dm = _CAT(OP_mfmsk,op)(sm >> lskew); ,) \
		PR_LOOPVP(h, \
			WRITE1(op, usemask, *d, dm, \
				_CAT(OP_mfgen,op)(sm, *s++) >> lskew); \
			PTR_INCR(ROP1_T *, d, doffset)); \
	} \
	else IFSHORT_ALIGN(, if (((int) d & 2) == 0)) { \
		usemask(ROP1W_T dm; ,) \
		lskew = -lskew; \
		usemask(dm = _CAT(OP_mfmsk,op)(sm << lskew);, )\
		PR_LOOPVP(h, \
			WRITE1(op, usemask, * (ROP1W_T *) d, dm, \
				_CAT(OP_mfgen,op)(sm, *s++) << lskew); \
			PTR_INCR(ROP1_T *, d, doffset)); \
	} \
	IFSHORT_ALIGN(, else { \
		IF68000(short,int) rskew = -lskew; \
		usemask(ROP1TMP lm; ROP1TMP rm; ,) \
		lskew += 16; \
		usemask(lm = _CAT(OP_mfmsk,op)(sm >> lskew); \
			rm = _CAT(OP_mfmsk,op)(sm << rskew); ,) \
		PR_LOOPVP(h, \
			ROP1TMP sword = _CAT(OP_mfgen,op)(sm, *s++); \
			WRITE1(op, usemask, d[0], lm, sword >> lskew); \
			WRITE1(op, usemask, d[1], rm, sword << rskew); \
			PTR_INCR(ROP1_T *, d, doffset)); \
	} ) \
)


#define	WRITE1(op,usemask,d,m,s) \
	((d) = _CAT(OP_mfill,op)((d), usemask((m), @Error), (s)))


/**************************************************************/


mem_batchrop(dpr, dx, dy, op, src, count)
	Pixrect *dpr;
	int dx, dy, op;
	struct pr_prpos *src;
	int count;
{
	int depth8;		/* 1 -> 8 op flag */
	int clip;		/* clipping flag */
	int dsizex, dsizey;	/* dst pixrect size */
	MPR_T *dimage;		/* dst image */
	UMPR_T *dleft;		/* first word of current line in dst */
	int dlinebytes;		/* dst bytes per line */
	int doffx;		/* dst pixrect offset */
	int opcode;		/* op function code only (0-15) */
	ROP8_T planes;		/* plane mask */
	ROP8_T *stab;		/* expansion table pointer */
	ROP8_T stab8[16];	/* space for generated expansion table */
	int errors = 0;		/* accumulated errors */

#ifdef PIC			/* memory pixrect ops vector */
	struct pixrectops *pr_mem_ops = &mem_ops;
#else
#define	pr_mem_ops &mem_ops
#endif

	/* table of implemented opcodes */
	static char optab[] = {
		-1, -1,  3,  2,   -1, -1,  4, -1,
		-1, -1, -1, -1,    0, -1,  1, -1
	};

	/* 4 pixel, 1 bit to 8 bit expansion table */
	static ROP8_T exptab8[16] = {
		0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff,
		0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
		0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff,
		0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff
	};

	/* expansion table for (color & planes) == 1 */
	static ROP8_T exptab8c1[16] = {
		0x00000000, 0x00000001, 0x00000100, 0x00000101,
		0x00010000, 0x00010001, 0x00010100, 0x00010101,
		0x01000000, 0x01000001, 0x01000100, 0x01000101,
		0x01010000, 0x01010001, 0x01010100, 0x01010101
	};

	{
		register struct mprp_data *dprd = mprp_d(dpr);

		opcode = PIXOP_OP(op);

#if !SHORT_ALIGN
		if (dprd->mpr.md_linebytes & 2)
			opcode = PIX_OPCLR;
		else
#endif !SHORT_ALIGN
		switch(dpr->pr_depth) {
		case 1:
			if (dprd->mpr.md_flags & MP_REVERSEVIDEO)
				opcode = PIX_OP_REVERSEDST(opcode);
			depth8 = 0;
			break;

		case 8: {
			register ROP8_T color;

			if (opcode == PIX_OPNOT(PIX_OPSRC)) {
				opcode = PIX_OPCLR;
				break;
			}

			if (color = PIX_OPCOLOR(op)) {
				color &= 255;
				color |= color << 8;
				color |= color << 16;
			}
			else
				color = ~0;

			planes = ~0;
			if (dprd->mpr.md_flags & MP_PLANEMASK) {
				planes = dprd->planes & 255;
				planes |= planes << 8;
				planes |= planes << 16;
				color &= planes;
			}

			if (color == 0x01010101)
				stab = exptab8c1;
			else {
				register int i;

				for (stab = stab8, i = 0; i < 16; i++)
					stab[i] = exptab8[i] & color;
			}

			depth8 = ~0;
			break;
		}

		default:
			opcode = PIX_OPCLR;
			break;
		}

		/* see if we implement this op */
		if ((opcode = optab[opcode]) < 0)
			return gen_batchrop(dpr, dx, dy, op, src, count);

		/* save dst size for later clipping */
		clip = 0;
		if (!(op & PIX_DONTCLIP)) {
			clip = 1;
			dsizex = dpr->pr_size.x;
			dsizey = dpr->pr_size.y;
		}

		/* save dst pixrect x offset, image, linebytes */
		doffx = dprd->mpr.md_offset.x;
		dimage = dprd->mpr.md_image;
		dlinebytes = dprd->mpr.md_linebytes;

		/* compute address of left pixel on first line of dst */
		dleft = (UMPR_T *) PTR_ADD(dimage,
			pr_product(dlinebytes, dy + dprd->mpr.md_offset.y));

		/* align dleft to long boundary for 8 bit code */
		if ((int) dleft & 2 && depth8) {
			dleft--;
			doffx += 2;
		}
	}

	while (--count >= 0) {
		int dw, dh;	/* clipped width and height */
		caddr_t dfirst;	/* first pixel of dst */
		UMPR_T *sfirst;	/* first pixel of src */
		Pixrect *spr;	/* current source pixrect */
		struct mpr_data *sprd;

		dx += src->pos.x;
		if (src->pos.y != 0) {
			dy += src->pos.y;
			dleft = (UMPR_T *) PTR_ADD(dleft,
				pr_product(dlinebytes, src->pos.y));
		}
		dfirst = (caddr_t) dleft;

		spr = src->pr;
		src++;
		if (!spr)
			continue;

		dw = spr->pr_size.x;
		dh = spr->pr_size.y;

		if (spr->pr_ops != pr_mem_ops &&
			MP_NOTMPR(spr) || 
			!((sprd = mpr_d(spr))->md_flags & MP_FONT) &&
				(spr->pr_depth != 1 ||
				sprd->md_linebytes != 2 ||
				sprd->md_offset.x != 0 ||
				sprd->md_flags & MP_REVERSEVIDEO ||
				sprd->md_image == dimage))
			goto toohard;

		/* 
		 * compute address of first line of src
		 * src linebytes must equal sizeof(*sfirst) !
		 */
		sfirst = (UMPR_T *) sprd->md_image + sprd->md_offset.y;

		if (clip) {
			register int extra;

			/* clipped on left? */
			if (dx < 0)
				/* completely clipped? */
				if (dx + dw <= 0)
					continue;
				else
					goto toohard;

			/* clipped on right? */
			if ((extra = dx + dw - dsizex) > 0)
				dw -= extra;

                        /* clipped on bottom? */
                        if ((extra = dy + dh - dsizey) > 0)
                                dh -= extra;

			/* clipped on top? */
			if ((extra = dy) < 0) {
				dh += extra;
				dfirst -= 
					pr_product(dlinebytes, extra);
				sfirst -= extra;
			}
		}

		/* clipped to nothing? */
		if (dw <= 0 || dh <= 0)
			continue;

		if (!depth8) {
			register UMPR_T *s, sm;
			register IF68000(short,int) lskew;
			register ROP1_T *d = (ROP1_T *)
				(dfirst + (dx + doffx >> 4 << 1));
			register LOOP_T h = dh - 1;
			register int doffset = dlinebytes;

			s = sfirst;
			sm = 0xffff0000 >> dw;

			lskew = (dx + doffx) & 15;

			if (lskew + dw > 16)
				lskew -= 16;

			CASE_BATCH(1,opcode);
		}
		else {
			register ROP8_T *d;
			register ROP8_T rm, lm;
			register int sskew, doffset;
			register LOOP_T w;

			/* XXX clean this up */
			{
				register u_int lbyte, rbyte;

				/* dst right byte number */
				rbyte = (lbyte = dx + doffx) + dw;

				/* point to right dst word */
				d = (ROP8_T *) (dfirst + (rbyte & ~3));

				/* no. of words */
				w = (rbyte >> 2) - (lbyte + 3 >> 2);

				sskew = 16 + (PRESKEW - 2) - dw;

				/* compute right mask */
				rm = ~0;
				if (rbyte &= 3) {
					rm <<= 4 - rbyte << 3;
					sskew -= 4 - rbyte;
				}
				else {
					w--;
					d--;
				}

				/* compute left mask */
				lm = 0;
				if (lbyte &= 3) {
					lm = (ROP8_T) ~0 >> (lbyte << 3);

					/* left partial only */
					/* XXX is this worthwhile? */
					if (w < 0) {
						rm &= lm;
						lm = 0;
						w = 0;
					}
				}
				else if (INRANGE(w, 1, 2)) {
					lm = ~0;
					w--;
				}
					
				/* dst line to line offset */
				doffset = dlinebytes + (w << 2);
			}

			{
				register ROP8_T *stp = stab;
				register UMPR_T *s = sfirst;
				register u_long sword;
				register LOOP_T h = dh - 1;

				CASE_BATCH(8,opcode);
			}
		}
		continue;

toohard:
		errors |= mem_rop(dpr, dx, dy, dw, dh, op, spr, 0, 0);
	}

	return errors;
}

/* generic batchrop -- should be in a separate file */
static
gen_batchrop(dpr, dx, dy, op, src, count)
	register Pixrect *dpr;
	register int dx, dy, op;
	register struct pr_prpos *src;
	register int count;
{
	register Pixrect *spr;
	register int errors = 0;

	while (--count >= 0) {
		dx += src->pos.x;
		dy += src->pos.y;

		if (spr = src->pr) 
			errors |= pr_rop(dpr, dx, dy, 
				spr->pr_size.x, spr->pr_size.y,
				op, spr, 0, 0);

		src++;
	}

	return errors;
}
