#ifndef lint
static        char sccsid[] = "@(#)testpolytrap.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/file.h>
#include <suntool/tool_hs.h>
#include <pixrect/traprop.h>

static unsigned treeimage[128] = {
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
mpr_static(tree, 64, 64, 1, (short *)treeimage);


struct point2d { int x,y; };
static int nbnds1 = 2;				/* polygon 1, + sign and hole */
static int bndsizes1[] = {12,4};
struct point2d pts1[] = { {300,300}, {300,350}, {350,350}, {350,400},
			  {400,400}, {400,350}, {450,350}, {450,300},
			  {400,300}, {400,250}, {350,250}, {350,300},
			  {350,300}, {350,350}, {400,350}, {400,300} };

static int nbnds2 = 2;				/* polygon 2, triangle hole */
static int bndsizes2[] = {3, 3};
struct point2d pts2[] = { {50,50}, {50,200}, {150,100}, {75,80},
			  {80,150}, {100,100} };

static int nbnds3 = 1;				/* polygon 3, diag squares */
static int bndsizes3[] = {7};
struct point2d pts3[] = { {300,100}, {300,150}, {400,150}, {400,200},
			  {350,200}, {350,100}, {300,100} };

static int nbnds4 = 1;				/* polygon 4, saddle bags */
static int bndsizes4[] = {9};
struct point2d pts4[] = { {300,350}, {300,400}, {450,400}, {450,350},
			  {400,350}, {400,400}, {350,400}, {350,350},
			  {300,350} };

static int nbnds5 = 1;				/* polygon 5, figure eight */
static int bndsizes5[] = {9};
struct point2d pts5[] = { {100,350}, {100,400}, {200,400}, {200,450},
			  {150,450}, {150,400}, {250,400}, {250,350},
			  {100,350} };

static int nbnds6 = 1;				/* polygon 6, triangle fit */
static int bndsizes6[] = {4};
struct point2d pts6[] = { {50,50}, {150,100}, {50,200}, {250,100} };

static int nbnds7 = 2;				/* polygon 7, pattern triangle*/
static int bndsizes7[] = {3, 3};
struct point2d pts7[] = { {50,200}, {50,350}, {250,250}, {75,230},
			  {80,300}, {150,250} };
/*
 * Here is the traprop data for an octagon
 */
int	shallowsteep[] = {0xbbbbbbbb, 0xbbbbbbbb, 0x44444444, 0x44444444},
	steepshallow[] = {0x44444444, 0x44444444, 0xbbbbbbbb, 0xbbbbbbbb};

struct pr_chain l1 = {0, {64, 64}, steepshallow},      /* left lower */
		l0 = {&l1, {-64, 64}, shallowsteep},   /* left upper */
		r1 = {0, {-64, 64}, steepshallow},     /* right lower */
		r0 = {&r1, {64, 64}, shallowsteep};    /* right upper */

struct pr_fall	left_oct  = {{0, 0}, &l0},             /* left side */
		right_oct  = {{0, 0}, &r0};            /* right side */

struct pr_trap	octagon  = {&left_oct, &right_oct, 0, 128};

/*
 * A graphics subwindow struct
 */


struct	gfxsubwindow {
	int	gfx_windowfd;
	int	gfx_flags;
#define	GFX_RESTART	0x01
#define	GFX_DAMAGED	0x02
	int	gfx_reps;
	struct	pixwin *gfx_pixwin;
	struct	rect gfx_rect;
	caddr_t	gfx_takeoverdata;
};

extern	struct gfxsubwindow *gfxsw_init();
struct gfxsubwindow *sw;

main(argc,argv) int argc; char *argv[];
{
	int fd,image_count;
	char *image_file;
	struct rasterfile header;
	unsigned char cmap[3][256];
	int i,j,k, op;
	int sizex, sizey;
	char *c;

	struct pixrect *pattern;

	if (argc < 2) {
	    printf("arguments: sizex sizey abcdefg\n");
	    exit(1);
	}

	if ((sw = gfxsw_init(0,0)) == (struct gfxsubwindow *) 0)
	    exit(1);
	cmap[0][0] = 255; cmap[0][1] = 255; cmap[0][2] = 0;
	cmap[1][0] = 255; cmap[1][1] = 0; cmap[1][2] = 255;
	cmap[2][0] = 255; cmap[2][1] = 0; cmap[2][2] = 0;
	pw_setcmsname(sw->gfx_pixwin, "test");
	pw_putcolormap(sw->gfx_pixwin, 0,16, cmap[0],cmap[1],cmap[2]);

	pattern = mem_create( 200, 200, 1);
	pr_replrop( pattern, 0,0,200,200, PIX_SRC, &tree, 0, 0);

	argv++;
	sizex = 1; sizey = 1;
	if (argc > 2 && argv[1] != '\0') sizex = atoi( argv[1]);
	if (argc > 3 && argv[2] != '\0') sizey = atoi( argv[2]);
	if (sizex < 1) sizex = 1;
	if (sizey < 1) sizey = 1;

	while (1) {
		if (sw->gfx_flags&GFX_DAMAGED) {
			gfxsw_handlesigwinch(sw);
		}
		if (sw->gfx_flags&GFX_RESTART) {
			sw->gfx_flags &= ~GFX_RESTART;
			pw_write( sw->gfx_pixwin, 0,0,2048,2048,PIX_SET,0,0,0);
			continue;
		}

		for (c = argv[0]; *c != '\0'; c++) {
			int nbnds, *bndsizes;
			struct point2d *pts;

			op = PIX_COLOR(1) | PIX_SRC;
			nbnds = nbnds1; bndsizes = bndsizes1; pts = pts1;
			switch (*c) {
			case 'a':
				nbnds = nbnds1;
				bndsizes = bndsizes1; pts = pts1; break;
			case 'b':
				nbnds = nbnds2;
				bndsizes = bndsizes2; pts = pts2; break;
			case 'c':
				nbnds = nbnds3;
				bndsizes = bndsizes3; pts = pts3; break;
			case 'd':
				op = PIX_COLOR(2) | PIX_SRC;
				nbnds = nbnds4;
				bndsizes = bndsizes4; pts = pts4; break;
			case 'e':
				nbnds = nbnds5;
				bndsizes = bndsizes5; pts = pts5; break;
			case 'f':
				op = PIX_COLOR(2) | PIX_SRC;
				nbnds = nbnds6;
				bndsizes = bndsizes6; pts = pts6; break;
			case 'g':
				nbnds = nbnds7;
				bndsizes = bndsizes7; pts = pts7; break;
			case 'h':
				nbnds = nbnds1;
				bndsizes = bndsizes1; pts = pts1; break;
			}
			i = 0;
			for (j=0; j<nbnds; j++)
				for (k=0; k<bndsizes[j]; k++) {
					pts[i].x = (pts[i].x/sizex) +
					(sw->gfx_pixwin->pw_pixrect->pr_size.x -
					sw->gfx_pixwin->pw_pixrect->pr_size.x/
						sizex)/2;
					pts[i].y = (pts[i].y/sizey) +
					(sw->gfx_pixwin->pw_pixrect->pr_size.y -
					sw->gfx_pixwin->pw_pixrect->pr_size.y/
						sizey)/2;
					i++;
				}
			if (*c == 'g') {
				pw_polygon_2( sw->gfx_pixwin, 10,13, nbnds,
					bndsizes,pts,op, pattern,-50,-200);
			} else if ( *c == 'h') {
				pw_traprop(sw->gfx_pixwin, 240, 130,
					octagon, op, pattern, 80, 7);
			} else
				pw_polygon_2( sw->gfx_pixwin, 0,0,
					nbnds,bndsizes,pts,op, 0,0,0);
		}
		sleep( 2);
	}
}
