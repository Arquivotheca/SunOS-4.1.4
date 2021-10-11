/*
**  USERDBM.H -- user-level definitions for the DBM library
**
**	Version:
**		@(#)userdbm.h	1.1	94/10/31	SMI; from UCB 3.1 10/13/82
*/

typedef struct
{
	char	*dptr;
	int	dsize;
} DATUM;

extern DATUM fetch();
