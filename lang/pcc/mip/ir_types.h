/*	@(#)ir_types.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# ifndef _ir_types_
# define _ir_types_
# include "pcc_types.h"

/*
 * IR extensions to the PCC type representation --
 *	These are common to iropt, f77pass1, and cgrdr, and
 *	should therefore replace the "same, but slightly different"
 *	definitions that prevail in these three places.
 */

# define IR_BOOL	(PCC_MAXSTDTYPE+1)
# define IR_EXTENDEDF	(IR_BOOL+1)
# define IR_COMPLEX	(IR_EXTENDEDF+1)
# define IR_DCOMPLEX	(IR_COMPLEX+1)
# define IR_STRING	(IR_DCOMPLEX+1)
# define IR_LABELNO	(IR_STRING+1)
# define IR_BITFIELD	(IR_LABELNO+1)

# define IR_FIRSTTYPE	IR_BOOL
# define IR_LASTTYPE	IR_BITFIELD
 
/*
 * macros to classify IR types -- note that in IR, unlike PCC, a CHAR
 * is not a small INT, and ISCHAR(x) is true for both CHARs and STRINGs
 * (FORTRAN hangover)
 */

# define IR_ISBOOL(x)	   ((x)==IR_BOOL)
# define IR_ISINT(x)	   (PCC_ISINTEGRAL(x) && !PCC_ISCHAR(x))
# define IR_ISCHAR(x)	   (PCC_ISCHAR(x) || (x)==IR_STRING)
# define IR_ISEXTENDEDF(x) ((x)==IR_EXTENDEDF)
# define IR_ISCOMPLEX(x)   ((x)==IR_COMPLEX || (x)==IR_DCOMPLEX)
# define IR_ISREAL(x)	   (PCC_ISFLOATING(x) || (x)==IR_EXTENDEDF)
# define IR_ISFTN(x)	   (PCC_ISFTN(x))
# define IR_ISPTRFTN(x)    (PCC_ISPTR(x) && PCC_ISFTN(PCC_DECREF(x)))

# define ir_set_type(x,t,sz,al)\
	{ (x).tword = (t); (x).size = (sz); (x).align = (al); }

# endif _ir_types_
