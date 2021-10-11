#ifndef lint
static	char sccsid[] = "@(#)demo_cg.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Demos for pixrect operations 
 *
 * 	demo hv		demonstrates hidden text and vectors
 *
 * Cases:
 *	a	set colormap, clear, write christmas tree stencils
 *	A	stencil test
 *	b	batchrop text test
 *	c	cursor test - slide blank cursor diagonally down screen
 *	C	clipping test - sends text beyond all four screen boundaries
 *	g	get/put test - simulates text test using get and put pixel
 *	h	hidden text test - slide text over text
 *	r	replicate test
 *	R	Region batchrop clipping test
 *	s	set test - sets a foreground rectangle (also tests null source)
 *	t	text - write "The quick brown..." to screen, read, print %x
 *	v	vector test - display asterisks with empty centers
 *	x	xor test - xor text to top left corner of text and to center
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>

struct	pixrect *mpr;
struct	pixrect  *screen;
PIXFONT *pf;
int	op;
static short cursorimage[16] = {
		0x8000, 0x6000, 0x7800, 0x3E00,
		0x3F80, 0x1FE0, 0x1FF8, 0x0FFE,
		0x0FE0, 0x07F0, 0x07F8, 0x037C,
		0x033E, 0x011F, 0x010E, 0x0004
};
short	stippleimage[32] = {
	0xcccc, 0xcccc, 0xaaaa, 0x5555,
	0xcccc, 0xcccc, 0xaaaa, 0x5555,
	0xcccc, 0xcccc, 0xaaaa, 0x5555,
	0xcccc, 0xcccc, 0xaaaa, 0x5555,
	0xa0a0, 0x4040, 0xa0a0, 0x0000,
	0x0a0a, 0x0404, 0x0a0a, 0x0000,
	0xa0a0, 0x4040, 0xa0a0, 0x0000,
	0x0a0a, 0x0404, 0x0a0a, 0x0000
};

static unsigned	gridimage[128] = {
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
			0xFFFFFFFF, 0xFFFFFFFF, 0xAAAAAAAA, 0xAAAAAAAA,
		};



static unsigned	ballsimage[128] = {
			0x00000002, 0x00000000, 0x00000007, 0x00000000,
			0x00000002, 0x00000000, 0x00000002, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000400, 0x00100000, 0x00000E00, 0x00380000,
			0x00000400, 0x00100000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000004, 0x00000000, 0x0000000E, 0x00000000,
			0x00000004, 0x00000000, 0x00000000, 0x00000000,
			0x00020000, 0x00000000, 0x00070000, 0x00000000,
			0x00020000, 0x00000000, 0x00000000, 0x00001000,
			0x00000000, 0x00003800, 0x00000000, 0x00001000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x08000000, 0x00000000, 0x1C000000,
			0x00000000, 0x08000000, 0x00000000, 0x00000000,
			0x04000000, 0x00000000, 0x0E000000, 0x00000100,
			0x04000000, 0x00000380, 0x00000400, 0x00000100,
			0x00000E00, 0x00000000, 0x00000400, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00400000,
			0x00000000, 0x00E00000, 0x00000000, 0x00400000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x10000000, 0x00000080,
			0x38000000, 0x000001C0, 0x10000000, 0x00000080,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000
		};
static unsigned	trunkimage[128] = {
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000003, 0xC0000000,
			0x0000000F, 0xE0000000, 0x0000003F, 0xF0000000,
			0x0000007F, 0xF0000000, 0x0000007F, 0xF0000000,
			0x0000007F, 0xF0000000, 0x0000007F, 0xF8000000,
			0x000000FF, 0xF8000000, 0x000000FF, 0xFC000000,
			0x000000FF, 0xFC000000, 0x000001FF, 0xFE000000,
			0x000001FF, 0xFF000000, 0x000003FF, 0xFF000000
		};
