/* LINTLIBRARY */

/*	@(#)cb_portability.h 1.1 94/10/31 SMI	*/

/*
 *	Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.
 *	Sun considers its source code as an unpublished, proprietary
 *	trade secret, and it is available only under strict license
 *	provisions.  This copyright notice is placed here only to protect
 *	Sun in the event the source is deemed a published work.  Dissassembly,
 *	decompilation, or other means of reducing the object code to human
 *	readable form is prohibited by the license agreement under which
 *	this code is provided to the user or company in possession of this
 *	copy.
 *	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *	Government is subject to restrictions as set forth in subparagraph
 *	(c)(1)(ii) of the Rights in Technical Data and Computer Software
 *	clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *	NASA FAR Supplement.
 */


/*
 *	This file defines some macros that are used to make code portable
 *	between the major C dialects. These currently include:
 *		Sun C (K&R C)
 *		ANSI C
 *		C++
 */

/*
 *	The following things should be defined for each compiler
 *		PORT_EXTERN(decl)
 *			This macro declares a C extern function. For C++
 *			it will use a special notation to get the
 *			linkage right.
 *
 *		PORT_0ARG
 *			A macro that indicates that this function takes
 *			no arguments.
 *
 *		PORT_1ARG(a1)
 *		PORT_2ARG(a1, a2)
 *		PORT_3ARG(a1, a2, a3)
 *		PORT_4ARG(a1, a2, a3, a4)
 *		PORT_5ARG(a1, a2, a3, a4, a5)
 *		PORT_6ARG(a1, a2, a3, a4, a5, a6)
 *		PORT_7ARG(a1, a2, a3, a4, a5, a6, a7)
 *		PORT_8ARG(a1, a2, a3, a4, a5, a6, a7, a8)
 *		PORT_9ARG(a1, a2, a3, a4, a5, a6, a7, a8, a9)
 *			A group of macros that are used to hide prototype
 *			lists in extern declaration from Sun C.
 *
 *		PORT_MANY_ARG
 *			A macro that indicates any number of args
 */

#ifndef cb_portability_h_INCLUDED
#define cb_portability_h_INCLUDED

#if defined(SUN_C) || (!defined(__STDC__) && !defined(c_plusplus))
/*
 * Definitions for Sun C
 */
#define PORT_EXTERN(decl) decl

#define PORT_0ARG
#define PORT_1ARG(a1)
#define PORT_2ARG(a1, a2)
#define PORT_3ARG(a1, a2, a3)
#define PORT_4ARG(a1, a2, a3, a4)
#define PORT_5ARG(a1, a2, a3, a4, a5)
#define PORT_6ARG(a1, a2, a3, a4, a5, a6)
#define PORT_7ARG(a1, a2, a3, a4, a5, a6, a7)
#define PORT_8ARG(a1, a2, a3, a4, a5, a6, a7, a8)
#define PORT_9ARG(a1, a2, a3, a4, a5, a6, a7, a8, a9)
#define PORT_MANY_ARG

#elif defined(__STDC__)
/*
 * Definitions for ANSI C
 */
#define PORT_EXTERN(decl) decl

#define PORT_0ARG void
#define PORT_1ARG(a1) a1
#define PORT_2ARG(a1, a2) a1,a2
#define PORT_3ARG(a1, a2, a3) a1,a2,a3
#define PORT_4ARG(a1, a2, a3, a4) a1,a2,a3,a4
#define PORT_5ARG(a1, a2, a3, a4, a5) a1,a2,a3,a4,a5
#define PORT_6ARG(a1, a2, a3, a4, a5, a6) a1,a2,a3,a4,a5,a6
#define PORT_7ARG(a1, a2, a3, a4, a5, a6, a7) a1,a2,a3,a4,a5,a6,a7
#define PORT_8ARG(a1, a2, a3, a4, a5, a6, a7, a8) a1,a2,a3,a4,a5,a6,a7,a8
#define PORT_9ARG(a1, a2, a3, a4, a5, a6, a7, a8, a9)a1,a2,a3,a4,a5,a6,a7,a8,a9
#define PORT_MANY_ARG ...

#elif defined(c_plusplus)
/*
 * Definitions for C++
 */
#define PORT_EXTERN(decl) decl

#define PORT_0ARG void
#define PORT_1ARG(a1) a1
#define PORT_2ARG(a1, a2) a1,a2
#define PORT_3ARG(a1, a2, a3) a1,a2,a3
#define PORT_4ARG(a1, a2, a3, a4) a1,a2,a3,a4
#define PORT_5ARG(a1, a2, a3, a4, a5) a1,a2,a3,a4,a5
#define PORT_6ARG(a1, a2, a3, a4, a5, a6) a1,a2,a3,a4,a5,a6
#define PORT_7ARG(a1, a2, a3, a4, a5, a6, a7) a1,a2,a3,a4,a5,a6,a7
#define PORT_8ARG(a1, a2, a3, a4, a5, a6, a7, a8) a1,a2,a3,a4,a5,a6,a7,a8
#define PORT_9ARG(a1, a2, a3, a4, a5, a6, a7, a8, a9)a1,a2,a3,a4,a5,a6,a7,a8,a9
#define PORT_MANY_ARG ...

#endif

#endif
