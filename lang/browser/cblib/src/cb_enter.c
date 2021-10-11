#ident "@(#)cb_enter.c 1.1 94/10/31 SMI"

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


#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif

#ifndef cb_file_h_INCLUDED
#include "cb_file.h"
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

#ifndef cb_sun_c_tags_h_INCLUDED
#include "cb_sun_c_tags.h"
#endif

#ifndef cb_write_h_INCLUDED
#include "cb_write.h"
#endif

#define	CB_QUEUE_LENGTH		5	/* Close delay queue length */

extern	char			*cb_program_name;

	Cb_file_write		cb_enter_cb_file;
static	Cb_source_type		cb_enter_top_source_type = cb_root_source_type;
static	Array			cb_file_stack = NULL;
static	char			*cb_language;
static	int			cb_major_version;
static	int			cb_minor_version;
#ifndef _CBQUERY
	Array			cb_enter_function_names = NULL;
	Array			cb_write_files = NULL;
static	Array			cb_function_starts = NULL;
#endif

#ifndef _CBQUERY
	int			cb_flag = 0;
	int			*cb_lineno_addr = NULL;
#endif

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void			cb_enter_start();
extern	void			cb_enter_push_file();
extern	void			cb_enter_pop_file();
extern	Cb_file_write		cb_enter_pop_file_delayed();
extern	void			cb_enter_enqueue();
extern	Cb_semtab_if		cb_enter_symbol();
extern	Cb_semtab_if		cb_enter_symbol_in_file();
extern	Cb_semtab_if		cb_enter_symbol_file();
extern	Cb_semtab_if		cb_enter_unique_symbol();
extern	Cb_symtab_im		cb_enter_create_symrec();
extern	Cb_semtab_if		cb_enter_append_semrec();
extern	void			cb_enter_line_id_block();
extern	void			cb_enter_inactive_lines();
extern	void			cb_enter_inactive_lines_actual();
extern	void			cb_enter_function_start();
extern	void			cb_enter_function_end();
extern	void			cb_enter_pound_line_seen();
extern	void			cb_enter_case_was_folded();
extern	void			cb_enter_set_multiple();
extern	void			cb_enter_include_error();

#ifndef _CBQUERY
/*
 *	cb_enter_start(lineno_addr, lang_name, major_version, minor_version)
 *		This will enable the writing of .cb files.
 */
void
cb_enter_start(lineno_addr, lang_name, major_version, minor_version)
	int		*lineno_addr;
	char		*lang_name;
	int		major_version;
	int		minor_version;
{
	cb_language = lang_name;
	cb_major_version = major_version;
	cb_minor_version = minor_version;
	cb_lineno_addr = lineno_addr;
	cb_flag = 1;
}
#endif

/*
 *	cb_enter_push_file(file_name, enter_src_name)
 *		Creates a new cb write object and pushes it onto the
 *		internal stack.
 */
