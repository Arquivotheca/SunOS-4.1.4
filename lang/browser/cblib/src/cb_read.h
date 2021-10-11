/* LINTLIBRARY */

/*	@(#)cb_read.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_read_h_INCLUDED
#define cb_read_h_INCLUDED

#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif


PORT_EXTERN(Cb_file_read	cb_read_open(PORT_2ARG(Cb_file, int)));
PORT_EXTERN(void		cb_read_close(PORT_1ARG(Cb_file_read)));
PORT_EXTERN(Cb_header		cb_read_header_section(
					PORT_1ARG(Cb_file_read)));
PORT_EXTERN(Cb_src_file_im_rec	cb_read_src_name_section(
					PORT_1ARG(Cb_file_read)));
PORT_EXTERN(Cb_read_refd_rec	cb_read_reset_refd_file(
					PORT_2ARG(Cb_file_read,
						  Cb_read_refd)));
PORT_EXTERN(Cb_refd_file_if	cb_read_next_refd_file(
					PORT_1ARG(Cb_file_read)));
PORT_EXTERN(Cb_read_symtab_rec	cb_read_reset_symbol(
					PORT_2ARG(Cb_file_read,
						  Cb_read_symtab)));
PORT_EXTERN(Cb_symtab_if	cb_read_next_symbol(PORT_1ARG(Cb_file_read)));
PORT_EXTERN(void		cb_read_reset_semtab(
					PORT_2ARG(Cb_file_read, unsigned)));
PORT_EXTERN(Cb_semtab_if	cb_read_next_semantic_record(
					PORT_1ARG(Cb_file_read)));
PORT_EXTERN(Cb_read_line_id_rec	cb_read_reset_line_id(
					PORT_2ARG(Cb_file_read,
						  Cb_read_line_id)));
PORT_EXTERN(Cb_line_id		cb_read_next_line_id(PORT_1ARG(Cb_file_read)));
PORT_EXTERN(Cb_section		cb_find_section(PORT_3ARG(Cb_file_read,
							  Cb_section_id,
							  int)));

#endif
