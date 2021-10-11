/* @(#) low.h 1.1 94/10/31 */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __LOW_H__
#define	__LOW_H__

#define	LENTRY(name)			\
	.global ___/**/name;		\
___/**/name:

#endif __LOW_H__