void
cb_enter_push_file(file_name, enter_src_name)
	char			*file_name;
	int			enter_src_name;
{
	char			source_name[MAXPATHLEN+100];
	char			absolute_name[MAXPATHLEN];
	Cb_heap			heap = cb_init_heap(0);
	unsigned int		*ip;
	int			n;
	register Cb_file_write	file;
	int			first_file = 0;
	char			obj[MAXPATHLEN];
	char			*objp;

	/*
	 * Strip the quotes cpp supplies from the filename
	 */
	if ((file_name[0] == '"') &&
	    (file_name[strlen(file_name) - 1] == '"')) {
		(void)strcpy(source_name, file_name + 1);
		source_name[strlen(source_name) - 1] = '\0';
	} else {
		(void)strcpy(source_name, file_name);
	}

	file = cb_allocate(Cb_file_write, heap);
	cb_enter_cb_file = file;
	file->relative = source_name[0] != '/';
	for (ip = &file->hashs[0],
	     n = sizeof(file->hashs) /
	     sizeof(file->hashs[0]);
	     n > 0;
	     n--) {
		*ip++ = 0;
	}
	file->hashs[(int)cb_src_name_section] =
	  file->relative;
	cb_make_absolute_path(source_name, 0, absolute_name);
	file_name = absolute_name;

	if (cb_write_files == NULL) {
		cb_write_files = array_create((Cb_heap) NULL);
	}
	file->file_number = array_size(cb_write_files);
	file->stack_depth = array_size(cb_write_files);
	array_append(cb_write_files, (Array_value)file);

	if (cb_file_stack == NULL) {
		cb_file_stack = array_create((Cb_heap)NULL);
		array_expand(cb_file_stack, 10);
		first_file = 1;
	}
	array_append(cb_file_stack, (Array_value)file);
	file->cb_file = NULL;	/* Filled in at write time */
	file->src.name = cb_string_unique(file_name);
	while (*file_name != 0) {
		file->hashs[(int)cb_src_name_section] +=
		  *file_name++;
	}
	file->heap = heap;
	file->symtab = hsh_create(512,
				  hsh_string_equal,
				  hsh_string_hash,
				  (Hash_value)NULL,
				  heap);
	file->refd_files = NULL;
	file->line_id_chunks = NULL;
	file->inactive_list = NULL;
	file->symtab_size = 0;
	file->next_semtab_slot = NULL;
	file->fd = NULL;
	file->symtab_list = NULL;
	file->symrec_list = NULL;
	(void)bzero((char *)&file->header, sizeof(Cb_header_rec));

	(void)sprintf(source_name,
		      CB_EX_FOCUS_UNIT_LANGUAGE_FORMAT,
		      cb_language);
	(void)cb_enter_symbol(source_name,
			      1,
			      (unsigned int)cb_focus_language_unit);

	if (enter_src_name) {
		file->node_head = cb_graph_node_head(file->src.name);
		file->include_node = file->node_head;
		(void)sprintf(source_name,
			      CB_EX_GRAPHER_NODE_PATTERN,
			      file->include_node);
		(void)cb_enter_symbol(source_name,
				      1,
				      (unsigned int)cb_grapher_source_file_node);

		(void)sprintf(source_name,
			      CB_EX_GRAPHER_PROP_PATTERN,
			      file->node_head);
		(void)cb_enter_symbol(source_name,
				      1,
				      (unsigned int)cb_grapher_source_file_property);

		if (first_file) {
			(void)strcpy(obj, file->src.name);
			objp = strrchr(obj, '/');
			if (objp == NULL) {
				objp = obj;
			} else {
				objp++;
			}
			objp[strlen(objp)-1] = 'o';
			(void)sprintf(source_name,
				      CB_EX_GRAPHER_ARC_PATTERN2,
				      CB_EX_GRAPHER_FILES_TAG,
				      objp,
				      file->node_head);
			(void)cb_enter_symbol(source_name,
					      1,
					      (unsigned int)cb_grapher_executable_object_file_arc);
			(void)sprintf(source_name,
				      CB_EX_GRAPHER_NODE_PATTERN,
				      objp);
			(void)cb_enter_symbol(source_name,
				    1,
				    (unsigned int)cb_grapher_object_file_node);
		}
		(void)sprintf(source_name,
			      CB_EX_FOCUS_UNIT_FILE_FORMAT,
			      file->src.name);
		(void)cb_enter_symbol(source_name,
				      1,
				      (unsigned int)cb_focus_file_unit);
	}
}

/*
 *	cb_enter_pop_file()
 *		Write out the cb object that is on the top of the stack.
 */
