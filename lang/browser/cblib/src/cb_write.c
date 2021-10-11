#ident "@(#)cb_write.c 1.1 94/10/31 SMI"

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

#ifndef cb_directory_h_INCLUDED
#include "cb_directory.h"
#endif

#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif

#ifndef cb_file_h_INCLUDED
#include "cb_file.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_init_h_INCLUDED
#include "cb_init.h"
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

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef cb_write_h_INCLUDED
#include "cb_write.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

#ifndef MAXNAMLEN
#include <dirent.h>
#endif




/*
 *	Data
 */
static	int		cb_do_callback_flag = 1;
static	Hash		file_table;

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		cb_write_cb_file();
extern	void		cb_write_src_name_section();
extern	void		cb_write_refd_files_section();
extern	int		cb_write_one_symbol();
extern	void		cb_write_symtab_section();
extern	int		cb_write_semtab_segment_for_symbol();
extern	void		cb_write_semtab_section();
extern	void		cb_write_line_id_section();
extern	void		cb_create_dir();
extern	void		cb_do_callback();
extern	Cb_refd_dir	cb_directory_find_file();
extern	void		cb_directory_find_read_root();
extern	void		cb_directory_find_read_one();
extern	void		cb_directory_mark();

/*
 *	cb_write_cb_file(file, ref_level, lang_name,
 *				top_source_type, major_version, minor_version)
 *		Dump the internal data structure built by the compiler
 *		to one .cb file.
 */
