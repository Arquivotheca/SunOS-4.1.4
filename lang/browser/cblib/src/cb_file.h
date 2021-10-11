/* LINTLIBRARY */

/*	@(#)cb_file.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_file_h_INCLUDED
#define cb_file_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
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

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

PORT_EXTERN(void	cb_file2string(PORT_2ARG(Cb_file, char*)));
PORT_EXTERN(int		cb_file_compare(PORT_2ARG(Cb_file, Cb_file)));
PORT_EXTERN(Cb_file	cb_file_create(PORT_4ARG(char*,
						 unsigned,
						 Cb_directory,
						 char*)));
PORT_EXTERN(Cb_file	cb_file_create2(PORT_4ARG(char*,
						 unsigned,
						 Cb_directory,
						 char*)));
PORT_EXTERN(void	cb_file_create2_enter(PORT_1ARG(Cb_file)));
PORT_EXTERN(void	cb_file_delete(PORT_1ARG(Cb_file)));
PORT_EXTERN(void	cb_file_destroy(PORT_0ARG));
PORT_EXTERN(Cb_file	cb_file_extract(PORT_2ARG(char *,
						  Cb_directory)));
PORT_EXTERN(void	cb_file_move(PORT_2ARG(Cb_file, Cb_directory)));
PORT_EXTERN(Hash	cb_file_table_get(PORT_0ARG));
PORT_EXTERN(Cb_file	cb_name2cb_file(PORT_2ARG(char*, Cb_directory)));
PORT_EXTERN(char	*cb_get_sub_dir_string(PORT_1ARG(int)));
PORT_EXTERN(int		cb_hash2string(PORT_2ARG(unsigned, char*)));
PORT_EXTERN(void	cb_src_hash2string(PORT_4ARG(Cb_directory,
						     char*,
						     unsigned,
						     char*)));
PORT_EXTERN(unsigned	cb_string2hash(PORT_1ARG(char*)));

#endif
