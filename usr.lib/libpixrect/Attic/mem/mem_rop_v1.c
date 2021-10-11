#ifndef lint
static  char sccsid[] = "@(#)mem_rop_v1.c 1.1 94/10/31 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Memory pixrect raster op
 */

#ifdef KERNEL
#include "../h/types.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/memreg.h"
#include "../pixrect/memvar.h"
#else
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#endif

extern int	mem_needinit;
extern char	pr_reversedst[];
extern char	pr_reversesrc[];

extern short mem_addla2a4, mem_addla3a5, mem_addql2a4, mem_andwd0, mem_andwd1,
	mem_dbrad6, mem_dbrad7, mem_movla4d1, mem_movld6, mem_movla4Md1,
	mem_moveq0d6, mem_movwa4Ma5M, mem_movla4Ma5M, mem_movwa4Md1,
	mem_movwa4Pa5P, mem_movwa4a5P, mem_movla4Pa5P, mem_movla4a5P,
	mem_movwa4Pd1, mem_movwa4d1, mem_movwa5Md0, mem_movwa5d0, mem_movwd6,
	mem_movwd0a5, mem_movwd0a5P, mem_orwd1d0, mem_rolld1, mem_rorld1,
	mem_rts, mem_subql2a4, mem_swapd1;

static short sml, dml, smr, dmr;
static short *srcincr, *dstincr, constsrc;

mem_rop(dst, op, src)
	struct pr_subregion dst;
	struct pr_prpos src;
{
	register int d7_line_cnt, d6=0, d5_smr, d4_dmr, d3_sml, d2_dml;
#define d4_dir d4_dmr
	register short *a5_dp, *a4_sp, *a3_dst_incr, *a2_src_incr;
#define a3_dprd ((struct mpr_data *)a3_dst_incr)
#define a2_sprd ((struct mpr_data *)a2_src_incr)
	short sskew, dskew, dsize;
	int originalop = op;
	int color;
	int (*mem_code)();
	short mem_codespace[MEM_CODESIZE+MEM_CODESIZETOUNROLL];

