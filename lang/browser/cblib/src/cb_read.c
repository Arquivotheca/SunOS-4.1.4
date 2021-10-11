#ident "@(#)cb_read.c 1.1 94/10/31"

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


#ifndef cb_cb_h_INCLUDED
#include "cb_cb.h"
#endif

#ifndef cb_file_h_INCLUDED
#include "cb_file.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_io_h_INCLUDED
#include "cb_io.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef cb_read_h_INCLUDED
#include "cb_read.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef cb_swap_bytes_h_INCLUDED
#include "cb_swap_bytes.h"
#endif


/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	Cb_file_read		cb_read_open();
extern	void			cb_read_close();
extern	Cb_header		cb_read_header_section();
extern	Cb_src_file_im_rec	cb_read_src_name_section();
extern	Cb_read_refd_rec	cb_read_reset_refd_file();
extern	Cb_refd_file_if		cb_read_next_refd_file();
extern	Cb_read_symtab_rec	cb_read_reset_symbol();
extern	Cb_symtab_if		cb_read_next_symbol();
extern	void			cb_read_reset_semtab();
extern	Cb_semtab_if		cb_read_next_semantic_record();
extern	Cb_read_line_id_rec 	cb_read_reset_line_id();
extern	Cb_line_id		cb_read_next_line_id();
extern	Cb_section		cb_find_section();

/*
 *	cb_read_open(cb_file, print_error)
 *		This will open the .cb file "cb_file" and prepare it for
 *		reading.  NULL is returned if "cb_file" does not exist.
 *		If "print_error" is TRUE, a fatal message will be printed
 *		if "cb_file" does not exist.
 */
Cb_file_read
cb_read_open(cb_file, print_error)
	Cb_file		cb_file;
{
	struct stat	stat_buff;
	Cb_file_read	file;
	Cb_heap		heap;
	int		fd;

	if (cb_file == NULL) {
		return NULL;
	}
	fd = open(cb_file->cb_file_name, O_RDONLY, 0);
	if (fd < 0) {
		cb_file->source_type = cb_bogus_source_type;
		if (print_error) {
			cb_message("%s does not exist\n",
				 cb_file->cb_file_name);
		}
		return NULL;
	}
	if (cb_file->size == 0) {
		cb_fstat(fd, &stat_buff);
		cb_file->cb_file_timestamp = stat_buff.st_mtime;
		cb_file->size = stat_buff.st_size;
	}
	heap = cb_init_heap(cb_file->size * 3 / 2);
	file = cb_allocate(Cb_file_read, heap);
	(void)bzero((char *)file, sizeof(*file));
	file->cb_file = cb_file;
	file->fd = fd;
	file->buffer = cb_mmap_file_private(fd, cb_file->size);
	file->heap = heap;

	(void)cb_read_header_section(file);
	if ((cb_file->src.name == NULL) || (cb_file->src.name[0] == 0)) {
		cb_file->src = cb_read_src_name_section(file);
		cb_file->src.name = cb_string_unique(cb_file->src.name);
	}
	cb_file->file_read = file;
	return file;
}

/*
 *	cb_read_close(file)
 *		Close "file" and dealloc the memory.
 */
void
cb_read_close(file)
	Cb_file_read		file;
{
	cb_close(file->fd);
	cb_free_heap(file->heap);
	file->cb_file->file_read = NULL;
}

/*
 *	cb_read_header_section(file)
 *		Read in and return the header associated with "file".
 */
#ifdef _CBQUERY
static
#endif
Cb_header
cb_read_header_section(file)
	Cb_file_read		file;
{
	Cb_section_rec		header_section;
	register Cb_header	header;
#ifdef CB_SWAP
	register int		i;
	Cb_section		section;
#endif

	if (file->header != NULL) {
		return file->header;
	}

	header_section.tag = cb_header_section;
	header_section.location.start = 0;
	header_section.location.length = sizeof(Cb_header_rec);
	header = (Cb_header)file->buffer;
	file->header = header;

	/* Byte swapping for little-endians: */
	cb_swap(header->magic_number);
	cb_swap(header->source_type);
	cb_swap(header->major_version_number);
	cb_swap(header->minor_version_number);
	cb_swap(header->language_major_version);
	cb_swap(header->language_minor_version);
	cb_swap(header->pound_line_seen);
	cb_swap(header->case_was_folded);
	cb_swap(header->section_count);
#ifdef CB_SWAP
	for (i = header->section_count-1, section = &header->section[0];
	     i >= 0;
	     i--, section++) {
		cb_swap(section->tag);
		cb_swap(section->location.start);
		cb_swap(section->location.length);
	}
#endif

	if (header->magic_number != CB_MAGIC_NUMBER) {
		cb_fatal("%s is a bogus %s file\n",
			 file->cb_file->cb_file_name,
			 CB_SUFFIX);
	}
	if (header->major_version_number != CB_MAJOR_VERSION_NUMBER) {
		cb_fatal("%s file %s version number mismatch. \
Please regenerate.\n",
			 CB_SUFFIX,
			file->cb_file->cb_file_name);
	}
	file->cb_file->source_type = header->source_type;
	file->cb_file->lang = cb_lang_create(header->language_name);
	return header;
}

