#ifndef lint
static	char sccsid[] = "@(#)space.c 1.1 94/10/31 SMI"; /* from UCB 4.1 6/27/83 */
#endif

# include "con.h"
float deltx = 4095.;
float delty = 4095.;
space(x0,y0,x1,y1){
	botx = -2047.;
	boty = -2047.;
	obotx = x0;
	oboty = y0;
	scalex = deltx/(x1-x0);
	scaley = delty/(y1-y0);
}