	if (mem_needinit)
		mem_init();
	if (MP_NOTMPR(dst.pr)) {
		return ((*dst.pr->pr_ops->pro_rop)(dst, op, src));
	}
	if (src.pr && (dst.pr->pr_depth != src.pr->pr_depth))
		return (-1);
	if (op & PIX_DONTCLIP)
		op &= ~PIX_DONTCLIP;
	else {
		pr_clip(&dst, &src);
		if (dst.size.x <= 0 || dst.size.y <= 0)
			return (0);
	}
	color = PIX_OPCOLOR( op);
	op = op&0x1F;
	if ((u_int)op > PIX_SET)
		return (-1);
	a3_dprd = mpr_d(dst.pr);
	if (a3_dprd->md_flags & MP_REVERSEVIDEO)
		op = (pr_reversedst[op>>1]<<1);
	if (PIXOP_NEEDS_SRC(op) && src.pr) {
		if (MP_NOTMPR(src.pr)) {
		    return ((*src.pr->pr_ops->pro_rop)(dst, originalop, src));
		}
		a2_sprd = mpr_d(src.pr);
	} else {
		src.pr = 0;
		a2_sprd = a3_dprd;	/* dummy value */
	}
	if (src.pr == 0 || a2_sprd->md_image != a3_dprd->md_image)
		d4_dir = ROP_UP|ROP_LEFT;
	else {
		d4_dir = rop_direction(src.pos, a2_sprd->md_offset,
			dst.pos, a3_dprd->md_offset);
		if (rop_isdown(d4_dir)) {
			src.pos.y += dst.size.y-1;
			dst.pos.y += dst.size.y-1;
		}
		if (rop_isright(d4_dir)) {
			src.pos.x += dst.size.x-1;
			dst.pos.x += dst.size.x-1;
		}
	}
	constsrc = 0;
	if (dst.pr->pr_depth > 1) {	/* handle more bits per pixel */
		if (src.pr) {
			if (a2_sprd->md_flags & MP_REVERSEVIDEO)
				op = (pr_reversesrc[op>>1]<<1);
			a4_sp = (short *)mprd8_addr(a2_sprd,src.pos.x,src.pos.y,
				src.pr->pr_depth);
		} else {
			if (dst.pr->pr_depth <= 8) color |= color<<8;
			if (dst.pr->pr_depth <= 16) color |= color<<16;
			a4_sp = (short*)(&color);
			constsrc = 1;
		}
		a5_dp = (short *)mprd8_addr(a3_dprd, dst.pos.x, dst.pos.y,
			dst.pr->pr_depth);
		dsize = pr_product( dst.size.x, dst.pr->pr_depth);
		if (rop_isright(d4_dir)) {
			(char *)a4_sp += dst.pr->pr_depth/8 - 1;
			sskew = ((int)a4_sp & 1) ? 16 : 8;
			a4_sp = ((short *)((int)a4_sp & ~1)) + 1; 
			(char *)a5_dp += dst.pr->pr_depth/8 - 1;
			dskew = ((int)a5_dp & 1) ? 16 : 8;
			a5_dp = ((short *)((int)a5_dp & ~1)) + 1; 
		} else {
			sskew = ((int)a4_sp & 1) ? 8 : 0;
			a4_sp = (short *)((int)a4_sp & ~1); 
			dskew = ((int)a5_dp & 1) ? 8 : 0;
			a5_dp = (short *)((int)a5_dp & ~1); 
		}
	} else {
		color = (color) ? -1 : 0;
		if (src.pr) {
			if (a2_sprd->md_flags & MP_REVERSEVIDEO)
				op = (pr_reversesrc[op>>1]<<1);
			a4_sp = mprd_addr(a2_sprd,src.pos.x,src.pos.y);
			sskew = mprd_skew(a2_sprd, src.pos.x, src.pos.y);
		} else {
			a4_sp = (short*)(&color);
			sskew = 0;
			constsrc = 1;
		}
		a5_dp = mprd_addr(a3_dprd, dst.pos.x, dst.pos.y);
		dskew = mprd_skew(a3_dprd, dst.pos.x, dst.pos.y);
		dsize = dst.size.x;
		if (rop_isright(d4_dir))
			sskew++, dskew++, a4_sp++, a5_dp++;
	}
	if (rop_isdown(d4_dir)) {
		srcincr = (short *)-a2_sprd->md_linebytes;
		dstincr = (short *)-a3_dprd->md_linebytes;
	} else {
		srcincr = (short *)a2_sprd->md_linebytes;
		dstincr = (short *)a3_dprd->md_linebytes;
	}
	sml = smr = dml = dmr = 0;
	mem_code = (*(rop_isright(d4_dir) ? mem_rlcode : mem_lrcode))
	    (dskew, dsize, op, sskew, mem_codespace);
	d7_line_cnt = dst.size.y;
	a2_src_incr = srcincr;
	a3_dst_incr = dstincr;
	asm("	moveq	#-1,d1");
	d2_dml = dml;
	d4_dmr = dmr;
	d3_sml = sml;
	d5_smr = smr;
#ifdef lint
	d7_line_cnt = d7_line_cnt;
	d6 = d6;
	d5_smr = d5_smr;
	d4_dmr = d4_dmr;
	d3_sml = d3_sml;
	d2_dml = d2_dml;
#endif
	flush();
	(*mem_code)();
	return (0);
}

/*
 * Macros which generate pieces of code for a rasterop.
 * All store generated code through short pointer pc.
 */

/*
 * Set register D6 (loop counter) to cnt (must be > 0), for a dbranch loop.
 */
/* Maximum code size: 3 words */
#define	SET_D6(cnt) {							\
    register int t = (cnt)-1;	/* dbra stops at -1 rather than 0 */	\
    if (t > 32767) {							\
	*pc++ = mem_movld6;		/* movl #cnt,d6 */		\
	*pc++ = (t)>>16;						\
	*pc++ = (t);							\
    } else if ((t) > 127) {						\
	*pc++ = mem_movwd6;		/* movw #cnt,d6 */		\
	*pc++ = (t);							\
    } else								\
	*pc++ = mem_moveq0d6|t;		/* moveq #cnt,d6 */		\
}

#define	ANY_CODE(table, op)						\
    (table[op>>1] != table[(op>>1)+1])
