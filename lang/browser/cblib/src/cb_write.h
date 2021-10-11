/* LINTLIBRARY */

/*	@(#)cb_write.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_write_h_INCLUDED
#define cb_write_h_INCLUDED

#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif

typedef struct Cb_refd_dir_tag		*Cb_refd_dir, Cb_refd_dir_rec;

struct Cb_refd_dir_tag {
	Cb_directory	dir;
	short		sub_dir;
};


PORT_EXTERN(void	cb_write_cb_file(PORT_6ARG(Cb_file_write,
						   int,
						   char*,
						   Cb_source_type,
						   int,
						   int)));
PORT_EXTERN(void	cb_do_callback(PORT_1ARG(int)));
PORT_EXTERN(Cb_refd_dir	cb_directory_find_file());
PORT_EXTERN(void	cb_directory_mark());

#endif
