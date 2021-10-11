	.text
	.asciz	"@(#)cg1_vecbres.S 1.1 94/10/31 SMI"
	.even

/*
 *	Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sun-1 Color vector drawing
 */

	.text
	.globl	_cg1_brespos, _cg1_bresminneg, _cg1_bresmajneg 	/* color */
	.globl	_cg1_horizln, _cg1_vertln

#define MOVB(reg,sign) .word 0x1002+reg+sign  /* builds a movb instruction */

#define RMAJ 04000	/* a4 holds rmaj address */
#define RMIN 03000	/* a3 holds rmin address */
#define POS 0300	/* an@+ scans positively */
#define NEG 0400	/* an@- scans negatively */

#define cat(a, b) a/**/b

#define minor(i) cat(_minor,i)
#define major(i) cat(_major,i)

/*
 * Macro to generate bresenham loops.  This code
 * takes a hardware cursor in a5 which accesses the
 * major axis and a cursor address in a4 which accesses
 * the minor axis.  The initialized error is stored in d4,
 * and the minor axis delta is in d4 and the major axis
 * delta in d5.
 *
 * The loops start by stepping 1 in the minor direction.
 * When going left-to-right, the auto-increment mode
 * post-increments, so this initializes the minor cursor,
 * and increments it 1 for the next time it is used.
 * When going right-to-left, this pre-decrements the
 * minor axis cursor, and then sets it.  Thus the minor
 * axis cursor must be 1 larger when the routine is entered
 * to get the right initial y.
 *
 * The main loop of bresenham steps in the major axis direction.
 * When moving in the major access direction, we add the minor
 * increment to the error.  If the error gets to be greater
 * than zero, we subtract the major increment from the error.
 * In either case, we stop after count steps.
 *
 * The code here implements this, but requires a detailed understanding
 * of the dbXX instructions to verify.  The instruction
 *	dbgt	d5,major(i)
 * implements the tight inner loop.  If the preceding
 *	addw	d6,d4
 * which increased the error left the error <= 0, then this instruction
 * decrements the count and continues at major(i).  If the error
 * became positive, then this instruction falls through WITHOUT
 * DECREMENTING THE COUNT.  In this latter case we decrease the error
 * by subtracting the major access increment from the error
 * and execute the instructions:
 *	tstw	d5
 *	dblt	d5,minor(i)
 * I don't see why this can't be just a
 *	dbra	d5,minor(i)
 */
#define bres(name, step, plot, i)					\
name:									\
minor(i): step;			/* step 1 in minor direction	 */	\
major(i): plot;			/* plot 1 in major direction	 */	\
	  addw	d6,d4;		/* count up minor		 */	\
	  dbgt	d5,major(i);	/* if no overflow, keep plotting */     \
	  subw	d7,d4;		/* on overflow, reset counter	 */	\
	  tstw	d5;		/* catch d5 underflow		 */	\
	  dblt	d5,minor(i);	/* and do both minor and major	 */	\
	  rts;			/* if exhausted, exit		 */	\

bres(_cg1_brespos,MOVB(RMIN,POS),MOVB(RMAJ,POS),0)
bres(_cg1_bresminneg,MOVB(RMIN,NEG),MOVB(RMAJ,POS),1)
bres(_cg1_bresmajneg,MOVB(RMIN,POS),MOVB(RMAJ,NEG),2)

/*
 * Horizontal line.
 *
 * Store the color through the x cursor count times.
 */
_cg1_horizln:
	MOVB(RMAJ,POS);
	dbf	d7,_cg1_horizln
	rts

/*
 * Vertical line.
 *
 * Store the color through the y cursor count times.
 */
_cg1_vertln:
	MOVB(RMIN,POS);
	dbf	d6,_cg1_vertln
	rts