#define	CODE(table, op) {						\
    register short *cs, *ce;						\
    cs = table[op>>1]; ce = table[(op>>1)+1];				\
    while (cs < ce)							\
	*pc++ = *cs++;							\
}
extern short *mem_sd_rop_lrl[17];
extern short *mem_sd_rop_lrr[17];
extern short *mem_sd_rop_rll[17];
extern short *mem_sd_rop_rlr[17];
extern short *mem_d1_to_dest_incr[17];
extern short *mem_d1_to_dest_decr[17];

/* Maximum code size: 2 words */
#define	DBRA_D6(ipc) {							\
    *pc++ = mem_dbrad6;			/* dbra d6,ipc */		\
    *pc = (int)ipc - (int)pc;						\
    pc++;								\
}

/*
 * Fetch the source value from (a4); increment source pointer
 * if source consumed.  If necessary fetch two words so can shift
 * to get field.  Align source and destination skews.
 */
/* Maximum code size: 2 words */
#define	FETCH_SRC_INCR(sskew, dskew, ww, dist) {			\
    (dist) = (dskew) - (sskew);						\
    if ((sskew) + (ww) > 16) {						\
	*pc++ = mem_movla4d1;		/* movl a4@,d1 */		\
	(dist) += 16;							\
	if ((ww) != width) {						\
	    *pc++ = mem_addql2a4;	/* addql #2,a4 */		\
	    srcincr--;							\
	}								\
    } else if ((ww) == width) {						\
	if (srcincr == (short *)2) {					\
	    *pc++ = mem_movwa4Pd1;	/* movw a4@+,d1 */		\
	    srcincr--;							\
	} else if (srcincr == (short *)-2) {				\
	    srcincr++;							\
	    *pc++ = mem_movwa4Md1;	/* movw a4@-,d1 */		\
	} else {							\
	    *pc++ = mem_movwa4d1;	/* movw a4@,d1 */		\
	}								\
    } else if ((sskew) + (ww) == 16) {					\
	*pc++ = mem_movwa4Pd1;		/* movw a4@+,d1 */		\
	srcincr--;							\
    } else 								\
	*pc++ = mem_movwa4d1;		/* movw a4@,d1 */		\
    srcincr++;	/* to make typical srcincr change 0 */			\
}

/* Maximum code size: 1 word */
#define	FETCH_SRC(dskew, ww, dist) {					\
    (dist) = (dskew);							\
    *pc++ = mem_movwa4d1;		/* movw a4@,d1 */		\
}

/* Maximum code size: 2 words */
#define	FETCH_SRC_DECR(sskew, dskew, ww, dist) {			\
    (dist) = (dskew) - (sskew);						\
    if ((ww) > (sskew)) {						\
	*pc++ = mem_movla4Md1;		/* movl a4@-,d1 */		\
	srcincr += 2;							\
    } else {								\
	*pc++ = mem_movwa4Md1;		/* movw a4@-,d1 */		\
	srcincr++;							\
    }									\
    if ((ww) != (sskew) && (ww) != width) {				\
	*pc++ = mem_addql2a4;		/* addql #2,a4 */		\
	srcincr--;							\
    }									\
    srcincr--;	/* to make typical srcincr change 0 */			\
}

/* Maximum code size: 2 words */
#define ALIGN_SRC(dist)	{						\
    if ((dist) != 0) {							\
	if ((dist) < -8) {						\
	    *pc++ = mem_swapd1; 	/* swap d1 */			\
	    (dist) += 16;						\
	} else if ((dist) > 8) {					\
	    *pc++ = mem_swapd1;		/* swap d1 */			\
	    (dist) -= 16;						\
	}								\
	if ((dist) < 0) {						\
	    /* rotate source left by sskew-dskew */			\
	    *pc++ = mem_rolld1|		/* roll #n,d1 */		\
		((-(dist)&7)<<9);					\
	} else if ((dist) > 0) {					\
	    /* rotate source right by dskew-sskew */			\
	    *pc++ = mem_rorld1|		/* rorl #n,d1 */		\
		(((dist)&7)<<9);					\
	}								\
    }									\
}

/*
 * Shift d1 so that the source bits are aligned with the destination,
 * given the source and dest skews.
 */

