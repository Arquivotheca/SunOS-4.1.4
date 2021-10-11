	.data
	.asciz	"@(#)mem_ropcode.s 1.1 94/10/31 Copyr 1983 Sun Micro"
	.even
	.text

/*
 *	Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Code tables for memory raster op code generator.
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
 * Table for code from intermediate register to full word destination.
 */
#define mem_code(d, dst, dst1, dst2)					\
	.long	md1d/**/d/**/0, md1d/**/d/**/1, md1d/**/d/**/2, md1d/**/d/**/3;\
	.long	md1d/**/d/**/4, md1d/**/d/**/5, md1d/**/d/**/6, md1d/**/d/**/7;\
	.long	md1d/**/d/**/8, md1d/**/d/**/9, md1d/**/d/**/a, md1d/**/d/**/b;\
	.long	md1d/**/d/**/c, md1d/**/d/**/d, md1d/**/d/**/e, md1d/**/d/**/f;\
	.long	md1d/**/d/**/end;					\
/* 0 */									\
md1d/**/d/**/0:								\
	clrw	dst;							\
/* !(s|d) */								\
md1d/**/d/**/1:								\
	orw	dst1,d1;						\
	notw	d1;							\
	movw	d1,dst2;						\
/* !s&d */								\
md1d/**/d/**/2:								\
	notw	d1;							\
	andw	d1,dst;							\
/* !s */								\
md1d/**/d/**/3:								\
	notw	d1;							\
	movw	d1,dst;							\
/* s&!d */								\
md1d/**/d/**/4:								\
	movw	dst1,d0;						\
	notw	d0;							\
	andw	d1,d0;							\
	movw	d0,dst2;						\
/* !d */								\
md1d/**/d/**/5:								\
	notw	dst;							\
/* s^d */								\
md1d/**/d/**/6:								\
	eorw	d1,dst;							\
/* !(s&d) */								\
md1d/**/d/**/7:								\
	andw	dst1,d1;						\
	notw	d1;							\
	movw	d1,dst2;						\
/* s&d */								\
md1d/**/d/**/8:								\
	andw	d1,dst;							\
/* !s^d */								\
md1d/**/d/**/9:								\
	notw	d1;							\
	eorw	d1,dst;							\
/* d */									\
md1d/**/d/**/a:								\
	addql	#2,a5;							\
/* !s|d */								\
md1d/**/d/**/b:								\
	notw	d1;							\
	orw	d1,dst;							\
/* s */									\
md1d/**/d/**/c:								\
	movw	d1,dst;							\
/* s|!d */								\
md1d/**/d/**/d:								\
	movw	dst1,d0;						\
	notw	d0;							\
	orw	d1,d0;							\
	movw	d0,dst2;						\
/* s|d */								\
md1d/**/d/**/e:								\
	orw	d1,dst;							\
/* 1 */									\
md1d/**/d/**/f:								\
	movw	d1,dst;							\
md1d/**/d/**/end:

starthere:

	.globl	_mem_d1_to_dest_incr
_mem_d1_to_dest_incr:
mem_code(i, a5@+, a5@, a5@+)

	.globl	_mem_d1_to_dest_decr
_mem_d1_to_dest_decr:
mem_code(d, a5@-, a5@-, a5@)

/*
 * Tables for rasteroping d1 with d0
 */
#define mem_sd_rop(d, dst, dst1, dst2, dm, src, sm)			\
	.globl	_mem_sd_rop_/**/d;					\
_mem_sd_rop_/**/d:							\
	.long	d/**/sd0,d/**/sd1,d/**/sd2,d/**/sd3,d/**/sd4,d/**/sd5;	\
	.long	d/**/sd6,d/**/sd7,d/**/sd8,d/**/sd9,d/**/sda,d/**/sdb;	\
	.long	d/**/sdc,d/**/sdd,d/**/sde,d/**/sdf,d/**/sdend;		\
/* 0 */									\
d/**/sd0:								\
	andw	dm,dst;							\
/* !(s|d) */								\
d/**/sd1:								\
	andw	sm,src;							\
	movw	dst1,d0;						\
	orw	src,d0;							\
	eorw	sm,d0;							\
	movw	d0,dst2;						\
/* !s&d */								\
d/**/sd2:								\
	notw	src;							\
	orw	dm,src;							\
	andw	src,dst;						\
/* !s */								\
d/**/sd3:								\
	notw	src;							\
	andw	sm,src;							\
	movw	dst1,d0;						\
	andw	dm,d0;							\
	orw	src,d0;							\
	movw	d0,dst2;						\
/* s&!d */								\
d/**/sd4:								\
	orw	dm,src;							\
	movw	dst1,d0;						\
	eorw	sm,d0;							\
	andw	src,d0;							\
	movw	d0,dst2;						\
