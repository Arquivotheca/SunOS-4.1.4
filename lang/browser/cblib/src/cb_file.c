#ident "@(#)cb_file.c 1.1 94/10/31"

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

#ifndef cb_file_h_INCLUDED
#include "cb_file.h"
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

/*
 *	Data
 */
static	Hash		cb_file_table = NULL;
static	Hash		base2file;
static	char		cb_hash_chars[] =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-";


/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		cb_file2string();
extern	int		cb_file_compare();
extern	Cb_file		cb_file_create();
extern	int		cb_file_create_equal();
extern	int		cb_file_create_hash();
extern	Cb_file		cb_file_create2();
extern	void		cb_file_create2_enter();
extern	int		cb_file_create2_equal();
extern	int		cb_file_create2_hash();
extern	void		cb_file_destroy();
extern	void		cb_file_delete();
extern	void		cb_file_move();
extern	Hash		cb_file_table_get();
extern	char		*cb_source_type2string();
extern	Cb_file		cb_name2cb_file();
extern	char		*cb_get_sub_dir_string();
extern	int		cb_hash2string();
extern	void		cb_src_hash2string();
extern	unsigned	cb_string2hash();
extern	void		cb_file_dump();
extern	int		cb_file_dump_print();

/*
 *	cb_file2string(file, destination)
 *		This routine will return the source file name associated
 *		with "file".
 */
void
cb_file2string(file, destination)
	Cb_file		file;
	char		*destination;
{
	char		*name;
	char		hash_value[32];

	if ((file->src.name == NULL) ||
	    (file->src.name[0] == '\0')) {
		name = file->base_name;
	} else {
		name = file->src.name;
	}
	(void)cb_hash2string(file->hash_value, hash_value);
	(void)sprintf(destination, "%s:%s:relative=%d:type=%s:lang=%s",
		      name, hash_value, file->src.relative,
		      cb_source_type2string(file->source_type),
		      (file->lang == NULL) ? "<None>" : file->lang->name);
}

/*
 *	cb_source_type2string(source_type)
 * 		This will return the string associated with "source_type".
 */
char *
cb_source_type2string(source_type)
	Cb_source_type	source_type;
{
	switch (source_type) {
	case cb_root_source_type:
		return "root";
	case cb_refd_source_type:
		return "header";
	case cb_multiple_source_type:
		return "multiple";
	case cb_bogus_source_type:
		return "bogus";
	default:
		return "<bad>";
	}
}

/*
 *	cb_file_compare(file1, file2)
 *		This routine will return -1, 0, or 1 depending upon whether
 *		"file1" is less than, equal to or greater than "file2".
 */
int
cb_file_compare(file1, file2)
	Cb_file		file1;
	Cb_file		file2;
{
	return strcmp(file2->cb_file_name, file1->cb_file_name);
}

/*
 *	cb_file_create(src_name, hash_value, directory, sub_dir)
 *		This routine will create and return a unique file object
 *		for "src_name" and "hash_value" associated with "directory".
 *		"src_name" and "sub_dir" must be a unique string returned
 *		from cb_string_unique().
 */
Cb_file
cb_file_create(src_name, hash_value, directory, sub_dir)
	char			*src_name;
	unsigned		hash_value;
	Cb_directory		directory;
	char			*sub_dir;
{
	char			*base_name;
	Hash_bucket		bucket;
	register Cb_file	file;
	static Cb_file_rec 	file_zero;
	static int		fnum = 0;
	int			is_full;
	char			name[MAXPATHLEN];
	Cb_file_rec		temp_file;

	if (cb_file_table == NULL) {
		cb_file_table = cb_file_table_get();
	}

	/* Figure out the base name. */
	base_name = rindex(src_name, '/');
	if (base_name == NULL) {
		base_name = src_name;
		is_full = FALSE;
	} else {
		base_name = cb_string_unique(++base_name);
		is_full = TRUE;
	}

	temp_file.base_name = base_name;
	temp_file.hash_value = hash_value;
	temp_file.directory = directory;
	temp_file.sub_dir = sub_dir;

	bucket = hsh_bucket_obtain(cb_file_table, (Hash_value)&temp_file);
	file = (Cb_file)hsh_bucket_get_value(bucket);
	if (file == NULL) {
		file = cb_allocate(Cb_file, cb_file_table->heap);
		*file = file_zero;
		file->hash_value = hash_value;
		cb_src_hash2string(directory,
				   sub_dir,
				   src_name,
				   hash_value,
				   name);
		file->sub_dir = sub_dir;
		file->cb_file_name = cb_string_unique(name);
		if (is_full) {
			file->src.name = src_name;
			file->base_name = temp_file.base_name;
		}
		file->base_name = temp_file.base_name;
		file->fnum = fnum++;
		file->directory = directory;
		HSH_BUCKET_SET_KEY(bucket, cb_file_table, (Hash_value)file);
		hsh_bucket_set_value(bucket, (Hash_value)file);
	}
	return file;
}

