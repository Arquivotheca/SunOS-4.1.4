#ifndef lint
static	char sccsid[] = "@(#)move.c 1.1 94/10/31 SMI"; /* from UCB 4.1 6/27/83 */
#endif

move(xi,yi){
		movep(xconv(xsc(xi)),yconv(ysc(yi)));
		return;
}
