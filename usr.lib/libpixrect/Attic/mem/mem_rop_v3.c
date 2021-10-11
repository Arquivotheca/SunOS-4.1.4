#ifndef lint
static char sccsid[] = "@(#)mem_rop_v3.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*-
	Rasterops for memory pixrects
  
	"I think that I am violating no confidence
	 when I say that Nature holds many
	 mysteries which we humans have not
	 fathomed as yet.  Some of them may not
	 even be worth fathoming."
			-- Robert Benchley
			   in "Mysteries from the Sky"
  
		=> clearly, he was talking about cpp
  
  
	mem_rop.c, Tue Jul 23 10:46:05 1985
  
		James Gosling,
		Sun Microsystems
 */


#ifdef KERNEL
#include "../h/types.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/memvar.h"
#include "../pixrect/pr_util.h"
#else
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>
#endif

/*********************************************************************\
  
Rasterop is implemented as a series of nested macro calls.  This is
done in an attempt to write similar pieces of code a minimum number of
times.  In particular, the goal was to write the inner loops exactly
once.  This was almost achieved.
  
A technique that is commonly used throughout the implementation of
rasterop is the passing of macro names as parameters to other macros;
these are generally used to specialize the macro.  For example if a loop
control variable in a macro could either be incremented or decremented
to run the loop forwards or backwards, then either the macro ADD1 or
the macro SUB1 will be passed in and that macro will be called to
update the loop variable.
  
A similar technique is used to simulate compile time boolean variables
that may also be runtime booleans.  For example, consider the parameter
multiword to the macro GENERAL_BINARY_OP.  Think of it as a boolean
that is true if there are multiple words in a row in this rop.  Inside
GENERAL_BINARY_OP it is invoked as multiword(a,b) where a should be
executed if there are multiple words in the row and b should be
executed otherwise.  There are several macros that can be passed to
GENERAL_BINARY_OP as the value of multiword.  The macros 'always' and
'never' act as the constants 'true' and 'false': these are compile time
booleans.  'whenwide', on the other hand, expands to a runtime test of
the boolean variable.
  
The choice between compiletime tests and runtime tests is based on the
desired speed of the opcode.  Each opcode is characterized as `fast' or
`slow' by CASE_BINARY_OP.  Fast opcodes will have many special cases
expanded out by using tests outside of the inner loop, slow ones will
have the tests done at runtime in the inner loop.
  
The usual cliche for doing this looks like this:
  
	speed ( booltest ( task ( always ), task ( never ))
	        task ( booltest ))
  
Here task is a macro that takes a boolean macro as a parameter, which
it will presumably use inside some inner loop.  Booltest is a macro
which will really test the condition.  So, if speed is `fast' then
booltest will be called and based on that task will be called with
the appropriate constant boolean.  Otherwise, the task will be called
with the actual test as its parameter.
  
Compiling some opcodes into slow or fast versions is based on the
assuption that only a few opcodes are really useful, and the rest are
not and hence are rarely used and can be slower.
  
The macros are called in a hierarchy:
	Fn, Sn: evaluate opcode
	GENERAL_BINARY_ROP, BINARY_ROP_SHIFT0:	do a single rasterop
	BINARY_ROP: do a rop, testing for shift==0 & short start.
	CASE_BINARY_OP: do a rop, testing the opcode
  
\*********************************************************************/

#define assert(x)

#define pr_div(a,b) ((short)(a) / (short)(b))

/* At the bottom level of the hierarchy of macro calls are those with
 * names Fn and Sn.  Fn and Sn are C expressions to evaluate the binary
 * operation n.  For example, the rasterop "PIX_SRC|PIX_DST" is opcode
 * 0xE, so FE(d,s) is d|s.  The difference between the F and S functions
 * is that the S functions cast their arguments to shorts at all possible
 * opportunities -- this often causes the C compiler to generate better
 * code.
 */
  
/*
 * The 16 possible rasterops.
 * Given opcode i, the correct function to execute it is Fi 
 */
#define F0(d,s) (0)
#define F1(d,s) (~((d)|(s)))
#define F2(d,s) ((d)&~(s))
#define F3(d,s) (~(s))
#define F4(d,s) (~(d)&(s))
#define F5(d,s) (~(d))
#define F6(d,s) ((d)^(s))
#define F7(d,s) (~((d)&(s)))
#define F8(d,s) ((d)&(s))
#define F9(d,s) (~((d)^(s)))
#define FA(d,s) (d)
#define FB(d,s) ((d)|~(s))
#define FC(d,s) (s)
#define FD(d,s) (~(d)|(s))
#define FE(d,s) ((d)|(s))
#define FF(d,s) (~0)