void
cb_write_cb_file(file, ref_level, lang_name,
		 top_source_type, major_version, minor_version)
	register Cb_file_write	file;
	int			ref_level;
	char			*lang_name;
	Cb_source_type		top_source_type;
	int			major_version;
	int			minor_version;
{
	register Cb_partition	partition;
	register int		index;
	register Cb_header	header = &file->header;
	char			cb_file_name[MAXPATHLEN];
	char			new_root_dir[MAXPATHLEN];
	Cb_init			init;
	char			io_buffer[8*1024];
	Cb_directory		directory;
	unsigned register int	hash_val;
	char			hash_string[32];
	char			*name;
static	char			*hostname = NULL;
static	short			sequence = 1;
	Cb_refd_dir		mark;
#ifdef CB_SWAP
	register int		i;
	Cb_section		section;
#endif
	Cb_refd_dir_rec		mark_rec;

	/* Finish computing the hash value: */
	if (ref_level == 0) {
		header->source_type = top_source_type;
	} else {
		header->source_type = cb_refd_source_type;
	}
	file->hashs[(int)cb_header_section] +=
	  CB_MAGIC_NUMBER +
	    (int)header->source_type +
	      CB_MAJOR_VERSION_NUMBER +
		CB_MINOR_VERSION_NUMBER +
		  major_version +
		    minor_version +
		      header->pound_line_seen +
		        header->case_was_folded;
	for (name = lang_name; *name != 0; ) {
		file->hashs[(int)cb_header_section] += *name++;
	}

#define hash_dump(label, field) \
	(void)fprintf(stderr, "\t%s:0x%x\n", label, file->hashs[(int)field]);

	hash_val =
	  file->hashs[(int)cb_header_section] +
	    file->hashs[(int)cb_src_name_section] +
	      file->hashs[(int)cb_refd_files_section] +
		file->hashs[(int)cb_symtab_section] +
		  file->hashs[(int)cb_semtab_section] +
		    file->hashs[(int)cb_line_id_section];
	if (getenv("SUN_SOURCE_BROWSER_HASH_VALUE_DEBUG") != NULL) {
		(void)fprintf(stderr, "file:%s\n", file->src.name);
		hash_dump("Header", cb_header_section);
		hash_dump("Src_Name", cb_src_name_section);
		hash_dump("Refd_files", cb_refd_files_section);
		hash_dump("Symtab", cb_symtab_section);
		hash_dump("Semtab", cb_semtab_section);
		hash_dump("Line_Id", cb_line_id_section);
	}

	(void)cb_hash2string(hash_val, hash_string);
	(void)sprintf(cb_file_name,
		      "%s.%s%s",
		      rindex(file->src.name, '/') + 1,
		      hash_string,
		      CB_SUFFIX);

	/* Find a partition to write .cb file into. */
	directory = cb_directory_home();
	cb_init_read(directory);
	init = directory->init;
	if (ref_level > 0) {
		mark = cb_directory_find_file(init, cb_file_name, 0, 1);
		if (mark != NULL) {
			/* File already exists! Do not rewrite! */
			partition =
			  (Cb_partition)ARRAY_FETCH(init->partitions,
					    array_size(init->partitions)-1);
			file->cb_file =
			  cb_file_create(cb_string_unique(file->src.name),
					 hash_val,
					 partition->dir,
					 cb_get_sub_dir_string(CB_REFD_DIR_ID));
			cb_directory_mark(mark);
			return;
		}
	}
	index = 0;
 try_another_dir:
	for (; index < array_size(init->partitions); index++) {
		partition = (Cb_partition)ARRAY_FETCH(init->partitions,
						      index);
		if ((strncmp(file->src.name,
			     partition->prefix,
			     partition->prefix_length) != 0)) {
			partition = NULL;
			continue;
		}
		index++;
		break;
	}
	if (partition == NULL) {
		cb_fatal("No place to write %s found\n",
			 cb_file_name);
	}
	directory = partition->dir;
	mark_rec.dir = directory;
	mark_rec.sub_dir = ref_level == 0 ? CB_NEW_DIR_ID : CB_REFD_DIR_ID;
	file->cb_file = cb_file_create(cb_string_unique(file->src.name),
				       hash_val,
				       directory,
				       ref_level == 0 ?
				       CB_NEW_DIR : CB_REFD_DIR);
	file->cb_file->lang = NULL;
	file->cb_file->heap = file->heap;
	file->cb_file->source_type = header->source_type;
			
	if (hostname == NULL) {
		(void)gethostname(cb_file_name, sizeof(cb_file_name));
		hostname = cb_string_unique(cb_file_name);
	}
	(void)sprintf(cb_file_name,
		      "%s/%s/%s/%s.%x.%x%s",
		      directory->name,
		      CB_DIRECTORY,
		      ref_level == 0 ? CB_NEW_DIR : CB_REFD_DIR,
		      hostname,
		      getpid(),
		      sequence++,
		      CB_SUFFIX_IN_PROGRESS);
	if ((file->fd = fopen(cb_file_name, "w")) == NULL) {
		/*
		 * If open failed we try to create the .cb dir
		 */
		char		*p;
		char		*q;

		p = strrchr(cb_file_name, '/');
		*p = 0;
		q = strrchr(cb_file_name, '/');
		*q = 0;
		cb_create_dir(cb_file_name);
		*q = '/';
		cb_create_dir(cb_file_name);
		*p = '/';
		if ((file->fd = fopen(cb_file_name, "w")) == NULL) {
			if (!partition->write_failed) {
				partition->write_failed = 1;
				cb_warning("Could not write to directory %s\n",
					   partition->dir->name);
			}
			if (index < array_size(init->partitions)) {
				goto try_another_dir;
			}
			return;
		}
	}
	(void)setvbuf(file->fd, io_buffer, _IOFBF, sizeof(io_buffer));
	if ((ref_level != 0) && !partition->new_root_made) {
		(void)sprintf(new_root_dir, "%s/%s/%s",
			      directory->name,
			      CB_DIRECTORY,
			      CB_NEW_DIR);
		(void)mkdir(new_root_dir, 0775);
		partition->new_root_made = TRUE;
	}

	cb_directory_mark(&mark_rec);

	if ((ref_level == 0) && (cb_do_callback_flag != 0)) {
		cb_callback_write_stab(file->cb_file->cb_file_name);
	}

	/* Start filling in the header. */
	header->magic_number = CB_MAGIC_NUMBER;
	header->major_version_number = CB_MAJOR_VERSION_NUMBER;
	header->minor_version_number = CB_MINOR_VERSION_NUMBER;
	bzero(&header->language_name[0], CB_LANG_NAME_SIZE);
	(void)strcpy(&header->language_name[0], lang_name);
	header->language_major_version = major_version;
	header->language_minor_version = minor_version;
	header->section_count = 1;
	header->section[0].location.start = 0;
	header->section[0].location.length = sizeof(Cb_header_rec);
	header->section[0].tag = cb_header_section;

	/*
	 * The sections must be written in the order in which
	 * they appear in the file.
	 */
	/* Write this block to position where source segment should be */
	cb_fwrite((char *)&file->header, sizeof(Cb_header_rec), file->fd);

	cb_write_src_name_section(file);
	if (file->refd_files != NULL) {
		cb_write_refd_files_section(file);
	}
	cb_write_symtab_section(file);
	/*
	 * The semantic segment must be written immediately after
	 * the symbol table section.
	 */
	cb_write_semtab_section(file);
	if (file->line_id_chunks != NULL) {
		cb_write_line_id_section(file);
	}

	/* Write the header now that we have all the info */
	cb_fseek(file->fd, (int)header->section[0].location.start);
#ifdef CB_SWAP
	for (i = header->section_count-1, section = &header->section[0];
	     i >= 0;
	     i--, section++) {
		cb_swap(section->tag);
		cb_swap(section->location.start);
		cb_swap(section->location.length);
	}
	cb_swap(header->magic_number);
	cb_swap(header->source_type);
	cb_swap(header->major_version_number);
	cb_swap(header->minor_version_number);
	cb_swap(header->language_major_version);
	cb_swap(header->language_minor_version);
	cb_swap(header->pound_line_seen);
	cb_swap(header->case_was_folded);
	cb_swap(header->section_count);
#endif
	cb_fwrite((char *)&file->header, sizeof(Cb_header_rec), file->fd);

	cb_fclose(file->fd);
	if (rename(cb_file_name, file->cb_file->cb_file_name) != 0) {
		/*
		 * The rename might fail because a query removed the NewRoot
		 * directory when the compile ran. Try to recreate the dir.
		 */
		char	*p;
		char	*q;

		(void)strcpy(new_root_dir, file->cb_file->cb_file_name);
		p = strrchr(new_root_dir, '/');
		*p = 0;
		q = strrchr(new_root_dir, '/');
		*q = 0;
		cb_create_dir(new_root_dir);
		*q = '/';
		cb_create_dir(new_root_dir);
		/*
		 * Then try again
		 */
		if (rename(cb_file_name, file->cb_file->cb_file_name) != 0) {
			/*
			 * Leave no turds behind
			 */
			(void)unlink(cb_file_name);
		}
	}
}

