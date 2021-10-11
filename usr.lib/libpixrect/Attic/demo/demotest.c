/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Test subroutines for pixrect operations.
 *
 * Test Functions:
 *	a	test_colormap
 *	b	test_stencil         draws trees 
 *	c	test_reverse_video   black on white 
 *	d	test_batchrop       The quick brown.printed to screen 
 *	e	test_cursor
 *	f	test_clipping
 *	g	test_getput          reads data from source 
 *	h	test_hidden          hides text behind moving text 
 *	i	Region batchrop clipping test
 *	j	test_setrect- foreground rectangle(tests null source)
 *	k	test_replrop
 *	l	vector test - display asterisks with empty centers
 *	m	test_readback-xor text top left corner of text & center
 *	n	test_memvectors
 */
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include "report.h"
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/cg1reg.h>
#include <pixrect/cg1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>

#include "demoglobals.h"


FILE *fopen(), *fp;

test_colormap( screen, mode, clip, report)
	struct	pixrect  *screen;
	int mode;		/* TEST, BENCH */
	int clip;
	struct test_report report;
{
	int i, x, planes = 255;
	register int iter = 20;
	char *oper = "pr_putcolormap 0..255";
	struct timeval tv1, tv2;

	fp = fopen("pix.analysis","a");
        clearscreen(screen);
	pr_putattributes(screen, &planes);
	for (i=0; i<64; i++) {			/* load color map arrays */
		x = ((i+1)<<2) -1;
		red[i] = x;
		red[i+192] = x;
		green[i+192] = x;
		green[i+64] = x;
		blue[i+128] = x;
	}
	red[0] = green[0] = blue[0] = 127;
	red[255] = green[255] = blue[255] = 255;

	if (mode == TEST) iter = 1;
	i = iter;
	gettimeofday(&tv1, 0);
	while (i-- > 0) {
		pr_putcolormap( screen, 0, 256, red, green, blue);
	}
	gettimeofday(&tv2, 0);
	fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}

test_stencil( screen, mode, clip, report)
	struct	pixrect  *screen;
	int mode;		/* TEST, BENCH */
	int clip;
	struct test_report report;
{
	int op, i;
	register int iter = 20;
	char *oper;
	struct timeval tv1, tv2;
	struct	pixrect  *mpr1;

	fp = fopen("pix.analysis","a");
	clearscreen(screen);
	clip |= PIX_SRC;
	if (mode == TEST) {	/* test null src cursor stencil */
		iter = 1;
		op = PIX_COLOR(63)|clip;
		for (i=100; i<200; i+=10) {
			pr_stencil(screen,i,i,16,16,op,&cursor,0,0,0,0,0);
		}
		op = PIX_COLOR(250)|clip;
		for (i=100; i<200; i+=10) {	/* mempr src cursor stencil */
		    pr_stencil(screen,i+50,i,16,16,op,&cursor,0,0,&stipple,0,0);
		}
		op = PIX_COLOR(127)|clip;	/* mem to scr, odd offsets */
		pr_stencil(screen,100,100,80,90,op,&tree,4,3,&grid,9,1);
		pr_stencil(screen,200,100,64,64,op,&tree,0,0,&grid,0,0);
				/* screen to screen stencil, all directions */
		pr_rop(screen,400,300,64,64,op,screen,0,0);
		pr_stencil(screen,350,250,64,64,op,&tree,0,0,screen,400,300);
		pr_stencil(screen,350,350,64,64,op,&tree,0,0,screen,400,300);
		pr_stencil(screen,450,250,64,64,op,&tree,0,0,screen,400,300);
		pr_stencil(screen,450,350,64,64,op,&tree,0,0,screen,400,300);
				/* screen to mem stencil */
		mpr1 = mem_create(64, 64, screen->pr_depth);
		pr_stencil(mpr1,0,0,64,64,op,&tree,0,0,screen,5,0);
		pr_rop(screen,250,0,64,64,op,mpr1,0,0);
		mem_destroy( mpr1);
	}
	i = iter;
	gettimeofday(&tv1, 0);
	while (i-- > 0) {
		op = PIX_COLOR( 127)|clip;
		pr_stencil(screen,300,220,64,64,op,&tree,0,0,0,0,0);
		op = PIX_COLOR( 10)|clip;
		pr_stencil(screen,300,220,64,64,op,&trunk,0,0,0,0,0);
		op = PIX_COLOR( 63)|clip;
		pr_stencil(screen,300,220,64,64,op,&balls,0,0,0,0,0);
	}
	gettimeofday(&tv2, 0);
	oper = "pr_stencil 64 by 64 mem to screen";
	fill_out_report( report, oper, iter*3, &tv1, &tv2,fp );
}