/*
 * The 16 possible rasterops.  
 * Given opcode i, the correct function to execute it is Fi
 * These are just like the F functions, except that they're fanatical 
 * about casting to short 
 */
#define S0(d,s) (0)
#define S1(d,s) st(~st(st(d)|st(s)))
#define S2(d,s) st(st(d)&~st(s))
#define S3(d,s) st(~st(s))
#define S4(d,s) st(~st(d)&st(s))
#define S5(d,s) st(~st(d))
#define S6(d,s) st(st(d)^st(s))
#define S7(d,s) st(~st(st(d)&st(s)))
#define S8(d,s) st(st(d)&st(s))
#define S9(d,s) st(~st(st(d)^st(s)))
#define SA(d,s) st(d)
#define SB(d,s) st(st(d)|~st(s))
#define SC(d,s) st(s)
#define SD(d,s) st(~st(d)|st(s))
#define SE(d,s) st(st(d)|st(s))
#define SF(d,s) st(~0)


/* Macros to execute some of the unary rasterops under a mask */
#define M0(m)	(*dst &= ~(m))
#define M5(m)	(*dst ^= (m))
#define MC(m)   (*dst = *dst&~m | color&m)
#define MF(m)	(*dst |= (m))


/* These macros are used for top->bottom, left->right inversions */
#define ADD1(x) ((x)++)
#define SUB1(x) ((x)--)
#define ADD1inverse SUB1
#define SUB1inverse ADD1


/* Change Kernel to KERNEL in the next line if you want rasterop in the
   kernel to be small but slow */
#ifndef Kernel
#define fast(a,b) {a;}
#define fastT(t,a,b) if (t) {a;} else {b;}
#else
#define fast(a,b) {b;}
#define fastT(t,a,b) {b;}
#endif
#define slow(a,b) {b;}
#define slowT(t,a,b) {b;}

/* 
 * CASE_BINARY_OP executes a macro for an opcode.  It effectively
 * translates a variable "op" argument into a constant.  That constant will
 * eventually be used to construct rasterop function names.  The "speed"
 * argument to the called macro is either "fast" or "slow", depending on
 * whether or not the fast version of the rasterop should be compiled.
 * Speed(a,b) acts as a boolean that executes "a" if speed is fast, and "b"
 * if speed is slow.  This trick of using macros as compile time booleans
 * is used frequently.  
 */

#define CASE_BINARY_OP(op,macro,incr)				\
    switch(op) {						\
	CASEOP(1,macro,incr,slow);				\
	CASEOP(2,macro,incr,fast);				\
	CASEOP(3,macro,incr,slow);				\
	CASEOP(4,macro,incr,slow);				\
	CASEOP(6,macro,incr,fast);				\
	CASEOP(7,macro,incr,slow);				\
	CASEOP(8,macro,incr,slow);				\
	CASEOP(9,macro,incr,slow);				\
	CASEOP(B,macro,incr,slow);				\
	CASEOP(C,macro,incr,fast);				\
	CASEOP(D,macro,incr,slow);				\
	CASEOP(E,macro,incr,fast);				\
    }


/* Execute the code s count times.  This should be a form that is efficient
   for this architecture.  For 680x0's, the macro only generates good code if
   there is room for one more register variable.  This particular form was
   chosen because it interacts well with the way that the Sun C compiler
   does code motion and with the way that it detects the possible use of
   the dbra instruction. */
#define rolledloop(count,s) {					\
    register short cnt;						\
    if ((cnt = (count)-1)>=0)					\
	do {s;} while (--cnt != -1);				\
}


#define lsrc	((long *)src)
#define ldst	((long *)dst)
#define st(x) ((short)(x))

#define always(x,y) {x;}
#define never(x,y) {y;}

/* The `widecols' test asks whether or not a row in this rop touches more
   than one word */
#define whenwide(x,y) {if(widecols>=0) {x;} else {y;}}

/* The `short_start' test asks whether or not the first memory reference in
   a row should just be a short, rather than a long.  This is primarily used
   to deal with the bug where rasterop would touch (but not use) the 16 bits
   immediatly preceeding the pixrect */
#define whenshortstart(x,y) { if(short_start) {x;} else {y;}}

/* execute a rasterop, making the widecols/speed tradeoff.
   incr indicates whether we're starting from the upper left or
   the lower right (it's either ADD1 or SUB1) */