void
cb_enter_pop_file()
{
	register Cb_file_write	file;
	Cb_file_write		top_level_file;
	Cb_file_write		included_file;
	char			arc_name[1000];

	if ((cb_file_stack == NULL) || array_empty(cb_file_stack)) {
		return;
	}
	included_file = cb_enter_cb_file;
	file = (Cb_file_write)array_tail(cb_file_stack);
	top_level_file = (Cb_file_write)ARRAY_FETCH(cb_file_stack, 0);
	cb_enter_enqueue(file, top_level_file);
	if (array_size(cb_file_stack) <= 1) {
		/* Flush the queue. */
		cb_enter_enqueue((Cb_file_write) NULL, top_level_file);
	}

	(void)cb_enter_pop_file_delayed();
	array_trim(cb_file_stack, array_size(cb_file_stack) - 1);

	if (cb_enter_cb_file != NULL) {
		(void)sprintf(arc_name,
			      CB_EX_GRAPHER_ARC_PATTERN2,
			      CB_EX_GRAPHER_FILES_TAG,
			      cb_enter_cb_file->include_node,
			      included_file->include_node);
		(void)cb_enter_symbol(arc_name,
				      1,
				      (unsigned int)cb_grapher_source_source_file_arc);
	}
}

/*
 *	cb_enter_enqueue(new_file, top_level_file)
 *		This will enter "file" onto a queue.  When the queue length
 *		exceeds CB_QUEUE_LENGTH, the first element from the queue
 *		will be written out and closed.  When "file" is NULL, the
 *		entire queue will be flushed.  This queue tends to solve the
 *		problem of delayed grammar reductions trying to enter a
 *		symbol into a closed .cb file.
 */
static void
cb_enter_enqueue(new_file, top_level_file)
	Cb_file_write		new_file;
	Cb_file_write		top_level_file;
{
	static Array		queue = NULL;
	Cb_file_write		file;
	char			*p;
	int			hash_value;

	if (queue == NULL) {
		queue = array_create((Cb_heap) NULL);
	}
	if (new_file != NULL) {
		array_append(queue, (Array_value)new_file);
	}
	while ((array_size(queue) > CB_QUEUE_LENGTH) ||
	       ((new_file == NULL) && (!array_empty(queue)))) {
		file = (Cb_file_write)array_lop(queue);
		array_store(cb_write_files, file->file_number,
			    (Array_value)NULL);
		if (file->inactive_list) {
			register int	end;
			register int	index;
			register int	size;
			register int	start;

			size = array_size(file->inactive_list);
			index = 0;
			while (index < size) {
				start = array_fetch(file->inactive_list,
						    index++);
				end = array_fetch(file->inactive_list,
						  index++);
				cb_enter_inactive_lines_actual(file, start,
							       end);
			}
		}
		cb_write_cb_file(file,
				 file->stack_depth,
				 cb_language,
				 cb_enter_top_source_type,
				 cb_major_version,
				 cb_minor_version);

		if (file->stack_depth > 0) {
			top_level_file =
			  (Cb_file_write)ARRAY_FETCH(cb_file_stack, 0);
			if (top_level_file->refd_files == NULL) {
				top_level_file->refd_files =
				  array_create(top_level_file->heap);
				array_expand(top_level_file->refd_files, 10);
			}
			array_append(top_level_file->refd_files,
				     (Array_value)file->cb_file);
			hash_value = file->cb_file->hash_value;
			p = rindex(file->cb_file->src.name, '/');
			if (p == NULL) {
				p = file->cb_file->src.name;
			} else {
				p++;
			}
			while (*p != '\0') {
				hash_value += (unsigned int)*p++;
			}
			top_level_file->hashs[(int)cb_refd_files_section] +=
			  hash_value;
		} else {
			array_trim(cb_write_files, 0);
		}
		cb_free_heap(file->heap);
	}
}

#ifndef _CBQUERY
/*
 *	cb_enter_pop_file_delayed()
 *		
 */
Cb_file_write
cb_enter_pop_file_delayed()
{
	if ((cb_file_stack == NULL) || array_empty(cb_file_stack)) {
		return NULL;
	}
	cb_enter_cb_file = NULL;
	if (array_size(cb_file_stack) > 1) {
		cb_enter_cb_file =
		  (Cb_file_write)ARRAY_FETCH(cb_file_stack,
					     array_size(cb_file_stack) - 2);
	}
	return cb_enter_cb_file;
}
#endif

/*
 *	cb_enter_symbol(symbol, line_number, usage)
 *		Enter one semantic record for the symbol "symbol".
 */