test_reverse_video( screen, mode, clip, report)
	struct	pixrect  *screen;
	int mode;		/* TEST, BENCH */
	int clip;
	struct test_report report;
{
	int i;
	register int iter = 20;
	char *oper;
	struct timeval tv1, tv2;

	fp = fopen("pix.analysis","a");
	putalphabet(screen);
	pr_rop(screen,0,0,100,100,PIX_NOT(PIX_DST),0,0,0);

	if (mode == TEST) iter = 1;
	i = iter;
	clip |= PIX_SRC;
	gettimeofday(&tv1, 0);
	while (i-- > 0) {
		if (mode == TEST) {
			printf("blackonwhite\n"); msdelay( 200);
		}
		pr_blackonwhite(screen, 0, 255);
		if (mode == TEST) {
			printf("whiteonblack\n"); msdelay( 200);
		}
		pr_whiteonblack(screen, 0, 255);
		if (mode == TEST) {
			printf("reversevideo\n"); msdelay( 200);
		}
		pr_reversevideo(screen, 0, 255);
	}
	gettimeofday(&tv2, 0);
	oper = "pr_blackonwhite, pr_whiteonblack, pr_reversevideo";
	fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}
test_batchrop( screen, mode, clip, report)
	struct	pixrect  *screen;
	int mode;		/* TEST, BENCH */
	int clip;
	struct test_report report;
{
	int i,j,op;
	register int iter = 10;
	char *oper;
	struct timeval tv1, tv2;
	PIXFONT *pf;

	fp = fopen("pix.analysis","a");
	clearscreen(screen);
	clip |= PIX_SRC;
	pf = pf_open(0);
	op = PIX_COLOR(250) | clip;
	if (mode == TEST) iter = 1;
	i = iter;
	gettimeofday(&tv1, 0);
	while (i-- > 0) {
		for (j = 20; j < 460; j += 20)
			pf_text(screen, 0, j, op, pf,
			"A quick brown fox jumped over the lazy dog.");
	}
	gettimeofday(&tv2, 0);
	oper = "pf_text 43 characters to screen (timing is per char)";
	fill_out_report( report, oper, iter*43*22, &tv1, &tv2,fp );
}

test_cursor( screen, mode, clip, report)
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;

{
        int op, x, y;
        register int iter = 400;
        char *oper;
        struct timeval tv1, tv2;
        struct  pixrect *mpr;

	fp = fopen("pix.analysis","a");
	clearscreen(screen);
        putalphabet(screen);
        mpr = mem_create(16, 16, screen->pr_depth);
        op = PIX_COLOR(250) | PIX_SRC | clip;
        gettimeofday(&tv1, 0);
for (x = 0, y = 0; x < iter; x++, y++) {
                pr_rop(mpr, 0, 0, 16, 16, op, screen, x, y);
                pr_rop(screen, x, y, 16, 16, op, &cursor, 0, 0);
                pr_rop(screen, x, y, 16, 16, op, mpr, 0, 0);
        }
        gettimeofday(&tv2, 0);
        mem_destroy( mpr);
        oper = "pr_rop 16 by 16: screen to mem, mem to scr, mem to scr.";
        fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}
test_clipping( screen, mode, clip, report)
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;

