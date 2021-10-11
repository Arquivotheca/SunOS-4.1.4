/*	@(#)expand_name.h 1.1 94/10/31 SMI; from UCB X.X XX/XX/XX	*/


/*	struct namelist *
 *	expand_name(str)
 *	    char	*str;
 *
 *	void
 *	free_namelist(nl)
 *	    struct namelist *nl;
 *
 *  expand_name returns a pointer to a struct namelist containing
 *	the client's shell's idea of what its argument expands to.
 *	If str contains no shell metacharacters, the shell is never
 *	consulted, and the argument string is simply returned
 *	in a singleton namelist.
 * 
 *	In case of errors, expand_name() writes an error message to
 *	stderr, and returns NONAMES.
 *
 *  free_namelist
 *	The struct namelist is dynamically allocated, and should be
 *	freed after use by a call to free_namelist().
 */

#ifndef	EXPAND_NAMES_DEFINED
#define EXPAND_NAMES_DEFINED

#define MAX_ARG_CHARS 20840

struct namelist	{
	unsigned	 count;
	char		*names[1];	/* LIE --it's actually names[count]
					 * followed by the strings themselves
					 */
};
typedef struct namelist *Namelist;
#define NONAMES	((struct namelist*) 0)

struct namelist	*expand_name();
void		 free_namelist();

/*
 *	For hysterical reasons, the following defines are also provided:
 */

#define globlist  namelist
#define glob	  expand_name
#define glob_free free_namelist

#endif
