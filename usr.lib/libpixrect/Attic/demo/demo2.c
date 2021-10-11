#ifndef lint
static	char sccsid[] = "@(#)demo2.c 1.1 94/10/31 Copyr 1983 Sun Micro";
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
 * Cases as in usage below:
 */

char usage[] = "\
a	test stencil with various src and dst\n\
b	batchrop sample text to screen\n\
B	like b but properly clipped\n\
c	cursor slides diagonally down\n\
C	clipping test on screen\n\
m	move test - central image l/r/u/d in region\n\
g	get/put - same effect as t\n\
h	hidden-window test - slide one window over another\n\
p	polygon test - draw and erase a 36-agon (vector test)\n\
P	polygon test, clipped\n\
r	replicate test\n\
R	rot test\n\
s	set test - set a foreground-color rectangle\n\
t	text test - basic read-write test\n\
v	vectors - display growing-asterisk pattern\n\
w	window boundary - full 1152x900 window\n\
x	xor test - xor characters (a) with self (b) with center\n";

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/bw2var.h>
#include <pixrect/pixfont.h>

#define RADIUS 350
#define AGON 36
#define XCENT 512
#define YCENT 400
#define BOX1X 0
#define BOX1Y 0
#define BOX2X 752
#define BOX2Y 0
#define BOX3X 400
#define BOX3Y 0
#define BOX4X 400
#define BOX4Y 600
#define BOX5X 400
#define BOX5Y 450
#define BOX6X 0
#define BOX6Y 0
#define BOX7X 0
#define BOX7Y 600
#define BOX8X 0
#define BOX8Y 450
#define BOX9X 752
#define BOX9Y 450
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

int xor();
int cos, sin, cx[AGON], cy[AGON], x, y, i, j;

