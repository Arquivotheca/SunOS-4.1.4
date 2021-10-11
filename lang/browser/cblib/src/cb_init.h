/*LINTLIBRARY*/

/* @(#)cb_init.h 1.1 94/10/31 SMI */

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



#ifndef cb_init_h_INCLUDED
#define cb_init_h_INCLUDED

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_literals_h_INCLUDED
#include "cb_literals.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif


#define CB_INIT_ENV_VAR			"SUN_SOURCE_BROWSER_INIT_FILE"

typedef struct Cb_import_tag	*Cb_import, Cb_import_rec;
typedef struct Cb_partition_tag	*Cb_partition, Cb_partition_rec;

/* Structure representing an import command: */
struct Cb_import_tag {
	Cb_directory	dir;
	int		parent:1;	/* Parent of Parent/child database */
	int		added_parent:1;
};

/* Structure representing a partition command: */
struct Cb_partition_tag {
	Cb_directory	dir;
	char		*prefix;	/* Prefix to match */
	int		prefix_length;	/* Prefix length */
	short		new_root_made;	/* TRUE => .cb/NewRoot dir. made */
	short		write_failed;
};

struct Cb_init_tag {
	Array		imports;	/* Imports list */
	Array		partitions;	/* Partitions list */
	time_t		timestamp;	/* .cbinit file timestamp */
	int		resynch;	/* Resynchronization counter */
	int		split;		/* Index size before split */
};

PORT_EXTERN(void		cb_init_read(PORT_1ARG(Cb_directory)));
PORT_EXTERN(void		cb_init_resynch(PORT_0ARG));
PORT_EXTERN(void		cb_init_add_parent(PORT_2ARG(Cb_init,
							     Cb_import)));
#endif
