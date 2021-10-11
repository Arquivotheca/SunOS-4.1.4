#ident "@(#)cb_f77pass1.c 1.1 94/10/31 SMI"

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


#ifndef cb_cpp_protocol_h_INCLUDED
#include "cb_cpp_protocol.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif

#ifndef cb_f77pass1_h_INCLUDED
#include "cb_f77pass1.h"
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

#ifndef cb_sun_f77_tags_h_INCLUDED
#include "cb_sun_f77_tags.h"
#endif

	int		cb_f77pass1_hidden = 0;
	int		cb_f77pass1_lineno = 0;
	int		cb_f77pass1_using_cpp = 0;
	Cb_semtab_if	cb_f77pass1_string_rec;
	Cb_line_id	cb_f77pass1_line_ids;
	Cb_line_id	cb_f77pass1_next_line_id;

static	Cb_char_action	*cb_f77pass1_ca_next_free;

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	char			cb_f77pass1_cpp_protocol();
extern	char			cb_f77pass1_get_char();
extern	char			*cb_f77pass1_get_cpp_id();
extern	Cb_semtab_if		cb_f77pass1_enter_name();
extern	void			cb_f77pass1_save_cb_file();
extern	void			cb_f77pass1_set_cb_file();
extern	void			cb_f77pass1_append_file_list();
extern	void			cb_f77pass1_init_lex();
extern	Cb_char_action		cb_f77pass1_get_char_action();
extern	void			cb_f77pass1_free_char_action();
extern	void			cb_f77pass1_flush_line_id();

/*
 *	cb_f77pass1_cpp_protocol(infile)
 *		This function is called from f77pass1 to handle cpp
 *		protocol information
 */
char
cb_f77pass1_cpp_protocol(infile)
	FILE	*infile;
{
	char	chr;
	int	hidden;

	switch (chr = getc(infile)) {
	case CB_CHR_SAYLINE:
		return 0;
	}
	hidden = cb_hidden;
	(void)cb_cpp_protocol(chr,
		cb_f77pass1_get_char,
		cb_f77pass1_get_cpp_id,
		cb_fread,
		infile);
	return hidden != cb_hidden;
}

/*
 *	cb_f77pass1_get_char(infile)
 */
static char
cb_f77pass1_get_char(infile)
	FILE	*infile;
{
	return getc(infile);
}

/*
 *	cb_f77pass1_get_cpp_id(infile)
 *		Read one identifier from the cpp byte stream
 */
static char *
cb_f77pass1_get_cpp_id(infile)
	FILE	*infile;
{
	char	*result;

	cbgetid(getc, infile, result);
	return result;
}

/*
 *	cb_f77pass1_enter_name(symbol, lineno, usage)
 *		Strip trailing whitespace from the name and enter it.
 */
Cb_semtab_if
cb_f77pass1_enter_name(symbol, lineno, usage)
	char		*symbol;
	int		lineno;
	unsigned int	usage;
{
	char		*p = index(symbol, ' ');
	Cb_semtab_if	result;

	if (p == NULL) {
		result = cb_enter_symbol(symbol, lineno, usage);
	} else {
		if (p[-1] == '_') {
			p--;
			*p = 0;
			result = cb_enter_symbol(symbol, lineno, usage);
			*p = '_';
		} else {
			*p = 0;
			result = cb_enter_symbol(symbol, lineno, usage);
			*p = ' ';
		}
	}
	return result;
}

/*
 *	Save a pointer to the current cb_file on the Cb_char_action
 *	object that is passed in.
 */
void
cb_f77pass1_save_cb_file(dest, file, close_file)
	Cb_file_list	*dest;
	Cb_file_write	file;
	int		close_file;
{
	Cb_file_list	p;
	Cb_file_list	q;

	p = cb_allocate(Cb_file_list, (Cb_heap)NULL);
	p->cb_file = file;
	p->close_file = close_file;
	p->next = NULL;

	if (*dest == NULL) {
		*dest = p;
	} else {
		for (q = *dest; q->next != NULL; q = q->next);
		q->next = p;
	}
}

/*
 *	Run down the list of cb_files that should be closed now and close them
 *	The fact that a cb file should be closed is detected when the source
 *	line is read into the buffer but we can't close until the stuff in
 *	the buffer before that spot has been read.
 */
void
cb_f77pass1_set_cb_file(p)
	Cb_file_list	p;
{
	while (p != NULL) {
		if (p->close_file) {
			cb_enter_pop_file();
		}
		cb_enter_set_cb_file(p->cb_file);
		p = p->next;
	}
}

/*
 *	Append a cb file list to another such list.
 *	This is used when Cb_char_action slots are merged.
 */
void
cb_f77pass1_append_file_list(dest, src)
	Cb_file_list	*dest;
	Cb_file_list	src;
{
	Cb_file_list	p;

	if (*dest == NULL) {
		*dest = src;
		return;
	}

	for (p = *dest; p->next != NULL; p = p->next);
	p->next = src;
}

/*
 *	cb_f77pass1_init_lex(size)
 */
void
cb_f77pass1_init_lex(size)
	int	size;
{
	Cb_char_action	p;
	Cb_char_action	*pool;
	int		n;

	pool = (Cb_char_action *)cb_get_memory(size * sizeof(Cb_char_action),
					       (Cb_heap)NULL);
	cb_f77pass1_ca_next_free = pool;
	p = (Cb_char_action)cb_get_memory(size * sizeof(Cb_char_action),
					  (Cb_heap)NULL);
	(void)bzero((char *)p, size * sizeof(Cb_char_action));
	for (n = 0 ; n < size; n++) {
		*pool++ = &p[n];
	}
}

/*
 *	Allocate one Cb_char_action cell
 */
Cb_char_action
cb_f77pass1_get_char_action()
{
	Cb_char_action	result;

	result = *cb_f77pass1_ca_next_free++;
	if (result->in_use == 1) {
		cb_abort("char action in use\n");
	}
	result->in_use = 1;
	result->lineno_incr = 0;
	result->cb_file = NULL;
	result->cb_hidden = cb_hidden;
	return result;
}

/*
 *	Free one Cb_char_action cell
 */
void
cb_f77pass1_free_char_action(p)
	Cb_char_action	*p;
{
	if ((*p)->in_use == 0) {
		cb_abort("char action not in use\n");
	}
	(*p)->in_use = 0;
	*--cb_f77pass1_ca_next_free = *p;
	*p = NULL;
}

/*
 *	When cpp is not used the scanner has to collect line length/hashval
 *	pairs for the browser
 */
void
cb_f77pass1_flush_line_id()
{
	if (cb_f77pass1_line_ids != NULL) {
		cb_enter_line_id_block(cb_f77pass1_line_ids,
				       cb_f77pass1_next_line_id -
							cb_f77pass1_line_ids);
		cb_f77pass1_line_ids = NULL;
	}
}