{
        int op, x, y;
        register int iter;
        char *oper;
        struct timeval tv1, tv2;
        struct  pixrect *mpr,*mpr1;
#define CHX 8
#define CHY 12
                        /* put centered text, save screen in mpr */
	fp = fopen("pix.analysis","a");
        clearscreen(screen);
        putalphabet(screen);
        mpr = mem_create(screen->pr_width, screen->pr_height, screen->pr_depth);        if (!mpr) return(0);
        op = PIX_SRC;   /* must clip to avoid disaster */
        pr_rop(screen,350,250,26*CHX+1, 13*CHY,op, screen, 0, 0);
        pr_rop(screen,0,0, 26*CHX+1, 15*CHY, op, screen, 16, 200);
        pr_rop(mpr, 0,0, screen->pr_width, screen->pr_height, op, screen, 0,0);
 
                        /* clip against four edges */
        pr_rop(screen,-100,200,26*CHX+1,13*CHY,op,mpr,350,250);
        pr_rop(mpr,   350, 250,26*CHX+1,13*CHY,op,screen,-100,200);
        pr_rop(screen,540, 300,26*CHX+1,13*CHY,op, mpr, 350, 250);
        pr_rop(screen,300,-100,26*CHX+1,13*CHY,op, mpr, 350, 250);
        pr_rop(screen,400, 450,26*CHX+1,13*CHY,op, mpr, 350, 250);

        for (x = 0; x >= -26*CHX+1; x--) {
                pr_rop(screen, x, 200, 26*CHX+1, 13*CHY,
                    op, mpr, 350, 250);
                pr_rop(mpr, 350, 250, 26*CHX+1, 13*CHY,
                    op, screen, x, 200);
       }
       mem_destroy( mpr);

       mpr1 = mem_create(16, 16, screen->pr_depth);
        pr_rop(mpr1, 0, 0, 16, 16, op, screen, 350, 250);
        for (x = 330; x >= -26*CHX+1; x--) {
                 pr_rop(screen, x, 300, 16, 16, op, mpr1, 0, 0);
                pr_rop(mpr1, 0, 0, 16, 16, op, screen, x, 300);
        }
        pr_rop(mpr1, 0, 0, 16, 16, op, screen, 380, 300);
        for (x = 330; x >= -26*CHX+1; x--)
                pr_rop(screen, x, 300, 16, 16, op, mpr1, 0, 0);
        mem_destroy( mpr1);
        mpr1 = mem_create(16, 12, screen->pr_depth);
        pr_rop(mpr1, 0, 0, 16, 12, op, screen, 350, 250);
        gettimeofday(&tv1, 0);
        for (x = 330; x >= -26*CHX+1; x--) {
                pr_rop(screen, x, 300, 16, 12, op, mpr1, 0, 0);
                pr_rop(mpr1, 0, 0, 16, 12, op, screen, x, 300);
        }
        gettimeofday(&tv2, 0);
        mem_destroy( mpr1);
        iter = 330 + 26*CHX;
        oper = "pr_rop 16 by 12: mem to screen, screen to mem.";
        fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}
test_getput( screen, mode, clip, report)        /* simulates text test */
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{
	int i, a, x, y;
        register int iter = 20;
        char *oper, *c,cc;
        struct timeval tv1, tv2;
        struct  pixrect *mpr1;

	fp = fopen("pix.analysis","a");
	clearscreen(screen);
        mpr1 = mem_create(32, 8, screen->pr_depth);
        if (screen->pr_depth = 1) {
	strcpy(mpr_d(mpr1)->md_image,"Data correctly read from source");                for (x = 0; x < 32; x++)
                for (y = 0; y < 8; y++)
                        pr_put(screen, 16+x, 16+y, pr_get(mpr1, x, y));
            strcpy(mpr_d(mpr1)->md_image, "get had no effect at all. ");
                a = 32*8; i = iter;
                gettimeofday(&tv1, 0);
                 while (i--) {
                        for (x = 0; x < 32; x++)
                        for (y = 0; y < 8; y++)
                                pr_put(mpr1, x, y, pr_get(screen, 16+x, 16+y));
                }
	 } else {
            strcpy(mpr_d(mpr1)->md_image,"Data correctly read from source");                for (y = 0; y < 2; y++)
                    for (x = 0; x < 16; x++)
                        pr_put(screen, 16+x, 16+y, pr_get(mpr1, x, y));
	strcpy(mpr_d(mpr1)->md_image,"get had no effect at all.      ");                a = 2*16; i = iter;
                gettimeofday(&tv1, 0);
                while (i--) {
                        for (y = 0; y < 2; y++)
                            for (x = 0; x < 16; x++)
                                pr_put(mpr1, x, y, pr_get(screen, 16+x, 16+y));
                }
	}
        gettimeofday(&tv2, 0);
        for (i = 0; i < 32; i++)
                printf("%c", ((char *)mpr_d(mpr1)->md_image)[i]);

        printf("\n");
        mem_destroy( mpr1);
        oper = "pr_put( mem.., pr_get(screen,...)): ";
        fill_out_report( report, oper, iter*a, &tv1, &tv2,fp );
}
test_hidden( screen, mode, clip, report)        /*  slide text over text */
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{
	int op, x, y;
        register int iter = 20;
        char *oper;
        struct timeval tv1, tv2;
        struct  pixrect *mpr;
 
        op = PIX_SRC | clip;
	fp = fopen("pix.analysis","a");
	clearscreen(screen);
        putalphabet(screen);
        mpr = mem_create(screen->pr_width, screen->pr_height, screen->pr_depth);                                /* central text */
        pr_rop(screen, 250, 150, 26*8, 13*12, op, screen, 0, 0);
                                /* clr top left */
        pr_rop(screen, 0, 0, 26*8, 15*12, op, screen, 0, 200);
                                /* save screen */
        pr_rop(mpr, 0, 0, 640, 480, op, screen, 0, 0);
                                /* sliding text */
        pr_rop(screen, 112, 0, 10*8, 6*12, op, screen, 250, 150);
        gettimeofday(&tv1, 0);
        for (x = 112, y = 0; x < 640; x++, y++) {
                pr_rop(screen,x+1,y+1,10*8,6*12,op,screen,x,y);
		pr_rop(screen, x, y, 1, 6*12,op, mpr, x, y);
                pr_rop(screen, x, y, 10*8, 1, op, mpr, x, y);
        }
	gettimeofday(&tv2, 0);
        mem_destroy( mpr);
        iter = 640 - 112;
        oper = "pr_rop: 80x72 scrn to scrn, 1x72 mem to scr, 80x1 mem to scr";
        fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}
