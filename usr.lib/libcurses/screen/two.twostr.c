/*	@(#)two.twostr.c 1.1 94/10/31 SMI	*/
/* Make a 2 letter code into an integer we can switch on easily */
#define	two( s1, s2 )	(s1 + 256 * s2 )
#define	twostr( str )	two( *str, str[ 1 ] )
