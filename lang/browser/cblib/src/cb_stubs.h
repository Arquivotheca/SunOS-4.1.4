/* LINTLIBRARY */

/*	@(#)cb_stubs.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_stubs_h_INCLUDED
#define cb_stubs_h_INCLUDED

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

extern	int	cb_abort_query;

PORT_EXTERN(int	qr_starting_update(PORT_0ARG));
PORT_EXTERN(int	qr_state_file_locked(PORT_0ARG));
PORT_EXTERN(int	qr_message_no_database(PORT_0ARG));
PORT_EXTERN(int	cb_message(PORT_2ARG(char*, PORT_MANY_ARG)));

#endif
