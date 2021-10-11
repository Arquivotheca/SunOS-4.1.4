
/*	@(#)messages.h 1.1 94/10/31 SMI; from S5R2 1.4	*/

#define	NUMMSGS	144

#ifndef WERROR
#    define	WERROR	werror
#endif

#ifndef UERROR
#    define	UERROR	uerror
#endif

#ifndef MESSAGE
#    define	MESSAGE(x)	msgtext[ x ]
#endif

extern char	*msgtext[ ];

