/*	@(#)cg1reg.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Description of SUN-1 hardware color frame buffer.
 */

/*
 * Structure defining the way in which the address bits to the
 * SUN-1 color frame buffer are decoded.
 */
struct	cg1fb {
	struct {
		struct {
			struct {
				struct {
					u_char xory[1024];
				} cursor[2];
			} y, x;
		} xysel;
		struct {
			struct {
				u_char dat[256];
			} reg[16];
		} regsel;
	} update[2];
};

#define	CG_REDMAP	0	/* select color map addr */
#define	CG_GREENMAP	1
#define	CG_BLUEMAP	2
#define	CG_STATUS	8	/* select register addr */
#define	CG_MASKREG	9
#define	CG_FUNCREG	10
/*
 * Status Register bit definitions
 */
#define	CG_INTENABLE	(1<<4)	/* enab intrpt nxt vert rtrace */
#define	CG_PAINT	(1<<5)	/* enab wrt 5 pixels in parallel */
#define	CG_VIDEOENABLE	(1<<6)	/* enab video display */
#define	CG_RETRACE	(1<<7)	/* on read, true if in vert retrace */
#define	CG_INTLEVEL(i)	((i)<<13)
#define	CG_INTCLEAR	~(1<<4)	/* and to this clears interrupt */
#define CG_RWMAP	0x3	/* read/write color map 0..3	*/
#define CG_VIDMAP	0xC	/* video color map 0..3		*/
/*
 * Function Register values are logical combinations of:
 */
#define	CG_SRC		0xCC
#define	CG_DEST		0xAA
#define	CG_MASK		0xf0
#define	CG_NOTMASK	0x0f
#define	CGOP_NEEDS_MASK(op)		( (((op)>>4)^(op)) & CG_NOTMASK)

#define	cg1_setreg(fb, r, val)	\
		((fb)->update[0].regsel.reg[(r)].dat[0] = (val))

#define	cg1_setfunction(fb, f)	\
		(cg1_setreg((fb), CG_FUNCREG, (f)))

#define	cg1_setcontrol(fb, s)	\
		(cg1_setreg((fb), CG_STATUS, (s)))

#define	cg1_setmask(fb, mask)	\
		(cg1_setreg((fb), CG_MASKREG, (mask)))

#define	cg1_intclear(fb)	\
    ((fb)->update[0].regsel.reg[CG_STATUS].dat[0] &= CG_INTCLEAR)

#define	cg1_setmap_rdwr(fb, n)	\
    (fb)->update[0].regsel.reg[CG_STATUS].dat[0] &= ~CG_RWMAP; \
    (fb)->update[0].regsel.reg[CG_STATUS].dat[0] |= (n & CG_RWMAP)

#define	cg1_setmap_video(fb, n)	\
    (fb)->update[0].regsel.reg[CG_STATUS].dat[0] &= ~CG_VIDMAP; \
    (fb)->update[0].regsel.reg[CG_STATUS].dat[0] |= ((n)<<2)&CG_VIDMAP

#define cg1_retrace(fb)	\
    ((fb)->update[0].regsel.reg[CG_STATUS].dat[0] & CG_RETRACE)

/*
 * Defines that return addresses into the frame buffer or colormap
 */
#define	cg1_xsrc(fb, xval) \
    (&(fb)->update[0].xysel.x.cursor[0].xory[(xval)])

#define	cg1_ysrc(fb, yval) \
    (&(fb)->update[1].xysel.y.cursor[0].xory[(yval)])

#define	cg1_xdst(fb, xval) \
    (&(fb)->update[0].xysel.x.cursor[1].xory[(xval)])

#define	cg1_ydst(fb, yval) \
    (&(fb)->update[1].xysel.y.cursor[1].xory[(yval)])

#define	cg1_xpipe(fb, xval, up, set) \
    (&(fb)->update[(up)].xysel.x.cursor[(set)].xory[(xval)])

#define	cg1_ypipe(fb, yval, up, set) \
    (&(fb)->update[(up)].xysel.y.cursor[(set)].xory[(yval)])

#define	cg1_map(fb, rgb, index) \
    (&(fb)->update[0].regsel.reg[(rgb)].dat[(index)] )


#define	cg1_touch(a)	((a)=0)
#define CG1_WIDTH	640
#define CG1_HEIGHT	480
#define CG1_DEPTH	8
