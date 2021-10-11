
#ifndef lint
static	char sccsid[] = "@(#)demosubs.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
* Test subroutines for pixrect operations.
 *
 * Test Functions:
 *      a       test_colormap
 *      b       test_stencil
 *      c       test_reverse_video
 *      d       test_batchrop
 *      e       test_cursor
 *      f       test_clipping
 *      g       test_getput
 *      h       test_hidden
 *      i       Region batchrop clipping test
 *      j       set test - sets a foreground rectangle (also tests null source)
 *      k       text - write "The quick brown..." to screen, read, print %x
 *      l       vector test - display asterisks with empty centers
 *      m       xor test - xor text to top left corner of text and to center
 */

#include "report.h"
#include <strings.h>
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
/*********************************************
 ************TEMPORARY*********
*/

struct	pixrect  *screen;
int	op;



main(argc, argv)
	int argc;
	char **argv;
{
	int clip = PIX_DONTCLIP;
	int mode = TEST;
	int i, x, y, z;
	struct pixrect *mpr1;
        int varargc;
	char *c,*s;

	if (argc == 1) {
		printf("Usage: %s x\n\
where x is any string of letters from:\n\
a	set colormap, clear, christmas tree stencil\n\
b	stencil test\n\
c       reverse video test\n\
d	batchrop sample text to screen\n\
e	cursor slides diagonally down\n\
f	clipping test on screen\n\
g	get/put - same effect as t\n\
h	hidden-window test - slide one window over another\n\
i	Region batchrop clipping test\n\
j	set test - set a foreground-color rectangle\n\
k	text test - basic read-write test\n\
l	vectors - display growing-asterisk pattern\n\
m	xor test - xor characters (a) with self (b) with center\n", argv[0]);
		exit(1);
	}

        /* Create ouput file for reports
	********************************
	*/
	/*
	 * Create screen pixrects.
	 */
	screen = pr_open("/dev/fb");  /* bwone0, bwtwo0, argu to main */
	if (screen == 0) {
		fprintf(stderr, "Can't open frame buffer (/dev/fb)\n");
		exit(1);
	}

/*	if (!strcmp(argv[1], "all"))
		argv[1] =
		    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
*/
/* Below we check for passwd parameters that indicate to benchmark or
   to clip, etc   
*/

        varargc = argc;

	while(--varargc > 0  && (*++argv)[0] == '-')
	    for(s = argv[0]+1; *s != '\0'; s++)
		switch(*s){
		
 			case 'b':   /* benchmark */
				mode = NO_TEST;
		printf("print NO TEST = %d \n",mode);
				break;
			case 'c':   /* clipping */
				clip = PIX_CLIP;
				printf("WE ARE CLIPPING now \n");
				break;
			default:
				printf("test: illegal option %c \n",*s);
				argc = 0;
				break;
			} /* end of switch */



 /*	argv++;   */	
	for (c = argv[0]; *c != '\0'; c++)
	switch (*c) {
	case 'a':    /* colormap */
                test_colormap(screen,mode,clip,report);
		break;
	case 'b':   /* stencil test */
		printf(" stencil test now \n");
		test_stencil(screen,mode,clip,report);
		break;
	case 'c':    /* reverse video test */
		printf(" reverse test now \n");
		test_reverse_video(screen,mode,clip,report);
		break;
	case 'd':     /* test batchdrop  */
		test_batchrop(screen,mode,clip,report);
		break;
      
	case 'e':    /* Cursor test */
		test_cursor(screen,mode,clip,report);
		break;
	case 'f':   /* test clipping */
		test_clipping(screen,mode,clip,report);
		break;
	case 'g':   /* test getput */
		test_getput(screen,mode,clip,report);
		break;
	case 'h':  /* hidden test */
		test_hidden(screen,mode,clip,report);
		break;
	case 'i':  /* region batchrop test */
		test_regionbatchclip(screen,mode,clip,report);
		break;
	case 'j':  /* sets a foreground rectangle */
		test_setrect(screen,mode,clip,report);
		break;
	case 'k':   /* don't know */
		test_replrop(screen,mode,clip,report);
		break;
	case 'l':  /* display vectors */
		test_screenvectors(screen,mode,clip,report);
		break;
	case 'm':  /* test reback ?? */
		test_readback(screen,mode,clip,report);
		break;
	case 'n': /* draw s with vectors */
		test_memvectors(screen,mode,clip,report);
		break;
	}
}
