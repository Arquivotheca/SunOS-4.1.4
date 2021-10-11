/*	@(#) pcc_types.h 1.1 94/10/31 SMI */

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

# define PCC_UNDEF	((PCC_TWORD)0)
# define PCC_FARG	((PCC_TWORD)1)
# define PCC_CHAR	((PCC_TWORD)2)
# define PCC_SHORT	((PCC_TWORD)3)
# define PCC_INT	((PCC_TWORD)4)
# define PCC_LONG	((PCC_TWORD)5)
# define PCC_FLOAT	((PCC_TWORD)6)
# define PCC_DOUBLE	((PCC_TWORD)7)
# define PCC_STRTY	((PCC_TWORD)8)
# define PCC_UNIONTY	((PCC_TWORD)9)
# define PCC_ENUMTY	((PCC_TWORD)10)
# define PCC_MOETY	((PCC_TWORD)11)
# define PCC_UCHAR	((PCC_TWORD)12)
# define PCC_USHORT	((PCC_TWORD)13)
# define PCC_UNSIGNED	((PCC_TWORD)14)
# define PCC_ULONG	((PCC_TWORD)15)
# define PCC_TVOID	((PCC_TWORD)16)

# define PCC_MINSTDTYPE	PCC_UNDEF	/* min standard basic type */
# define PCC_MAXSTDTYPE	PCC_TVOID	/* max standard basic type */
# define PCC_MAXTYPE	((PCC_TWORD)31)	/* max basic type, period */

# define PCC_STDTYPE(t) ((unsigned)(t)<=PCC_MAXSTDTYPE)

/*
 * type modifiers
 */

# define PCC_PTR 	((PCC_TWORD)0100)
# define PCC_FTN 	((PCC_TWORD)0200)
# define PCC_ARY 	((PCC_TWORD)0300)

/*
 *  add a most significant type modifier, m, to a type, t
 */
# define PCC_ADDTYPE(t,m) \
	((((t)&~PCC_BTMASK)<<PCC_TSHIFT) | (m) \
	| ((t)&PCC_BTMASK))

/* type packing constants */
 
# define PCC_TMASK	((PCC_TWORD)0300)	/* most significant qualifier */
# define PCC_BTMASK	((PCC_TWORD)077)	/* basic type */
# define PCC_BTSHIFT	((PCC_TWORD)6)		/* width of basic type field */
# define PCC_TSHIFT	((PCC_TWORD)2)		/* width of qualifiers */

/* operations on types */

# define PCC_MODTYPE(x,y) ((x)=((x)&(~PCC_BTMASK))|(y)) /* set x's basic type */
# define PCC_BTYPE(x) ((x)&PCC_BTMASK)  /* basic type of x */
# define PCC_UNSIGNABLE(x) ((x)<=PCC_LONG&&(x)>=PCC_CHAR)
# define PCC_ENUNSIGN(x) ((x)+(PCC_UNSIGNED-PCC_INT))
# define PCC_DEUNSIGN(x) ((x)+(PCC_INT-PCC_UNSIGNED))
# define PCC_ISPTR(x) (((x)&PCC_TMASK)==PCC_PTR)
# define PCC_ISFTN(x) (((x)&PCC_TMASK)==PCC_FTN)
# define PCC_TVOIDPTR(x) ((x) == (PCC_PTR|PCC_TVOID)) /* (void*) */
# define PCC_ISARY(x) (((x)&PCC_TMASK)==PCC_ARY)
# define PCC_INCREF(x) PCC_ADDTYPE((x),PCC_PTR)
# define PCC_DECREF(x) ((((x)>>PCC_TSHIFT)&~PCC_BTMASK)|( (x)&PCC_BTMASK))

# define PCC_ISUNSIGNED(x) ((x)>=PCC_UCHAR && (x)<=PCC_ULONG)
# define PCC_ISINTEGRAL(x) ((x)>=PCC_CHAR && (x)<=PCC_LONG || PCC_ISUNSIGNED(x))
# define PCC_ISFLOATING(x) ((x)==PCC_FLOAT || (x)==PCC_DOUBLE)
# define PCC_ISCHAR(x)     ((x)==PCC_CHAR || (x)==PCC_UCHAR)

/* pack and unpack field descriptors (size and offset) */

# define PCC_PKFIELD(s,o) (( (o)<<6)|(s))
# define PCC_UPKFSZ(v)  ( (v) &077)
# define PCC_UPKFOFF(v) ( (v) >>6)

# endif _pcc_types_

