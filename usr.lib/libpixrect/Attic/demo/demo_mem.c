#ifndef lint
static	char sccsid[] = "@(#)demo_mem.c 1.1 94/10/31 Copyr 1983 Sun Micro";
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

#ifdef notdef
a	aop test - same effect as xor test\n\
p	polygon test - draw and erase a 36-agon (vector test)\n\
P	polygon test, clipped\n\
v	vectors - display growing-asterisk pattern\n\

#endif
char usage[] = "\
b	batchrop sample text to screen\n\
B	like b but properly clipped\n\
c	cursor slides diagonally down\n\
C	clipping test on screen\n\
g	get/put - same effect as t\n\
h	hidden-window test - slide one window over another\n\
o	operation test - draw tables for all 16 operations\n\
r	replicate test\n\
s	set test - set a foreground-color rectangle\n\
t	text test - basic read-write test\n\
x	xor test - xor characters (a) with self (b) with center\n";

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>

extern mem_nobatchrop;

#define RADIUS 350
#define AGON 36
#define XCENT 512
#define YCENT 400
#define BOX1X 0
#define BOX1Y 0
#define BOX2X 624
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
#define BOX9X 624
#define BOX9Y 450

struct	pixrect *mpr;
struct	pixrect  *screen;
struct	pixrect  *mscreen;
PIXFONT *pf;

int xor();
int cos, sin, cx[AGON], cy[AGON], x, y, i, j;

char polymsg[] = "  CLIPPED 36-AGON  ";