/*
 *	cb_write_src_name_section(file)
 */
static void
cb_write_src_name_section(file)
	register Cb_file_write	file;
{
	register Cb_section	section = &file->header.section
					[file->header.section_count];
	char			buffer[MAXPATHLEN*2];
	register Cb_src_file_if	rec = (Cb_src_file_if)buffer;

	section->tag = cb_src_name_section;
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(int));
	section->location.length = cb_align(strlen(file->src.name) +
					    sizeof(Cb_src_file_if_rec),
					    sizeof(char *));

	rec->relative = file->relative;
	(void)strcpy(rec->name, file->src.name);
#ifdef CB_SWAP
	cb_swap(rec->relative);
#endif
	cb_fwrite((char *)rec, (int)section->location.length, file->fd);
	file->header.section_count++;
}

/*
 *	cb_write_refd_files_section(file)
 */
static void
cb_write_refd_files_section(file)
	Cb_file_write	file;
{
	register int		rec_length;
	register int		n;
	char			buffer[MAXPATHLEN*2];
	register Cb_section	section = &file->header.section
					[file->header.section_count];
	register Cb_file		refd;
	register Cb_refd_file_if rec = (Cb_refd_file_if)buffer;
	register char		*basename;

	section->tag = cb_refd_files_section;
	section->location.start = cb_align((section-1)->location.start +
						(section-1)->location.length,
					   sizeof(rec->hash_value));
	section->location.length = 0;

	for (n = array_size(file->refd_files)-1; n >= 0; n--) {
		refd = (Cb_file)ARRAY_FETCH(file->refd_files, n);
		if (refd->src.name == NULL) {
			cb_abort("No source name for refd file\n");
		}
		if ((basename = rindex(refd->src.name, '/')) == NULL) {
			basename = refd->src.name;
		} else {
			basename++;
		}
		rec_length = cb_align(strlen(basename) +
					sizeof(Cb_refd_file_if_rec),
				      sizeof(rec->hash_value));
		(void)strcpy(rec->refd_name, basename);
		rec->hash_value = refd->hash_value;
#ifdef CB_SWAP
		cb_swap(rec->hash_value);
#endif
		cb_fwrite((char *)rec, rec_length, file->fd);
		section->location.length += rec_length;
	}
	file->header.section_count++;
}

