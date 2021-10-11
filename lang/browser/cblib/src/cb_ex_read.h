/* LINTLIBRARY */

/*  @(#)cb_ex_read.h 1.1 94/10/31 SMI */

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


#ifndef cb_ex_read_h_INCLUDED
#define cb_ex_read_h_INCLUDED

#ifndef cb_ex_h_INCLUDED
#include "cb_ex.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

/*
 * Some macros for converting .ex file offsets to pointers
 */
#define cb_ex_read_menu_item(ex_file, offset) \
		((Cb_ex_menu_item_if)((int)((ex_file)->buffer) + (offset)))
#define cb_ex_read_string(ex_file, offset) \
		((ex_file)->buffer + (offset))


typedef struct Cb_ex_read_language_tag	*Cb_ex_read_language,
						Cb_ex_read_language_rec;
typedef struct Cb_ex_read_menu_item_tag	*Cb_ex_read_menu_item,
						Cb_ex_read_menu_item_rec;
typedef struct Cb_ex_read_focus_tag	*Cb_ex_read_focus,
						Cb_ex_read_focus_rec;
typedef struct Cb_ex_read_tag_tag	*Cb_ex_read_tag,
						Cb_ex_read_tag_rec;
typedef struct Cb_ex_read_rendering_tag	*Cb_ex_read_rendering,
						Cb_ex_read_rendering_rec;
typedef struct Cb_ex_read_node_menu_tag	*Cb_ex_read_node_menu,
						Cb_ex_read_node_menu_rec;
typedef struct Cb_ex_read_graph_tag	*Cb_ex_read_graph,
						Cb_ex_read_graph_rec;
typedef struct Cb_ex_file_read_tag	*Cb_ex_file_read, Cb_ex_file_read_rec;

struct Cb_ex_read_language_tag {
	Cb_ex_language		next;
	Cb_ex_language		last;
};

struct Cb_ex_read_menu_item_tag {
	Cb_ex_menu_item_if	next;
	int			left;
};

struct Cb_ex_read_focus_tag {
	Cb_ex_focus_if		next;
};

struct Cb_ex_read_tag_tag {
	char			*next;
	char			*last;
};

struct Cb_ex_read_rendering_tag {
	Cb_ex_rendering_if	next;
	Cb_ex_rendering_if	last;
};

struct Cb_ex_read_graph_tag {
	Cb_ex_graph_if		next;
	Cb_ex_graph_if		last;
};

struct Cb_ex_read_node_menu_tag {
	Cb_ex_node_menu_if	next;
	Cb_ex_node_menu_if	last;
};

struct Cb_ex_file_read_tag {
	Cb_ex_file			file;
	short				fd;
	char				*buffer;
	Cb_ex_header			header;
	Cb_heap				heap;
	Hash				tags;
	Cb_ex_read_language_rec		language;
	Cb_ex_read_menu_item_rec	menu_item;
	Cb_ex_read_focus_rec		focus;
	Cb_ex_read_tag_rec		tag;
	Cb_ex_read_rendering_rec	rendering;
	Cb_ex_read_node_menu_rec	node_menu;
	Cb_ex_read_graph_rec		graph;
};

PORT_EXTERN(Cb_ex_file_read	cb_ex_open_read(PORT_0ARG));
PORT_EXTERN(void		cb_ex_close_read(PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(Cb_ex_header	cb_ex_read_header(PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(Cb_ex_read_language_rec cb_ex_read_reset_language(
					PORT_2ARG(Cb_ex_file_read,
						  Cb_ex_read_language)));
PORT_EXTERN(Cb_ex_language	cb_ex_read_next_language(
					PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(Cb_ex_menu_if	cb_ex_read_menu(
					PORT_2ARG(Cb_ex_file_read, int)));
PORT_EXTERN(Cb_ex_menu_item_if	cb_ex_read_next_menu_item(
					PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(Cb_ex_read_focus_rec cb_ex_read_reset_focus(
					PORT_2ARG(Cb_ex_file_read,
						  Cb_ex_read_focus)));
PORT_EXTERN(Cb_ex_focus_if	cb_ex_read_next_focus_unit(
					PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(void		cb_ex_read_tag_group(
					PORT_2ARG(Cb_ex_file_read, char*)));
PORT_EXTERN(char		*cb_ex_read_next_tag(
					PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(char		*cb_ex_fetch_tag(PORT_2ARG(char*, unsigned)));
PORT_EXTERN(void		cb_ex_read_comment_delimiter(
					PORT_4ARG(Cb_ex_file_read,
						  char*,
						  char**,
						  char**)));
PORT_EXTERN(Cb_ex_read_rendering_rec cb_ex_read_reset_rendering(
					PORT_3ARG(Cb_ex_file_read,
						  char*,
						  Cb_ex_read_rendering)));
PORT_EXTERN(Cb_ex_rendering_if	cb_ex_read_next_rendering(PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(Cb_ex_read_node_menu_rec cb_ex_read_reset_node_menu(
					PORT_3ARG(Cb_ex_file_read,
						  char*,
						  Cb_ex_read_node_menu)));
PORT_EXTERN(Cb_ex_node_menu_if	cb_ex_read_next_node_menu(PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(Cb_ex_read_graph_rec cb_ex_read_reset_graph(
					PORT_2ARG(Cb_ex_file_read,
						  Cb_ex_read_graph)));
PORT_EXTERN(Cb_ex_graph_if	cb_ex_read_next_graph(PORT_1ARG(Cb_ex_file_read)));
PORT_EXTERN(char		*cb_ex_read_weight_group(
					PORT_2ARG(Cb_ex_file_read, char*)));

#endif