/*
 *	cb_read_src_name_section(file)
 *		Read in and return the source file name for the .cb file.
 */
Cb_src_file_im_rec
cb_read_src_name_section(file)
	Cb_file_read		file;
{
	Cb_section		section;
	Cb_src_file_im_rec	result;
	Cb_src_file_if		src;
	short			relative;

	section = cb_find_section(file, cb_src_name_section, TRUE);

	src = (Cb_src_file_if)(file->buffer + section->location.start);
	result.name = src->name;
	relative = src->relative;

	cb_swap(relative);

	result.relative = relative; /* 386i needs this extra step */
	return result;
}

/*
 *	cb_read_reset_refd_file(file, new)
 *		Reset the read pointer for refd files.
 *		If  "new" is non-NULL it is used to set the pointer.
 *		The old read pointer is returned.
 */
Cb_read_refd_rec
cb_read_reset_refd_file(file, new)
	Cb_file_read		file;
	Cb_read_refd		new;
{
	Cb_read_refd_rec	old;

	old = file->refd;
	if (new != NULL) {
		file->refd = *new;
#ifdef CB_SWAP
		file->refd.swapped = old.swapped;
#endif
	} else {
		file->refd.next = NULL;
		file->refd.last = NULL;
	}
	return old;
}

/*
 *	cb_read_next_refd_file(file)
 *		This routine will return a pointer the next refd .cb file
 *		name in "file".  NULL is returned if there are no more
 *		refd .cb file names.
 */
Cb_refd_file_if
cb_read_next_refd_file(file)
	Cb_file_read		file;
{
	Cb_refd_file_if		result;
	Cb_section		section;
#ifdef CB_SWAP
	register Cb_refd_file_if	refd;
#endif

	if (file->refd.last == NULL) {
		section = cb_find_section(file, cb_refd_files_section, FALSE);
		if (section == NULL) {
			return NULL;
		}
		file->refd.next = (Cb_refd_file_if)
			(file->buffer + section->location.start);
		file->refd.last = (Cb_refd_file_if)
			((int)(file->refd.next) +
				section->location.length);
#ifdef CB_SWAP
		if (file->refd.swapped == 0) {
			file->refd.swapped = 1;
			for (refd = file->refd.next;
			     refd < file->refd.last;
			     refd = (Cb_refd_file_if)cb_align(
			      ((int)refd) + strlen(refd->refd_name) +
			      sizeof(Cb_refd_file_if_rec),
			      sizeof(refd->hash_value))) {
				cb_swap(refd->hash_value);
			}
		}
#endif
	}

	if (file->refd.next >= file->refd.last) {
		return NULL;
	}

	result = file->refd.next;
	file->refd.next = (Cb_refd_file_if)
		cb_align((int)(file->refd.next) +
			strlen(file->refd.next->refd_name) +
			sizeof(Cb_refd_file_if_rec),
				sizeof(result->hash_value));
	return result;
}

/*
 *	cb_read_reset_symbol(file, new)
 *		Reset the read pointer for symbols.
 *		If  "new" is non-NULL it is used to set the pointer.
 *		The old read pointer is returned.
 */
Cb_read_symtab_rec
cb_read_reset_symbol(file, new)
	Cb_file_read		file;
	Cb_read_symtab		new;
{
	Cb_read_symtab_rec	old;

	old = file->symtab;
	if (new != NULL) {
		file->symtab = *new;
#ifdef CB_SWAP
		file->symtab.swapped = old.swapped;
#endif
	} else {
		file->symtab.next = NULL;
		file->symtab.last = NULL;
	}
	return old;
}

/*
 *	cb_read_next_symbol(file)
 *		Return a pointer to the next symbol from the file.
 *		NULL is returned at EOF
 */
Cb_symtab_if
cb_read_next_symbol(file)
	Cb_file_read		file;
{
	Cb_symtab_if		result;
	Cb_section		section;
#ifdef CB_SWAP
	register Cb_symtab_if	symbol;
#endif

	if (file->symtab.last == NULL) {
		section = cb_find_section(file, cb_symtab_section, FALSE);
		if (section == NULL) {
			return NULL;
		}
		file->symtab.next = (Cb_symtab_if)
			(file->buffer + section->location.start);
		file->symtab.last = (Cb_symtab_if)
			((int)file->symtab.next + section->location.length);
#ifdef CB_SWAP
		if (!file->symtab.swapped) {
			for (symbol = file->symtab.next;
			    symbol < file->symtab.last;
			    symbol = (Cb_symtab_if)cb_align( ((int)symbol) +
			      sizeof(Cb_symtab_if_rec) + strlen(symbol->name),
			      sizeof(symbol->semtab))) {
				cb_swap(symbol->semtab);
			}
			file->symtab.swapped = 1;
		}
#endif
	}

	result = file->symtab.next;
	if (result != NULL) {
		file->symtab.next = (Cb_symtab_if)
			cb_align(((int)result +
				sizeof(Cb_symtab_if_rec) +
				strlen(result->name)),
					sizeof(result->semtab));
		if (file->symtab.next >= file->symtab.last) {
			file->symtab.next = NULL;
		}
	}
	return result;
}