/* Maximum code size: 1 word */
#define	INCR_SRCP(amt) {						\
    if ((amt) != 0)							\
	*pc++ = mem_addla2a4;		/* addl a2,a4 */		\
}
/* Maximum code size: 1 word */
#define	INCR_DSTP(amt) {						\
    if ((amt) != 0)							\
	*pc++ = mem_addla3a5;		/* addl a3,a5 */		\
}
/* Maximum code size: 2 words */
#define	DBRA_D7(ipc) {							\
    *pc++ = mem_dbrad7;			/* dbra #n,d7 */		\
    *pc = (int)ipc - (int)pc;						\
    pc++;								\
}
/* Maximum code size: 1 word */
#define MOVE_SRC_INC_TO_DST_INC() {					\
    *pc++ = mem_movwa4Pa5P;		/* movw a4@+,a5@+ */		\
}
/* Maximum code size: 1 word */
#define MOVE_SRC_TO_DST_INC() {						\
    *pc++ = mem_movwa4a5P;		/* movw a4@,a5@+ */		\
}
/* Maximum code size: 1 word */
#define MOVE_SRC_DEC_TO_DST_DEC() {					\
    *pc++ = mem_movwa4Ma5M;		/* movw a4@-,a5@- */		\
}
/* Maximum code size: 1 word */
#define MOVE_LONG_SRC_INC_TO_DST_INC() {				\
    *pc++ = mem_movla4Pa5P;		/* movl a4@+,a5@+ */		\
}
/* Maximum code size: 1 word */
#define MOVE_LONG_SRC_TO_DST_INC() {					\
    *pc++ = mem_movla4a5P;		/* movl a4@,a5@+ */		\
}
/* Maximum code size: 1 word */
#define MOVE_LONG_SRC_DEC_TO_DST_DEC() {				\
    *pc++ = mem_movla4Ma5M;		/* movl a4@-,a5@- */		\
}
/*
 * Generate an rts.
 */
/* Maximum code size: 1 word */
#define RTS() {								\
    *pc++ = mem_rts;			/* rts */			\
}

/*
 * Space for generated code.
 *
 * Worst case size is (words per pseudo-code defined above):
 *	3 SET_D6, 2 FETCH_SRC, 2 ALIGN_SRC, 4 CODE, 2 DBRA_D6,  = 13
 *	2 * (2 FETCH_SRC, 2 ALIGN_SRC, 6 CODE, = 10),		= 20
 *	1 INCR_SRCP, 1 INCR_DSTP, 2 DBRA_D7, 1 RTS,		= 5
 * or 38 words.
 *
 */

/*
 * Generate code to rasterop the interval [src,src+width)
 * to the interval [dst,dst+width), where the interval endpoints
 * are given as absolute bit addresses.  a4,a5 should be initialized
 * to even addresses prior to calling the function; then:
 *	src = a4*8 + sskew
 *	dst = a5*8 + dskew
 * Idea is that width, dskew, and sskew are invariant over the
 * lines of a full rectangular rasterop.  After each line
 * a4,a5 increment by their associated bytes/raster-line, where
 * the increments are stored in a2,a3 respectively.
 * The generated code also dbranches on d7 as the count of lines.
 * d6 counts words within a scan line.
 * d1 is the holding register through which data flows from src to dst
 * (except when sskew == dskew and op = PIX_SRC).
 * d0 is the working register for the destination when end-masking is needed
 */

