/*	@(#)pcc_types.h 1.1 94/10/31 */
/*
 * THIS FILE IS A COPY OF ...
 *	pcc_types.h 1.1 86/08/19 SMI
 * and really should be dependent on it!!!
 */

# ifndef _pcc_types_
# define _pcc_types_

/*
 * Note on type encodings in the Portable C compiler:
 *
 * They are divided into a basic type (char, short, int, long, etc.)
 * and qualifications on those basic types (pointer, function, array).
 * The basic type is kept in the low 6 bits of the type descriptor,
 * and the qualifications are arranged in two bit fields, with the
 * most significant on the right and the least significant on the
 * left.
 *
 * Example:
 *	the C declaration "int *foo();" is interpreted as
 *
 *	    FTN PTR INT
 * 	    (a function returning a pointer to an integer)
 *
 * 	and stored as
 *				     
 *	    -----------------------------
 * 	    |    0    |<ptr>|<ftn>|<int>|
 *	    -----------------------------
 *				     ^-basic type
 *			       ^-------most significant qualifier
 *			 ^-------------next significant qualifier
 *
 * Qualifications are limited to a maximum of 13 levels.
 * NOTE: this encoding is a change relative to the Berkeley 4.3
 * and Sun 3.2 versions.
 *
 * Basic types are divided into the following ranges:
 *	0..15	supported by code generator
 *	16..31	reserved for use by front ends
 *	32..63	reserved for future code generator extensions
 * Note that front ends are responsible for mapping operations on
 * nonstandard types to operations on the supported basic types.
 */

typedef unsigned long	PCC_TWORD;

/*
 *  type names
 */

# define PCC_UNDEF	0
# define PCC_FARG	1
# define PCC_CHAR	2
# define PCC_SHORT	3
# define PCC_INT	4
# define PCC_LONG	5
# define PCC_FLOAT	6
# define PCC_DOUBLE	7
# define PCC_STRTY	8
# define PCC_UNIONTY	9
# define PCC_ENUMTY	10
# define PCC_MOETY	11
# define PCC_UCHAR	12
# define PCC_USHORT	13
# define PCC_UNSIGNED	14
# define PCC_ULONG	15
# define PCC_VOID	16

# define PCC_MINSTDTYPE	PCC_UNDEF	/* min standard basic type */
# define PCC_MAXSTDTYPE	PCC_VOID	/* max standard basic type */
# define PCC_MAXTYPE	31		/* max basic type, period */

# define PCC_STDTYPE(t) ((t)>=PCC_MINSTDTYPE && (t)<=PCC_MAXSTDTYPE)

/*
 * type modifiers
 */

# define PCC_PTR 	0100
# define PCC_FTN 	0200
# define PCC_ARY 	0300

/*
 *  add a most significant type modifier, m, to a type, t
 */
# define PCC_ADDTYPE(t,m) \
	((((t)&~PCC_BTMASK)<<PCC_TSHIFT) | (m) \
	| ((t)&PCC_BTMASK))

/* type packing constants */
 
# define PCC_TMASK	0300		/* most significant qualifier */
# define PCC_BTMASK	077		/* basic type */
# define PCC_BTSHIFT	6		/* width of basic type field */
# define PCC_TSHIFT	2		/* width of qualifiers */

/* operations on types */

# define PCC_MODTYPE(x,y) ((x)=((x)&(~PCC_BTMASK))|(y)) /* set x's basic type */
# define PCC_BTYPE(x) ((x)&PCC_BTMASK)  /* basic type of x */
# define PCC_UNSIGNABLE(x) ((x)<=PCC_LONG&&(x)>=PCC_CHAR)
# define PCC_ENUNSIGN(x) ((x)+(PCC_UNSIGNED-PCC_INT))
# define PCC_DEUNSIGN(x) ((x)+(PCC_INT-PCC_UNSIGNED))
# define PCC_ISPTR(x) (((x)&PCC_TMASK)==PCC_PTR)
# define PCC_ISFTN(x) (((x)&PCC_TMASK)==PCC_FTN)
# define PCC_ISARY(x) (((x)&PCC_TMASK)==PCC_ARY)
# define PCC_INCREF(x) PCC_ADDTYPE((x),PCC_PTR)
# define PCC_DECREF(x) ((((x)>>PCC_TSHIFT)&~PCC_BTMASK)|( (x)&PCC_BTMASK))

# define PCC_ISUNSIGNED(x) ((x)>=PCC_UCHAR && (x)<=PCC_ULONG)
# define PCC_ISINTEGRAL(x) ((x)>=PCC_CHAR && (x)<=PCC_LONG || PCC_ISUNSIGNED(x))
# define PCC_ISFLOATING(x) ((x)==PCC_FLOAT || (x)==PCC_DOUBLE)
# define PCC_ISCHAR(x)     ((x)==PCC_CHAR || (x)==PCC_UCHAR)

/* pack and unpack field descriptors (size and offset) */

# define PCC_PKFIELD(s,o) (( (o)<<6)| (s) )
# define PCC_UPKFSZ(v)  ( (v) &077)
# define PCC_UPKFOFF(v) ( (v) >>6)

# endif _pcc_types_

