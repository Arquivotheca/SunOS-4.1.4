/* LINTLIBRARY */

/*	@(#)cb_string.h 1.1 94/10/31 SMI	*/

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

#ifndef cb_string_h_INCLUDED
#define cb_string_h_INCLUDED

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

PORT_EXTERN(char	*cb_string_cat(PORT_1ARG(PORT_MANY_ARG)));
PORT_EXTERN(char	*cb_string_copy(PORT_2ARG(char*, Cb_heap)));
PORT_EXTERN(char	*cb_string_copy_n(PORT_3ARG(char*, int, Cb_heap)));
PORT_EXTERN(char	*cb_string_unique(PORT_1ARG(char*)));
PORT_EXTERN(char	*cb_string_unique_n(PORT_2ARG(char*, int)));

#endif
