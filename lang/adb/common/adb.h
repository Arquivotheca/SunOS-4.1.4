/*	@(#)adb.h 1.1 94/10/31 SMI	*/

/*
 * adb - a debugger
 *
 * symbolic and kernel enhanced 4.2bsd version.
 *
 * this is a 32 bit machine version of this program.
 * it keeps large data structures in core, and expects
 * in several places that an int can represent 32 integers
 * and file indices.
 */

#include <a.out.h>
#include <stab.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <machine/psl.h>
#include <machine/pte.h>
#ifdef	sparc
#include <vfork.h>
#endif	sparc

#if	KADB
#   include <sys/core.h>
#else
#   if cross
#	include "core.h"
#	include "user.h"
#   else
#	include <sys/core.h>
#	include <sys/user.h>
#   endif cross
#endif	!KADB

#include <ctype.h>
#include <malloc.h>

#undef NFILE    /* from sys/param.h */

#define	NSIG	32

#ifdef	vax
# include "vax.h"
#endif

#ifdef	sun
#  ifdef sparc
#    include "../sparc/sparc.h"
#  else !sparc
#    ifdef mc68000
#      include "../m68k/sun.h"
#    endif mc68000
#  endif !sparc
#endif	sun

#include "process.h"


/* Address space constants	  cmd	which space		*/
#define	NSP	0		/* =	(no- or number-space)	*/
#define	ISP	1		/* ?	(object/executable)	*/
#define	DSP	2		/* /	(core file)		*/
#define	SSP	3		/* @	(source file addresses) */

#define	STAR	4

#define	NSYM	0		/* no symbol space */
#define	ISYM	1		/* symbol in I space */
#define	DSYM	1		/* symbol in D space (== I space on VAX) */

#define	NVARS	10+26+26	/* variables [0..9,a..z,A..Z] */

/* result type declarations */
char	*exform();
/* VARARGS */
int	printf();
int	ptrace();
long	lseek();

char	*strcpy(), *sprintf();

/* miscellaneous globals */
char	*errflg;	/* error message, synchronously set */
char	*lp;		/* input buffer pointer */
int	interrupted;	/* was command interrupted ? */
int	ditto;		/* last address expression */
int	lastcom;	/* last command (=, /, ? or @) */
int	var[NVARS];	/* variables [0..9,a..z,A..Z] */
int	expv;		/* expression value from last expr() call */
char	sigpass[NSIG];	/* pass-signal-to-subprocess flags	  */
int	adb_pgrpid;	/* used by SIGINT and SIGQUIT signal handlers */

/*
 * Earlier versions of adb kept the registers within the "struct core".
 * On the sparc, I broke them out into their own separate structure
 * to make it easier to deal with the differences among adb, kadb and
 * adb -k.  This structure "allregs" is defined in "allregs.h" and the
 * variable (named "adb_regs") is declared only in accesssr.c.
 */
struct	core	 core;
struct	adb_raddr adb_raddr;

extern	errno;

int	hadaddress;	/* command had an address ? */
int	address;	/* address on command */
int	hadcount;	/* command had a count ? */
int	count;		/* count on command */

int	radix;		/* radix for output */
int	maxoff;

char	nextc;		/* next character for input */
char	lastc;		/* previous input character */

int	xargc;		/* externally available argc */
int	wtflag;		/* -w flag ? */

void	(*sigint)();
void	(*sigqit)();
void	(*signal())();
