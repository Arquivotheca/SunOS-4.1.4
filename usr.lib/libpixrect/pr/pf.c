#ifndef lint
static char sccsid[] = "@(#)pf.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986-1989 Sun Microsystems, Inc.
 */

/*
 * Pixfont open/close: pf_open(), pf_open_private(), pf_close()
 */

#include <vfont.h>
#include <stdio.h>
#include <strings.h>

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_impl_util.h>

extern char *calloc(), *malloc();

extern Pixfont *pf_resident(), *pf_bltindef();
static Pixfont *pf_load_vfont();

/* shared font list */
static struct font_use {
	struct font_use *next;
	char *name;
	Pixfont *pf;
	int count;
	int resident;
} Fonts[1];


/* public functions */

Pixfont *
pf_open(fontname)
	char *fontname;
{
	register struct font_use *use, *prev;
	register char *basename;

	if (fontname == 0) 
		return pf_default();

	if (basename = rindex(fontname, '/'))
		basename++;
	else
		basename = fontname;

	/* look for the font in the shared font list */
	for (prev = Fonts; use = prev->next; prev = use) 
		if (strcmp(use->name, basename) == 0) {
			/* found it */
			use->count++;
			return use->pf;
		}

	/* didn't find it, add it to the end of the list */
	if (use = (struct font_use *) 
		malloc((unsigned) (sizeof *use + strlen(basename) + 1))) {
		use->next = 0;
		(void) strcpy(use->name = (char *) (use + 1), basename);
		use->count = 1;
		use->resident = 0;

		if (use->pf = pf_resident(basename))
			use->resident = 1;
		else 
			use->pf = pf_load_vfont(fontname);

		if (use->pf) {
			prev->next = use;
			return use->pf;
		}
		else
			(void) free((char *) use);
	}

	return 0;
}

Pixfont *
pf_open_private(fontname)
	char *fontname;
{
	/* 
	 * We don't know the name of the default font, and
	 * there is no code to copy it, so we punt.
	 */
	if (fontname == 0)
		return 0;

	return pf_load_vfont(fontname);
}

pf_close(pf)
	Pixfont *pf;
{
	register struct font_use *use, *prev;

	if (pf == 0 || pf == pf_bltindef())
		return 0;

	/* look for a font in the shared font list */
	for (prev = Fonts; use = prev->next; prev = use) {
		if (use->pf == pf) {
			/* found it */
			if (--use->count <= 0) {
				if (!use->resident)
					free((char *) pf);

				prev->next = use->next;
				(void) free((char *) use);
			}
			return 0;
		}
	}

	/* didn't find font, must be private */
	free((char *) pf);

	return 0;
}


/* implementation */

static Pixfont *
pf_load_vfont(fontname)
	char *fontname;
{
	FILE *fontf;
	struct header hd;
	int defx, defy;
	register struct dispatch *d;
	register unsigned nfont, nimage;
	register Pixfont *pf = 0;
	struct dispatch disp[NUM_DISPATCH];

	if (!(fontf = fopen(fontname, "r")))
		return pf;

	if (fread((char *) &hd, sizeof hd, 1, fontf) != 1 ||
		fread((char *) disp, sizeof disp, 1, fontf) != 1 ||
		hd.magic != VFONT_MAGIC)
		goto bad;

	/* 
	 * Set default sizes.  The default width of the font is taken to be
	 * the width of a lower-case a, if there is one. The default
	 * interline spacing is taken to be 3/2 the height of an
	 * upper-case A above the baseline.
	 */
	if (disp['a'].nbytes && disp['a'].width > 0 &&
		disp['A'].nbytes && disp['A'].up > 0) {
		defx = disp['a'].width;
		defy = disp['A'].up * 3 >> 1;
	}
	else {
		defx = hd.maxx;
		defy = hd.maxy + hd.xtend;
	}

	if (defx <= 0 || defy <= 0)
		goto bad;

	/* compute total font and image size */
	nfont = sizeof (struct pixfont);
	nimage = 0;

	for (d = disp; d < &disp[NUM_DISPATCH]; d++)
		if (d->nbytes) {
			register int w, h;

			if ((w = d->left + d->right) <= 0 ||
				(h = d->up + d->down) <= 0) {
				d->nbytes = 0;
				continue;
			}

			nfont += sizeof (struct pixrect) +
				sizeof (struct mpr_data);

			w = w <= 16 ? 2 : w + 31 >> 5 << 2;

			/* may need extra space to preserve 32 bit alignment */
			if (w > 2 && nimage & 2)
				nimage += 2;

			nimage += pr_product(w, h);
		}

	/* allocate space for pixfont, pixrects, and images */
	if (pf = (Pixfont *) calloc(1, nimage += nfont)) {
		pf->pf_defaultsize.x = defx;
		pf->pf_defaultsize.y = defy;

		/* construct pixfont */
		if (pf_build(fontf, disp, &pf->pf_char[0], 
			(caddr_t) (pf + 1), PTR_ADD(pf, nfont))) {
			free((char *) pf);
			pf = 0;
		}
	}

bad:
	(void) fclose(fontf);
	return pf;
}