/*
 *	cb_file_create_equal(file1, file2)
 *		This routine will return TRUE if "file1" equals "file2".
 *		We can't compare cb_file_name since the temp object
 *		does not have that field set.
 */
static int
cb_file_create_equal(file1, file2)
	Cb_file		file1;
	Cb_file		file2;
{
	return
	  (file1->base_name == file2->base_name) &&
	  (file1->hash_value == file2->hash_value) &&
	  (file1->directory == file2->directory) &&
	  (file1->sub_dir == file2->sub_dir);
}

/*
 *	cb_file_create_hash(file)
 *		This routine will return a hash value for "file".
 *		We can't use cb_file_name since the temp object
 *		does not have that field set.
 */
static int
cb_file_create_hash(file)
	Cb_file		file;
{
	return
	  (int)file->base_name +
	    (int)file->hash_value +
	      (int)file->directory +
		(int)file->sub_dir;
}

typedef struct Base_tag		*Base, Base_rec;
struct Base_tag {
	unsigned	hash_value;
	char		*base_name;
};

/*
 *	cb_file_create2(src_name, hash_value, directory, sub_dir)
 */
Cb_file
cb_file_create2(src_name, hash_value, directory, sub_dir)
	char			*src_name;
	unsigned		hash_value;
	Cb_directory		directory;
	char			*sub_dir;
{
	Base_rec		base;
	Cb_file			result;

	base.hash_value = hash_value;
	base.base_name = src_name;
	if (base2file == NULL) {
		result = NULL;
	} else {
		result = (Cb_file)hsh_lookup(base2file, (Hash_value)&base);
	}
	if ((result == NULL) && (directory != NULL)) {
		result = cb_file_create(src_name,
					hash_value,
					directory,
					sub_dir);
	}
	return result;
}

/*
 *	cb_file_create2_enter(file)
 */
void
cb_file_create2_enter(file)
	Cb_file		file;
{
	Base		base;

	if (base2file == NULL) {
		base2file = hsh_create(100,
				       cb_file_create2_equal,
				       cb_file_create2_hash,
				       (Hash_value)NULL,
				       cb_init_heap(0));
	}

	base = cb_allocate(Base, base2file->heap);
	base->hash_value = file->hash_value;
	base->base_name = file->base_name;
	(void)hsh_insert(base2file, (Hash_value)base, (Hash_value)file);
}

/*
 *	cb_file_create2_equal(a, b)
 */
static int
cb_file_create2_equal(a, b)
	Base	a;
	Base	b;
{
	return (a->hash_value == b->hash_value) &&
	  (a->base_name == b->base_name);
}

/*
 *	cb_file_create2_hash(a)
 */
static int
cb_file_create2_hash(a)
	Base	a;
{
	return (int)a->hash_value + (int)a->base_name;
}

#ifndef _CBQUERY
/*
 *	cb_file_destroy()
 *		This will deallocate all of the storage associated with
 *		cb_file objects.
 */
void
cb_file_destroy()
{
	if (cb_file_table != NULL) {
		cb_free_heap(cb_file_table->heap);
		cb_file_table = NULL;
	}
}
#endif

/*
 *	cb_file_delete(file)
 *		This will delete .cb file associated with "file".
 */
