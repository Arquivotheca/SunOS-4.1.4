/* @(#)assert.h	1.1 (Sun) 10/31/94 */

# ifdef BUG4
#    define _assert(ex) \
	{ \
	    if (!(ex)) cerror("Assertion failed: file %s, line %d\n", \
		__FILE__, __LINE__); \
	}
#    define assert(ex) _assert(ex)
# else
# define _assert(ex) ;
# define assert(ex) ;
# endif