/* !d	*/								\
d/**/sd5:								\
	eorw sm,dst;							\
/* s^d */								\
d/**/sd6:								\
	andw	sm,src;							\
	eorw	src,dst;						\
/* !(s&d) */								\
d/**/sd7:								\
	orw	dm,src;							\
	movw	dst1,d0;						\
	andw	src,d0;							\
	eorw	sm,d0;							\
	movw	d0,dst2;						\
/* s&d */								\
d/**/sd8:								\
	orw	dm,src;							\
	andw	src,dst;						\
/* !s^d */								\
d/**/sd9:								\
	andw	sm,src;							\
	movw	dst1,d0;						\
	eorw	src,d0;							\
	eorw	sm,d0;							\
	movw	d0,dst2;						\
/* d */									\
d/**/sda:								\
/* !s|d */								\
d/**/sdb:								\
	notw	src;							\
	andw	sm,src;							\
	orw	src,dst;						\
/* s */									\
d/**/sdc:								\
	andw	sm,src;							\
	movw	dst1,d0;						\
	andw	dm,d0;							\
	orw	src,d0;							\
	movw	d0,dst2;						\
/* s|!d */								\
d/**/sdd:								\
	andw	sm,src;							\
	movw	dst1,d0;						\
	eorw	sm,d0;							\
	orw	src,d0;							\
	movw	d0,dst2;						\
/* s|d */								\
d/**/sde:								\
	andw	sm,src;							\
	orw	src,dst;						\
/* 1 */									\
d/**/sdf:								\
	orw	sm,dst;							\
d/**/sdend:

mem_sd_rop(lrl, a5@+, a5@, a5@+, d2, d1, d3)
mem_sd_rop(lrr, a5@+, a5@, a5@+, d4, d1, d5)
mem_sd_rop(rll, a5@-, a5@-, a5@, d2, d1, d3)
mem_sd_rop(rlr, a5@-, a5@-, a5@, d4, d1, d5)

/* Table of miscellaneous opcodes */
	.globl	_mem_addla2a4, _mem_addla3a5, _mem_addql2a4, _mem_andwd0
	.globl	_mem_andwd1, _mem_dbrad6, _mem_dbrad7, _mem_movla4Ma5M
	.globl	_mem_movla4Pa5P, _mem_movla4a5P, _mem_movla4d1, _mem_movla4Md1
	.globl  _mem_movld6, _mem_moveq0d6, _mem_moveqM1d1, _mem_movwa4Ma5M
	.globl  _mem_movwa4Md1, _mem_movwa4Pa5P, _mem_movwa4a5P, _mem_movwa4Pd1
	.globl  _mem_movwa4d1, _mem_movwa5Md0, _mem_movwa5d0, _mem_movwd6
	.globl  _mem_movwd0a5, _mem_movwd0a5P, _mem_orwd1d0, _mem_rolld1
	.globl  _mem_rorld1, _mem_rts, _mem_subql2a4, _mem_swapd1
_mem_addla2a4:	addl a2,a4
_mem_addla3a5:	addl a3,a5
_mem_addql2a4:	addql #2,a4
_mem_andwd0:	andw #0,d0
_mem_andwd1:	andw #0,d1
_mem_dbrad6:	dbra d6,starthere
_mem_dbrad7:	dbra d7,starthere
_mem_movla4Ma5M:	movl a4@-,a5@-
_mem_movla4Md1:	movl a4@-,d1
_mem_movla4Pa5P:	movl a4@+,a5@+
_mem_movla4a5P:	movl a4@,a5@+
_mem_movla4d1:	movl a4@,d1
_mem_movld6:	movl #0,d6
_mem_moveq0d6:	moveq #0,d6
_mem_moveqM1d1:	moveq #-1,d1
_mem_movwa4Ma5M:	movw a4@-,a5@-
_mem_movwa4Md1:	movw a4@-,d1
_mem_movwa4Pa5P:	movw a4@+,a5@+
_mem_movwa4a5P:	movw a4@,a5@+
_mem_movwa4Pd1:	movw a4@+,d1
_mem_movwa4d1:	movw a4@,d1
_mem_movwa5Md0:	movw a5@-,d0
_mem_movwa5d0:	movw a5@,d0
_mem_movwd6:	movw #0,d6
_mem_movwd0a5:	movw d0,a5@
_mem_movwd0a5P:	movw d0,a5@+
_mem_orwd1d0:	orw d1,d0
_mem_rolld1:	roll #8,d1
_mem_rorld1:	rorl #8,d1
_mem_rts:	rts
_mem_subql2a4:	subql #2,a4
_mem_swapd1:	swap d1
