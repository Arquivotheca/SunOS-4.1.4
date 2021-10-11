/* @(#)pr_shlib_stub.h 1.1 94/10/31 SMI */

/*
 * Copyright 1987-1989 by Sun Microsystems,  Inc.
 */

#ifndef	pr_shlib_stub_DEFINED
#define	pr_shlib_stub_DEFINED

/* special shared library entry point stubs */

#if mc68000
#define	STUB(fun, args) _STUB(_/**/fun,__/**/fun)
#define	_STUB(label,target) \
	asm(".text"); \
	asm(".globl label"); \
	asm("label:"); \
	asm("movl	#__GLOBAL_OFFSET_TABLE_, a0"); \
	asm("lea	pc@(0,a0:l), a0"); \
	asm("movl	a0@(target), a0"); \
	asm("jmp	a0@");
#endif mc68000

#if sparc
#define	STUB(fun, args) _STUB(_/**/fun,__/**/fun)
#define	_STUB(label,target) \
	asm(".seg \"text\""); \
	asm(".global label"); \
	asm("label:"); \
	asm("mov %o7, %g1"); \
	asm("call target"); \
	asm("mov %g1, %o7");
#endif sparc

#ifndef STUB
#define	STUB(fun,args) \
	fun args { \
		extern _/**/fun(); \
		return _/**/fun args ; \
	}
#endif	!STUB

#define	ROPSTUB(name) STUB(name,(dpr, dx, dy, dw, dh, op, spr, sx, sy))

#endif	pr_shlib_stub_DEFINED
