/* LINTLIBRARY */

/*	@(#)cb_f77pass1.h 1.1 94/10/31 SMI	*/

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


#ifndef cb_f77pass1_h_INCLUDED
#define cb_f77pass1_h_INCLUDED


#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

#ifndef cb_sun_f77_tags_h_INCLUDED
#include "cb_sun_f77_tags.h"
#endif

#define cb_enter_name_line(symbol, lineno, usage) \
	{    if (cb_flag && (cb_f77pass1_hidden == 0) && ((lineno) > 0)) \
		(void)cb_f77pass1_enter_name((symbol), (lineno), (usage)); }

#define cb_enter_name_line_val(symbol, lineno, usage, val) \
	{    if (cb_flag && (cb_f77pass1_hidden == 0) && ((lineno) > 0)) \
		(val) = cb_f77pass1_enter_name((symbol), (lineno), (usage)); }

#define cb_enter_name(symbol, usage) \
		cb_enter_name_line((symbol), cb_f77pass1_lineno, (usage))

#define cb_enter_adjust_tag(rec, tag) \
	{ \
	cb_enter_get_cb_file()->hashs[(int)cb_semtab_section] -= \
		(unsigned int)(rec)->name_ref.usage; \
	(rec)->name_ref.usage = (unsigned int)(tag); \
	cb_enter_get_cb_file()->hashs[(int)cb_semtab_section] += \
		(unsigned int)(tag); \
	}

/*
 *	We keep a shadow buffer for the linebuffer `s'. The shadow
 *	buffer is a vector of pointers to Cb_char_action cells.
 *	Each character that provokes a change in the cb reader state
 *	gets a Cb_char_ction cell in in the slot it corresponds to.
 *	When the line buffer is compacted the action cells
 *	are moved to stay in slots matching their character.
 *	When the line buffer is read they are acted upon.
 *
 *	The status that is tracked includes:
 *		Current line number
 *		Current cb_hidden (are we inside a macro expansion?)
 *		Current Cb_file (which file should we save symbols in?)
 */

typedef struct Cb_char_action_tag	*Cb_char_action, Cb_char_action_rec;
typedef struct Cb_file_list_tag		*Cb_file_list, Cb_file_list_rec;

struct Cb_char_action_tag {
	int		lineno_incr;
	Cb_file_list	cb_file;
	short		in_use;
	short		cb_hidden;
};
struct Cb_file_list_tag {
	Cb_file_write	cb_file;
	short		close_file;
	Cb_file_list	next;
};


#define cb_f77pass1_maybe_set_cb_file(slot) \
	if (cb_flag && ((*(slot))->cb_file != NULL)) { \
		cb_f77pass1_set_cb_file((*(slot))->cb_file); \
	}

#define cb_f77pass1_use_char_action(slot) \
	if (cb_flag && (*(slot) != NULL)) { \
		cb_f77pass1_lineno += (*(slot))->lineno_incr; \
		cb_f77pass1_maybe_set_cb_file(slot); \
		cb_f77pass1_hidden = (*(slot))->cb_hidden; \
		cb_f77pass1_free_char_action(slot); \
	}

#define cb_f77pass1_set_char_action(p) \
	if ((p) == NULL) { \
		(p) = cb_f77pass1_get_char_action(); \
	}

#define CB_LINE_ID_MAX 100

extern	int			cb_f77pass1_hidden;
extern	int			cb_f77pass1_lineno;
extern	int			cb_f77pass1_using_cpp;
extern	Cb_semtab_if		cb_f77pass1_string_rec;
extern	Cb_line_id		cb_f77pass1_line_ids;
extern	Cb_line_id		cb_f77pass1_next_line_id;

PORT_EXTERN(char		*alloca(PORT_1ARG(unsigned)));
PORT_EXTERN(char		cb_f77pass1_cpp_protocol(PORT_1ARG(FILE *)));
PORT_EXTERN(Cb_semtab_if	cb_f77pass1_enter_name(
					PORT_3ARG(char*,
						  int,
						  unsigned)));
PORT_EXTERN(void		cb_f77pass1_save_cb_file(
					PORT_3ARG(Cb_file_list*,
						  Cb_file_write,
						  int)));
PORT_EXTERN(void		cb_f77pass1_set_cb_file(
					PORT_1ARG(Cb_file_list)));
PORT_EXTERN(void		cb_f77pass1_append_file_list(
					PORT_2ARG(Cb_file_list*,
						  Cb_file_list)));
PORT_EXTERN(void			cb_f77pass1_init_lex(PORT_1ARG(int)));
PORT_EXTERN(Cb_char_action	cb_f77pass1_get_char_action(PORT_0ARG));
PORT_EXTERN(void		cb_f77pass1_free_char_action(
					PORT_1ARG(Cb_char_action*)));
PORT_EXTERN(void		cb_f77pass1_flush_line_id(PORT_0ARG));

#endif
