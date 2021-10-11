/* LINTLIBRARY */

/*	@(#)cb_directory.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_directory_h_INCLUDED
#define cb_directory_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

/*
 * There is a recursive dependency between cb_qr.h and cb_directory.h
 * therefore this typedef is sitting here
 */
typedef	struct Cb_qr_read_tag		*Cb_qr_read, Cb_qr_read_rec;
typedef	struct Cb_init_tag		*Cb_init, Cb_init_rec;

typedef struct Cb_directory_tag	*Cb_directory, Cb_directory_rec;

/*
 *	Quickref and cb files live in directories.
 */
struct Cb_directory_tag {
	Cb_heap		heap;
	char		*name;		/* Absolute directory path */
	Cb_qr_read	qr_read;	/* Quickref file object */
	Cb_init		init;
	Array		refd_files;
	Array		old_files;
	Array		new_files;
	int		refd_read:1;
	int		root_read:1;
	int		refd_marked:1;
	int		root_marked:1;
	int		split_happened:1;
	int		is_local_dir:1;
};


PORT_EXTERN(Cb_directory	cb_directory_create(PORT_1ARG(char*)));
PORT_EXTERN(Cb_directory	cb_directory_home(PORT_0ARG));
PORT_EXTERN(char		*cb_directory_home_path(PORT_0ARG));
PORT_EXTERN(void		cb_directory_flush(PORT_0ARG));
PORT_EXTERN(void		cb_directory_resynch(PORT_0ARG));

#endif
