/* LINTLIBRARY */

/*	@(#)cb_query.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_query_h_INCLUDED
#define cb_query_h_INCLUDED

#ifndef cb_ex_h_INCLUDED
#include "cb_ex.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef cb_rw_h_INCLUDED
#include "cb_rw.h"
#endif

typedef	struct Cb_request_tag		*Cb_request, Cb_request_rec;
typedef struct Cb_focusing_tag		*Cb_focusing, Cb_focusing_rec;
typedef	struct Cb_match_tag		*Cb_match, Cb_match_rec;


enum Cb_query_result_tag {
	cb_no_result,

	cb_regular_result,
	cb_files_only_result,
	cb_symbols_only_result,

	cb_last_result
};
typedef enum Cb_query_result_tag	*Cb_query_result_ptr, Cb_query_result;

/*
 *	This struct defines one query request. It is passed to cb_do_query().
 */
struct Cb_request_tag {
	int		max_size;	/* How much memory to use for query */
	Cb_ex_menu_item_if menu_item;	/* Menu item with semantic mask */
	Array		symbols;	/* Symbols to query */
	FILE		*out_file;
	int		(*pattern_op)();
	Cb_query_result	result;
	short		fast;		/* Don't update datebase */
	short		no_case;	/* Ignore case */
	short		no_source;	/* Do not show source line in match */
	short		focusing;	/* Focus this query */
};
/*
 *	This is used to store one focusing unit from the request block
 */
struct Cb_focusing_tag {
	Cb_ex_focus_if		focus_unit;
	char			*pattern;
	Cb_ex_menu_item_if	menu_item;
	Array			items;
};

#define CB_SECONDARY_WEIGHT	1000	/* Mark secondary matches with this */
/*
 *	This is the package of things that are saved as one component
 *	of a query answer.
 *	The full answer is a list of Cb_match structs.
 */
struct Cb_match_tag {
	char		*symbol;
	char		*function;
	char		*line;
	Cb_file		match_file;
	Cb_line_id_rec	line_id;
	unsigned int	line_number;
	unsigned int	usage;
	short		weight;
	short		secondary_match;
};

PORT_EXTERN(Array	cb_do_query(PORT_2ARG(Cb_request, Cb_heap)));
PORT_EXTERN(Array	cb_init_directories(PORT_2ARG(Cb_heap, int)));

#endif