short	cursorimage[12] = {
	0xa0a0, 0x4040, 0xa0a0, 0x0000, 0x0a0a, 0x0404,
	0x0a0a, 0x0000, 0xa0a0, 0x4040, 0xa0a0, 0x0000,
};
mpr_static(cursor, 12, 12, 1, cursorimage);

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

	if (argc == 1) {
		printf("Usage: %s x\nwhere x is any string from:\n%s",
			     argv[0], 				usage);
		exit(1);
	}

	/*
	 * Create and fill memory pixrect.
	 */
	mpr = mem_create(1057, 813, 1);
	if (mpr == 0) {
		fprintf(stderr, "Can't create mpr\n");
		exit(1);
	}
	mscreen = mem_create(1024, 800, 1);
	if (mscreen == 0) {
		fprintf(stderr, "Can't create mscreen\n");
		exit(1);
	}

	/*
	 * Open screen pixrect.
	 * SHOULD BE ABLE TO JUST OPEN A GENERIC, ALA /dev/console...
	 */
	screen = bw1_create(1024, 800, 1);
	if (screen == 0) {
		fprintf(stderr, "Can't open frame buffer (/dev/console)\n");
		exit(1);
	}

	/*
	 * REVERSE VIDEO SHOULD BE MORE INSTITUTIONALIZED.
	 */
	if (argv[0][0] == 'r')
		bw1_d(screen)->bwpr_flags &= ~BW_REVERSEVIDEO;

	if (!strcmp(argv[1], "all"))
		argv[1] =
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Boxes 1-5 partition the screen for clipping tests as follows:
________________________________________________________________
|			|     box3	|			|
|			|    224x450	|			|
|			|---------------|			|
|	box1		|     box5	|	box2		|
|      400x800		|    224x150	|     400x800		|
|			|---------------|			|
|			|     box4	|			|
|			|    224x200	|			|
|_______________________|_______________|_______________________|
Boxes 5-9 partition the screen as follows:
________________________________________________________________
|			      box6	 			|
|			    1024x450	 			|
|---------------------------------------------------------------|
|	box8		|     box5	|	box9		|
|     400x150		|    224x150	|     400x150		|
|---------------------------------------------------------------|
|			      box7	 			|
|			    1024x200	 			|
________________________________________________________________
*/

	box1 = pr_region(mscreen, BOX1X, BOX1Y, 400, 800);
	box2 = pr_region(mscreen, BOX2X, BOX2Y, 400, 800);
	box3 = pr_region(mscreen, BOX3X, BOX3Y, 224, 450);
	box4 = pr_region(mscreen, BOX4X, BOX4Y, 224, 200);
	box5 = pr_region(mscreen, BOX5X, BOX5Y, 224, 150);
	box6 = pr_region(mscreen, BOX6X, BOX6Y,1024, 450);
	box7 = pr_region(mscreen, BOX7X, BOX7Y,1024, 200);
	box8 = pr_region(mscreen, BOX8X, BOX8Y, 400, 150);
	box9 = pr_region(mscreen, BOX9X, BOX9Y, 400, 150);

	pf = pf_open(0);

	for (c = argv[1]; *c; c++) switch (*c) {

#ifdef notdef
/* 'a' - aop test - same effect as xor test */
	case 'a': {
		extern xor();

		putalphabet();
		pr_rop(mscreen, 350, 250, 26*12, 13*22,
		    PIX_SRC, mscreen, 16, 16);		/* central text */
		pr_rop(mscreen, 16, 16, 26*12, 15*22,
		    PIX_SRC, mscreen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, 1024, 800,
		    PIX_SRC, mscreen, 0, 0);      	/* save mscreen */
		pr_rop(mscreen, 112, 0, 10*12, 6*22,
		    PIX_SRC, mscreen, 350, 250);		/* sliding text */
		pr_aop(mscreen, 350, 250, 10*12, 6*22,
		    xor, mscreen, 112, 0);
		pr_aop(mscreen, 400, 300, 10*12, 6*22,
		    xor, mscreen, 112, 0);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		msdelay(1000);
		break;
	}
#endif
	
	case 'B':
		clip = 0;
/* 'b' - Batchrop test - write gall10 to mscreen */
	case 'b': {
#include <sys/time.h>
		PIXFONT *pf = pf_open(0);
		struct timeval tv, tv2;
		msdelay(10);
		gettimeofday(&tv, 0);
		for (x = 0; x < 10; x++) {
			int j;
			for (j = 30; j < 780; j += 20)
				pf_text(mscreen, 0, j,
				    clip|(PIX_SRC^PIX_DST),
				    pf,
"Now is the time for all good men to come to the aid of their party.");
			pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		}
		gettimeofday(&tv2, 0);
		printf("\033[H\f\n\n");
		printf("%d microseconds per character\n", 
		     ((tv2.tv_sec-tv.tv_sec)*1000000 + (tv2.tv_usec-tv.tv_usec))
		     /(((780-30)/20)*67*10));
		msdelay(300);
		clip = PIX_DONTCLIP;
		break;
		}

/* 'c' - Cursor test - move blank cursor diagonally down mscreen */
	case 'c':
		putalphabet();
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		for (x = 0, y = 0; x < 700; x++, y++) {
			pr_rop(mpr, 0, 0, 16, 16, PIX_SRC, mscreen, x, y);
			pr_rop(mscreen, x, y, 16, 16, PIX_SET, 0, 0, 0);
			pr_rop(screen, x, y, 16, 16, PIX_SRC, mscreen, x, y);
			pr_rop(mscreen, x, y, 16, 16, PIX_SRC, mpr, 0, 0);
			pr_rop(screen, x, y, 16, 16, PIX_SRC, mscreen, x, y);
		}
		msdelay(300);
		break;

/* 'C' - Clipping test - duplicate mscreen center in four places */
	case 'C':
		putalphabet();
		pr_rop(mscreen, 350, 250, 26*12, 13*22,
		    PIX_SRC, mscreen, 16, 16);		/* central text */
		pr_rop(mscreen, 16, 16, 26*12, 15*22,
		    PIX_SRC, mscreen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, 1024, 800,
		    PIX_SRC, mscreen, 0, 0);   		/* save mscreen */
		pr_rop(mscreen, -100, 200, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(mpr, 350, 250, 26*12, 13*22,
		    PIX_SRC, mscreen, -100, 200);
		pr_rop(mscreen, 900, 300, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(mscreen, 300, -100, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(mscreen, 400, 700, 26*12, 13*22,
		    PIX_SRC, mpr, 350, 250);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		for (x = 0; x >= -26*12; x--) {
			pr_rop(mscreen, x, 200, 26*12, 13*22,
			    PIX_SRC, mpr, 350, 250);
			pr_rop(mpr, 350, 250, 26*12, 13*22,
			    PIX_SRC, mscreen, x, 200);
			pr_rop(screen, x, 200, 26*12, 13*22,
			    PIX_SRC, mscreen, x, 200);
		}
		mpr1 = mem_create(16, 16, 1);
		pr_rop(mpr1, 0, 0, 16, 16,
		    PIX_SRC, mscreen, 350, 250);	/* save mscreen */

		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);

		for (x = 330; x >= -26*12; x--) {
			pr_rop(mscreen, x, 300, 16, 16,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(mpr1, 0, 0, 16, 16,
			    PIX_SRC, mscreen, x, 300);
			pr_rop(screen, x, 300, 16, 16,
			    PIX_SRC, mscreen, x, 300);
		}
		pr_rop(mpr1, 0, 0, 16, 16,
		    PIX_SRC, mscreen, 380, 300);
		for (x = 330; x >= -26*12; x--) {
			pr_rop(mscreen, x, 300, 16, 16,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(screen, x, 300, 16, 16,
			    PIX_SRC, mscreen, x, 300);
		}
		mpr1 = mem_create(16,12,1);
		pr_rop(mpr1, 0, 0, 16, 12,
		    PIX_SRC, mscreen, 350, 250);		/* save mscreen */
		for (x = 330; x >= -26*12; x--) {
			pr_rop(mscreen, x, 300, 16, 12,
			    PIX_SRC, mpr1, 0, 0);
			pr_rop(screen, x, 300, 16, 12,
			    PIX_SRC, mscreen, x, 300);
			pr_rop(mpr1, 0, 0, 16, 12,
			    PIX_SRC, mscreen, x, 300);
		}
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
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
			pr_put(mscreen, 16+x, 16+y, pr_get(mpr1, x, y));
		strcpy(mpr_d(mpr1)->md_image,
		    "get had no effect at all. ");
		for (x = 0; x < 32; x++)
		for (y = 0; y < 8; y++)
			pr_put(mpr1, x, y, pr_get(mscreen, 16+x, 16+y));
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		for (i = 0; i < 32; i++)
		printf("%c", 
		   ((char *)mpr_d(mpr1)->md_image)[i]);
		printf("\n");
		msdelay(300);
		break;

/* 'h' - Hidden-window test - slide text over text */
	case 'h':
		putalphabet();
		pr_rop(mscreen, 350, 250, 26*12, 13*22,
		    PIX_SRC, mscreen, 16, 16);		/* central text */
		pr_rop(mscreen, 16, 16, 26*12, 15*22,
		    PIX_SRC, mscreen, 16, 400);		/* clr top left */
		pr_rop(mpr, 0, 0, 1024, 800,
		    PIX_SRC, mscreen, 0, 0);		/* save mscreen */
		pr_rop(mscreen, 112, 0, 10*12, 6*22,
		    PIX_SRC, mscreen, 350, 250);	/* sliding text */
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		for (x = 112, y = 0; x < 1024; x++, y++) {
			pr_rop(mscreen, x+1, y+1, 10*12, 6*22,
			    PIX_SRC, mscreen, x, y);
			pr_rop(mscreen, x, y, 1, 6*22,
			    PIX_SRC, mpr, x, y);
			pr_rop(mscreen, x, y, 10*12, 1,
			    PIX_SRC, mpr, x, y);
			pr_rop(screen, x, y, 10*12+1, 6*22+1, PIX_SRC,
				mscreen, x, y);
		}
		break;

	case 'o':
		{
			struct pixrect *srcpr, *dstpr;
			pr_rop(mscreen, 0, 0, 1024, 800, PIX_CLR, 0, 0, 0);
			srcpr = mem_create(131, 100, 1);
			dstpr = mem_create(131, 100, 1);
			pr_rop(srcpr, 0, 53, 131, 47, PIX_SET, 0, 0, 0);
			pr_rop(dstpr, 65, 0, 66, 100, PIX_SET, 0, 0, 0);
			for (x = 0; x < 4; x++)
				for (y = 0; y < 4; y++) {
					pr_rop(mscreen, x*256, y*200, 131, 100,
						PIX_SRC, dstpr, 0, 0);
					pr_rop(mscreen, x*256, y*200, 131, 100,
						(y*4+x)*2, srcpr, 0, 0);
			}
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		msdelay(1000);
		break;
		}

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
	pr_rop(mscreen, 0, 0, 1024, 800, PIX_CLR, 0, 0, 0);
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
	    pr_rop(box1, 0, 0, 1024, 800, PIX_CLR, 0, 0, 0); /* flush cursor */
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
		pr_replrop(mscreen, 18, 18, 1024-36, 800-36,
		    PIX_SRC, &dtgray, 0, 0);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		msdelay(300);
		pr_replrop(mscreen, 18, 18, 1024-36, 800-36,
		    PIX_SRC^PIX_DST, &dtgray, 0, 0);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		pr_replrop(mscreen, 18, 18, 1024-36, 800-36,
		    PIX_SRC^PIX_DST, &dkgray, 0, 0);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		msdelay(300);
		break;

/* 's' - set test - put up a set (foreground) rectangle */
	case 's':
		pr_rop(mscreen, 300, 200, 200, 107, PIX_SET, 0, 0, 0);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
		msdelay(300);
		break;

/* 't' - read test - compare mscreen with what is read */
	case 't':
		printf("\033[H\f\n\n");
		msdelay(10);
		pr_rop(mscreen, 0, 0, 1024, 800, PIX_SRC, screen, 0, 0);
		outback(mscreen, (struct pixrect *)mem_create(32,8,1));
		outback(mscreen, (struct pixrect *)mem_create(16,16,1));
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
			pr_vector(mscreen, 100+z*z*2+x,100+z*z*2+y,
			    100+z*z*2+x*z,100+z*z*2+y*z, PIX_SRC, 1);
	/* test 3 pixel vector */
		pr_vector(mscreen, 402, 400, 404, 401, PIX_SRC, 1);
	/* test 4 pixel vector */
		pr_vector(mscreen, 412, 400, 415, 401, PIX_SRC, 1);
	/* test other color */
		pr_vector(mscreen, 0, 500, 1023, 500, PIX_SRC, 0);
		msdelay(1000);
		break;

/* 'x' - Xor hidden-window test - xor-slide text over text */
	case 'x':
		putalphabet();
		pr_rop(mscreen, 350, 250, 26*12, 13*22,
		    PIX_SRC, mscreen, 16, 16);		/* central text */
		pr_rop(mscreen, 16, 16, 26*12, 15*22,
		    PIX_SRC, mscreen, 16, 400);		/*clr top left */
		pr_rop(mpr, 0, 0, 1024, 800,
		    PIX_SRC, mscreen, 0, 0);
		pr_rop(mscreen, 112, 0, 10*12, 6*22,
		    PIX_SRC, mscreen, 350, 250);		/*sliding text*/
		pr_rop(mscreen, 350, 250, 10*12, 6*22,
		    PIX_SRC^PIX_DST, mscreen, 112, 0);
		pr_rop(mscreen, 400, 300, 10*12, 6*22,
		    PIX_SRC^PIX_DST, mscreen, 112, 0);
		pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
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

outback(mscreen, mpr1)
	struct pixrect *mscreen, *mpr1;
{
	int i;

	strcpy(mpr_d(mpr1)->md_image, "Data correctly read from source");
	pr_rop(mscreen, 16 , 16, 32, 1, PIX_SRC, mpr1, 0, 0);
	pr_rop(mscreen, 16 , 17,  3, 1, PIX_SRC, mpr1, 0, 1);
	pr_rop(mscreen, 19 , 17, 29, 1, PIX_SRC, mpr1, 3, 1);
	pr_rop(mscreen, 16 , 18,  3, 2, PIX_SRC, mpr1, 0, 2);
	pr_rop(mscreen, 19 , 18, 29, 2, PIX_SRC, mpr1, 3, 2);
	pr_rop(mscreen, 16 , 20,  3, 4, PIX_SRC, mpr1, 0, 4);
	pr_rop(mscreen, 19 , 20,  3, 4, PIX_SRC, mpr1, 3, 4);
	pr_rop(mscreen, 22 , 20, 26, 4, PIX_SRC, mpr1, 6, 4);
	pr_rop(mscreen, 48 , 16, 32, 8, PIX_SRC, mpr1, 0, 0);
	strcpy(mpr_d(mpr1)->md_image, "readback had no effect at all. ");
	pr_rop(mpr1, 0, 0, 2, 4, PIX_SRC, mscreen, 16, 16);
	pr_rop(mpr1, 2, 0,30, 4, PIX_SRC, mscreen, 18, 16);
	pr_rop(mpr1, 0, 4, 2, 4, PIX_SRC, mscreen, 16, 20);
	pr_rop(mpr1, 2, 4, 3, 4, PIX_SRC, mscreen, 18, 20);
	pr_rop(mpr1, 5, 4,27, 4, PIX_SRC, mscreen, 21, 20);
	pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
	for (i = 0; i < 32; i++)
		printf("%c", 
		   ((char *)mpr_d(mpr1)->md_image)[i]);
	printf("\n");
	msdelay(50);
	pr_rop(mscreen, 0, 0, 1024, 200, PIX_SRC, screen, 0, 0);
}

msdelay(ms)
	register int ms;
{

	ms *= 1000;
	while (--ms >= 0)
		;
}

char *alphabet[] = {
"abcdefghijklmnopqrstuvwxyz",
"bcdefghijklmnopqrstuvwxyza",
"cdefghijklmnopqrstuvwxyzab",
"defghijklmnopqrstuvwxyzabc",
"efghijklmnopqrstuvwxyzabcd",
"fghijklmnopqrstuvwxyzabcde",
"ghijklmnopqrstuvwxyzabcdef",
"hijklmnopqrstuvwxyzabcdefg",
"ijklmnopqrstuvwxyzabcdefgh",
"jklmnopqrstuvwxyzabcdefghi",
"klmnopqrstuvwxyzabcdefghij",
"lmnopqrstuvwxyzabcdefghijk",
"mnopqrstuvwxyzabcdefghijkl",
};

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
	pr_rop(mscreen, 0, 0, 1024, 800, PIX_SRC, screen, 0, 0);
	pr_rop(screen, 0, 0, 1024, 800, PIX_CLR, 0, 0, 0);
}

clearscreen()
{

	printf("\033[H\f");
}

#ifdef notdef
putalphabet()
{

	int j;
	clearscreen();
	for (j = 0; j < 13; j++)
		pf_text(mscreen, 16, 30+j*20, PIX_SRC, pf, alphabet[j]);
}

clearscreen()
{

	pr_rop(mscreen, 0, 0, 1024, 800, 0, 0, 0, 0);
	pr_rop(screen, 0, 0, 1024, 800, PIX_SRC, mscreen, 0, 0);
}
#endif
