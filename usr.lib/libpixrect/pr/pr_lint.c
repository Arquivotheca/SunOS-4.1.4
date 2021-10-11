/* @(#)pr_lint.c 1.1 94/10/31 SMI */

/*
 * Copyright 1987 by Sun Microsystems,  Inc.
 */

#ifndef LINT_INCLUDE
#define LINT_INCLUDE
#include <pixrect/pixrect_hs.h>
#include <stdio.h>
#endif

/* 
 * pixrect creation 
 */
Pixrect * 
pr_open(fbname) 
	char *fbname; 
	{ return (Pixrect *) 0; }

/* 
 * unstructured pixrect ops 
 */
pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy) 
	Pixrect *dpr, *spr;
	int dx, dy, w, h, op, sx, sy;
	{ return 0; }

pr_stencil(dpr, dx, dy, w, h, op, stpr, stx, sty, spr, sx, sy) 
	Pixrect *dpr, *stpr, *spr;
	int dx, dy, w, h, op, stx, sty, sx, sy;
	{ return 0; }

pr_batchrop(dpr, x, y, op, items, n) 
	Pixrect *dpr;
	int x, y, op, n;
	struct pr_prpos items[];
	{ return 0; }

pr_destroy(pr) 
	Pixrect *pr;
	{ return 0; }

pr_close(pr)
	Pixrect *pr;
	{ return 0; }

pr_get(pr, x, y) 
	Pixrect *pr;
	int x, y;
	{ return 0; }

pr_put(pr, x, y, value) 
	Pixrect *pr;
	int x, y, value;
	{ return 0; }

pr_vector(pr, x0, y0, x1, y1, op, color) 
	Pixrect *pr;
	int x0, y0, x1, y1, op, color;
	{ return 0; }

Pixrect *
pr_region(pr, x, y, w, h) 
	Pixrect *pr;
	int x, y, w, h;
	{ return (Pixrect *) 0; }

pr_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

pr_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

pr_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

pr_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }


/* 
 * structured pixrect ops 
 */

prs_rop(dstreg, op, srcprpos) 
	struct pr_subregion dstreg;
	int op;
	struct pr_prpos srcprpos;
	{ return 0; }

prs_stencil(dstreg, op, stenprpos, srcprpos) 
	struct pr_subregion dstreg;
	int op;
	struct pr_prpos stenprpos, srcprpos;
	{ return 0; }

prs_batchrop(dstprpos, op, items, n) 
	struct pr_prpos dstprpos, items[];
	int op, n;
	{ return 0; }

prs_destroy(pr)
	Pixrect *pr;
	{ return 0; }

prs_get(srcprpos) 
	struct pr_prpos srcprpos;
	{ return 0; }

prs_put(dstprpos, val) 
	struct pr_prpos dstprpos;
	int val;
	{ return 0; }

prs_vector(pr, pos0, pos1, op, color) 
	Pixrect *pr;
	struct pr_pos pos0, pos1;
	int op, color;
	{ return 0; }

Pixrect *
prs_region(dstreg) 
	struct pr_subregion dstreg;
	{ return (Pixrect *) 0; }

prs_putcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

prs_getcolormap(pr, index, count, red, green, blue) 
	Pixrect *pr;
	int index, count;
	unsigned char red[], green[], blue[];
	{ return 0; }

prs_putattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

prs_getattributes(pr, planes) 
	Pixrect *pr;
	int *planes;
	{ return 0; }

/* 
 * non-ops vector 
 */

pr_line(pr, x0, y0, x1, y1, brush, tex, op)
	Pixrect *pr;
	int x0, y0, x1, y1;
	struct pr_brush *brush;
	Pr_texture *tex;
	{ return 0; }

pr_polygon_2(dpr, dx, dy, nbnds, npts, vlist, op, spr, sx, sy)
	Pixrect *dpr, *spr;
	int dx, dy, nbnds, npts[], op, sx, sy;
	struct pr_pos *vlist;
	{ return 0; }

pr_polyline(pr, dx, dy, npts, ptlist, mvlist, brush, tex, op)
	Pixrect *pr;
	int dx, dy, npts;
	struct pr_pos *ptlist;
	u_char *mvlist;
	struct pr_brush *brush;
	Pr_texture *tex;
	{ return 0; }

pr_polypoint(pr, x, y, n, ptlist, op)
	Pixrect *pr;
	int x, y, n, op;
	struct pr_pos *ptlist;
	{ return 0; }			

pr_replrop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr, *spr;
	int dx, dy, dw, dh, op, sx, sy;
	{ return 0; }

prs_replrop(dstreg, op, srcprpos) 
	struct pr_subregion dstreg;
	int op;
	struct pr_prpos srcprpos;
	{ return 0; }

pr_texvec(pr, x0, y0, x1, y1, tex, op)
	Pixrect *pr;
	int x0, y0, x1, y1, op;
	Pr_texture *tex;
	{ return 0; }

pr_traprop(dpr, dx, dy, t, op, spr, sx, sy)
	Pixrect *dpr, *spr;
	int dx, dy, op, sx, sy;
	struct pr_trap t;
	{ return 0; }

/*
 * undocumented
 */

pr_getfbtype_from_fd(fd)
	int fd;
	{ return 0; }

pr_gen_batchrop(dpr, dx, dy, op, src, count)
        Pixrect *dpr;
        int dx, dy, op, count; 
        struct pr_prpos *src;
	{ return 0; }

/* 
 * monochrome colormaps 
 */