#define CASEOP(op,macro,incr,speed)					\
    case CAT(0x,op):							\
	speed (								\
	    if (widecols>=0) {macro(op,incr,always,speed);}		\
	    else {macro(op,incr,never,speed);}				\
	,								\
	    {macro(op,incr,whenwide,speed);}				\
	)								\
	break;

/* Do a rasterop where the shift is 0: that is, the source and destination
   are aligned.  In this case we do long operations.  `oddwords' is true iff
   there is an odd number of 16 bit words in a row, in which case an extra
   16 bit operation will be done */
#define BINARY_ROP_SHIFT0(op,incr,multiword,oddwords)					\
    widecols >>= 1;									\
    for (shift=dh; --shift != -1; ) {							\
	*dst = st(CAT(S,op)(*dst, *incr(src))&mask1) | st(*dst&notmask1); incr(dst);	\
        multiword (									\
	    rolledloop(widecols,((*ldst = CAT(F,op)(*ldst,*incr(lsrc))),incr(ldst)));	\
	    oddwords ( ((*dst = CAT(S,op)(*dst,*incr(src))),incr(dst));, )		\
	    *dst = st(CAT(S,op)(*dst, *incr(src))&mask2) | st(*dst&notmask2); incr(dst);\
	,)										\
        (int) src += deltasrc;  (int) dst += deltadst;					\
    }

#define ADD1forward(a,b) a
#define SUB1forward(a,b) b

#define neverodd(a,b) a;
#define alwaysodd(a,b) if(widecols&1) {a;} else {b;}
#define whenwideodd(a,b) if(widecols&1) {a;} else {b;}

/* Do a rasterop: this contains the code for shift==0 (going backwards).
   Otherwise it does the short start test and invokes GENERAL_BINARY_ROP
   appropriatly. */
#define BINARY_ROP(op,incr,multiword,speed) {						\
  multiword (, mask1 &= mask2; notmask1 = ~mask1; )					\
  CAT(speed,T) (shift == 0, \
    /* src++; */ \
    CAT(incr,forward)( \
	CAT(multiword,odd) (BINARY_ROP_SHIFT0(op,incr,multiword,always);, \
			    BINARY_ROP_SHIFT0(op,incr,multiword,never);) \
    , \
    for (shift=dh; --shift != -1; ) { \
	*dst = st(st(CAT(S,op)(*dst, *incr(src)))&mask1) | st(*dst&notmask1); incr(dst); \
        multiword ( \
	    rolledloop(widecols,((*dst = CAT(S,op)(*dst,*incr(src))),incr(dst))); \
	    *dst = st(st(CAT(S,op)(*dst, *incr(src)))&mask2) | st(*dst&notmask2); incr(dst); \
	,) \
        (int) src += deltasrc;  (int) dst += deltadst; \
    }) \
    , \
    speed (whenshortstart(GENERAL_BINARY_ROP(op,incr,multiword,always), \
			  GENERAL_BINARY_ROP(op,incr,multiword,never)), \
	   GENERAL_BINARY_ROP(op,incr,multiword,whenshortstart)); \
  ) \
}

/*
 * The meat of rasterop.  It does the rasterop 16 bits at a time: it fetches 
 * a long source word, shifts it to align it with the destination, then does 
 * a short operation to memory. 
 */
#define GENERAL_BINARY_ROP(op,incr,multiword,shortstart) { \
    int rows; \
    shortstart(deltasrc += sizeof(short), ); \
    for (rows=dh; --rows >= 0; ) { \
      shortstart( \
	*dst = st(st(CAT(S,op)(*dst, st(*src>>shift)))&mask1) | st(*dst&notmask1); incr(dst); \
      , \
	*dst = st(st(CAT(S,op)(*dst, st(*lsrc>>shift)))&mask1) | st(*dst&notmask1); incr(dst); \
	incr(src); \
      ) \
        multiword ( \
	    rolledloop(widecols,(*dst = CAT(S,op)(*dst,st(*lsrc>>shift)), \
				    incr(src), incr(dst))); \
	    *dst = st(st(CAT(S,op)(*dst, st(*lsrc>>shift)))&mask2) | st(*dst&notmask2); incr(dst); \
	    incr(src); \
	,) \
        (int) src += deltasrc;  (int) dst += deltadst; \
    } \
}


/* Unary (destination only) rasterops are relatively trivial.  
   They only bother to exploit the widecols test */