/*
 *	cb_write_one_symbol(symbol_name, symbol, file)
 */
static int
cb_write_one_symbol(symbol_name, symbol, file)
	register char		*symbol_name;
	Cb_symtab_im		symbol;
	register Cb_file_write	file;
{
	register Cb_symtab_if	sym;
	register int		length;

	length = cb_align(sizeof(Cb_symtab_if_rec) + strlen(symbol_name),
			  sizeof(sym->semtab));
	sym = (Cb_symtab_if)alloca(length);
	sym->semtab = file->next_semtab_slot;
	(void)strcpy(sym->name, symbol_name);
#ifdef CB_SWAP
	cb_swap(sym->semtab);
#endif
	cb_fwrite((char *)sym, length, file->fd);

	file->next_semtab_slot += symbol->semseg_length;

	return 1;
}

/*
 *	cb_write_symtab_section(file)
 */
static void
cb_write_symtab_section(file)
	register Cb_file_write	file;
{
	register Cb_section	section = &file->header.section
					[file->header.section_count];
	register int		index;
	register int		size;
	register Array		symtab_list;
	register Array		symrec_list;

	section->tag = cb_symtab_section;
	section->location.start = cb_align((section-1)->location.start +
						(section-1)->location.length,
					   sizeof(((Cb_symtab_if)0)->semtab));
	section->location.length = file->symtab_size;
	file->next_semtab_slot = cb_align(section->location.start +
						section->location.length,
					  sizeof(*((Cb_semtab_if)0)));
	if (file->symtab_list != NULL) {
		symtab_list = file->symtab_list;
		symrec_list = file->symrec_list;
		size = array_size(symrec_list);
		for (index = 0; index < size; index++) {
			(void)cb_write_one_symbol(
				(char *)ARRAY_FETCH(symtab_list, index),
				(Cb_symtab_im)ARRAY_FETCH(symrec_list, index),
				file);
		}
	}
	if (hsh_iterate(file->symtab, cb_write_one_symbol, (int)file)) {
		file->header.section_count++;
	}
}

/*
 *	cb_write_semtab_segment_for_symbol(symbol_name, symbol, file)
 *		This writes the semantic table segment for one symbol.
 *		It fills in the location of the segment in the
 *		corresponding symbol record.
 */
/*ARGSUSED*/
static int
cb_write_semtab_segment_for_symbol(symbol_name, symbol, file)
	char			*symbol_name;
	register Cb_symtab_im	symbol;
	register Cb_file_write	file;
{
	register Cb_semtab_if	*semrec;
	register int		n;
	Cb_semtab_if_rec	rec;
	register char		*p;

	file->header.section[file->header.section_count].location.length +=
							symbol->semseg_length;

	/*
	 * Write all the records in the segment
	 */
	semrec = (Cb_semtab_if *)symbol->semtab->data;
	for (n = array_size(symbol->semtab); n > 0; n--, semrec++) {
#ifdef CB_SWAP
		p = (char *)*semrec + sizeof(*((Cb_semtab_if)0)) - 1;
		putc(*p--, file->fd);
		putc(*p--, file->fd);
		putc(*p--, file->fd);
		putc(*p, file->fd);
#else
		p = (char *)*semrec;
		putc(*p++, file->fd);
		putc(*p++, file->fd);
		putc(*p++, file->fd);
		putc(*p, file->fd);
#endif
	}

	/*
	 * Write the end of segment record
	 */
	rec.end.type = (unsigned int)cb_end_semrec;
#ifdef CB_SWAP
	p = (char *)(&rec) + sizeof(*((Cb_semtab_if)0)) - 1;
	putc(*p--, file->fd);
	putc(*p--, file->fd);
	putc(*p--, file->fd);
	putc(*p, file->fd);
#else
	p = (char *)(&rec);
	putc(*p++, file->fd);
	putc(*p++, file->fd);
	putc(*p++, file->fd);
	putc(*p, file->fd);
#endif

	return 1;
}

