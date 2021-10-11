/* LINTLIBRARY */

/*	@(#)cb_misc.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_misc_h_INCLUDED
#define cb_misc_h_INCLUDED

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

/*
 *	Utility macros
 */
#define cb_ord(member)		((int)(member))
#define cb_abs(a)		((a) < 0 ? -(a) : (a))
#define cb_align(x,align)	(((int)(x))+((align)-1) & ~((align)-1))

PORT_EXTERN(void	cb_create_dir(PORT_1ARG(char*)));
PORT_EXTERN(void	cb_abort(PORT_2ARG(char*, PORT_MANY_ARG)));
PORT_EXTERN(void	cb_fatal(PORT_2ARG(char*, PORT_MANY_ARG)));
PORT_EXTERN(void	cb_warning(PORT_2ARG(char*, PORT_MANY_ARG)));
PORT_EXTERN(void	cb_free(PORT_1ARG(char*)));
PORT_EXTERN(char	*cb_get_wd(PORT_1ARG(int)));
PORT_EXTERN(int		cb_line_hash_value(PORT_1ARG(char*)));
PORT_EXTERN(void	cb_make_absolute_path(PORT_3ARG(char*, int, char*)));
PORT_EXTERN(void	cb_make_relative_path(PORT_2ARG(char*, char*)));
PORT_EXTERN(time_t	cb_get_timestamp(PORT_0ARG));
PORT_EXTERN(time_t	cb_get_time_of_day(PORT_0ARG));

#endif