test_replrop( screen, mode, clip, report)
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{
        int i, op;
        register int iter = 20;
        char *oper;
        struct timeval tv1, tv2;

	fp = fopen("pix.analysis","a");
	clearscreen(screen);
        op = PIX_COLOR(127) | PIX_SRC;
        pr_replrop(screen,18,18,640-36,480-36,op,&dtgray,0,0);
        pr_replrop(screen,0,1,1152,900,op,&tree,0,0);
        op = PIX_SRC^PIX_DST;
	pr_replrop(screen,0,0,1152,900,op,&dkgray,0,0);
        op = PIX_SRC;
        if (mode == TEST) iter = 1;
        i = iter;
	gettimeofday(&tv1, 0);
        while (i--) {
                pr_replrop(screen,0,0,1152,900,op,&dtgray,0,0);
        }
	gettimeofday(&tv2, 0);
        oper = "pr_replrop: 1152x900 mem to scrn";
        fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}
test_setrect( screen, mode, clip, report)       /* put up a PIX_SET rectangle */        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{
 	int i, op;
        register int iter = 10;
        char *oper;
        struct timeval tv1, tv2;
 
        fp = fopen("pix.analysis","a");
        clearscreen(screen);
        if (mode == TEST) iter = 1;
        i = iter;
        gettimeofday(&tv1, 0);
	while (i--) {
		pr_rop(screen, 300, 200, 200, 100, PIX_SET, 0, 0, 0);
                op = PIX_COLOR( 63)  | PIX_SRC | clip;
		pr_rop(screen, 100, 100, 200, 100, op, 0, 0, 0);
                op = PIX_COLOR( 127) | PIX_SRC | clip;
                pr_rop(screen, 100, 300, 200, 100, op, 0, 0, 0);
                op = PIX_COLOR( 191) | PIX_SRC | clip;
                pr_rop(screen, 500, 100, 200, 100, op, 0, 0, 0);
                op = PIX_COLOR( 254) | PIX_SRC | clip;
                pr_rop(screen, 500, 300, 200, 100, op, 0, 0, 0);
        }
	gettimeofday(&tv2, 0);
        oper = "pr_rop: 200x100 null src to screen";
        fill_out_report( report, oper, iter*5, &tv1, &tv2,fp );
}
test_regionbatchclip( screen, mode, clip, report)
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;

{
        int i, op;
        register int iter = 20;
        char *oper;
        struct timeval tv1, tv2;

	int     OFFSET = 5, BASELINE = 18, HEIGHT = BASELINE-OFFSET;
        PIXFONT *pf;
        struct pixrect *region; 

	fp = fopen("pix.analysis","a");
	pf = pf_open(0);
	clearscreen(screen);
	region = pr_region(screen,0,OFFSET,80,HEIGHT);
        op = PIX_DONTCLIP|(PIX_SRC^PIX_DST);
        pf_text(screen, BASELINE, BASELINE, op, pf, "abcgpqy");
        op = PIX_SRC^PIX_DST;
	pf_text(region, BASELINE, BASELINE-OFFSET, op, pf, "abcgpqy");
        pr_destroy(region);
        if (mode == TEST) iter = 1;
        i = iter;
	gettimeofday(&tv1, 0);
        while (i--) {
                region = pr_region(screen, 20, 20, 36, 70);
                putalphabet( region);
		}
	gettimeofday(&tv2, 0);
        pr_destroy(region);
        oper = "pr_text: batched char clipped to screen region";
	fill_out_report( report, oper, iter*13*26, &tv1, &tv2,fp );
}

