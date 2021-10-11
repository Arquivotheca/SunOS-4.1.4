/*	@(#)getpar.h 1.1 94/10/31 SMI; from UCB 4.1 83/03/23	*/

struct cvntab		/* used for getcodpar() paramater list */
{
	char	*abrev;
	char	*full;
	int	(*value)();
	int	value2;
};

extern double		getfltpar();
extern struct cvntab	*getcodpar();