/*
 *	cb_write_semtab_section(file)
 */
static void
cb_write_semtab_section(file)
	register Cb_file_write	file;
{
	register Cb_section	section = &file->header.section
					[file->header.section_count];
	register int		index;
	register int		size;
	register Array		symrec_list;

	section->tag = cb_semtab_section;
	section->location.start = cb_align((section-1)->location.start +
					(section-1)->location.length,
						sizeof(*((Cb_semtab_if)0)));
	section->location.length = 0;

	if (file->symtab_list != NULL) {
		symrec_list = file->symrec_list;
		size = array_size(symrec_list);
		for (index = 0; index < size; index++) {
			(void)cb_write_semtab_segment_for_symbol(
				(char *)NULL,
				(Cb_symtab_im)ARRAY_FETCH(symrec_list, index),
				file);
		}
	}
	if (hsh_iterate(file->symtab,
			cb_write_semtab_segment_for_symbol,
			(int)file)) {
		file->header.section_count++;
	}
}

/*
 *	cb_write_line_id_section(file)
 */
static void
cb_write_line_id_section(file)
	register  Cb_file_write	file;
{
	register Cb_section	section = &file->header.section
					[file->header.section_count];
	register Cb_line_id_chunk chunk;
	register int		n;
	register int		length;
#ifdef CB_SWAP
	register int		i;
#endif

	section->tag = cb_line_id_section;
	section->location.start = cb_align((section-1)->location.start +
						(section-1)->location.length,
					   sizeof(Cb_line_id_rec));
	section->location.length = 0;
	for (n = 0; n < array_size(file->line_id_chunks); n++) {
		chunk = (Cb_line_id_chunk)ARRAY_FETCH(file->line_id_chunks, n);
		length = chunk->size * sizeof(Cb_line_id_rec);
		section->location.length += length;
#ifdef CB_SWAP
		for (i = chunk->size-1; i >= 0; i--) {
			cb_swap(chunk->chunk[i]);
		}
#endif
		cb_fwrite((char *)chunk->chunk, length, file->fd);
	}
	file->header.section_count++;
}

/*
 *	cb_do_callback(value)
 *		Turn the callback procedure on/off
 */
void
cb_do_callback(value)
	int	value;
{
	cb_do_callback_flag = value;
}

/*
 *	cb_directory_find_file(init, name, try_root, try_refd)
 */
Cb_refd_dir
cb_directory_find_file(init, name, try_root, try_refd)
	Cb_init		init;
	char		*name;
	int		try_root;
	int		try_refd;
{
	int		i;
	Cb_refd_dir	found_it;
	Cb_import	import;

	if (file_table == NULL) {
		file_table = hsh_create(10,
					      hsh_string_equal,
					      hsh_string_hash,
					      (Hash_value)NULL,
					      cb_init_heap(0));
	} else {
		found_it = (Cb_refd_dir)hsh_lookup(file_table,
						   (Hash_value)name);
		if (found_it != NULL) {
			return found_it;
		}
	}

	for (i = 0; i < array_size(init->imports); i++) {
		import = (Cb_import)ARRAY_FETCH(init->imports, i);
		if (try_root) {
			cb_directory_find_read_root(import->dir);
		}
		if (try_refd && !import->dir->refd_read) {
			import->dir->refd_read = 1;
			cb_directory_find_read_one(import->dir,
						   CB_REFD_DIR,
						   CB_REFD_DIR_ID);
			if (!import->added_parent &&
			    !import->parent &&
			    import->dir->split_happened) {
				import->added_parent = 1;
				cb_init_add_parent(init, import);
			}
		}
		found_it = (Cb_refd_dir)hsh_lookup(file_table,
						   (Hash_value)name);
		if (found_it != NULL) {
			return found_it;
		}
	}

	return NULL;
}

