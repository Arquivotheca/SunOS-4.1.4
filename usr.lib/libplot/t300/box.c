#ifndef lint
static	char sccsid[] = "@(#)box.c 1.1 94/10/31 SMI"; /* from UCB 4.1 6/27/83 */
#endif

box(x0, y0, x1, y1)
{
	move(x0, y0);
	cont(x0, y1);
	cont(x1, y1);
	cont(x1, y0);
	cont(x0, y0);
	move(x1, y1);
}