static unsigned	treeimage[128] = {
			0x00000002, 0x00000000, 0x00000002, 0x00000000,
			0x00000006, 0x00000000, 0x00000007, 0x00000000,
			0x00000007, 0x00000000, 0x0000000F, 0x80000000,
			0x0000000F, 0x80000000, 0x0000000F, 0x80000000,
			0x0000001F, 0xC0000000, 0x0000001F, 0xC0000000,
			0x0000001F, 0xE0000000, 0x0000003F, 0xF0000000,
			0x0000007F, 0xF8000000, 0x000000FF, 0xFE000000,
			0x000007FF, 0xFF800000, 0x000003FF, 0xFFE00000,
			0x0000003F, 0xFFC00000, 0x0000003F, 0xF0000000,
			0x0000007F, 0xF8000000, 0x0000007F, 0xFC000000,
			0x000000FF, 0xFC000000, 0x000001FF, 0xFE000000,
			0x000003FF, 0xFF000000, 0x000007FF, 0xFF000000,
			0x00000FFF, 0xFFC00000, 0x00003FFF, 0xFFE00000,
			0x0000FFFF, 0xFFF80000, 0x0003FFFF, 0xFFFF0000,
			0x0000FFFF, 0xFFFFF000, 0x000007FF, 0xFFFFE000,
			0x00000FFF, 0xFFFE0000, 0x00000FFF, 0xFFF00000,
			0x00001FFF, 0xFF800000, 0x00003FFF, 0xFF800000,
			0x00007FFF, 0xFFE00000, 0x0000FFFF, 0xFFF00000,
			0x0001FFFF, 0xFFF80000, 0x0007FFFF, 0xFFFE0000,
			0x000FFFFF, 0xFFFF8000, 0x003FFFFF, 0xFFFFF000,
			0x00FFFFFF, 0xFFFFFF00, 0x07FFFFFF, 0xFFFFFF00,
			0x03FFCFFF, 0xFFFFFC00, 0x007E1FFF, 0xFFFFE000,
			0x00003FFF, 0xFFFE0000, 0x00003FFF, 0xFFFE0000,
			0x00007FFF, 0xFFFF0000, 0x0000FFFF, 0xFFFFC000,
			0x0001FFFF, 0xFFFFF000, 0x0003FFFF, 0xFFFFFC00,
			0x0007FFFF, 0xFFFFFF00, 0x001FFFFF, 0xFFFFFFF8,
			0x003FFFFC, 0x3FFFFFF8, 0x03FFFFF0, 0x3FFFFFF0,
			0x1FFFFFC0, 0x0FFFFFE0, 0x07FFF000, 0x07FFFFE0,
			0x00000000, 0x01FFFF00, 0x00000000, 0x007FFE00,
			0x00000000, 0x000FE000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000
		};
mpr_static(cursor, 16, 16, 1, cursorimage);
mpr_static(stipple, 16, 32, 1, stippleimage);
mpr_static(grid, 64, 64, 1, (short *)gridimage);
mpr_static(tree, 64, 64, 1, (short *)treeimage);
mpr_static(trunk, 64, 64, 1, (short *)trunkimage);
mpr_static(balls, 64, 64, 1, (short *)ballsimage);

short	dtgrayimage[16] = {
	0x8888, 0x8888,0x2222, 0x2222,
	0x8888, 0x8888,0x2222, 0x2222,
	0x8888, 0x8888,0x2222, 0x2222,
	0x8888, 0x8888,0x2222, 0x2222
};
mpr_static(dtgray, 16, 16, 1, dtgrayimage);

short	dkgrayimage[2] = {
	0xaaaa, 0xaaaa
};
mpr_static(dkgray, 16, 1, 1, dkgrayimage);

static int planes = 255;
static int color;
static u_char red[256], green[256], blue[256];