Cb_semtab_if
cb_enter_symbol(symbol, line_number, usage)
	register char		*symbol;
	int			line_number;
	unsigned int		usage;
{
	return cb_enter_symbol_file(symbol, line_number, usage,
				    cb_enter_cb_file);
}

/*
 *	cb_enter_symbol_in_file(symbol, line_number, usage, file_number)
 *		Enter one semantic record for symbol "symbol" into
 *		"file_number".
 */
Cb_semtab_if
cb_enter_symbol_in_file(symbol, line_number, usage, file_number)
	char			*symbol;
	int			line_number;
	unsigned int		usage;
	int			file_number;
{
	Cb_file_write		file;
	static Cb_semtab_if_rec	bogus;

	file = (Cb_file_write)array_fetch(cb_write_files, file_number);
	if (file == NULL) {
		cb_enter_include_error();
		return &bogus;
	} else {
		return cb_enter_symbol_file(symbol, line_number, usage, file);
	}
}

/*
 *	cb_enter_symbol_file(symbol, line_number, usage, file)
 */
static Cb_semtab_if
cb_enter_symbol_file(symbol, line_number, usage, file)
	register char		*symbol;
	int			line_number;
	unsigned int		usage;
	register Cb_file_write	file;
{
	register Cb_symtab_im	symrec;
	register Hash_bucket	bucket;

	if (file == NULL) {
		return NULL;
	}

	bucket = hsh_bucket_obtain(file->symtab, (Hash_value)symbol);
	symrec = (Cb_symtab_im)hsh_bucket_get_value(bucket);
	if (symrec == NULL) {
		HSH_BUCKET_SET_KEY(bucket,
				   file->symtab,
				   (Hash_value)cb_string_copy((char *)symbol,
							      file->heap));
		symrec = cb_enter_create_symrec((unsigned char *)symbol,
						file);
		hsh_bucket_set_value(bucket, (Hash_value)symrec);
	}

	return cb_enter_append_semrec(symrec, line_number, usage, file);
}

/*
 *	cb_enter_unique_symbol(symbol, line_number, usage)
 *		This will enter "symbol", "line_number", and "usage" onto
 *		a list associated with "cb_enter_cb_file". You must ensure
 *		that "symbol" is unique.  The list order is preserved and
 *		output in the .cb file first.
 */
Cb_semtab_if
cb_enter_unique_symbol(symbol, line_number, usage)
	char			*symbol;
	int			line_number;
	unsigned int		usage;
{
	Cb_file_write		file = cb_enter_cb_file;
	register Cb_symtab_im	symrec;

	if (file == NULL) {
		return NULL;
	}
	symbol = cb_string_copy(symbol, file->heap);
	if (file->symtab_list == NULL) {
		file->symtab_list = array_create(file->heap);
		file->symrec_list = array_create(file->heap);
	}
	array_append(file->symtab_list, (Array_value)symbol);
	symrec = cb_enter_create_symrec((unsigned char *)symbol, file);
	array_append(file->symrec_list, (Array_value)symrec);
	return cb_enter_append_semrec(symrec, line_number, usage, file);
}

/*
 *	cb_enter_create_symrec(symbol, file)
 *		This will create and return a new symrec object containing
 *		"symbol".
 */
static Cb_symtab_im
cb_enter_create_symrec(symbol, file)
	register unsigned char	*symbol;
	register Cb_file_write	file;
{	
	register unsigned char	c;
	register int		hash_value = 0;
	register int		length = 0;
	register Cb_symtab_im	symrec;

	while ((hash_value += (c = *symbol++)), (c != 0)) {
		length++;
	}
	file->hashs[(int)cb_symtab_section] += hash_value;
	file->hashs[(int)cb_semtab_section] += (int)cb_end_semrec;
	file->symtab_size +=
		cb_align(sizeof(Cb_symtab_if_rec) + length,
			 sizeof(((Cb_symtab_if)0)->semtab));
	symrec = cb_allocate(Cb_symtab_im, file->heap);
	array_expand(symrec->semtab = array_create(file->heap), 4);
	symrec->semseg_length = sizeof(*((Cb_semtab_if)0));

	return symrec;
}

