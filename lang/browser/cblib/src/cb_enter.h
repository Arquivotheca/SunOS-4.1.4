/* LINTLIBRARY */

/*	@(#)cb_enter.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_enter_h_INCLUDED
#define cb_enter_h_INCLUDED

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif



#define cb_enter_get_cb_file() cb_enter_cb_file 
#define cb_enter_set_cb_file(file) cb_enter_cb_file = (file)
#define cb_enter_get_file_number() \
	((cb_enter_cb_file == NULL) ? -1 :cb_enter_cb_file->file_number)


extern	Cb_file_write	cb_enter_cb_file;
extern	int		cb_flag;
extern	int		*cb_lineno_addr;
extern	Array		cb_enter_function_names;

PORT_EXTERN(void		cb_enter_start(PORT_4ARG(int*,
							 char*,
							 int,
							 int)));
PORT_EXTERN(void		cb_enter_push_file(PORT_2ARG(char*, int)));
PORT_EXTERN(void		cb_enter_pop_file(PORT_0ARG));
PORT_EXTERN(Cb_file_write	cb_enter_pop_file_delayed(PORT_0ARG));
PORT_EXTERN(Cb_semtab_if	cb_enter_symbol(PORT_3ARG(char*,
							  int,
							  unsigned)));
PORT_EXTERN(Cb_semtab_if	cb_enter_symbol_in_file(PORT_4ARG(char*,
								  int,
								  unsigned,
								  int)));
PORT_EXTERN(Cb_semtab_if	cb_enter_unique_symbol(PORT_3ARG(char*,
							 int,
							 unsigned int)));
PORT_EXTERN(void		cb_enter_line_id_block(PORT_2ARG(Cb_line_id,
								 int)));
PORT_EXTERN(void		cb_enter_inactive_lines(PORT_2ARG(int, int)));
PORT_EXTERN(void		cb_enter_function_start(PORT_2ARG(char*,
								  int)));
PORT_EXTERN(void		cb_enter_function_end(PORT_1ARG(int)));
PORT_EXTERN(void		cb_enter_pound_line_seen(PORT_0ARG));
PORT_EXTERN(void		cb_enter_case_was_folded(PORT_0ARG));
PORT_EXTERN(void		cb_enter_set_multiple(PORT_0ARG));
PORT_EXTERN(void		cb_enter_adjust_tag(PORT_3ARG(int,
							      Cb_semtab_if,
							      unsigned)));

#endif
