/*	@(#)machdep.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# ifndef _machdep_
# define _machdep_ 1

/* Define SPARC registers */
# define RG0 0
# define RG1 1
# define RG2 2
# define RG3 3
# define RG4 4
# define RG5 5
# define RG6 6
# define RG7 7
# define RO0 8
# define RO1 9
# define RO2 10
# define RO3 11
# define RO4 12
# define RO5 13
# define RO6 14
# define RO7 15
# define RL0 16
# define RL1 17
# define RL2 18
# define RL3 19
# define RL4 20
# define RL5 21
# define RL6 22
# define RL7 23
# define RI0 24
# define RI1 25
# define RI2 26
# define RI3 27
# define RI4 28
# define RI5 29
# define RI6 30
# define RI7 31
# define FR0 32
# define FR1 33
# define FR2 34
# define FR3 35
# define FR4 36
# define FR5 37
# define FR6 38
# define FR7 39
# define FR8 40
# define FR28 60
# define FR31 63

/* register cookie for stack pointer */

# define  SPREG  RO6		/* stack temp area pointer */
# define  ARGREG RO6		/* outgoing argument pointer */
# define  STKREG RI6		/* local frame pointer */
# define  AUTOREG STKREG	/* base reg for automatics */
# define  INARGREG STKREG	/* base reg for incoming arguments */
# define  BASEREG RL7		/* base reg for pic global reference */

# define LOREG   RG0
# define HIREG	 FR31

/*	maximum and minimum register variables */

# define MIN_DVAR RL0
# define MAX_DVAR RI5
# define MIN_FVAR FR8
# define MAX_FVAR FR31
# define MAX_XVAR FR28
# define MAXINREG RI5    /* input parameters in Ri0 - Ri5 */
# define MIN_GREG RG2	/* first g-register usable for code generation */
# define MAX_GREG RG5	/* last  g-register usable for code generation */

/*
 * macros for register variable bookkeeping.  This logically involves
 * two counters (one for each register bank).
 * Rather than simple counters, we will actually use masks with one bit
 * on for every register available as a variable. These are called regvar
 * and floatvar. Their structure looks like this:
 * +-----------------------------------------------+
 * | i7 ... i0 | l7 ... l0 | o7 ... o0 | g7 ... g0 |
 * +-----------------------------------------------+
 * +-----------------------------------------------+
 * |                f31  ...  f0                   |
 * +-----------------------------------------------+
 * They are manipulated using the PUSH_ALLOCATION and
 * POP_ALLOCATION macros, below.
 *
 * Note that in the 'twopass' case, we do not reset autooff.
 * We do this so that variables allocated in nested scopes will
 * get non-overlapping addresses.  This is done to ensure that
 * nested local variables have a fair chance at register allocation.
 *
 * I realize this is a kludge, but here are my reasons for doing it:
 * 1. Using overlapped storage makes such variables look like members
 *    of unions, which generally cannot be kept in registers.
 * 2. Using scope information to resolve this conflict is impractical
 *    in IR, since a program's tree structure has long since disappeared
 *    by optimization time.
 */
extern long floatvar;
extern int twopass;

#define PUSH_ALLOCATION( sp ) { \
	*sp++ = autooff; \
	*sp++ = regvar; \
	*sp++ = floatvar; \
}

#define POP_ALLOCATION( sp ) { \
	floatvar = *--sp; \
	regvar = *--sp; \
	if (twopass) { \
		--sp; \
	} else { \
		autooff = *--sp; \
	} \
}

/* ask about a specific register */
#define D_AVAIL( i, rmask ) ((rmask)&(1<<(i)-RG0))
#define F_AVAIL( i, rmask ) ((rmask)&(1<<(i)-FR0))
/* find the next available register */
#define NEXTD(i, rmask ) \
	for(i=MAX_DVAR; i>=MIN_DVAR; i--){ \
		if (D_AVAIL(i, rmask)){ \
			if (picflag && i == BASEREG) continue; \
			break; \
		} \
	}
#define NEXTF(i, rmask )  \
	for(i=MAX_FVAR; i>=MIN_FVAR; i--){ if (F_AVAIL(i, rmask)) break; }
/* reserve a register */
#define SETD( rmask, i ) (rmask) &= ~ (1<<(i)-RG0)
#define SETF( rmask, i ) (rmask) &= ~ (1<<(i)-FR0)

#define markused(x) {;} /* fill in later */


#   define makecc(val,i)	lastcon = (i ? (((lastcon)<<8)|(val)) : (val))
#   define MULTIFLOAT
#   ifdef FORT
#       define FLOATMATH 2
#   else
#       define FLOATMATH floatmath
	extern int floatmath;
#   endif

# define  SAVESIZE (16*SZINT) /* size of register save area: 8 ins, 8 locals */
# define  ARGINIT (SAVESIZE+SZINT)
# define  STRUCT_VAL_OFF (SAVESIZE) /* hidden parameter to struct-valued functions */

# define  BUILTIN_VA_ALIST "__builtin_va_alist" /* magic name from varargs.h */
# define  AUTOINIT 0

# include "align.h"

/*	size in which constants are converted */
/*	should be long if feasable */

# define CONSZ long
# define CONFMT "0%lo"
# define CONBASE 8

/*	size in which offsets are kept
/*	should be large enough to cover address space in bits
*/

# define OFFSZ long

/* 	character set macro */

# define  CCTRANS(x) x

	/* various standard pieces of code are used */
# define LABFMT "L%d"

/* show stack grows negatively */
#define BACKAUTO

/* show field hardware support on VAX */
/* pretend hardware support on 68000  */

/* bit fields are never sign-extended */
#define UFIELDS

/* we want prtree included */
# define STDPRTREE
# if !defined(FORT) && !defined(VCOUNT)
#    define ONEPASS
# endif

# define ENUMSIZE(high,low) INT


# define ADDROREG
# define FIXDEF(p) fixdef(p)
# define FIXARG(p) fixarg(p)

# define LCOMM
# define ASSTRINGS
# define FLEXNAMES
# define FIXSTRUCT outstruct

/* Machines with multiple register banks have a hard time representing
 * a one-component cost function for code generation. What we really
 * need is a cost vector, with one dimension for each resource (register type).
 * This approximates that.
 * N.B.: SUTYPE MUST BE THE SAME SIZE AS AN INTEGER ON THE COMPILING MACHINE!
 */
typedef struct sunum{
    char d, /* data registers */
	 f, /* float registers */
	 calls; /* nested calls */
    char dummy; /* pad to 32-bit int size */
} SUTYPE;

#define IN_IMMEDIATE_RANGE(n) ( (1<<12) > (n) && (n) >= -(1<<12) )

# endif _machdep_