/*
 *	cb_directory_find_read_root(directory)
 */
static void
cb_directory_find_read_root(directory)
	Cb_directory		directory;
{
	if (directory->root_read) {
		return;
	}
	directory->root_read = 1;
	cb_directory_find_read_one(directory, CB_OLD_DIR, CB_OLD_DIR_ID);
	cb_directory_find_read_one(directory, CB_NEW_DIR, CB_OLD_DIR_ID);
	cb_directory_find_read_one(directory, CB_LOCKED_DIR, CB_OLD_DIR_ID);
}

/*
 *	cb_directory_find_read_one(directory, sub_dir, sub_dir_id)
 */
static void
cb_directory_find_read_one(directory, sub_dir, sub_dir_id)
	Cb_directory		directory;
	char			*sub_dir;
	int			sub_dir_id;
{
	char			*dir_name = directory->name;
	register DIR		*dir;
	register struct dirent	*dirfile;
	register char		*name;
	char			dir_path[MAXPATHLEN];
	Cb_refd_dir		mark;

	(void)sprintf(dir_path,
		      "%s/%s/%s",
		      dir_name,
		      CB_DIRECTORY,
		      sub_dir);
	if ((dir = opendir(dir_path)) == NULL) {
		return;
	}
	while ((dirfile = readdir(dir)) != NULL) {
		if (dirfile->d_namlen <= 3) {
			continue;
		}

		if (dirfile->d_name[0] == CB_MARK_FILES_HEADER) {
			if (!strcmp(dirfile->d_name, CB_SPLIT_HAPPENED)) {
				directory->split_happened = 1;
				continue;
			} else if (!strcmp(dirfile->d_name, CB_NO_GC_REFD)) {
				directory->refd_marked = 1;
				continue;
			} else if (!strcmp(dirfile->d_name, CB_NO_GC_ROOT)) {
				directory->root_marked = 1;
				continue;
			}
		}

		if (strcmp(CB_SUFFIX,
			   dirfile->d_name +
			   dirfile->d_namlen -
			   CB_SUFFIX_LEN) != 0) {
			continue;
		}
		name = cb_string_unique(dirfile->d_name);
		mark = cb_allocate(Cb_refd_dir, file_table->heap);
		mark->dir = directory;
		mark->sub_dir = sub_dir_id;
		(void)hsh_insert(file_table,
				 (Hash_value)name,
				 (Hash_value)mark);
	}
	(void)closedir(dir);
}

/*
 *	cb_directory_mark(dir)
 */
void
cb_directory_mark(dir)
	Cb_refd_dir	dir;
{
	char		path[MAXPATHLEN];
	int		fd;
	char		*filename;
	char		*p;

	if (/*#### dir->parent || */dir->dir->is_local_dir) {
		return;
	}
	switch (dir->sub_dir) {
	case CB_NEW_DIR_ID:
	case CB_OLD_DIR_ID:
	case CB_LOCKED_DIR_ID:
		if (dir->dir->root_marked == 1) {
			return;
		}
		dir->dir->root_marked = 1;
		filename = CB_NO_GC_ROOT;
		break;

	case CB_REFD_DIR_ID:
		if (dir->dir->refd_marked == 1) {
			return;
		}
		dir->dir->refd_marked = 1;
		filename = CB_NO_GC_REFD;
		break;
	}

	(void)sprintf(path,
		      "%s/%s/%s/%s",
		      dir->dir->name,
		      CB_DIRECTORY,
		      cb_get_sub_dir_string(CB_REFD_DIR_ID),
		      filename);
	if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1) {
		p = strrchr(path, '/');
		*p = 0;
		(void)mkdir(path, 0777);
		fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	}
	(void)close(fd);
}

