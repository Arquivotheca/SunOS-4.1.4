#ident "@(#)cb_graph.c 1.1 94/10/31"

#ifndef lint
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
Sun considers its source code as an unpublished, proprietary \
trade secret, and it is available only under strict license \
provisions.  This copyright notice is placed here only to protect \
Sun in the event the source is deemed a published work.  Dissassembly, \
decompilation, or other means of reducing the object code to human \
readable form is prohibited by the license agreement under which \
this code is provided to the user or company in possession of this \
copy. \
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the \
Government is subject to restrictions as set forth in subparagraph \
(c)(1)(ii) of the Rights in Technical Data and Computer Software \
clause at DFARS 52.227-7013 and in similar clauses in the FAR and \
NASA FAR Supplement.";
#endif

#ifdef INCLUDE_COPYRIGHT_REFERENCE
static char _copyright_notice_reference_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
See copyright notice in cb_copyright.o in the libsb.a library.";
#endif
#endif

#ifndef cb_graph_h_INCLUDED
#include "cb_graph.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	char		*cb_graph_node_head();

/*
 *	cb_graph_node_head(name)
 *		Construct the header part of node names for the source file
 */
char *
cb_graph_node_head(name)
	char		*name;
{
	int		node_hash;
	char		*p;
	char		head[MAXPATHLEN];
	char		*tail;

	if ((tail = strrchr(name, '/')) == NULL) {
		(void)strcpy(head, name);
	} else {
		*tail = 0;
		for (p = name, node_hash = 0; *p != 0; p++) {
			node_hash += *p;
		}
		(void)sprintf(head, "#%x/%s", node_hash, tail + 1);
		*tail = '/';
	}

	return cb_string_unique(head);
}
