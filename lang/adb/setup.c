/*	@(#)setup.c 1.1 94/10/31 SMI	*/

#ifdef vax
#	include "setupvax.c"
#endif
#ifdef sun
#	include "setup68.c"
#endif