#define UNARY_ROP(op,incr,multiword,speed) {						\
    register short rows;								\
    for (rows=dh; --rows >= 0; ) {							\
        CAT(M,op)(mask1); incr(dst);							\
        multiword (									\
	    rolledloop(widecols, ((*dst = CAT(F,op)(*dst,color)), incr(dst)));		\
	    CAT(M,op)(mask2); incr(dst);						\
	,)										\
        ((int) dst) += deltadst;							\
    }											\
}


#if defined(KERNEL) && defined(mc68010)
int         mem_needinit;
#endif

extern char pr_reversesrc[];
extern char pr_reversedst[];

/*
 * General memory-to-memory rasterop
 */

mem_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
    struct pixrect *dpr, *spr;
{
    struct mpr_data *mprd = mpr_d(dpr);
    int         lcolor = PIX_OPCOLOR(op);
#if defined(KERNEL) && defined(mc68010)
    if (mem_needinit)
	mem_init();
#endif
    if (dpr->pr_ops->pro_rop != mem_rop)
	return -1;
    if (spr && spr->pr_ops->pro_rop != mem_rop)
	return (*(spr)->pr_ops->pro_rop) (dpr, dx, dy, dw, dh, op, spr, sx, sy);
    if ((op & PIX_DONTCLIP) == 0)
	pr_clip((struct pr_subregion *) & dpr, (struct pr_prpos *) & spr);
    op = (op >> 1) & 0xF;
    if (dw <= 0 || dh <= 0)
	return 0;
    dx += mprd->md_offset.x;
    dy += mprd->md_offset.y;
    if (mprd->md_flags & MP_REVERSEVIDEO)
	op = pr_reversedst[op];
    if (op == (PIX_DST >> 1))
	return 0;
    {
	register    depth = dpr->pr_depth;
	if (depth > 1) {	/* Fudge for destination depth being
				 * greater that 1.  We just multiply the x
				 * coordinates by the depth */
	    dx = pr_product(dx, depth);
	    dw = pr_product(dw, depth);
	}
    }

/**************************  FILL OPERATIONS **************************/