main(argc, argv)
	int argc;
	char **argv;
{
	int clip = PIX_DONTCLIP;
	int i, x, y, z;
	struct pixrect *mpr1;
	char *c;

	if (argc == 1) {
		printf("Usage: %s x\n\
where x is any string of letters from:\n\
a	set colormap, clear, christmas tree stencil\n\
A	stencil test\n\
b	batchrop sample text to screen\n\
c	cursor slides diagonally down\n\
C	clipping test on screen\n\
g	get/put - same effect as t\n\
h	hidden-window test - slide one window over another\n\
r	replicate test\n\
R	Region batchrop clipping test\n\
s	set test - set a foreground-color rectangle\n\
t	text test - basic read-write test\n\
v	vectors - display growing-asterisk pattern\n\
x	xor test - xor characters (a) with self (b) with center\n", argv[0]);
		exit(1);
	}

	/*
	 * Create memory and screen pixrects.
	 */
	mpr = mem_create(640, 480, 8);
	if (mpr == 0) {
		fprintf(stderr, "Can't allocate memory pixrect\n");
		exit(1);
	}
	screen = pr_open("/dev/cgtwo0");
	if (screen == 0) {
		fprintf(stderr, "Can't open frame buffer (/dev/cgtwo0)\n");
		exit(1);
	}
	if (argv[0][0] == 'r')
		pr_reversevideo( screen);

	if (!strcmp(argv[1], "all"))
		argv[1] =
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	
	for (c = argv[1]; *c; c++)
	switch (*c) {
	case 'a':
		pr_putattributes(screen, &planes);
		for (i=0; i<64; i++) {
			x = ((i+1)<<2) -1;
			red[i] = x;
			red[i+192] = x;
			green[i+192] = x;
			green[i+64] = x;
			blue[i+128] = x;
		}
		red[0] = green[0] = blue[0] = 127;
		red[255] = green[255] = blue[255] = 255;
		pr_putcolormap( screen, 0, 256, red, green, blue);

		clearscreen();
		op = PIX_COLOR( 127)|PIX_SRC;
		pr_stencil(screen,300,220,64,64,op,&tree,0,0,0,0,0);
		op = PIX_COLOR( 10)|PIX_SRC;
		pr_stencil(screen,300,220,64,64,op,&trunk,0,0,0,0,0);
		op = PIX_COLOR( 63)|PIX_SRC;
		pr_stencil(screen,300,220,64,64,op,&balls,0,0,0,0,0);
		break;
	case 'A':
		putalphabet(screen);
		pr_rop(screen,0,0,100,100,PIX_NOT(PIX_DST),0,0,0);
		msdelay( 200);
		printf("blackonwhite\n");
		pr_blackonwhite(screen, 0, 255);
		msdelay( 200);
		printf("whiteonblack\n");
		pr_whiteonblack(screen, 0, 255);
		msdelay( 200);
		printf("reversevideo\n");
		pr_reversevideo(screen, 0, 255);
		msdelay( 200);
		op = PIX_COLOR(63)|PIX_SRC;
		for (i=100; i<200; i+=10) {
			pr_stencil(screen,i,i,16,16,op,&cursor,0,0,0,0,0);
		}
		op = PIX_COLOR(250)|PIX_SRC;
		for (i=100; i<200; i+=10) {
		    pr_stencil(screen,i+50,i,16,16,op,&cursor,0,0,&stipple,0,0);
		}
		op = PIX_COLOR(127)|PIX_SRC;
		pr_stencil(screen,100,100,80,90,op,&tree,4,3,&grid,9,1);
		pr_stencil(screen,200,100,64,64,op,&tree,0,0,&grid,0,0);

		pr_rop(screen,400,300,64,64,op,screen,0,0);
		pr_stencil(screen,350,250,64,64,op,&tree,0,0,screen,400,300);
		pr_stencil(screen,350,350,64,64,op,&tree,0,0,screen,400,300);
		pr_stencil(screen,450,250,64,64,op,&tree,0,0,screen,400,300);
		pr_stencil(screen,450,350,64,64,op,&tree,0,0,screen,400,300);
		mpr1 = mem_create(64, 64, 8);
		pr_stencil(mpr1,0,0,64,64,op,&tree,0,0,screen,5,0);
		pr_rop(screen,250,0,64,64,op,mpr1,0,0);

		break;
	case 'B':
		clip = 0;
	case 'b': {	/* ---------- Batchrop test ------------ */
		struct timeval tv, tv2;
		pf = pf_open(0);
		op = PIX_COLOR(250) | PIX_SRC | clip;
		gettimeofday(&tv, 0);
		for (x = 0; x < 10; x++) {
			int j;
			for (j = 20; j < 460; j += 20)
				pf_text(screen, 0, j, op, pf,
				"A quick brown fox jumped over the lazy dog.");
		}
		gettimeofday(&tv2, 0);
		printf("%d microseconds per character\n", 
		     ((tv2.tv_sec-tv.tv_sec)*1000000 + (tv2.tv_usec-tv.tv_usec))
		     /(((460-20)/20)*(43)*10));
		break;
		}
	case 'c':	/* ---------- Cursor test ---------------------- */
		putalphabet(screen);
		op = PIX_COLOR(250)|PIX_SRC;
		for (x = 0, y = 0; x < 400; x++, y++) {
			pr_rop(mpr, 0, 0, 16, 16, op, screen, x, y);
			pr_rop(screen, x, y, 16, 16, op, &cursor, 0, 0);
			pr_rop(screen, x, y, 16, 16, op, mpr, 0, 0);
		}
		msdelay(200);
		break;

	case 'C': 	/* ---------- Clipping test ---------------------*/
#define CHX 8
#define CHY 12
				/* put centered text, save screen in mpr */
		putalphabet(screen);
		pr_rop(screen,350,250,26*CHX+1, 13*CHY,PIX_SRC, screen, 0, 0);
		pr_rop(screen,0,0, 26*CHX+1, 15*CHY, PIX_SRC, screen, 16, 200);
		pr_rop(mpr, 0, 0, 640, 480, PIX_SRC, screen, 0, 0);

				/* clip against four edges */
		pr_rop(screen,-100,200,26*CHX+1,13*CHY,PIX_SRC,mpr,350,250);
		pr_rop(mpr,   350, 250,26*CHX+1,13*CHY,PIX_SRC,screen,-100,200);
		pr_rop(screen,540, 300,26*CHX+1,13*CHY,PIX_SRC, mpr, 350, 250);
		pr_rop(screen,300,-100,26*CHX+1,13*CHY,PIX_SRC, mpr, 350, 250);
		pr_rop(screen,400, 450,26*CHX+1,13*CHY,PIX_SRC, mpr, 350, 250);

		for (x = 0; x >= -26*CHX+1; x--) {
			pr_rop(screen, x, 200, 26*CHX+1, 13*CHY,
			    PIX_SRC, mpr, 350, 250);
			pr_rop(mpr, 350, 250, 26*CHX+1, 13*CHY,
			    PIX_SRC, screen, x, 200);
		}
		mpr1 = mem_create(16, 16, 8);
		pr_rop(mpr1, 0, 0, 16, 16, PIX_SRC, screen, 350, 250);
		for (x = 330; x >= -26*CHX+1; x--) {
			pr_rop(screen, x, 300, 16, 16, PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 16, PIX_SRC, screen, x, 300);
		}
		pr_rop(mpr1, 0, 0, 16, 16, PIX_SRC, screen, 380, 300);
		for (x = 330; x >= -26*CHX+1; x--)
			pr_rop(screen, x, 300, 16, 16, PIX_SRC, mpr1, 0, 0);
		mpr1 = mem_create(16,12,8);
		pr_rop(mpr1, 0, 0, 16, 12, PIX_SRC, screen, 350, 250);
		for (x = 330; x >= -26*CHX+1; x--) {
			pr_rop(screen, x, 300, 16, 12, PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 12, PIX_SRC, screen, x, 300);
		}
		msdelay(200);
		break;

/* 'g' - Get/put test - simulates text test */
	case 'g': {
		short a, b;
		clearscreen();
		mpr1 = mem_create(16, 2, 8);
		strcpy(mpr_d(mpr1)->md_image,"Data correctly read from source");
		for (y = 0; y < 2; y++)
		    for (x = 0; x < 16; x++) {
			a = pr_get(mpr1, x, y);
			printf("%c",(u_char)a);
			pr_put(screen, 16+x, 16+y, a);
		    }
		printf("\n");
		strcpy(mpr_d(mpr1)->md_image,"get had no effect at all.      ");
		for (y = 0; y < 2; y++)
		    for (x = 0; x < 16; x++) {
			b = pr_get(screen, 16+x, 16+y);
			printf("%c", (u_char)b);
			pr_put(mpr1, x, y, b);
		    }
		printf("\n");
		for (i = 0; i < 32; i++)
			printf("%c", ((char *)mpr_d(mpr1)->md_image)[i]);
		printf("\n");
		msdelay(200);
		break;
		}
/* 'h' - Hidden-window test - slide text over text */
	case 'h':
		putalphabet(screen);
				/* central text */
		pr_rop(screen, 250, 150, 26*8, 13*12, PIX_SRC, screen, 0, 0);
				/* clr top left */
		pr_rop(screen, 0, 0, 26*8, 15*12, PIX_SRC, screen, 0, 200);
				/* save screen */
		pr_rop(mpr, 0, 0, 640, 480, PIX_SRC, screen, 0, 0);
				/* sliding text */
		pr_rop(screen, 112, 0, 10*8, 6*12, PIX_SRC, screen, 250, 150);
		for (x = 112, y = 0; x < 640; x++, y++) {
			pr_rop(screen,x+1,y+1,10*8,6*12,PIX_SRC,screen,x,y);
			pr_rop(screen, x, y, 1, 6*12,PIX_SRC, mpr, x, y);
			pr_rop(screen, x, y, 10*8, 1, PIX_SRC, mpr, x, y);
		}
		break;

/* 'r' - replrop test */
	case 'r':
		clearscreen();
		op = PIX_COLOR(127) | PIX_SRC;
		pr_replrop(screen,18,18,640-36,480-36,op,&dtgray,0,0);
		msdelay(200);
		pr_replrop(screen,0,1,1152,900,op,&tree,0,0);
		msdelay(200);
		op = PIX_SRC^PIX_DST;
		pr_replrop(screen,0,0,1152,900,op,&dkgray,0,0);
		msdelay(200);
		op = PIX_SRC;
		pr_replrop(screen,0,0,1152,900,op,&dtgray,0,0);
		msdelay(200);
		break;

	case 'R':
/* 'R' - Region batchrop clipping test - write gall10 to screen */
		{ int	OFFSET = 5, BASELINE = 18, HEIGHT = BASELINE-OFFSET;
		PIXFONT *pf = pf_open(0);
		struct pixrect *region =
			pr_region(screen, 0, OFFSET, 80, HEIGHT);

		clearscreen();
		pf_text(screen, BASELINE, BASELINE,
		    PIX_DONTCLIP|(PIX_SRC^PIX_DST), pf, "abcgpqy");
		msdelay(400);
		pf_text(region, BASELINE, BASELINE-OFFSET,
		    (PIX_SRC^PIX_DST), pf, "abcgpqy");
		msdelay(400);
		pr_destroy(region);
		region = pr_region(screen, 20, 20, 36, 70);
		putalphabet( region);
		pr_destroy(region);
		break;
		}

/* 's' - set test - put up a set (foreground) rectangle */
	case 's':
		pr_rop(screen, 300, 200, 200, 107, PIX_SET, 0, 0, 0);
		op = PIX_COLOR( 63) | PIX_SRC;
		pr_rop(screen, 100, 100, 200, 100, op, 0, 0, 0);
		op = PIX_COLOR( 127) | PIX_SRC;
		pr_rop(screen, 100, 307, 200, 100, op, 0, 0, 0);
		op = PIX_COLOR( 191) | PIX_SRC;
		pr_rop(screen, 500, 100, 200, 100, op, 0, 0, 0);
		op = PIX_COLOR( 254) | PIX_SRC;
		pr_rop(screen, 500, 307, 200, 100, op, 0, 0, 0);
		msdelay(100);
		break;

/* 'r' - read test - compare screen with what is read */
	case 't':
		clearscreen();
		outback(screen, (struct pixrect *)mem_create(32,8,8));
		outback(screen, (struct pixrect *)mem_create(16,16,8));
		msdelay(100);
		break;

/* 'v' - vector test - display asterisks each with an empty center */
	case 'v':
		for (z = 0; z < 16; z++)
		for (x = -1; x<2; x++)
		for (y = -1; y<2; y++)
		if (x != 0 || y != 0)
			pr_vector(screen, 10+z*z*2+x,10+z*z*2+y,
			    10+z*z*2+x*z,10+z*z*2+y*z, z<<1, z*16); 
	/* test 3 pixel vector */
		pr_vector(screen, 402, 400, 404, 401, PIX_SRC, 127);
	/* test 4 pixel vector */
		pr_vector(screen, 412, 400, 415, 401, PIX_SRC, 127);
	/* test other color */
		pr_vector(screen, 0, 450, 639, 450, PIX_SRC, 191);
		pr_vector(screen, 0, 0, 100, 60, PIX_SRC, 63);
		pr_vector(screen, 100, 60, 200, 120, PIX_SRC, 63);
		pr_vector(screen, 0, 0, 60, 100, PIX_SRC, 63);
		pr_vector(screen, 60, 100, 120, 200, PIX_SRC, 63);
		msdelay(200);
	/* test memory vectors */
		mpr1 = mem_create(640, 480, 8);
		for (z = 0; z < 16; z++)
		for (x = -1; x<2; x++)
		for (y = -1; y<2; y++)
		if (x != 0 || y != 0)
			pr_vector(mpr1, 10+z*z*2+x,10+z*z*2+y,
			    10+z*z*2+x*z,10+z*z*2+y*z, z<<1, z*16);
	/* test 3 pixel vector */
		pr_vector(mpr1, 402, 400, 404, 401, PIX_SRC, 127);
	/* test 4 pixel vector */
		pr_vector(mpr1, 412, 400, 415, 401, PIX_SRC, 127);
		pr_vector(mpr1, 0, 0, 100, 60, PIX_SRC, 63);
		pr_vector(mpr1, 100, 60, 200, 120, PIX_SRC, 63);
		pr_vector(mpr1, 0, 0, 60, 100, PIX_SRC, 63);
		pr_vector(mpr1, 60, 100, 120, 200, PIX_SRC, 63);
		pr_rop(screen, 1, 1, 640, 480, PIX_SRC, mpr1, 0, 0);
		break;

/* 'x' - Xor hidden-window test - xor-slide text over text */
	case 'x':
		putalphabet(screen);
				/* central text */
		pr_rop(screen,250,150,26*8,13*12,PIX_SRC,screen,0,0);
				/*clr top left */
		pr_rop(screen,0,0,26*8,15*12,PIX_SRC,screen,0,200);
				/*save screen*/
		pr_rop(mpr, 0, 0, 640, 480, PIX_SRC, screen, 0, 0);
				/*sliding text*/
		pr_rop(screen,112,0,10*8,6*12,PIX_SRC,screen,250,150);
		pr_rop(screen,250,150,10*8,6*12,PIX_SRC^PIX_DST,screen,112,0);
		pr_rop(screen,400,300,10*8,6*12,PIX_SRC^PIX_DST,screen,112,0);
		msdelay(100);
		break;
	}
}