static
pf_build(fontf, disp, pc, data, image)
	FILE *fontf;
	register struct dispatch *disp;
	struct pixchar *pc;
	caddr_t data;
	char *image;
{
	int nchar;
	register unsigned short addr = 0;

	/* hack to recycle registers */
	register caddr_t Aptr, Bptr;

#define	Apc	((struct pixchar *) Aptr)
#define	Bpr	((Pixrect *) Bptr)
#define Aprd	((struct mpr_data *) Aptr)
#define	Bimage	((char *) Bptr)
#define	Afile	((FILE *) Aptr)

	for (nchar = NUM_DISPATCH; --nchar >= 0; disp++, pc++)
		if (disp->nbytes) {
			register int w, h, pad;

			/* create pixchar */
			Aptr = (caddr_t) pc;
			w = disp->left;
			Apc->pc_home.x = -w;
			w += disp->right;
			h = disp->up;
			Apc->pc_home.y = -h;
			h += disp->down;
			Apc->pc_adv.x = disp->width;
			Apc->pc_adv.y = 0;

			/* create pixrect */
			Bptr = (caddr_t) data;
			Apc->pc_pr = Bpr;
			Bpr->pr_ops = &mem_ops;
			Bpr->pr_size.x = w;
			Bpr->pr_size.y = h;
			Bpr->pr_depth = 1;

			/* create pixrect data */
			Aptr = (caddr_t) (Bpr + 1);
			Bpr->pr_data = (caddr_t) Aprd;
			w = (w + 7) >> 3;
			pad = w <= 2 ? 2 : w + 3 & ~3;
			Aprd->md_linebytes = pad;

			Bptr = (caddr_t) image;
			if (pad > 2 && (int) Bptr & 2)
				Bptr += 2;
			Aprd->md_image = (short *) Bimage;
			Aprd->md_flags = w <= 2 ? MP_FONT : 0;
			data = (caddr_t) (Aprd + 1);

			Aptr = (caddr_t) fontf;

			if (addr != disp->addr) {
				(void) fseek(Afile, 
					(long) (sizeof (struct header) + 
					sizeof (struct dispatch) * 
						NUM_DISPATCH + 
					disp->addr),
					0);
				addr = disp->addr;
			}

#ifdef MUTANT
			/* check for mutant short-padded vfont */
			if (w & 1 && pr_product(w + 1, h) == disp->nbytes)
				w++;
#endif MUTANT

			pad -= w;
			while (--h >= 0) {
				PR_LOOPP(w - 1,
					*Bimage = getc(Afile);
					Bptr++);
				Bptr += pad;
				addr += w;
			}

			image = Bptr;
		}

	return ferror(Afile) || feof(Afile);
}
