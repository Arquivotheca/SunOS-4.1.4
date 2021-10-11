#ifndef lint
static char     sccsid[] = "@(#)mem32_cmap.c	1.1 94/10/31 SMI";
#endif

/* Copyright 1990 by Sun Microsystems, Inc. */

/*
 * 8 bit emulation in 24/32 bit memory pixrect colormap and
 * attribute functions.
 *
 * see cg9_colormap.c for detailed comments.
 */

/* these eight files are necessary just for cmschange! */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/mem32_var.h>
#include <pixrect/pr_planegroups.h>

mem32_getcolormap(pr, index, count, red, green, blue)
Pixrect            *pr;
int                 index,
                    count;
unsigned char      *red,
                   *green,
                   *blue;
{
    int                 cc,
                        i,
                        plane;
    struct mprp32_data *mem32d;
    struct fbcmap       fbmap;

    mem32d = mprp32_d(pr);
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = mem32d->plane_group;

    if (index & PR_FORCE_UPDATE || plane == PIXPG_24BIT_COLOR)
    {
	fbmap.index = index | PIX_GROUP(plane);
	fbmap.count = count;
	fbmap.red = red;
	fbmap.green = green;
	fbmap.blue = blue;

	return ioctl(mem32d->fd, FBIOGETCMAP, (caddr_t) & fbmap, 0);
    }
    else
    {
	if (plane == PIXPG_OVERLAY_ENABLE)
	    return 0;

	if (plane == PIXPG_OVERLAY)
	{
	    for (cc = i = 0; count && !cc; i++, index++, count--)
	    {
		/* Index 0 is mapped to 1.  All others mapped to 3. */
		fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
		fbmap.count = 1;
		fbmap.red = red + i;
		fbmap.green = green + i;
		fbmap.blue = blue + i;
		cc = ioctl(mem32d->fd, FBIOGETCMAP, (caddr_t) & fbmap, 0);
	    }
	    return cc;
	}
    }
    return PIX_ERR;
}

mem32_win_getcms(windowfd, cms, cmap)
int                 windowfd;
struct colormapseg *cms;
struct cms_map     *cmap;
{
    struct cmschange    cmschange;

    cmschange.cc_cms = *cms;
    cmschange.cc_map = *cmap;
    return ioctl(windowfd, WINGETCMS, &cmschange);
}

mem32_get_index_cmap(pr, cms, cmap)
Pixrect            *pr;
struct colormapseg *cms;
struct cms_map     *cmap;
{
    int                 i;
    unsigned char       r[MEM32_8BIT_CMAPSIZE],
                        g[MEM32_8BIT_CMAPSIZE],
                        b[MEM32_8BIT_CMAPSIZE];
    struct colormapseg  tmp_cms;
    struct cms_map      tmp_cmap;

    tmp_cms = *cms = mprp32_d(pr)->cms;
    pr_getcolormap(pr, 0, MEM32_8BIT_CMAPSIZE, cmap->cm_red, cmap->cm_green,
	cmap->cm_blue);

    /* if winfd >= 0, then index set by pwo_putcolormap (pixwin application) */

    if (mprp32_d(pr)->windowfd >= 0)
    {
	/* win_getcms requires addr to be zero based */
	tmp_cms.cms_addr = 0;
	tmp_cmap.cm_red = r;
	tmp_cmap.cm_green = g;
	tmp_cmap.cm_blue = b;
	mem32_win_getcms(mprp32_d(pr)->windowfd, &tmp_cms, &tmp_cmap);
	for (i = 0; i < cms->cms_size; i++)
	{
	    cmap->cm_red[i + cms->cms_addr] = tmp_cmap.cm_red[i];
	    cmap->cm_green[i + cms->cms_addr] = tmp_cmap.cm_green[i];
	    cmap->cm_blue[i + cms->cms_addr] = tmp_cmap.cm_blue[i];
	}
    }

    /* otherwise index set by pixrect application */

    return 0;
}

mem32_get_index(pr, true_color)
Pixrect            *pr;
int                 true_color;
{
    int                 i,
                        j;
    unsigned char       r[MEM32_8BIT_CMAPSIZE],
                        g[MEM32_8BIT_CMAPSIZE],
                        b[MEM32_8BIT_CMAPSIZE];
    struct colormapseg  cms;
    struct cms_map      cmap;

    cmap.cm_red = r;
    cmap.cm_green = g;
    cmap.cm_blue = b;
    mem32_get_index_cmap(pr, &cms, &cmap);
    for (i = cms.cms_addr, j = cms.cms_addr + cms.cms_size; i < j; i++)
    {
	if (true_color == b[i] << 16 | g[i] << 8 | r[i])
	    return i;
    }
    return i - 1;
}

mem32_get_true(pr, index, planes)
Pixrect            *pr;
int                 index,
                    planes;
{
    unsigned char       r[MEM32_8BIT_CMAPSIZE],
                        g[MEM32_8BIT_CMAPSIZE],
                        b[MEM32_8BIT_CMAPSIZE];
    struct colormapseg  cms;
    struct cms_map      cmap;

    cmap.cm_red = r;
    cmap.cm_green = g;
    cmap.cm_blue = b;
    mem32_get_index_cmap(pr, &cms, &cmap);
    index &= MEM32_8BIT_CMAPSIZE - 1;
    if (index < cms.cms_addr && (planes <= (cms.cms_size - 1)))
	index += cms.cms_addr;
    if (index > cms.cms_addr + cms.cms_size && (planes <= (cms.cms_size - 1)))
	index = cms.cms_addr + cms.cms_size - 1;
    return (b[index] << 16 | g[index] << 8 | r[index]);
}
