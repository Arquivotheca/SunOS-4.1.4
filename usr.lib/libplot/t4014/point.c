#ifndef lint
static	char sccsid[] = "@(#)point.c 1.1 94/10/31 SMI"; /* from UCB 4.1 6/27/83 */
#endif

point(xi,yi){
	move(xi,yi);
	cont(xi,yi);
}