    if (spr == 0 || (op == 0 || op == 5 || op == 0xF)) {
	register unsigned long *dst;
	register unsigned long mask1, mask2;
	register    widecols, deltadst;
	int         color;

	assert(sizeof(int) == 4 && sizeof(long) == 4);
	color = lcolor;		/* compute fill color */
	if ((widecols = dpr->pr_depth) > 1) {
	    color &= (1 << widecols) - 1;
	    while (widecols < 32) {
		color |= color << widecols;
		widecols *= 2;
	    }
	}
	else if (color != 0)
	    color = ~0;
	mask1 = ((unsigned long) 0xFFFFFFFF) >> (dx & 0xF);
	mask2 = 0xFFFFFFFF << (0x1F - ((dw - 1 + (dx & 0xF)) & 0x1F));
	widecols = (((dx & 0xF) + dw - 1) >> 5) - 1;
	if (widecols < 0)
	    mask1 &= mask2;
	deltadst = mprd->md_linebytes - (widecols + 2) * 4;
	dst = (unsigned long *) ((char *) mprd->md_image
				 + pr_product(dy, mprd->md_linebytes)
				 + ((dx >> 3) & ~1));
	assert(spr == 0);
	/*
	 * There are really only 3 mem1 'fill' operations: fill 0's, fill
	 * 1's and invert. Some people use non-obvious opcodes to pick one
	 * of these.  For example, op==PIX_SRC with color==1 means fill
	 * 1's. 
	 */
	switch (op) {
	default:		/* decode weird opcode */
	    if (color)
		op >>= 2;
	    switch (op & 3) {
	    case 0:
		goto fillzero;
	    case 1:
		goto invert;
	    case 3:
		goto fillone;
	    }
	    break;
    fillzero:
	    CASEOP(0, UNARY_ROP, ADD1, fast);
    invert:
	    CASEOP(5, UNARY_ROP, ADD1, fast);
	case 3:
	    color = ~color;
	case 0xC:
	    if (color == 0)
		goto fillzero;
	    if (color == ~0)
		goto fillone;
	    /*
	     * Fill with a specified color.  NOTE: for depth>1 pixrects
	     * this isn't done correctly for opcodes other than PIX_SRC. 
	     * eg. PIX_SRC|PIX_DST doesn't or the color into the pixels 
	     */
	    if (widecols >= 0) {
		UNARY_ROP(C, ADD1, always, fast);
	    }
	    else {
		UNARY_ROP(C, ADD1, never, fast);
	    }
	    break;
    fillone:
	    CASEOP(F, UNARY_ROP, ADD1, fast);
	}
    }
    else {
	if (mpr_d(spr)->md_flags & MP_REVERSEVIDEO)
	    op = pr_reversesrc[op];

/**************************  MEM1->MEMn OPERATIONS **************************/

	if (spr->pr_depth != dpr->pr_depth)
	    if (spr->pr_depth == 1) {
		/*
		 * depth-1 to depth>1 pixrect operations.  Only actually
		 * handles destinations with a depth of 4, 8, 16 or 32.  2
		 * could be handled, but the mask table would be too
		 * large. Currently the only opcode supported is
		 * PIX_SRC|PIX_DST, and it has a slightly unusual
		 * interpretation: source pixels which are 0 dont affect
		 * the destination, source pixels which are 1 cause the
		 * destination to be set to the color argument 
		 */
#ifdef KERNEL
		return -1;
#else
		static int *masks;
		static int  mask_size;
		static int  mask_depth;
		assert(op == ((PIX_SRC | PIX_DST) >> 1));
		/*
		 * mem1->memN rops are based on the use of a table that
		 * translates k 1-bit pixels into a mask of k N-bit
		 * pixels.  k*N = 32.  For example if N=8 then k=4.  There
		 * are 16=1<<4 4-bit numbers, so the table will be 16
		 * entries long.  Entry 5 (101 in binary) will be
		 * 0x00FF00FF 
		 */
		{
		    register    depth = dpr->pr_depth;
		    assert( /* depth == 2 || */ depth == 4 || depth == 8 || depth == 16 || depth == 32);
		    if (depth != mask_depth) {	/* Avoid recomputing the
						 * table every time */
			int         pixels_per_word = pr_div(32, depth);
			register    dsize = 1 << pixels_per_word;
			int         onemask = (1 << depth) - 1;
			if (dsize > mask_size) {
			    extern char *malloc();
			    if (masks)
				free((char *) masks);
			    masks = (int *) malloc((unsigned) (dsize * sizeof(int)));
			    mask_size = dsize;
			}
			while (--dsize >= 0) {
			    register    i;
			    register    tmask = 1;
			    register    orin = onemask;
			    register    thismask = 0;
			    for (i = 0; i < pixels_per_word; i++) {
				if (dsize & tmask)
				    thismask |= orin;
				tmask <<= 1;
				orin <<= depth;
			    }
			    masks[dsize] = thismask;
			}
		    }
		}
		{
		    /*
		     * The inner loop extracts k bits from the source,
		     * indexes into the table for a 32 bit mask, then does
		     * a 32 bit operation on the destination 
		     */
		    unsigned int
		                lmask = ((unsigned) ~0) >> (dx & 0x1F),
		                rmask = (~0) << (0x1F - ((dx + dw - 1) & 0x1F));
		    int         widecols = ((dx + dw - 1) >> 5) - (dx >> 5) - 1,
		                bytes_per_row = (widecols + 2) * sizeof(int),
		                rows;
		    register char *deltadstp, *deltasrcp;
#define deltadst ((int)deltadstp)
#define deltasrc ((int)deltasrcp)
		    register unsigned short *src;
		    register unsigned long *dst;
		    register    bits_left;
		    register unsigned bits = 0;
		    register    count = dpr->pr_depth;
		    register    bit_shift = pr_div(32, count);
		    register    mask;
		    register    peel_mask = (1 << bit_shift) - 1;
		    while (count < 32) {
			lcolor |= lcolor << count;
			count *= 2;
		    }
		    deltadst = mprd->md_linebytes;
		    deltasrc = mpr_d(spr)->md_linebytes;
		    dst = (unsigned long *) ((char *) mprd->md_image
					     + pr_product(dy, deltadst)
					     + ((dx >> 3) & ~3));
		    sx += mpr_d(spr)->md_offset.x - pr_div(dx & 0x1F, dpr->pr_depth);
		    sy += mpr_d(spr)->md_offset.y;
		    src = (unsigned short *) ((char *) mpr_d(spr)->md_image
					      + pr_product(sy, deltasrc)
					      + ((sx >> 3) & ~1));
		    deltasrc -= ((pr_product(widecols + 2, bit_shift) + (sx & 0xF) + 15) >> 3) & ~1;
		    deltadst -= bytes_per_row;
		    for (rows = dh; --rows >= 0;) {
			bits_left = sx & 0xF;	/* setup the first source
						 * word */
			bits = *src++ << bits_left;
			bits_left = 16 - bits_left;
			if ((bits_left -= bit_shift) < 0) {	/* extract some bits */
			    bits = ((bits << (bits_left + bit_shift)) | *src++) << -bits_left;
			    bits_left += 16;
			}
			else {
			    bits <<= bit_shift;
			}
			/* Do the operation on the left edge */
			mask = masks[(bits >> 16) & peel_mask] & lmask;
			*dst = *dst & ~mask | lcolor & mask;
			dst++;
			if (widecols >= 0) {	/* The middle part */
			    for (count = widecols; --count != -1;) {	/* extract more bits */
				if ((bits_left -= bit_shift) < 0) {
				    bits = ((bits << (bits_left + bit_shift)) | *src++) << -bits_left;
				    bits_left += 16;
				}
				else {
				    bits <<= bit_shift;
				}
				mask = masks[(bits >> 16) & peel_mask];	/* Do interior operation */
				*dst = *dst & ~mask | lcolor & mask;
				dst++;
			    }
			    if ((bits_left -= bit_shift) < 0) {
				bits = ((bits << (bits_left + bit_shift)) | *src++) << -bits_left;
				bits_left += 16;
			    }
			    else {
				bits <<= bit_shift;
			    }
			    /* Do right edge */
			    mask = masks[(bits >> 16) & peel_mask] & rmask;
			    *dst = *dst & ~mask | lcolor & mask;
			    dst++;
			}
			(int) src += deltasrc;
			(int) dst += deltadst;
		    }
		}
#endif KERNEL
	    }
	    else
		return -1;	/* Fail on MEMn->MEMm */
	else {

/**************************  MEMn->MEMn OPERATIONS **************************/

	    unsigned int
	                lmask = 0xFFFF >> (dx & 0xF),
	                rmask = 0xFFFF << (0xF - ((dx + dw - 1) & 0xF));
	    int         widecols = ((dx + dw - 1) >> 4) - (dx >> 4) - 1,
	                bytes_per_row = (widecols + 2) * 2;
	    register char *deltadstp, *deltasrcp;
#define deltadst ((int)deltadstp)
#define deltasrc ((int)deltasrcp)
	    register unsigned short
	               *dst, *src;
	    register    shift;
	    int         different_pixrects;
	    int         short_start;
	    register unsigned short
	                mask1, mask2, notmask1, notmask2;

	    deltadst = mprd->md_linebytes;
	    deltasrc = mpr_d(spr)->md_linebytes;
	    dst = (unsigned short *) ((char *) mprd->md_image
				      + pr_product(dy, deltadst)
				      + ((dx >> 3) & ~1));
	    sx = pr_product(sx, spr->pr_depth) + mpr_d(spr)->md_offset.x;
	    sy += mpr_d(spr)->md_offset.y;
	    different_pixrects = ((int) mpr_d(spr)->md_image) - ((int) mprd->md_image);
	    src = (unsigned short *) ((char *) mpr_d(spr)->md_image
				      + pr_product(sy, deltasrc)
				      + ((sx >> 3) & ~1));

	    if ((shift = (dx & 0xF) - (sx & 0xF)) < 0) {
		short_start = 0;
		shift = 16 + shift;
	    }
	    else
		 /* src-- */ short_start = 1;
	    if (dy > sy && !different_pixrects) {	/* bottom to top */
		(int) src += pr_product(dh - 1, deltasrc);
		(int) dst += pr_product(dh - 1, deltadst);
		deltasrc = -deltasrc;
		deltadst = -deltadst;
	    }
	    if (dx < sx || dy != sy || different_pixrects) {	/* left to right */
		deltasrc -= bytes_per_row;
		deltadst -= bytes_per_row;
		mask1 = lmask;
		mask2 = rmask;
		notmask1 = ~mask1;
		notmask2 = ~mask2;
		CASE_BINARY_OP(op, BINARY_ROP, ADD1);
	    }
	    else {		/* right to left */
		deltasrc += bytes_per_row;
		deltadst += bytes_per_row;
		(int) src += bytes_per_row - sizeof(short);
		(int) dst += bytes_per_row - sizeof(short);
		mask1 = rmask;
		mask2 = lmask;
		notmask1 = ~mask1;
		notmask2 = ~mask2;
		if (short_start && shift) {
		    src--;
#define short_start 0
#undef whenshortstart
#define whenshortstart never
		}
		CASE_BINARY_OP(op, BINARY_ROP, SUB1);
	    }
	}
    }
    return 0;
}
