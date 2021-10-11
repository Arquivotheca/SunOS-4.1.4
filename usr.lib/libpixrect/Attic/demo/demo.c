#ifndef lint
static	char sccsid[] = "@(#)demo.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Demos for pixrect operations 
 *
 * 	demo hv		demonstrates hidden text and vectors
 *	rdemo hv	ditto, video reversed (in sense of black background)
 *
 * Cases:
 *	a	test stencil with various src and dst
 *	c	cursor test - slide blank cursor diagonally down screen
 *	C	clipping test - sends text beyond all four screen boundaries
 *	g	get/put test - simulates text test using get and put pixel
 *	h	hidden text test - slide text over text
 *	p	point test - display a diagonal line of points
 *	r	replicate test
 *	R	Region batchrop clipping test
 *	s	set test - sets a foreground rectangle (also tests null source)
 *	t	text - write bits of "this is ..." to screen, read, print %x
 *	v	vector test - display asterisks with empty centers
 *	x	xor test - xor text to top left corner of text and to center
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>

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
struct	pixrect *mpr;
struct	pixrect  *screen;
struct	pixrect  *pr_open();

short	dtgrayimage[4] = {
	0x8888,
};
mpr_static(dtgray, 16, 1, 1, dtgrayimage);

short	dkgrayimage[2] = {
	0xaaaa,
};
mpr_static(dkgray, 16, 1, 1, dkgrayimage);