char polymsg[] = "  CLIPPED 36-AGON  ";

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
	int i, x, y, z;
	struct pixrect *mpr1, *box1, *box2, *box3, *box4, *box5,
			*box6, *box7, *box8, *box9;
	char *c;
	int op;
	int times[16];

	if (argc == 1) {
		printf("Usage: %s x\nwhere x is any string from:\n%s",
			     argv[0], 				usage);
		exit(1);
	}

	/*
	 * Open screen pixrect.
	 * SHOULD BE ABLE TO JUST OPEN A GENERIC, ALA /dev/console...
	 */
	screen = (struct pixrect *)pr_open("/dev/fb");
	if (screen == 0) {
		fprintf(stderr, "Can't open frame buffer (/dev/fb)\n");
		exit(1);
	}
	if (argv[0][0] == 'r')
		pr_reversevideo( screen);

	/*
	 * Create and fill memory pixrect.
	 */
	mpr = mem_create(BW2SIZEX, BW2SIZEY, 1);
	if (mpr == 0) {
		fprintf(stderr, "Can't allocate memory pixrect\n");
		exit(1);
	}

	if (!strcmp(argv[1], "all"))
		argv[1] =
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Boxes 1-5 partition the screen for clipping tests as follows:
________________________________________________________________
|			|     box3	|			|
|			|    352x450	|			|
|			|---------------|			|
|	box1		|     box5	|	box2		|
|      400x900		|    352x150	|     400x900		|
|			|---------------|			|
|			|     box4	|			|
|			|    352x300	|			|
|_______________________|_______________|_______________________|
Boxes 5-9 partition the screen as follows:
________________________________________________________________
|			      box6	 			|
|			    BW2SIZEXx450	 			|
|---------------------------------------------------------------|
|	box8		|     box5	|	box9		|
|     400x150		|    352x150	|     400x150		|
|---------------------------------------------------------------|
|			      box7	 			|
|			    BW2SIZEXx300	 			|
________________________________________________________________
*/

	box1 = mem_region(screen, BOX1X, BOX1Y, 400, BW2SIZEY);
	box2 = mem_region(screen, BOX2X, BOX2Y, 400, BW2SIZEY);
	box3 = mem_region(screen, BOX3X, BOX3Y, 352, 450);
	box4 = mem_region(screen, BOX4X, BOX4Y, 352, 300);
	box5 = mem_region(screen, BOX5X, BOX5Y, 352, 150);
	box6 = mem_region(screen, BOX6X, BOX6Y,BW2SIZEX, 450);
	box7 = mem_region(screen, BOX7X, BOX7Y,BW2SIZEX, 300);
	box8 = mem_region(screen, BOX8X, BOX8Y, 400, 150);
	box9 = mem_region(screen, BOX9X, BOX9Y, 400, 150);

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
	case 'b': 
		{
#include <sys/time.h>
#include <sys/resource.h>
		PIXFONT *pf;
		struct rusage ru, ru2;
		char buf[200];

		if ((pf = pf_open("/usr/local/lib/stdfont")) == 0)
			pf = pf_open(0);
		for (op = 0; op < 32; op += 2) {
			pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY,
				PIX_CLR, 0, 0, 0);
			sprintf(buf, "%2x: %s", op, 
	"Now is the time for all good men to come to the aid of their party.");
			getrusage(0,&ru);
			for (x = 0; x < 5; x++) {
				int j;
				for (j = 30; j < 780; j += 20)
					pf_text(screen, 0, j, clip|op, pf, buf);
			}
			getrusage(0,&ru2);
			printf("\033[H\f\n\n");
			times[op>>1] = 
			   ((ru2.ru_utime.tv_sec-ru.ru_utime.tv_sec)*1000000
			      + (ru2.ru_utime.tv_usec-ru.ru_utime.tv_usec))
			   /(((780-30)/20)*67*5);
			printf("%d microseconds per character for op %x\n", 
				times[op>>1], op);
			msdelay(300);
		}
		clip = PIX_DONTCLIP;
		}
		for (op = 0; op < 16; op++) {
			printf("%6d usec per char for op %2x\n",
				times[op], op);
		}
		break;

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
		pr_rop(screen, 350, 250, 26*12+1, 13*22,
		    PIX_SRC, screen, 16, 16);		/* central text */
		pr_rop(screen, 16, 16, 26*12+1, 15*22,
		    PIX_SRC, screen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, BW2SIZEX, BW2SIZEY,
		    PIX_SRC, screen, 0, 0);    		/* save screen */
		pr_rop(screen, -100, 200, 26*12+1, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(mpr, 350, 250, 26*12+1, 13*22,
		    PIX_SRC, screen, -100, 200);
		pr_rop(screen, 900, 300, 26*12+1, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(screen, 300, -100, 26*12+1, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(screen, 400, 700, 26*12+1, 13*22,
		    PIX_SRC, mpr, 350, 250);
		for (x = 0; x >= -26*12+1; x--) {
			pr_rop(screen, x, 200, 26*12+1, 13*22,
			    PIX_SRC, mpr, 350, 250);
			pr_rop(mpr, 350, 250, 26*12+1, 13*22,
			    PIX_SRC, screen, x, 200);
		}
		mpr1 = mem_create(16, 16, 1);
		pr_rop(mpr1, 0, 0, 16, 16,
		    PIX_SRC, screen, 350, 250);		/* save screen */
		for (x = 330; x >= -26*12+1; x--) {
			pr_rop(screen, x, 300, 16, 16,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 16,
			    PIX_SRC, screen, x, 300);
		}
		pr_rop(mpr1, 0, 0, 16, 16,
		    PIX_SRC, screen, 380, 300);
		for (x = 330; x >= -26*12+1; x--)
			pr_rop(screen, x, 300, 16, 16,
			    PIX_SRC, mpr1, 0, 0);
		mpr1 = mem_create(16,12,1);
		pr_rop(mpr1, 0, 0, 16, 12,
		    PIX_SRC, screen, 350, 250);		/* save screen */
		for (x = 330; x >= -26*12+1; x--) {
			pr_rop(screen, x, 300, 16, 12,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 12,
			    PIX_SRC, screen, x, 300);
		}
		msdelay(300);
		break;

/* 'm' - Move test - scroll text l/r/u/d in region */
	case 'm': {
		int	X = 350, Y = 250, W = 26*12, H = 13*22, D = 16;

		clearscreen();
		mpr1 = pr_region(screen, X, Y, W+32, H+32);
		pr_rop(mpr1, 16, 16, W, H,
		    PIX_SET, 0, 0, 0);			/* central image */
		pr_rop(mpr1, 16, 16-D, W, H,
		    PIX_SRC, mpr1, 16, 16);		/* scroll up */
		pr_rop(mpr1, 16, 16+D, W, H,
		    PIX_SRC, mpr1, 16, 16);		/* scroll down */
		pr_rop(mpr1, 16-D, 16, W, H,
		    PIX_SRC, mpr1, 16, 16);		/* scroll left */
		pr_rop(mpr1, 16+D, 16, W, H,
		    PIX_SRC, mpr1, 16, 16);		/* scroll right */
		msdelay(300);
		break;
		}

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
		pr_rop(screen, 350, 250, 26*12+1, 13*22,
		    PIX_SRC, screen, 16, 16);		/* central text */
		pr_rop(screen, 16, 16, 26*12+1, 15*22,
		    PIX_SRC, screen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, BW2SIZEX, BW2SIZEY,
		    PIX_SRC, screen, 0, 0);		/* save screen */
		pr_rop(screen, 112, 0, 10*12, 6*22,
		    PIX_SRC, screen, 350, 250);		/* sliding text */
		for (x = 112, y = 0; x < BW2SIZEX; x++, y++) {
			pr_rop(screen, x+1, y+1, 10*12, 6*22,
			    PIX_SRC, screen, x, y);
			pr_rop(screen, x, y, 1, 6*22,
			    PIX_SRC, mpr, x, y);
			pr_rop(screen, x, y, 10*12, 1,
			    PIX_SRC, mpr, x, y);
		}
		break;

	case 'P':
	clip = 0;
	case 'p':
	cx[0] = RADIUS;
	cy[0] = 0;
	cos = 32270;	/* cos 10 degrees * 32768 */
	sin = 5690;	/* sin 10 degrees * 32768 */
	for (i = 1; i < AGON; i++) 
	  {
		x = cx[i-1]*cos - cy[i-1]*sin + 16384;
		y = cx[i-1]*sin + cy[i-1]*cos + 16384;
		cx[i] = x>>15;
		cy[i] = y>>15;
	  }
	for (i = 0; i < AGON; i++) 
	  {
		cx[i] = cx[i] + XCENT;
		cy[i] = cy[i] + YCENT;
	  }
	pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0);
	if (clip)
		for (x = 0; x < 11; x++)
			for (i = 0; i < AGON; i++)
				for (j = i+1; j < AGON; j++)
					pr_vector(box1, cx[i], cy[i],
							cx[j], cy[j],
						  clip|PIX_NOT(PIX_DST), 0);
	else {
	    printf("\033[%d;%dH%s\n",(525-16)/22+1, (400-16)/12+1, polymsg);
	    for (x = 0; x < 10000; x++);	/* pause to let output settle */
	    pr_rop(box1, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0); /* flush cursor */
	    for (x = 0; ; x++) {
		for (i = 0; i < AGON; i++)
			for (j = i+1; j < AGON; j++) {
				pr_vector(box1, cx[i]-BOX1X, cy[i]-BOX1Y,
						cx[j]-BOX1X, cy[j]-BOX1Y,
						clip|PIX_NOT(PIX_DST), 0);
				pr_vector(box2, cx[i]-BOX2X, cy[i]-BOX2Y,
						cx[j]-BOX2X, cy[j]-BOX2Y,
						clip|PIX_NOT(PIX_DST), 0);
				pr_vector(box3, cx[i]-BOX3X, cy[i]-BOX3Y,
						cx[j]-BOX3X, cy[j]-BOX3Y,
						clip|PIX_NOT(PIX_DST), 0);
				pr_vector(box4, cx[i]-BOX4X, cy[i]-BOX4Y,
						cx[j]-BOX4X, cy[j]-BOX4Y,
						clip|PIX_NOT(PIX_DST), 0);
			}
		if (x == 11) 
			break;
		for (i = 0; i < AGON; i++)
			for (j = i+1; j < AGON; j++) {
				pr_vector(box6, cx[i]-BOX6X, cy[i]-BOX6Y,
						cx[j]-BOX6X, cy[j]-BOX6Y,
						clip|PIX_NOT(PIX_DST), 0);
				pr_vector(box7, cx[i]-BOX7X, cy[i]-BOX7Y,
						cx[j]-BOX7X, cy[j]-BOX7Y,
						clip|PIX_NOT(PIX_DST), 0);
				pr_vector(box8, cx[i]-BOX8X, cy[i]-BOX8Y,
						cx[j]-BOX8X, cy[j]-BOX8Y,
						clip|PIX_NOT(PIX_DST), 0);
				pr_vector(box9, cx[i]-BOX9X, cy[i]-BOX9Y,
						cx[j]-BOX9X, cy[j]-BOX9Y,
						clip|PIX_NOT(PIX_DST), 0);
			}
	    }
	}
	for (x = 0; x < 30000; x++);	/* pause after demo */
	clip = PIX_DONTCLIP;
	break;

/* 'r' - replrop test */
	case 'r':
		pr_replrop(screen, 18, 18, BW2SIZEX-36, BW2SIZEY-36,
		    PIX_SRC, &dtgray, 0, 0);
		msdelay(300);
		pr_replrop(screen, 18, 18, BW2SIZEX-36, BW2SIZEY-36,
		    PIX_SRC^PIX_DST, &dtgray, 0, 0);
		pr_replrop(screen, 18, 18, BW2SIZEX-36, BW2SIZEY-36,
		    PIX_SRC^PIX_DST, &dkgray, 0, 0);
		msdelay(300);
		break;

/* 'R' - Rot test - try dup sun2 rot problem */
	case 'R': {
		int	x, y, xc, yc, w = 8, h = 8, sizex = 30, sizey = 12, d, delay = 10000;

		clearscreen();
		for (x = 0;x < sizex;x++) {
			for (y = 0;y < sizey;y++) {
				mpr1 = pr_region(screen, x, y, 400, 400);
				for (xc = 0;xc < sizex-w;xc++) {
					for (yc = 0;yc < sizey-h;yc++) {
						pr_rop(mpr1, xc, yc, w, h,
						    PIX_NOT(PIX_DST), 0, 0, 0);
						for (d = 0;d < delay;d++) {}
					}
				}
				pr_destroy(mpr1);
			}
		}
		break;
		}

/* 's' - set test - put up a set (foreground) rectangle */
	case 's':
		pr_rop(screen, 300, 200, 200, 107, PIX_SET, 0, 0, 0);
		msdelay(300);
		break;

/* 't' - read test - compare screen with what is read */
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
		pr_vector(screen, 0, 500, 1023, 500, PIX_SRC, 0);
		msdelay(1000);
		break;

	case 'w':
		pr_rop(screen, 0, 0, BW2SIZEX, BW2SIZEY, PIX_CLR, 0, 0, 0);
		pr_vector(screen,0,0,BW2SIZEX-1,0,PIX_SRC,1);
		pr_vector(screen,BW2SIZEX-1,0,BW2SIZEX-1,BW2SIZEY-1,PIX_SRC,1);
		pr_vector(screen,BW2SIZEX-1,BW2SIZEY-1,0,BW2SIZEY-1,PIX_SRC,1);
		pr_vector(screen,0,BW2SIZEY-1,0,0,PIX_SRC,1);
		pr_vector(screen,2,2,BW2SIZEX-3,2,PIX_SRC,1);
		pr_vector(screen,BW2SIZEX-3,2,BW2SIZEX-3,BW2SIZEY-3,PIX_SRC,1);
		pr_vector(screen,BW2SIZEX-3,BW2SIZEY-3,2,BW2SIZEY-3,PIX_SRC,1);
		pr_vector(screen,2,BW2SIZEY-3,2,2,PIX_SRC,1);
		msdelay(1000);
		break;

/* 'x' - Xor hidden-window test - xor-slide text over text */
	case 'x':
		putalphabet();
		pr_rop(screen, 350, 250, 26*12+1, 13*22,
		    PIX_SRC, screen, 16, 16);		/* central text */
		pr_rop(screen, 16, 16, 26*12+1, 15*22,
		    PIX_SRC, screen, 16, 400);		/*clr top left */
		pr_rop(mpr, 0, 0, BW2SIZEX, BW2SIZEY,
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