void
cb_file_delete(file)
	Cb_file		file;
{
	unsigned	max_compile_time;
	char		new_name[MAXPATHLEN];
	struct stat	status;

	(void)strcpy(new_name, file->cb_file_name);
	new_name[strlen(new_name) - CB_SUFFIX_LEN] = 0;
	(void)strcat(new_name, CB_SUFFIX_PENDING_DELETE);
	if (file->pending_delete) {
		max_compile_time = CB_MAX_COMPILE_TIME;
		if (getenv(CB_MAX_COMPILE_TIME_ENV_VAR) != NULL) {
			max_compile_time =
				atoi(getenv(CB_MAX_COMPILE_TIME_ENV_VAR));
		}
		if (stat(new_name, &status) >= 0) {
			if (cb_get_time_of_day() - status.st_mtime >
			    max_compile_time) {
				(void)unlink(new_name);
			}
		}
	} if (rename(file->cb_file_name, new_name) != 0) {
		cb_delete(file->cb_file_name);
	}
	file->source_type = cb_bogus_source_type;
	file->refd_files = NULL;
	(void)hsh_delete(cb_file_table, (Hash_value)file);
}

/*
 * cb_file_extract(file_name, directory)
 *	This will take a "file_name" from "directory" and create the
 *	associated Cb_file object.
 */
Cb_file
cb_file_extract(file_name, directory)
	char		*file_name;
	Cb_directory	directory;
{
	char		*base_name;
	Cb_file		file;
	char		*hash_value;
	char		*sub_dir;
	char		temp_name[MAXPATHLEN];

	(void)strcpy(temp_name, file_name);
	hash_value =  rindex(temp_name, '.');
	*hash_value = '\0';
	hash_value = rindex(temp_name, '.');
	*hash_value++ = '\0';
	base_name = rindex(temp_name, '/');
	*base_name++ = '\0';
	sub_dir = rindex(temp_name, '/');
	*sub_dir++ = '\0';
	file = cb_file_create2(cb_string_unique(base_name),
			       cb_string2hash(hash_value),
			       directory,
			       cb_string_unique(sub_dir));
	return file;
}

/*
 *	cb_file_move(file, directory, sub_dir)
 *		This will move "file" to be in "directory".
 */
void
cb_file_move(file, directory, sub_dir)
	register Cb_file file;
	Cb_directory	directory;
	char		*sub_dir;
{
	Hash		file_table;
	char		name[MAXPATHLEN];

	file_table = cb_file_table_get();
	(void)hsh_delete(file_table, (Hash_value)file);
	file->directory = directory;
	file->sub_dir = sub_dir;
	cb_src_hash2string(directory,
			   sub_dir,
			   file->base_name,
			   file->hash_value,
			   name);
	file->cb_file_name = cb_string_unique(name);
	(void)hsh_insert(file_table, (Hash_value)file, (Hash_value)file);
}

/*
 *	cb_file_table_get()
 *		This will return the file table used by cb_file_create().
 */
Hash
cb_file_table_get()
{
	if (cb_file_table == NULL) {
		cb_file_table = hsh_create(1000,
					   cb_file_create_equal,
					   cb_file_create_hash,
					   (Hash_value)NULL,
					   cb_init_heap(0));
	}
	return cb_file_table;
}

#ifndef _CBQUERY
/*
 *	cb_name2cb_file(filename, directory, sub_dir)
 *		Translate .cb string filename to Cb_file struct
 */
Cb_file
cb_name2cb_file(filename, directory, sub_dir)
	char		*filename;
	Cb_directory	directory;
	char		*sub_dir;
{
	char		name[MAXPATHLEN];
	char		*p;

	p = rindex(filename, '/');
	if (p == NULL) {
		(void)strcpy(name, filename);
	} else {
		(void)strcpy(name, p+1);
	}
	p = rindex(name, '.');
	if (p == NULL) {
		return NULL;
	}
	*p = 0;
	p = rindex(name, '.');
	if (p == NULL) {
		return NULL;
	}
	*p = 0;
	p++;		/* p now points to start of hash val */
	return cb_file_create(cb_string_unique(name),
			      cb_string2hash(p),
			      directory,
			      sub_dir);
}
#endif

/*
 *	cb_get_sub_dir_string(sub_dir_id)
 *		Maintain a cache of unique sub directory strings.
 */
