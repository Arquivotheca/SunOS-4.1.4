/* Copyright (c) 1979 Regents of the University of California */

/* static	char sccsid[] = "@(#)whoami.h 1.1 10/31/94"; */

/*
 *	am i generating an obj file (OBJ),
 *	postfix binary input to the 2nd pass of the portable c compiler (PC),
 *	or pTrees (PTREE)?
 */
#undef	OBJ
#define	PC
#undef	PTREE

/*
 *	we assume one of the following will be defined by the preprocessor:
 *	vax	for vaxes
 *	pdp11	for pdp11's
 *	mc68000	for motorola mc68000's
 *	sparc	for SPARC
 *	i386	for Intel iAPX386
 *	z8000	for Zilog Z8000
 */

/*
 *	hardware characteristics:
 *	address size (16 or 32 bits) and byte ordering (normal or dec11 family).
 */
#if defined(vax) || defined(i386)
#   undef	ADDR16
#   define	ADDR32
#   define	DEC11
#endif vax || i386
#ifdef pdp11
#   define	ADDR16
#   undef	ADDR32
#   define	DEC11
#endif pdp11
#if defined(mc68000) || defined(sparc)
#   undef	ADDR16
#   define	ADDR32
#   undef	DEC11
#endif mc68000 || sparc
#ifdef z8000
#   define	ADDR16
#   undef	ADDR32
#   undef	DEC11
#endif z8000

/*
 *	am i pi or pxp?
 */
#define PI
#undef	PXP

/*
 *	am i both passes, or am i only one of the two passes pi0 or pi1?
 */
#define	PI01
#undef	PI0
#undef	PI1
