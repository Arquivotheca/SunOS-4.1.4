/*	@(#)bw1reg.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Description of SUN-1 hardware frame buffer.
 */

/*
 * Structure defining the way in which the address bits to the
 * SUN-1 frame buffer are decoded.
 *
 * On this device a 1 bit means "on" (white) instead of "make a mark"
 * (black).  This is not intuitive for most programmers (assuming a
 * standard display appearrance of black bits on a white background).
 * So, the source is inverted on writes and destination is inverted on
 * reads in order to compensate for the hardware.
 */
struct	bw1fb {
	struct {
		struct {
			struct {
				struct {
					short xory[1024];
				} x, y;
			} cursor[4];
		} reg[4];
	} update[2];
};

#define	BW_REGNONE	0
#define	BW_REGOTHERS	1
#define	BW_REGSOURCE	2
#define	BW_REGMASK	3

#define	BWXY_FUNCTION	0	/* current rasterop function */
#define	BWXY_WIDTH	1	/* rasterop width */
#define	BWXY_CONTROL	2	/* control bits for interrupts and video */
#define		BWCONTROL_INTENABLE	(1<<8)
#define		BWCONTROL_VIDEOENABLE	(1<<9)
#define		BWCONTROL_INTLEVEL(i)	((i)<<13)
#define	BWXY_INTCLEAR	3	/* assign to this clears interrupt */

#define	bw1_setreg(fb, r, val, xy)\
    ((fb)->update[0].reg[(r)].cursor[3].x.xory[(xy)] = (val))

#define	bw1_setfunction(fb, f) \
		(bw1_setreg((fb), BW_REGOTHERS, (f), BWXY_FUNCTION))
#define	bw1_setwidth(fb, f) \
		(bw1_setreg((fb), BW_REGOTHERS, (f), BWXY_WIDTH))
#define	bw1_setcontrol(fb, s) \
		(bw1_setreg((fb), BW_REGOTHERS, (s), BWXY_CONTROL))
#define	bw1_intclear(fb) \
		(bw1_setreg((fb), BW_REGOTHERS, 0, BWXY_INTCLEAR))
#define	bw1_setsource(fb, src) \
		(bw1_setreg((fb), BW_REGSOURCE, (src), 0))
#define	bw1_setmask(fb, mask) \
		(bw1_setreg((fb), BW_REGMASK, (mask), 0))

#define	bw1_xsrc(fb, xval) \
    (&(fb)->update[0].reg[BW_REGNONE].cursor[0].x.xory[(xval)])
#define	bw1_ysrc(fb, yval) \
    (&(fb)->update[0].reg[BW_REGSOURCE].cursor[0].y.xory[(yval)])
#define	bw1_xdst(fb, xval) \
    (&(fb)->update[0].reg[BW_REGNONE].cursor[1].x.xory[(xval)])
#define	bw1_ydst(fb, yval) \
    (&(fb)->update[1].reg[BW_REGSOURCE].cursor[1].y.xory[(yval)])
#define	bw1_ydstcopy(fb, yval) \
    (&(fb)->update[1].reg[BW_REGNONE].cursor[1].y.xory[(yval)])

/* for converting update[0].reg[BW_REGNONE] to update[1].reg[BW_REGSOURCE] */
#define bw1_activatex							\
((char *)(&((struct bw1fb*)0)->update[1].reg[BW_REGSOURCE].cursor[0].x.xory[0])\
-(char *)(&((struct bw1fb*)0)->update[0].reg[BW_REGNONE  ].cursor[0].x.xory[0]))
#define bw1_activatey							\
((char *)(&((struct bw1fb*)0)->update[1].reg[BW_REGSOURCE].cursor[0].x.xory[0])\
-(char *)(&((struct bw1fb*)0)->update[0].reg[BW_REGSOURCE].cursor[0].x.xory[0]))

#define	bw1_touch(a)	((a)=0)

#define	BW_MASK	0xf0
#define	BW_SRC	0xcc
#define	BW_DEST	0xaa
