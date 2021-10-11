	.data
	.asciz	"@(#)mem_veccode.s 1.1 94/10/31 Copyr 1983 Sun Micro"
	.even
	.text

/*
 *	Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Code tables for memory vector op code generator.
 */

/*
 * OPS are:
 *	0000	PIX_CLR 
 *	0001	PIX_NOT(PIX_SRC|PIX_DST)
 *	0010	PIX_NOT(PIX_SRC)&PIX_DST
 *	0011	PIX_NOT(PIX_SRC)
 *	0100	PIX_SRC&PIX_NOT(PIX_DST)
 *	0101	PIX_NOT(PIX_DST)
 *	0110	PIX_SRC^PIX_DST
 *	0111	PIX_NOT(PIX_SRC&PIX_DST)
 *	1000	PIX_SRC&PIX_DST
 *	1001	PIX_NOT(PIX_SRC^PIX_DST)
 *	1010	PIX_DST
 *	1011	PIX_NOT(PIX_SRC)|PIX_DST
 *	1100	PIX_SRC
 *	1101	PIX_SRC|PIX_NOT(PIX_DST)
 *	1110	PIX_SRC|PIX_DST
 *	1111	PIX_SET
 */

/*
 * Table for 16 ops with (dst1 = a5@+) or without (dst1 = a5@) auto increment
 * of destination addr.  Do not autoincrement dst or src!
 */
#define mem_code(d, src, dst, dst1)					\
	.long	md1d/**/d/**/0, md1d/**/d/**/1, md1d/**/d/**/2, md1d/**/d/**/3;\
	.long	md1d/**/d/**/4, md1d/**/d/**/5, md1d/**/d/**/6, md1d/**/d/**/7;\
	.long	md1d/**/d/**/8, md1d/**/d/**/9, md1d/**/d/**/a, md1d/**/d/**/b;\
	.long	md1d/**/d/**/c, md1d/**/d/**/d, md1d/**/d/**/e, md1d/**/d/**/f;\
	.long	md1d/**/d/**/end;					\
/* 0 */									\
md1d/**/d/**/0:								\
	clrb	dst1;							\
/* !(s|d) */								\
md1d/**/d/**/1:								\
	orb	src,dst;						\
	notb	dst1;							\
/* !s&d */								\
md1d/**/d/**/2:								\
	movb	src,d1;							\
	notb	d1;							\
	andb	d1,dst1;						\
/* !s */								\
md1d/**/d/**/3:								\
	movb	src,dst;						\
	notb	dst1;							\
/* s&!d */								\
md1d/**/d/**/4:								\
	notb	dst;							\
	andb	src,dst1;						\
/* !d */								\
md1d/**/d/**/5:								\
	notb	dst1;							\
/* s^d */								\
md1d/**/d/**/6:								\
	eorb	src,dst1;						\
/* !(s&d) */								\
md1d/**/d/**/7:								\
	andb	src,dst;						\
	notb	dst1;							\
/* s&d */								\
md1d/**/d/**/8:								\
	andb	src,dst1;						\
/* !s^d */								\
md1d/**/d/**/9:								\
	eorb	src,dst;						\
	notb	dst1;							\
/* d */									\
md1d/**/d/**/a:								\
	tstb	dst1;							\
/* !s|d */								\
md1d/**/d/**/b:								\
	movb	src,d1;							\
	notb	d1;							\
	orb	d1,dst1;						\
/* s */									\
md1d/**/d/**/c:								\
	movb	src,dst1;						\
/* s|!d */								\
md1d/**/d/**/d:								\
	notb	dst;							\
	orb	src,dst1;						\
/* s|d */								\
md1d/**/d/**/e:								\
	orb	src,dst1;						\
/* 1 */									\
md1d/**/d/**/f:								\
	movb	#-1,dst1;						\
md1d/**/d/**/end:

/*
 * Generate two tables 
 */

starthere:

	.globl	_mem_vec_majorx
_mem_vec_majorx:
mem_code(i, d3, a5@, a5@+)

	.globl	_mem_vec_majory
_mem_vec_majory:
mem_code(d, d3, a5@, a5@)

/*
 * Table of miscellaneous vector drawing instructions
 */
	.globl	_mem_addla4a5, _mem_addwd6d4, _mem_addql1a5, _mem_subwd7d4,
	.globl	_mem_tstwd5, _mem_dbgtd5, _mem_dbltd5, _mem_dbfd6,
	.globl	_mem_dbfd7, _mem_vec_rts
_mem_addla4a5:	addl a4,a5
_mem_addwd6d4:	addw d6,d4
_mem_addql1a5:	addql #1,a5
_mem_subwd7d4:	subw d7,d4
_mem_tstwd5:	tstw d5
_mem_dbgtd5:	dbgt d5,starthere
_mem_dbltd5:	dblt d5,starthere
_mem_dbfd6:	dbf d6,starthere
_mem_dbfd7:	dbf d7,starthere
_mem_vec_rts:	rts
