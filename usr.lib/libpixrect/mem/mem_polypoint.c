#ifndef lint
static char sccsid[] = "@(#)mem_polypoint.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1989 Sun Microsystems, Inc.
 */
 

/*
 * Draw a list of points on the screen
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>
#include <pixrect/mem_rop_impl_ops.h>

/* types */
typedef	u_short	PIX1;
typedef	u_char	PIX8;
typedef	u_short	PIX16;
typedef	u_long	PIX32;

typedef	IFSPARC(u_long,PIX1) TMP1;
typedef IFSPARC(u_long,PIX8) TMP8;
typedef	IFSPARC(u_long,PIX16) TMP16;
typedef PIX32 TMP32;

/* leftmost bit of specified type */
#define	LEFTBIT(type)	((type) (1 << (sizeof (type) * 8 - 1)))


#define	POLY1() \
	{ \
		register PIX1 *d; \
		\
		switch (op) { \
		case  0: _POLY(0,IFTRUE,IFFALSE,PIX1,IFFALSE); break; \
		case  5: _POLY(5,IFTRUE,IFFALSE,PIX1,IFFALSE); break; \
		case 15: _POLY(F,IFTRUE,IFFALSE,PIX1,IFFALSE); break; \
		} \
	}

#define	POLYN(depth) \
	{ \
		register _CAT(PIX,depth) *d; \
		register _CAT(TMP,depth) t, m = planes, k = color; \
		\
		switch (op) { \
		case  4: _POLYN(4,IFTRUE,depth,IFFALSE); break; \
		case  6: _POLYN(6,IFFALSE,depth,IFFALSE); break; \
		case  7: _POLYN(7,IFTRUE,depth,IFFALSE); break; \
		case  8: _POLYN(8,IFFALSE,depth,IFFALSE); break; \
		case 12: _POLYN(C,IFFALSE,depth,IFFALSE); break; \
		case 14: _POLYN(E,IFFALSE,depth,IFFALSE); break; \
		} \
	}

#define	_POLYN(op,usetmp,depth,nomask) \
	_POLY(op,IFFALSE,usetmp,_CAT(PIX,depth),nomask)

#define	_POLY(op,depth1,usetmp,dtype,nomask) _STMT( \
	depth1(, \
		nomask(\
			k = _CAT(OP_ufgen,op)(k); \
		, \
			k = _CAT(OP_mfgen,op)(m,k); \
			m = _CAT(OP_mfmsk,op)(m); \
		) \
		daddr = (caddr_t) (((dtype *) daddr) + doffx); \
	) \
	PR_LOOPVP(n, \
		register int x = dx + pts->x; \
		register int y = dy + pts->y; \
		pts++; \
		if (noclip || CLIPCHECK(x, y)) { \
			d = (dtype *) PTR_ADD(daddr, \
				pr_product(y, linebytes)); \
			depth1( \
				x += doffx; \
				d += x >> 4; \
			, \
				d += x; \
			) \
			POINT(op,nomask,usetmp,t,d, \
				depth1(_CAT(OP_mfmsk,op) \
				(LEFTBIT(dtype) >> (x &= 15)), m),k); \
		} \
	); \
)

#define	CLIPCHECK(x, y) \
	((unsigned) (x) < dsizex && (unsigned) (y) < dsizey)

#define	POINT(op,nomask,usetmp,t,d,m,k) \
	nomask( \
		(*(d) = _CAT(OP_ufill,op)(*(d), (k))) \
	, \
		_STMT(usetmp((t) = *(d);,) \
		*(d) = _CAT(OP_mfill,op)(usetmp((t), *(d)), (m), (k));) \
	)

/********************************/

mem_polypoint(pr, dx, dy, n, pts, op)
	Pixrect *pr;
	int dx, dy;
	register int n;
	register struct pr_pos *pts;
	int op;
{
	register int noclip;
	register caddr_t daddr;
	int dsizex, dsizey, doffx;
	PIX32 color, planes;
	int linebytes;
	int depth;

	if (--n < 0)
		return PIX_ERR;

	{
		register Pixrect *rpr = pr;
		register struct mprp_data *mprd = mprp_d(rpr);
		register int rop = op, rcolor, rdepth;

		if (!(noclip = rop & PIX_DONTCLIP)) {
			dsizex = rpr->pr_size.x;
			dsizey = rpr->pr_size.y;
		}

		doffx = mprd->mpr.md_offset.x;

		linebytes = mprd->mpr.md_linebytes;
		daddr = (caddr_t) mprd->mpr.md_image;
		if (mprd->mpr.md_offset.y)
			daddr += pr_product(mprd->mpr.md_offset.y, linebytes);

		noclip = rop & PIX_DONTCLIP;
		rcolor = PIXOP_COLOR(rop);
		rop = PIXOP_OP(rop);

		if ((rdepth = pr->pr_depth) == 1) {
			depth = 0;

			if (mprd->mpr.md_flags & MP_REVERSEVIDEO)
				rop = PIX_OP_REVERSEDST(rop);

			if (rcolor)
				rop >>= 2;
			else
				rop &= 3;

			rop |= rop << 2;
		}
		else {
			register int rplanes = ~0, allplanes;

#define	INV	16
#define	CLR	32
#define	SET	(CLR | INV)

			static char optab[16 * 2] = {
				 8|CLR,	 4|INV,	 8|INV,	12|INV,
				 4,	 6|SET,	 6,	 7,
				 8,	 6|INV,	10,	14|INV,
				12,	 7|INV,	14,	14|SET,
				12|CLR,	 4|INV,	 8|INV,	12|INV,
				 4,	 6|SET,	 6,	 7,
				 8,	 6|INV,	10,	14|INV,
				12,	 7|INV,	14,	12|SET
			};

			if (mprd->mpr.md_flags & MP_PLANEMASK)
				rplanes = mprd->planes;

			switch (rdepth) {
			case 8: 
				depth = 1;
				allplanes = 0xff;
				break;
			case 16:
				depth = 2;
				allplanes = 0xffff;
				break;
			case 32:
				depth = 3;
				allplanes = 0xffffffff;
				break;
			default:
				return PIX_ERR;
			}

			rplanes &= allplanes;
			rcolor &= rplanes;

			if (rcolor == 0) {
				rop &= 3;
				rop |= rop << 2;
			}
			else if (rcolor == rplanes) {
				rcolor = 0;
				rop >>= 2;
				rop |= rop << 2;
			}

			if (rplanes == allplanes)
				rop += 16;

			rop = optab[rop];
			if (rop & CLR)
				rcolor = 0;
			if (rop & INV)
				rcolor = ~rcolor & rplanes;
			rop &= 15;

			planes = rplanes;	
			color = rcolor;
#undef	SET
#undef	CLR
#undef	INV
		}

		op = rop;
	}

	switch (depth) {
	case 0: POLY1(); break;
	case 1: POLYN(8); break;
	case 2: POLYN(16); break;
	case 3: POLYN(32); break;
	}

	return 0;
}