pr_blackonwhite(pr, min, max)
	Pixrect *pr; 
	int min, max; 
	{ return 0; }

pr_whiteonblack(pr, min, max)
	Pixrect *pr; 
	int min, max; 
	{ return 0; }

pr_reversevideo(pr, min, max)
	Pixrect *pr; 
	int min, max; 
	{ return 0; }

/* 
 * plane groups 
 */
pr_available_plane_groups(pr, maxgroups, groups)
	Pixrect *pr;
	int maxgroups;
	char *groups;
	{ return 0; }

pr_get_plane_group(pr)
	Pixrect *pr;
	{ return 0; }

void
pr_set_plane_group(pr, group)
	Pixrect *pr;
	int group;
	{ return; }

void
pr_set_planes(pr, group, planes)
	Pixrect *pr;
	int group, planes;
	{ return; }

/*
 * double buffering
 */

pr_dbl_get(pr, attr)
	Pixrect *pr;
	int attr;
	{ return 0; }

/*VARARGS2*/
pd_dbl_set(pr, attr)
	Pixrect *pr;
	int attr;
	{ return 0; }

/*
 * pixrect I/O 
 */

pr_dump(input_pr, output, colormap, type, copy_flag)
	Pixrect	*input_pr;
	FILE *output;
	colormap_t *colormap;
	int type, copy_flag;
	{ return 0; }

pr_dump_header(output, rh, colormap)
	FILE *output;
	struct rasterfile *rh;
	colormap_t *colormap;
	{ return 0; }

pr_dump_image(pr, output, rh)
	Pixrect *pr;
	FILE *output;
	struct rasterfile *rh;
	{ return 0; }

Pixrect *
pr_dump_init(input_pr, rh, colormap, type, copy_flag)
	Pixrect *input_pr;
	struct rasterfile *rh;
	colormap_t *colormap;
	int type, copy_flag;
	{ return (Pixrect *) 0; }

Pixrect *
pr_load(input, colormap)
	FILE *input;
	colormap_t *colormap;
	{ return (Pixrect *) 0; }

pr_load_colormap(input, rh, colormap)
	FILE *input;
	struct rasterfile *rh;
	colormap_t *colormap;
	{ return 0; }

pr_load_header(input, rh)
	FILE *input;
	struct rasterfile *rh;
	{ return 0; }

Pixrect *
pr_load_image(input, rh, colormap)
	FILE *input;
	struct rasterfile *rh;
	colormap_t *colormap;
	{ return (Pixrect *) 0; }

Pixrect *
pr_load_std_image(input, rh, colormap)
	FILE *input;
	struct rasterfile *rh;
	colormap_t *colormap;
	{ return (Pixrect *) 0; }

/*
 * pixfonts 
 */

Pixfont * 
pf_open(fontname) 
	char *fontname;
	{ return (Pixfont *) 0; }

Pixfont * 
pf_open_private(fontname) 
	char *fontname;
	{ return (Pixfont *) 0; }

Pixfont * 
pf_default()
	{ return (Pixfont *) 0; }

pf_close(pf) 
	Pixfont *pf; 
	{ return 0; }

pf_text(where, op, font, text)
	struct pr_prpos where;
	int op;
	Pixfont *font;
	char *text;
	{ return 0; }

pf_ttext(where, op, font, text)
	struct pr_prpos where;
	int op;
	Pixfont *font;
	char *text;
	{ return 0; }

struct pr_size 
pf_textbatch(where, lengthp, font, text)
	struct pr_prpos where[];
	int *lengthp;
	Pixfont *font;
	char *text;
	{ static struct pr_size prs; return prs; }

pf_textbound(bound, len, font, text)
        struct pr_subregion *bound; 
	int len; 
	Pixfont *font;
	char *text;
	{ return 0; }

struct pr_size 
pf_textwidth(len, font, text)
        int len; 
	Pixfont *font;
	char *text;
	{ static struct pr_size prs; return prs; }

pr_text(pr, x, y, op, font, text)
	Pixrect *pr;
	int x, y, op;
	Pixfont *font;
	char *text;
	{ return 0; }

pr_ttext(pr, x, y, op, font, text)
	Pixrect *pr;
	int x, y, op;
	Pixfont *font;
	char *text;
	{ return 0; }

prs_text(where, op, font, text)
	struct pr_prpos where;
	int op;
	Pixfont *font;
	char *text;
	{ return 0; }

prs_ttext(where, op, font, text)
	struct pr_prpos where;
	int op;
	Pixfont *font;
	char *text;
	{ return 0; }

/* pixrect implementation */

char pr_reversedst[16];
char pr_reversesrc[16];

pr_clip(dstsubregion, srcprpos)
	struct pr_subregion *dstsubregion;
	struct pr_prpos *srcprpos;
	{ return 0; }

Pixrect *
pr_makefromfd(fd, size, depth, devdata, curdd, 
	mmapbytes, privdatabytes, mmapoffsetbytes)
	int fd;
	struct pr_size size;
	int depth;
	struct pr_devdata **devdata, **curdd;
	int mmapbytes, privdatabytes, mmapoffsetbytes;
	{ return (Pixrect *) 0; }

pr_unmakefromfd(fd, devdata)
	struct pr_devdata **devdata;
	int fd;
	{ return 0; }


/*
 * Global data
 */

short pr_tex_dotted[1];
short pr_tex_dashed[1];
short pr_tex_dashdot[1];
short pr_tex_dashdotdotted[1];
short pr_tex_longdashed[1];
