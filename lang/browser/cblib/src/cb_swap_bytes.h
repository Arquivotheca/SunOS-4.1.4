/* LINTLIBRARY */

/*	@(#)cb_swap_bytes.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_swap_bytes_h_INCLUDED
#define cb_swap_bytes_h_INCLUDED

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

/*
 * Macros to deal with byte-swapping on little-endian machines:
 */
#ifdef sun386

#define	CB_SWAP
#define cb_swap(field)	\
	if (sizeof(field) == sizeof(short)) { \
		cb_swap_short(field); \
	} else if (sizeof(field) == sizeof(int)) { \
		cb_swap_int(field); \
	}

#define	cb_swap_int(cell) \
	{ \
	register unsigned char	temp; \
	temp = ((unsigned char *)&(cell))[0]; \
	((unsigned char *)&(cell))[0] = ((unsigned char *)&(cell))[3]; \
	((unsigned char *)&(cell))[3] = temp; \
	temp = ((unsigned char *)&(cell))[1]; \
	((unsigned char *)&(cell))[1] = ((unsigned char *)&(cell))[2]; \
	((unsigned char *)&(cell))[2] = temp; \
	}

#define cb_swap_short(cell) \
	{ \
	register unsigned char	temp; \
	temp = ((unsigned char *)&(cell))[0]; \
	((unsigned char *)&(cell))[0] = ((unsigned char *)&(cell))[1]; \
	((unsigned char *)&(cell))[1] = temp; \
	}


PORT_EXTERN(void	cb_swap_int_func(PORT_1ARG(int)));
PORT_EXTERN(void	cb_swap_short_func(PORT_1ARG(short)));

#else

#define	cb_swap(field)
#define cb_swap_int(address)
#define	cb_swap_short(address)

#endif

#endif
