#ifndef lint
static	char sccsid[] = "@(#)cg.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "cg.h"
#if NCG > 0

/*
 * Sun Color Graphics Board(s) Driver
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"

#include "../sun/mmu.h"
#include "../sun/fbio.h"

#include "../sundev/mbvar.h"
#include "../sundev/cgreg.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	cgprobe(), cgintr();
struct	mb_device *cginfo[NCG];
u_long	cgstd[] = { 0xe8000, 0xec000, 0 };
struct	mb_driver cgdriver = {
	cgprobe, 0, 0, 0, 0, cgintr, cgstd, 0, CGSIZE,
	"cg", cginfo, 0, 0, 0,
};

/*
 * We determine that the thing we're addressing is a color
 * board by setting it up to invert the bits we write and then writing
 * and reading back DATA1, making sure to deal with FIFOs going and coming.
 */
#define DATA1 0x5C
#define DATA2 0x33
cgprobe(reg)
	caddr_t reg;
{
	register caddr_t CGXBase;
	register u_char *xaddr, *yaddr;

	CGXBase = reg;
	if (pokec((caddr_t)GR_freg, GR_copy_invert))
		return (0);
	if (pokec((caddr_t)GR_mask, 0))
		return (0);
	xaddr = (u_char *)(CGXBase + GR_x_select + GR_update + GR_set0);
	yaddr = (u_char *)(CGXBase + GR_y_select + GR_set0);
	if (pokec((caddr_t)yaddr, 0))
		return (0);
	if (pokec((caddr_t)xaddr, DATA1))
		return (0);
	peekc((caddr_t)xaddr);
	pokec((caddr_t)xaddr, DATA2);
	if (peekc((caddr_t)xaddr) == (~DATA1 & 0xFF))
		return (CGSIZE);
	return (0);
}

/*
 * Only allow opens for writing or reading and writing
 * because reading is nonsensical.
 */
cgopen(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);

	if (unit >= NCG || cginfo[unit] == 0 || cginfo[unit]->md_alive == 0)
		return (ENXIO);
	if (!(flag & FWRITE))
		return (EINVAL);
	return (0);
}

/*ARGSUSED*/
cgioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *)data;

		fb->fb_type = FBTYPE_SUN1COLOR;
		fb->fb_height = 480;
		fb->fb_width = 640;
		fb->fb_depth = 8;
		fb->fb_cmsize = 256;
		fb->fb_size = 512*640;
		break;
		}

	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * We need to handle vertical retrace interrupts here.
 * The color map(s) can only be loaded during vertical 
 * retrace; we should put in ioctls for this to synchronize
 * with the interrupts.
 * FOR NOW, just turn off any interrupting color board.
 */
cgintr()
{
	register int i, found=0;
	register struct mb_device *md;
	register caddr_t CGXBase;

	for (i = 0; i < NCG; i++) {
		if ((md = cginfo[i]) == NULL)
			continue;
		if (!md->md_alive)
			continue;
		CGXBase = md->md_addr;
		if ((*GR_sreg & GR_inten) && (*GR_sreg & GR_vretrace)) {
			found = 1;
			*GR_sreg &= ~GR_inten;
		}
	}
	return (found);
}

/*ARGSUSED*/
cgmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	register caddr_t addr = cginfo[minor(dev)]->md_addr;
	register int page, uc;

	if (off >= CGSIZE)
		return (-1);
	uc = getusercontext();
	setusercontext(KCONTEXT);
	page = getpgmap(addr + off) & PG_PFNUM;
	setusercontext(uc);
	return (page);
}
#endif
