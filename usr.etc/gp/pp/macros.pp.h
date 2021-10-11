/*	@(#)macros.pp.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* Painting processor macros.
*/

#define IDENT(a)	a
#define CAT(a,b)	IDENT(a)b

/* Instruction aliases. */
#define decw	sub2nw 0,
#define decws	sub2nw,s 0,
#define testw(x)	movw,s x, y
#define IMM
    /* For code readability.  Means that 29116 field is occupied by a value. */
#define gbr	zbr
#define gbrd	zbrd
#define gbloptr	zbloptr
#define gbhiptr	zbhiptr
#define gbwdreg zbwdreg
#define gbrdreg	zbrdreg