/*
 *	cb_enter_append_semrec(symrec, line_number, usage, file)
 *		This creates and appends a new semrec object containing
 *		"line_number" and "usage" to "symrec".  The hash value
 *		associated with "file" will be updated.
 */
static Cb_semtab_if
cb_enter_append_semrec(symrec, line_number, usage, file)
	register Cb_symtab_im	symrec;
	int			line_number;
	unsigned int		usage;
	Cb_file_write		file;
{
	register Cb_semtab_if	semrec;

	semrec =
	  (Cb_semtab_if)cb_get_memory(sizeof(*((Cb_semtab_if)0)),
				      file->heap);
	file->hashs[(int)cb_semtab_section] +=
	  (semrec->name_ref.type = (unsigned int)cb_name_ref_semrec) +
	    (semrec->name_ref.line_number = line_number) +
	      (semrec->name_ref.usage = usage);
	symrec->semseg_length += sizeof(*((Cb_semtab_if)0));
	array_append(symrec->semtab, (Array_value)semrec);
	return semrec;
}

#ifndef _CBQUERY
/*
 *	cb_enter_line_id_block(line_id_block, size)
 *		Enter one block of line ids.
 *		The block is just remembered for when we write
 *		it to the .cb file.
 */
void
cb_enter_line_id_block(line_id_block, size)
	Cb_line_id	line_id_block;
	int		size;
{
	Cb_file_write	file = cb_enter_cb_file;
	Cb_line_id_chunk line_id_chunk;

	if (file == NULL) {
		return;
	}
	line_id_chunk = cb_allocate(Cb_line_id_chunk, file->heap);
	line_id_chunk->chunk = line_id_block;
	line_id_chunk->size = size;

	if (file->line_id_chunks == NULL) {
		file->line_id_chunks = array_create(file->heap);
		array_expand(file->line_id_chunks, 10);
	}
	array_append(file->line_id_chunks, (Array_value)line_id_chunk);

	for (; size-- > 0; line_id_block++) {
		file->hashs[(int)cb_line_id_section] +=
		  line_id_block->length + line_id_block->hash;
	}
}
#endif

#ifndef _CBQUERY
/*
 *	cb_enter_inactive_lines(start, end)
 *		Patch line_id entries for lines that are ifdefd out
 */
void
cb_enter_inactive_lines(start, end)
	int			start;
	int			end;
{
	register Cb_file_write	file = cb_enter_cb_file;

	if (file->inactive_list == NULL) {
		file->inactive_list = array_create(file->heap);
	}
	array_append(file->inactive_list, (Array_value)start);
	array_append(file->inactive_list, (Array_value)end);
}

/*
 *	cb_enter_inactive_lines_actual(file, start, end)
 *		This does the actual work of marking the beginning and ending
 *		of inactive sections.
 */
static void
cb_enter_inactive_lines_actual(file, start, end)
	Cb_file_write		file;
	int			start;
	int			end;
{
	int			n;
	int			line = 1;
	Cb_line_id_chunk	chunk;
	Cb_line_id		p;
	int			length;
	register int		hash_value = 0;

	if (file == NULL) {
		return;
	}
	start++;
	end--;
	/*
	 * Find the first chunk with lines we want to patch.
	 */
	for (n = 0; n < array_size(file->line_id_chunks); n++) {
		chunk = (Cb_line_id_chunk)ARRAY_FETCH(file->line_id_chunks,
						      n);
		line += chunk->size;
		if (line > start) {
			break;
		}
	}
	/*
	 * Patch lines from chunks
	 */
	line -= chunk->size;
	while (1) {
		length = chunk->size - (start - line);
		for (p = chunk->chunk + (start - line);
		     (start <= end) && (length > 0);
		     start++, p++, length--) {
			if (!p->is_inactive) {
				p->is_inactive = 1;
				hash_value++;
			}
		}
		if (length != 0) {
			break;
		}
		line += chunk->size;
		if (++n >= array_size(file->line_id_chunks)) {
			break;
		}
		chunk = (Cb_line_id_chunk)ARRAY_FETCH(file->line_id_chunks, n);
	}
	file->hashs[(int)cb_line_id_section] += hash_value;
}
#endif