outback(screen, mpr1)
	struct pixrect *screen, *mpr1;
{
	int i;

	strcpy(mpr_d(mpr1)->md_image, "Data correctly read from source");
	pr_rop(screen, 16 , 16, 32, 1, PIX_SRC, mpr1, 0, 0);
	pr_rop(screen, 16 , 17,  3, 1, PIX_SRC, mpr1, 0, 1);
	pr_rop(screen, 19 , 17, 29, 1, PIX_SRC, mpr1, 3, 1);
	pr_rop(screen, 16 , 18,  3, 2, PIX_SRC, mpr1, 0, 2);
	pr_rop(screen, 19 , 18, 29, 2, PIX_SRC, mpr1, 3, 2);
	pr_rop(screen, 16 , 20,  3, 4, PIX_SRC, mpr1, 0, 4);
	pr_rop(screen, 19 , 20,  3, 4, PIX_SRC, mpr1, 3, 4);
	pr_rop(screen, 22 , 20, 26, 4, PIX_SRC, mpr1, 6, 4);
	pr_rop(screen, 48 , 16, 32, 8, PIX_SRC, mpr1, 0, 0);
	strcpy(mpr_d(mpr1)->md_image, "readback had no effect at all. ");
	pr_rop(mpr1, 0, 0, 2, 4, PIX_SRC, screen, 16, 16);
	pr_rop(mpr1, 2, 0,30, 4, PIX_SRC, screen, 18, 16);
	pr_rop(mpr1, 0, 4, 2, 4, PIX_SRC, screen, 16, 20);
	pr_rop(mpr1, 2, 4, 3, 4, PIX_SRC, screen, 18, 20);
	pr_rop(mpr1, 5, 4,27, 4, PIX_SRC, screen, 21, 20);
	for (i = 0; i < 32; i++)
		printf("%c", 
		   ((char *)mpr_d(mpr1)->md_image)[i]);
	printf("\n");
}

