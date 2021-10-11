/* LINTLIBRARY */

/*	@(#)cb_libc.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_libc_h_INCLUDED
#define cb_libc_h_INCLUDED

#include <alloca.h>

#ifndef isalpha
#include <ctype.h>
#endif

#include <errno.h>

#ifndef FILE
#include <stdio.h>
#endif

#include <string.h>

#ifndef va_dcl
#include <varargs.h>
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif



/*
 *	externs for library routines
 */

extern	char		*sys_errlist[];
extern	int		sys_nerr;
extern	int		errno;

PORT_EXTERN(char	*alloca(PORT_1ARG(int)));
PORT_EXTERN(char	*getenv(PORT_1ARG(char*)));
PORT_EXTERN(long	gethostid(PORT_0ARG));
PORT_EXTERN(char	*getwd(PORT_0ARG));
PORT_EXTERN(char	*index(PORT_1ARG(char*)));
PORT_EXTERN(char	*rindex(PORT_1ARG(char*)));

#ifndef c_plusplus
PORT_EXTERN(int		fread(PORT_4ARG(char*, int, int, FILE *)));
PORT_EXTERN(char	*sprintf(PORT_2ARG(char*, PORT_MANY_ARG)));
#endif

#endif