#ifndef _CBQUERY
/*
 *	cb_enter_function_start(name, start_line_number)
 */
void
cb_enter_function_start(name, start_line_number)
	char	*name;
	int	start_line_number;
{
	if (!cb_flag) {
		return;
	}
	if (cb_enter_function_names == NULL) {
		cb_enter_function_names = array_create((Cb_heap)NULL);
		cb_function_starts = array_create((Cb_heap)NULL);
	}
	array_append(cb_enter_function_names, (Array_value)name);
	array_append(cb_function_starts, (Array_value)start_line_number);
}

/*
 *	cb_enter_function_end(end_line_number)
 *		Enter one function decl block
 */
void
cb_enter_function_end(end_line_number)
	int	end_line_number;
{
	char		*name;
	int		start_line_number;
	char		symbol[2000];

	if (cb_flag && (cb_enter_function_names != NULL) &&
	    (cb_enter_cb_file != NULL)) {
		name = (char *)array_pop(cb_enter_function_names);
		start_line_number = array_pop(cb_function_starts);
		(void)sprintf(symbol,
			      CB_EX_FOCUS_UNIT_FUNCTION_FORMAT,
			      name,
			      start_line_number,
			      end_line_number);
		(void)cb_enter_unique_symbol(symbol,
				start_line_number,
				(unsigned int)cb_focus_function_unit);

		(void)sprintf(symbol,
		      CB_EX_GRAPHER_PROP_PATTERN,
		      name);		      
		(void)cb_enter_symbol(symbol,
				      start_line_number,
				      (unsigned)cb_grapher_function_property);
		(void)sprintf(symbol,
			      CB_EX_GRAPHER_NODE_PATTERN,
			      name);
		(void)cb_enter_symbol(symbol,
				      start_line_number,
				      (unsigned)cb_grapher_function_regular_call_node);
	}
}
#endif

#ifndef _CBQUERY
/*
 *	cb_enter_pound_line_seen()
 */
void
cb_enter_pound_line_seen()
{
	if (cb_enter_cb_file != NULL) {
		if (cb_enter_cb_file->header.pound_line_seen == 0) {
			cb_message("%s: #line seen in source file. SunSourceBrowser data is likely to be incorrect.\n",
				     cb_program_name);
		}
		cb_enter_cb_file->header.pound_line_seen = 1;
	}
}
#endif

#ifndef _CBQUERY
/*
 *	cb_enter_case_was_folded()
 */
void
cb_enter_case_was_folded()
{
	if (cb_enter_cb_file != NULL) {
		cb_enter_cb_file->header.case_was_folded = 1;
	}
}
#endif

/*
 *	cb_enter_set_multiple();
 *		Set the top-level file type to be cb_multiple_source_type.
 */
void
cb_enter_set_multiple()
{
	cb_enter_top_source_type = cb_multiple_source_type;
}

/*
 *	cb_enter_include_error()
 *		This will print a warning message once.
 */
static void
cb_enter_include_error()
{
	static int	warning_printed = 0;

	if (!warning_printed) {
		warning_printed = 1;
		(void)fprintf(stderr,
			      "#include file structure is too complicated; data from %s may be slightly wrong.\n",
			      CB_OPTION);
		(void)fflush(stderr);
	}
}

/*
 *	cb_enter_adjust_tag(file_number, semrec, new_tag)
 */
void
cb_enter_adjust_tag(file_number, semrec, tag)
	int		file_number;
	Cb_semtab_if	semrec;
	unsigned int	tag;
{
	Cb_file_write	file;

	file = (Cb_file_write)array_fetch(cb_write_files, file_number);
	if (file == NULL) {
		cb_enter_include_error();
	} else {
		file->hashs[(int)cb_semtab_section] +=
					tag - semrec->name_ref.usage;
		semrec->name_ref.usage = (int)tag;
	}
}
