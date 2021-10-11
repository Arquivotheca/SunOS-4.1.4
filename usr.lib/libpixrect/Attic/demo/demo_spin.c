#ifndef lint
static	char sccsid[] = "@(#)demo_spin.c 1.1 94/10/31 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>

int cx, cy;
int x, y;
int x1, y1, x2, y2, x3, y3, x4, y4;
int nx1, ny1, nx2, ny2, nx3, ny3, nx4, ny4;
int px1, py1, px2, py2, px3, py3, px4, py4;
int pnx1, pny1, pnx2, pny2, pnx3, pny3, pnx4, pny4;
int yd;
int z, zsc;
int op;
struct	pixrect  *screen;

main(argc,argv)
char **argv;
{

	int sp;
	screen = (struct pixrect *)bw1_create(1024,800,1);
	if ( screen == 0 ) exit(1);
	pr_rop( screen, 0, 0, 1024, 800, PIX_CLR, 0, 0, 0);
	zsc = 10;
	x = -200<<16, y = 200<<16, z = 200<<zsc;
	yd = 1600;
	cx = 512, cy = 400;
	while (1) {
		for (sp = 10; sp > 0; sp--)
			spin( sp );
		for (sp = 2; sp < 10; sp++)
			spin( sp );
	}
}

spin(sp)
{

	int i;
	for (i = 0; i < 50; i++) {
		square(x,y);
		x = x-(y>>sp);
		y = y+(x>>sp);
	}
}

square(x,y)
{

	register xx = x>>16, yy = y>>16;
	nx1 = xx;
	ny1 = yy;
	nx2 = yy;
	ny2 = - xx;
	nx3 = - xx;
	ny3 = - yy;
	nx4 = - yy;
	ny4 = xx;
	pnx1 = (nx1<<zsc)/(yd+ny1);
	pny1 = z/(yd+ny1);
	pnx2 = (nx2<<zsc)/(yd+ny2);
	pny2 = z/(yd+ny2);
	pnx3 = (nx3<<zsc)/(yd+ny3);
	pny3 = z/(yd+ny3);
	pnx4 = (nx4<<zsc)/(yd+ny4);
	pny4 = z/(yd+ny4);
	op = PIX_CLR;
	nvec(px1,py1,px1,-py1);
	op = PIX_SET;
	nvec(pnx1,pny1,pnx1,-pny1);
	op = PIX_CLR;
	nvec(px2,py2,px2,-py2);
	op = PIX_SET;
	nvec(pnx2,pny2,pnx2,-pny2);
	op = PIX_CLR;
	nvec(px3,py3,px3,-py3);
	op = PIX_SET;
	nvec(pnx3,pny3,pnx3,-pny3);
	op = PIX_CLR;
	nvec(px4,py4,px4,-py4);
	op = PIX_SET;
	nvec(pnx4,pny4,pnx4,-pny4);
	op = PIX_CLR;
	nvec(px1,py1,px2,py2);
	op = PIX_SET;
	nvec(pnx1,pny1,pnx2,pny2);
	op = PIX_CLR;
	nvec(px2,py2,px3,py3);
	op = PIX_SET;
	nvec(pnx2,pny2,pnx3,pny3);
	op = PIX_CLR;
	nvec(px3,py3,px4,py4);
	op = PIX_SET;
	nvec(pnx3,pny3,pnx4,pny4);
	op = PIX_CLR;
	nvec(px4,py4,px1,py1);
	op = PIX_SET;
	nvec(pnx4,pny4,pnx1,pny1);
	op = PIX_CLR;
	nvec(px1,-py1,px2,-py2);
	op = PIX_SET;
	nvec(pnx1,-pny1,pnx2,-pny2);
	op = PIX_CLR;
	nvec(px2,-py2,px3,-py3);
	op = PIX_SET;
	nvec(pnx2,-pny2,pnx3,-pny3);
	op = PIX_CLR;
	nvec(px3,-py3,px4,-py4);
	op = PIX_SET;
	nvec(pnx3,-pny3,pnx4,-pny4);
	op = PIX_CLR;
	nvec(px4,-py4,px1,-py1);
	op = PIX_SET;
	nvec(pnx4,-pny4,pnx1,-pny1);
	x1 = nx1, y1 = ny1;
	x2 = nx2, y2 = ny2;
	x3 = nx3, y3 = ny3;
	x4 = nx4, y4 = ny4;
	px1 = pnx1, py1 = pny1;
	px2 = pnx2, py2 = pny2;
	px3 = pnx3, py3 = pny3;
	px4 = pnx4, py4 = pny4;
}

nvec(x1,y1,x2,y2)
{

	pr_vector(screen, cx+x1, cy+y1, cx+x2, cy+y2, op, 1);
}