test_screenvectors( screen, mode, clip, report)
	struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{   
      /* draws with empty centers */
	register short x, y, z, j;
        short i, k;
        int op, iter = 20;
        char *oper;
        struct timeval tv1, tv2;
	
	fp = fopen("pix.analysis","a");
	clearscreen(screen);
        if (mode == TEST) iter = 1;
        i = iter;
        gettimeofday(&tv1, 0);
        while (i--) {
		for (z = 0; z < 16; z++) {
                        j = 10+z*z*2;  k = z*16;
                        for (x = -1; x<2; x++)
                        for (y = -1; y<2; y++)
                            if (x != 0 || y != 0)
				pr_vector(screen,j+x,j+y,j+x*z,j+y*z,z<<1,k);
                }
        }
	 gettimeofday(&tv2, 0);
        oper = "pr_vector: random short vectors ";
        fill_out_report( report, oper, iter*(16*2*2), &tv1, &tv2,fp );
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
}
test_readback( screen, mode, clip, report)
        struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{
	int i, op;
        register int iter = 28;
        char *oper;
        struct timeval tv1, tv2;

	fp = fopen("pix.analysis","a");
        clearscreen(screen);
	gettimeofday(&tv2, 0);
        outback(screen, (struct pixrect *)mem_create(32,8,screen->pr_depth));
         gettimeofday(&tv2, 0);
        oper = "pr_rop: random sizes (readback test, not a benchmark)";
        fill_out_report( report, oper, iter, &tv1, &tv2,fp );
}
test_memvectors( screen, mode, clip, report) 
	struct  pixrect  *screen;
        int mode;               /* TEST, BENCH */
        int clip;
        struct test_report report;
{
	/* draw *s with empty centers */
	int x,y,z,i, op;
        register int iter = 28;
        char *oper;
        struct timeval tv1, tv2;
	struct pixrect *mpr1;
	
	fp = fopen("pix.analysis","a");
	clearscreen(screen);
	mpr1 = mem_create(640, 480, 8);
        for (z = 0; z < 16; z++)
        for (x = -1; x<2; x++)
        for (y = -1; y<2; y++)
        if (x != 0 || y != 0)
        pr_vector(mpr1, 10+z*z*2+x,10+z*z*2+y,
            10+z*z*2+x*z,10+z*z*2+y*z, z<<1, z*16);
			/* test 3 pixel vector */
	pr_vector(mpr1, 402, 400, 404, 401, PIX_SRC, 127);
		/* teset 4 pixel vectors */
	pr_vector(mpr1, 412, 400, 415, 401, PIX_SRC, 127);
        pr_vector(mpr1, 0, 0, 100, 60, PIX_SRC, 63);
	pr_vector(mpr1, 100, 60, 200, 120, PIX_SRC, 63);
	pr_vector(mpr1, 0, 0, 60, 100, PIX_SRC, 63);
	pr_vector(mpr1, 60, 100, 120, 200, PIX_SRC, 63);
	pr_rop(screen, 1, 1, 640, 480, PIX_SRC, mpr1, 0,0);


	oper = " pr_vectors draw an s ";
	fill_out_report(report,oper,iter,&tv1,&tv2,fp);
}
fill_out_report( report, oper, iter, t1, t2 ,fp)
	struct test_report report;
	char *oper;
	int iter;
	struct timeval *t1, *t2;
        FILE *fp;
{
		strcpy( report.oper, oper);
		report.iter = iter;
		report.total_time = (t2->tv_sec - t1->tv_sec)*1000000 +
			(t2->tv_usec - t1->tv_usec);
		report.oper_time = report.total_time / iter;
    printf(" String is %s \n", report.oper);
    printf(" Iterations %d \n",report.iter);
    printf(" Time in milli %d \n",report.total_time);
    fprintf(fp,"        String is =    %s \n",report.oper);
    fprintf(fp,"     	Iterations =   %d \n",report.iter);
    fprintf(fp,"Time in milliseconds = %d \n",report.total_time);
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
		printf("%c", ((char *)mpr_d(mpr1)->md_image)[i]);
	printf("\n");
}

clearscreen(screen)
   struct pixrect *screen;
{

   pr_rop(screen,0,0,screen->pr_width,screen->pr_height,PIX_SRC,0,0,0);
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
        int    color,op;
    PIXFONT    *pf;

	pf = pf_open(0);  
	clearscreen(scr);
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
