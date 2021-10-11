#ident "@(#)cb_string.c 1.1 94/10/31 SMI"

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


#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"		
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"		
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"		
#endif

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	char		*cb_string_cat();
extern	char		*cb_string_copy();
extern	char		*cb_string_copy_n();
extern	char		*cb_string_unique();
extern	char		*cb_string_unique_n();

/*
 *	cb_string_cat(args)
 *		This will concatonate the NULL termianted list of strings
 *		passed in as "args" and return the result as a unique string.
 */
/*VARARGS0*/
char *
cb_string_cat(va_alist)
	va_dcl
{
	va_list		pointer;
	int		size;
	char		*next;
	char		*buffer;

	va_start(pointer);
	size = 0;
	while ((next = va_arg(pointer, char *)) != NULL) {
		size += strlen(next);
	}
	va_end(pointer);
	buffer = alloca(size + 1);
	*buffer = '\0';
	va_start(pointer);
	while ((next = va_arg(pointer, char *)) != NULL) {
		(void)strcat(buffer, next);
	}
	va_end(pointer);
	return cb_string_unique(buffer);
}

/*
 *	cb_string_copy(string, heap)
 *		Malloc memory, copy "string" into it, and return the result.
 *		Ensure that the returned string is word aligned so that
 *		very fast string compares can be used.
 */
char *
cb_string_copy(string, heap)
	char		*string;
	Cb_heap		heap;
{
#	define mask	(sizeof(int) - 1)
	char		*p;

	p = (char *)cb_get_memory(((strlen(string) + 1) + mask) & (~mask),
				  heap);
	return strcpy(p, string);
}

/*
 *	cb_string_copy_n(string, length, heap)
 *		Malloc memory, copy "string" into it, and return the result.
 *		Ensure that the returned string is word aligned so that
 *		very fast string compares can be used.
 */
char *
cb_string_copy_n(string, length, heap)
	char		*string;
	int		length;
	Cb_heap		heap;
{
#	define mask	(sizeof(int) - 1)
	char		*p;

	p = (char *)cb_get_memory(((length + 1) + mask) & (~mask), heap);
	p[length] = 0;
	return strncpy(p, string, length);
}

/*
 *	cb_string_unique(string)
 *		This will return a pointer to a unique copy of "string".
 */
char *
cb_string_unique(string)
	char			*string;
{
	return cb_string_unique_n(string, strlen(string));
}

/*
 *	cb_string_unique_n(string, length)
 *		This will return a pointer to a unique copy of "string".
 */
char *
cb_string_unique_n(string, length)
	char			*string;
	int			length;
{
	register Hash_bucket	bucket;
static	Hash			string_table = NULL;
	register char		*result;

	if (string[length] != 0) {
		result = alloca(length+1);
		(void)strncpy(result, string, length);
		result[length] = 0;
		string = result;
	}

	if (string_table == NULL) {
		string_table = hsh_create(1000,
					  hsh_string_equal,
					  hsh_string_hash,
					  (Hash_value)NULL,
					  cb_init_heap(0));
		cb_free_heap_on_exit(string_table->heap);
	}
	bucket = hsh_bucket_obtain(string_table, (Hash_value)string);
	if ((result = (char *)hsh_bucket_get_value(bucket)) == NULL) {
		result = cb_string_copy_n(string, length, string_table->heap);
		HSH_BUCKET_SET_KEY(bucket, string_table, (Hash_value)result);
		hsh_bucket_set_value(bucket, (Hash_value)result);
	}
	return result;
}