msdelay(ms)
	register int ms;
{

	ms *= 1000;
	while (--ms >= 0)
		;
}

putalphabet(scr)
	struct	pixrect  *scr;
{
	pf = pf_open(0);
	clearscreen();
	op = PIX_COLOR(255) | PIX_SRC;
	pf_text(scr,0,12,op,pf, "abcdefghijklmnopqrstuvwxyz");
	pf_text(scr,0,24,op,pf, "bcdefghijklmnopqrstuvwxyza");
	pf_text(scr,0,36,op,pf, "cdefghijklmnopqrstuvwxyzab");
	pf_text(scr,0,48,op,pf, "defghijklmnopqrstuvwxyzabc");
	pf_text(scr,0,60,op,pf, "efghijklmnopqrstuvwxyzabcd");
	pf_text(scr,0,72,op,pf, "fghijklmnopqrstuvwxyzabcde");
	pf_text(scr,0,84,op,pf, "ghijklmnopqrstuvwxyzabcdef");
	pf_text(scr,0,96,op,pf, "hijklmnopqrstuvwxyzabcdefg");
	pf_text(scr,0,108,op,pf, "ijklmnopqrstuvwxyzabcdefgh");
	pf_text(scr,0,120,op,pf, "jklmnopqrstuvwxyzabcdefghi");
	pf_text(scr,0,132,op,pf, "klmnopqrstuvwxyzabcdefghij");
	pf_text(scr,0,144,op,pf, "lmnopqrstuvwxyzabcdefghijk");
	pf_text(scr,0,156,op,pf, "mnopqrstuvwxyzabcdefghijkl");
}

clearscreen()
{
	pr_rop(screen,0,0,1152,900,PIX_SRC,0,0,0);
}