/*
 *	cb_read_reset_semtab(file, segment)
 *		This sets the symbol subsequent
 *		cb_read_next_semantic_record() will fetch records for.
 */
void
cb_read_reset_semtab(file, segment)
	Cb_file_read	file;
	unsigned int	segment;
{
	Cb_section	section;

	section = cb_find_section(file, cb_semtab_section, FALSE);
	if (section == NULL) {
		file->semtab.next = NULL;
		return;
	}

	file->semtab.next = (Cb_semtab_if)(file->buffer + segment);
	file->semtab.first = (Cb_semtab_if)(file->buffer +
						     section->location.start);
#ifdef CB_SWAP
	if (file->semtab.swapped == NULL) {
		int	size;

		size = section->location.length / sizeof(Cb_semtab_if_rec);
		file->semtab.swapped = (char *)cb_get_memory(size, file->heap);
		(void)bzero(file->semtab.swapped, size);
	}
#endif
}

/*
 *	cb_read_next_semantic_record(file)
 */
Cb_semtab_if
cb_read_next_semantic_record(file)
	Cb_file_read		file;
{
	Cb_semtab_if	result;

	result = file->semtab.next;
	if (result != NULL) {
#ifdef CB_SWAP
		if (!file->semtab.swapped[result - file->semtab.first]) {
			file->semtab.swapped[result - file->semtab.first] =
			  TRUE;
			cb_swap(result->swap);
		}
#endif
		file->semtab.next =
			(Cb_semtab_if)((int)file->semtab.next +
				sizeof(*((Cb_semtab_if)0)));
		if (result->end.type == (unsigned int)cb_end_semrec) {
			file->semtab.next = NULL;
		}
	}
	return result;
}

/*
 *	cb_read_reset_line_id(file, new)
 *		Reset the read pointer for line_ids.
 *		If  "new" is non-NULL it is used to set the pointer.
 *		The old read pointer is returned.
 */
Cb_read_line_id_rec
cb_read_reset_line_id(file, new)
	Cb_file_read		file;
	Cb_read_line_id		new;
{
	Cb_read_line_id_rec	old;

	old = file->line_id;
	if (new != NULL) {
		file->line_id = *new;
#ifdef CB_SWAP
		file->line_id.swapped = old.swapped;
#endif
	} else {
		file->line_id.next = NULL;
		file->line_id.last = NULL;
	}
	return old;
}

/*
 *	cb_read_next_line_id(file)
 */
Cb_line_id
cb_read_next_line_id(file)
	Cb_file_read	file;
{
	Cb_line_id	result;
	Cb_section	section;

	if (file->line_id.next == NULL) {
		section = cb_find_section(file, cb_line_id_section, FALSE);
		if (section == NULL) {
			return NULL;
		}
		file->line_id.next = (Cb_line_id)
			(file->buffer + section->location.start);
		if (file->line_id.last == NULL) {
			file->line_id.last = (Cb_line_id)
				((int)file->line_id.next +
					section->location.length);
#ifdef CB_SWAP
			if (file->line_id.swapped == 0) {
				file->line_id.swapped = 1;
				for (result = file->line_id.next;
				     result < file->line_id.last;
				     result++) {
					cb_swap_int(*result);
				}
			}
#endif
		}
	}

	result = file->line_id.next;
	if (result >= file->line_id.last) {
		result = NULL;
	} else {
		file->line_id.next =
			(Cb_line_id)((int)result + sizeof(Cb_line_id_rec));
	}
	return result;
}

/*
 *	cb_find_section(file, section_tag, abort)
 *		Return a pointer the section corresponding to "section_tag"
 *		in "file".  If "abort" is TRUE, cb_abort() will be called
 *		if the section can not be found.  Otherwise, NULL will be
 *		returned.
 */
Cb_section
cb_find_section(file, section_tag, abort)
	register Cb_file_read	file;
	register Cb_section_id	section_tag;
	int			abort;
{
	register int		index;
	register Cb_section	section;
	register Cb_header	header;

	header = cb_read_header_section(file);
	section = &header->section[0];
	for (index = header->section_count-1; index >= 0; index--, section++) {
		if (section->tag == section_tag) {
			return section;
		}
	}
	if (abort) {
		cb_abort("Could not find %s section type %d in %s\n",
			  CB_SUFFIX,
			 section_tag,
			 file->cb_file->cb_file_name);
	}
	return NULL;
}