char *
cb_get_sub_dir_string(sub_dir_id)
	int		sub_dir_id;
{
	static char	*sub_dirs[5];

	if ((sub_dir_id < 1) || (sub_dir_id > 4)) {
		cb_abort("Illegal sub_dir id %d\n", sub_dir_id);
	}
	if (sub_dirs[1] == NULL) {
		sub_dirs[CB_NEW_DIR_ID] = cb_string_unique(CB_NEW_DIR);
		sub_dirs[CB_OLD_DIR_ID] = cb_string_unique(CB_OLD_DIR);
		sub_dirs[CB_REFD_DIR_ID] = cb_string_unique(CB_REFD_DIR);
		sub_dirs[CB_LOCKED_DIR_ID] = cb_string_unique(CB_LOCKED_DIR);
	}
	return sub_dirs[sub_dir_id];
}

/*
 *	cb_hash2string(hash_value, destination)
 *		Build the string representation of the hash value for a .cb name
 *		Returns the length of the string delivered.
 */
int
cb_hash2string(hash_value, destination)
	register unsigned	hash_value;
	register char		*destination;
{
	register int	count;

	destination += 5;
	count = 6;
	while (count-- > 0) {
		*destination-- = cb_hash_chars[hash_value & 077];
		hash_value >>= 6;
	}
	destination += 7;
	*destination = 0;
	return 6;
}

/*
 *	cb_src_hash2string(directory, subdir, src, hash_value, destination)
 *		Build the string representation of a .cb name.
 */
void
cb_src_hash2string(directory, subdir, src, hash_value, destination)
	Cb_directory		directory;
	char			*subdir;
	register char		*src;
	register unsigned	hash_value;
	register char		*destination;
{
	register char	*dir;
	register char	*p;
	register char	*q = NULL;
	register int	count;

	dir = directory->name;
	if ((dir[0] != '.') || (dir[1] != 0)) {
		for (p = dir; *p != '\0'; ) {
			*destination++ = *p++;
		}
		*destination++ = '/';
	}
	for (p = CB_DIRECTORY; *p != 0; ) {
		*destination++ = *p++;
	}
	*destination++ = '/';
	while (*subdir != 0) {
		*destination++ = *subdir++;
	}
	*destination++ = '/';
	for (p = src; *p != 0; p++) {
		if (*p == '/') {
			q = p;
		}
	}
	if (q == NULL) {
		p = src;
	} else {
		p = q + 1;
	}
	for (; *p != 0; ) {
		*destination++ = *p++;
	}
	*destination++ = '.';
	destination += 5;
	count = 6;
	while (count-- > 0) {
		*destination-- = cb_hash_chars[hash_value & 077];
		hash_value >>= 6;
	}
	destination += 7;
	for (p = CB_SUFFIX; *p != 0; ) {
		*destination++ = *p++;
	}
	*destination = 0;
}

/*
 *	cb_string2hash(hash_value)
 *		Scan the string representation of a .cb name hash value
 *		The function will read the "src.c.hash.cb" representation
 *		or just "hash".
 */
unsigned
cb_string2hash(hash_value)
	char	*hash_value;
{
	register char	*p;
	char		*q;
	register unsigned result = 0;
	register int	t;
static	char		char_value[128];
static	int		char_value_init = 0;

	if (char_value_init == 0) {
		char_value_init = 1;
		for (p = char_value, result = sizeof(char_value);
		     result-- > 0; ) {
			*p++ = 65;
		}
		for (result = 0, p = cb_hash_chars; *p != 0; p++) {
			char_value[*p] = result++;
		}
		result = 0;
	}
	q = rindex(hash_value, '.');
	if (q == NULL) {
		p = hash_value;
	} else {
		*q = 0;
		p = rindex(hash_value, '.');
		*q = '.';
		if (p == NULL) {
			return result;
		}
		p++;		/* p now points to start of hash val */
	}

	for (; *p != 0; ) {
		if ((t = char_value[*p++]) == 65) {
			return result;
		}
		result <<= 6;
		result |= t;
	}
	return result;
}

/*
 *	cb_file_dump()
 *		This will dump out the contents of the file table.
 */
void
cb_file_dump()
{
	Hash		file_table;

	file_table = cb_file_table_get();
	(void)hsh_iterate(file_table, cb_file_dump_print, 0);
}

/*
 *	cb_file_dump_print(zilch, file)
 *		This will print "file" out.
 */
/*ARGSUSED*/
static int
cb_file_dump_print(zilch, file)
	Cb_file		zilch;
	Cb_file		file;
{
	(void)printf("0x%x: %s\n", file, file->cb_file_name);
}