main(argc, argv)
	int argc;
	char **argv;
{
	int clip = PIX_DONTCLIP;
	int i, x, y, z, op;
	struct pixrect *mpr1;
	char *c, *device;

	if (argc == 1) {
		printf("Usage: %s x\n\
where x is any string of letters from:\n\
a	test stencil with various src and dst\n\
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
	 * Open screen pixrect.
	 */
	if (argc == 3)
		device = *(argv+2);
	else
		device = "/dev/fb";
	screen = pr_open(device);
	if (screen == 0) {
		fprintf(stderr, "Can't open frame buffer (%s)\n", device);
		exit(1);
	}

	/*
	 * REVERSE VIDEO HAS BEEN INSTITUTIONALIZED.
	 */
	if (argv[0][0] == 'r')
		pr_reversevideo( screen);

	/*
	 * Create and fill memory pixrect.
	 */
	mpr = mem_create(screen->pr_width+13, screen->pr_height+13, 1);
	if (mpr == 0) {
		fprintf(stderr, "Can't allocate memory pixrect\n");
		exit(1);
	}

	if (!strcmp(argv[1], "all"))
		argv[1] =
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	for (c = argv[1]; *c; c++) switch (*c) {

	case 'a':
		op = PIX_SET;
		for (i=100; i<200; i+=10) {
			pr_stencil(screen,i,i,16,16,op,&cursor,0,0,0,0,0);
		}
		op = PIX_SRC;
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
		mpr1 = mem_create(64, 64, 1);
		pr_stencil(mpr1,0,0,64,64,op,&tree,0,0,screen,5,0);
		pr_rop(screen,250,0,64,64,op,mpr1,0,0);
		break;
	case 'B':
		clip = 0;
/* 'b' - Batchrop test - write gall10 to screen */
	case 'b': {
#include <sys/time.h>
		PIXFONT *pf = pf_open(0);
		struct timeval tv, tv2;
		msdelay(10);
		gettimeofday(&tv, 0);
		for (x = 0; x < 10; x++) {
			int j;
			for (j = 30; j < 780; j += 20)
				pf_text(screen, 0, j,
				    clip|(PIX_SRC^PIX_DST),
				    pf,
"Now is the time for all good men to come to the aid of their party.");
		}
		gettimeofday(&tv2, 0);
		printf("\033[H\f\n\n");
		printf("%d microseconds per character\n", 
		     ((tv2.tv_sec-tv.tv_sec)*1000000 + (tv2.tv_usec-tv.tv_usec))
		     /(((780-30)/20)*(127-' ')*10));
		msdelay(300);
		clip = PIX_DONTCLIP;
		break;
		}

/* 'c' - Cursor test - move blank cursor diagonally down screen */
	case 'c':
		putalphabet();
		for (x = 0, y = 0; x < 700; x++, y++) {
			pr_rop(mpr, 0, 0, 16, 16, PIX_SRC, screen, x, y);
			pr_rop(screen, x, y, 16, 16, PIX_SET, 0, 0, 0);
			pr_rop(screen, x, y, 16, 16, PIX_SRC, mpr, 0, 0);
		}
		msdelay(300);
		break;

/* 'C' - Clipping test - duplicate screen center in four places */
	case 'C':
		putalphabet();
		pr_rop(screen, 350, 250, 26*12, 13*22,
		    PIX_SRC, screen, 16, 16);		/* central text */
		pr_rop(screen, 16, 16, 26*12, 15*22,
		    PIX_SRC, screen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, screen->pr_width, screen->pr_height,
		    PIX_SRC, screen, 0, 0);    		/* save screen */
		pr_rop(screen, -100, 200, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(mpr, 350, 250, 26*12, 13*22,
		    PIX_SRC, screen, -100, 200);
		pr_rop(screen, 900, 300, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(screen, 300, -100, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(screen, 400, 700, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		for (x = 0; x >= -26*12; x--) {
			pr_rop(screen, x, 200, 26*12, 13*22,
			    PIX_SRC, mpr, 350, 250);
			pr_rop(mpr, 350, 250, 26*12, 13*22,
			    PIX_SRC, screen, x, 200);
		}
		mpr1 = mem_create(16, 16, 1);
		pr_rop(mpr1, 0, 0, 16, 16,
		    PIX_SRC, screen, 350, 250);		/* save screen */
		for (x = 330; x >= -26*12; x--) {
			pr_rop(screen, x, 300, 16, 16,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 16,
			    PIX_SRC, screen, x, 300);
		}
		pr_rop(mpr1, 0, 0, 16, 16,
		    PIX_SRC, screen, 380, 300);
		for (x = 330; x >= -26*12; x--)
			pr_rop(screen, x, 300, 16, 16,
			    PIX_SRC, mpr1, 0, 0);
		mpr1 = mem_create(16,12,1);
		pr_rop(mpr1, 0, 0, 16, 12,
		    PIX_SRC, screen, 350, 250);		/* save screen */
		for (x = 330; x >= -26*12; x--) {
			pr_rop(screen, x, 300, 16, 12,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 12,
			    PIX_SRC, screen, x, 300);
		}
		msdelay(300);
		break;

/* 'g' - Get/put test - simulates text test */
	case 'g':
		printf("\033[H\f\n\n");
		msdelay(10);
		mpr1 = mem_create(32, 8, 1);
		strcpy(mpr_d(mpr1)->md_image,
		    "Data correctly read from source");
		for (x = 0; x < 32; x++)
		for (y = 0; y < 8; y++)
			pr_put(screen, 16+x, 16+y, pr_get(mpr1, x, y));
		strcpy(mpr_d(mpr1)->md_image,
		    "get had no effect at all. ");
		for (x = 0; x < 32; x++)
		for (y = 0; y < 8; y++)
			pr_put(mpr1, x, y, pr_get(screen, 16+x, 16+y));
		for (i = 0; i < 32; i++)
		printf("%c", 
		   ((char *)mpr_d(mpr1)->md_image)[i]);
		printf("\n");
		msdelay(300);
		break;

/* 'h' - Hidden-window test - slide text over text */
	case 'h':
		putalphabet();
		pr_rop(screen, 350, 250, 26*12, 13*22,
		    PIX_SRC, screen, 16, 16);		/* central text */
		pr_rop(screen, 16, 16, 26*12, 15*22,
		    PIX_SRC, screen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, screen->pr_width, screen->pr_height,
		    PIX_SRC, screen, 0, 0);		/* save screen */
		pr_rop(screen, 112, 0, 10*12, 6*22,
		    PIX_SRC, screen, 350, 250);		/* sliding text */
		for (x = 112, y = 0; x < screen->pr_width; x++, y++) {
			pr_rop(screen, x+1, y+1, 10*12, 6*22,
			    PIX_SRC, screen, x, y);
			pr_rop(screen, x, y, 1, 6*22,
			    PIX_SRC, mpr, x, y);
			pr_rop(screen, x, y, 10*12, 1,
			    PIX_SRC, mpr, x, y);
		}
		break;

/* 'r' - replrop test */
	case 'r':
		pr_replrop(screen, 18, 18, screen->pr_width-36, 100-36,
		    PIX_SRC, &dtgray, 0, 0);
		msdelay(300);
		pr_replrop(screen, 18, 18, screen->pr_width-36, 100,
		    PIX_SRC^PIX_DST, &dtgray, 0, 0);
		pr_replrop(screen, 18, 18, screen->pr_width-36, 100,
		    PIX_SRC^PIX_DST, &dkgray, 0, 0);
		msdelay(300);
		break;

	case 'R':
/* 'R' - Region batchrop clipping test - write gall10 to screen */
		{ int	OFFSET = 5, BASELINE = 18, HEIGHT = BASELINE-OFFSET;
		PIXFONT *pf = pf_open(0);
		struct pixrect *region = pr_region(screen,
		    0, OFFSET, 80, HEIGHT);

		pf_text(screen, BASELINE, BASELINE,
		    PIX_DONTCLIP|(PIX_SRC^PIX_DST), pf, "abcg");
		pf_text(region, BASELINE, BASELINE-OFFSET,
		    (PIX_SRC^PIX_DST), pf, "abcg");
		pr_destroy(region);
		break;
		}

/* 's' - set test - put up a set (foreground) rectangle */
	case 's':
		pr_rop(screen, 300, 200, 200, 107, PIX_SET, 0, 0, 0);
		msdelay(300);
		break;

/* 'r' - read test - compare screen with what is read */
	case 't':
		printf("\033[H\f\n\n");
		msdelay(10);
		outback(screen, (struct pixrect *)mem_create(32,8,1));
		outback(screen, (struct pixrect *)mem_create(16,16,1));
		msdelay(300);
		break;

/* 'v' - vector test - display asterisks each with an empty center */
	case 'v':
		printf("\033[H\f\n");
		msdelay(10);
		for (z = 0; z < 16; z++)
		for (x = -1; x<2; x++)
		for (y = -1; y<2; y++)
		if (x != 0 || y != 0)
			pr_vector(screen, 100+z*z*2+x,100+z*z*2+y,
			    100+z*z*2+x*z,100+z*z*2+y*z, PIX_SRC, 1);
	/* test 3 pixel vector */
		pr_vector(screen, 402, 400, 404, 401, PIX_SRC, 1);
	/* test 4 pixel vector */
		pr_vector(screen, 412, 400, 415, 401, PIX_SRC, 1);
	/* test other color */
		pr_vector(screen, 0, 500, screen->pr_width-1, 500, PIX_SRC, 0);
		msdelay(1000);
		break;

/* 'x' - Xor hidden-window test - xor-slide text over text */
	case 'x':
		putalphabet();
		pr_rop(screen, 350, 250, 26*12, 13*22,
		    PIX_SRC, screen, 16, 16);		/* central text */
		pr_rop(screen, 16, 16, 26*12, 15*22,
		    PIX_SRC, screen, 16, 400);		/*clr top left */
		pr_rop(mpr, 0, 0, screen->pr_width, screen->pr_height,
		    PIX_SRC, screen, 0, 0);
		pr_rop(screen, 112, 0, 10*12, 6*22,
		    PIX_SRC, screen, 350, 250);		/*sliding text*/
		pr_rop(screen, 350, 250, 10*12, 6*22,
		    PIX_SRC^PIX_DST, screen, 112, 0);
		pr_rop(screen, 400, 300, 10*12, 6*22,
		    PIX_SRC^PIX_DST, screen, 112, 0);
		msdelay(1000);
		break;

	}
}

#ifdef notdef
xor(a, i, j, b)
	int a, i, j, b;
{

	return (a ^ b);
}
#endif

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

putalphabet()
{

	clearscreen();
	printf("%s",
"abcdefghijklmnopqrstuvwxyz\n\
bcdefghijklmnopqrstuvwxyza\n\
cdefghijklmnopqrstuvwxyzab\n\
defghijklmnopqrstuvwxyzabc\n\
efghijklmnopqrstuvwxyzabcd\n\
fghijklmnopqrstuvwxyzabcde\n\
ghijklmnopqrstuvwxyzabcdef\n\
hijklmnopqrstuvwxyzabcdefg\n\
ijklmnopqrstuvwxyzabcdefgh\n\
jklmnopqrstuvwxyzabcdefghi\n\
klmnopqrstuvwxyzabcdefghij\n\
lmnopqrstuvwxyzabcdefghijk\n\
mnopqrstuvwxyzabcdefghijkl\n");
	msdelay(50);
}

clearscreen()
{

	printf("\033[H\f");
}
