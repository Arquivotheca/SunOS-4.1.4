#ifndef lint
static	char sccsid[] = "@(#)playgame.c 1.1 94/10/31 SMI"; /* from UCB */
#endif
# include	"hangman.h"

/*
 * playgame:
 *	play a game
 */
playgame()
{
	register bool	*bp;

	getword();
	Errors = 0;
	bp = Guessed;
	while (bp < &Guessed[26])
		*bp++ = FALSE;
	while (Errors < MAXERRS && index(Known, '-') != NULL) {
		prword();
		prdata();
		prman();
		getguess();
	}
	endgame();
}