int (*
mem_lrcode(dskew, width, op, sskew, mem_codespace))()
	register int dskew;
	int width, op;
	register int sskew;
	short *mem_codespace;
{
	register short *pc = mem_codespace;
	register int ww, dist;
	int (*ipc)();
	register u_short sm;
	short count; /* number of short words */
	int end;

	end = 0;
	pc = mem_codespace;
	while (width > 0) {
		ww = 16 - dskew;
		if (width < ww)
			ww = width;
		if (ww == 16) {
			/* middle case */
			count = width >> 4; /* is width divided by 16 */
			if (op == PIX_SRC && sskew == 0) { /* no source skewing */
				if (count >= 4) /* why case out count?? */
					SET_D6(count>>1);
				(int)ipc = (int)pc;
				if (count >= 2) {
					if (constsrc) {
						MOVE_LONG_SRC_TO_DST_INC();
					} else {
						MOVE_LONG_SRC_INC_TO_DST_INC();
					}
				}
				if (count >= 4)
					DBRA_D6(ipc);
				if (count&1) {
					if (constsrc) {
						MOVE_SRC_TO_DST_INC();
					} else {
						MOVE_SRC_INC_TO_DST_INC();
					}
				}
			} else { /* skew source */
				if (count >= 2)
					SET_D6(count);
				(int)ipc = (int)pc;
				if (PIXOP_NEEDS_SRC(op)) {
					if (constsrc) {
					    FETCH_SRC(dskew, ww, dist);
					} else {
					    FETCH_SRC_INCR(sskew,dskew,ww,dist);
					}
					ALIGN_SRC(dist);
				}
				CODE(mem_d1_to_dest_incr, op);
				if (count >= 2)
					DBRA_D6(ipc);
			}
			width &= 15;	/* residue for epilogue */
		} else {
			/* prologue or epilogue */
			count = 1;
			if (PIXOP_NEEDS_SRC(op)) {
				if (constsrc) {
					FETCH_SRC(dskew, ww, dist);
				} else {
					FETCH_SRC_INCR(sskew, dskew, ww, dist);
				}
				ALIGN_SRC(dist);
			}
			sm = ((u_short)(0xffff0000 >> ww)) >> dskew;
			if (end == 0) {
				sml = sm;
				dml = ~sm;
				CODE(mem_sd_rop_lrl, op);
			} else {
				smr = sm;
				dmr = ~sm;
				CODE(mem_sd_rop_lrr, op);
			}
			end++;
			sskew = (sskew + ww) & 15;
			dskew = (dskew + ww) & 15;
			width -= ww;
		}
		srcincr -= count;
		dstincr -= count;
	}
	if (PIXOP_NEEDS_SRC(op) && !constsrc) {
		INCR_SRCP(srcincr);
	}
	INCR_DSTP(dstincr);
	(int)ipc = (int)pc;
	DBRA_D7(mem_codespace);
	RTS();
	return (ipc);
}

int (*
mem_rlcode(dskew, width, op, sskew, mem_codespace))()
	register int dskew;
	int width, op;
	register int sskew;
	short *mem_codespace;
{
	register short *pc;
	register int ww, dist;
	register u_short sm;
	int (*ipc)();
	short count;
	int end;

	end = 0;
	pc = mem_codespace;
	while (width > 0) {
		ww = dskew;
		if (width < ww)
			ww = width;
		if (ww == 16) {
			/* middle case */
			count = width >> 4;
			if (op == PIX_SRC && sskew == 16) {
				if (count >= 4)
					SET_D6(count>>1);
				(int)ipc = (int)pc;
				if (count >= 2)
					MOVE_LONG_SRC_DEC_TO_DST_DEC();
				if (count >= 4)
					DBRA_D6(ipc);
				if (count&1)
					MOVE_SRC_DEC_TO_DST_DEC();
			} else {
				if (count >= 2)
					SET_D6(count);
				(int)ipc = (int)pc;
				if (PIXOP_NEEDS_SRC(op)) {
					FETCH_SRC_DECR(sskew, dskew, ww, dist);
					ALIGN_SRC(dist);
				}
				CODE(mem_d1_to_dest_decr, op);
				if (count >= 2)
					DBRA_D6(ipc);
			}
			width &= 15;	/* residue for epilogue */
		} else {
			/* prologue or epilogue */
			count = 1;
			if (PIXOP_NEEDS_SRC(op)) {
				FETCH_SRC_DECR(sskew, dskew, ww, dist);
				ALIGN_SRC(dist);
			}
			sm = ((u_short)(0xffff0000 >> ww)) >> (dskew - ww);
			if (end == 0) {
				smr = sm;
				dmr = ~sm;
				CODE(mem_sd_rop_rlr, op);
			} else {
				sml = sm;
				dml = ~sm;
				CODE(mem_sd_rop_rll, op);
			}
			end++;
			sskew = ((sskew - ww - 1) & 15) + 1;
			dskew = ((dskew - ww - 1) & 15) + 1;
			width -= ww;
		}
		srcincr += count;
		dstincr += count;
	}
	if (PIXOP_NEEDS_SRC(op)) {
		INCR_SRCP(srcincr);
	}
	INCR_DSTP(dstincr);
	(int)ipc = (int)pc;
	DBRA_D7(mem_codespace);
	RTS();
	return (ipc);
}
