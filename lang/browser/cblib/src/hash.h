/*	@(#)hash.h 1.1 94/10/31 SMI	*/

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


/*
 * This file contains the routine definitions for all of the routines exported
 * by the hash table package.
 */

#ifndef hash_h_INCLUDED
#define hash_h_INCLUDED

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

/* Typedef's */
typedef int 			(*Hash_Int_func)();
typedef	unsigned long		*Hash_value_ptr, Hash_value;
typedef struct Hash_bucket_tag	*Hash_bucket, Hash_bucket_rec;
typedef struct Hash_header_tag	*Hash_header, Hash_header_rec;
typedef struct Hash_tag		*Hash, Hash_rec;

/* Data structures: */
struct Hash_bucket_tag {
	Hash_bucket	next;		/* Next hash bucket */
	int		index;		/* Hash index = key_hsh(key) */
	Hash_value	key;		/* Key */
	Hash_value	value;		/* Bucket value */
};

struct Hash_header_tag {
	Hash_bucket	buckets;	/* Pointer to first bucket in chain */
	unsigned int	length : 24;	/* Length of bucket chain */
	unsigned int	power : 8;	/* Table size for this bucket chain */
};

struct Hash_tag {
	Cb_heap		heap;		/* Chunk to allocate memory from */
	int		count;		/* Current number of buckets */
	Hash_header	headers;	/* Array of bucket headers */
	Hash_Int_func	key_equal;	/* Key equality test routine */
	Hash_Int_func	key_hsh;	/* Function to hash a key */
	int		index;		/* Last hash index */
	int		limit;		/* Max buckets before rehash */
	int		mask;		/* Hash index mask */
	int		power;		/* Log base 2 of table size */
	int		value_empty;	/* hsh_lookup() empty value */
	int		size;		/* Number of bucket headers */
};

/* Macros: */
#define		hsh_size(hsh) ((hsh)->count)

/*
 *	hsh_bucket_get_value(bucket)
 *		This will return the value stored in "bucket".
 */
#define hsh_bucket_get_value(bucket) \
	((bucket)->value)

/*
 *	hsh_bucket_set_value(bucket, value)
 *		This will store "value" into "bucket".
 */
#define hsh_bucket_set_value(bucket, val) \
	((bucket)->value = (val))

#define HSH_BUCKET_SET_KEY(bucket, dummy, key_value) \
	((bucket)->key = (key_value))

#define HSH_STRING_EQUAL(cap, cbp, result) \
	result = 0; \
	while (*cap == *cbp++) { \
		if (*cap++ == 0) { \
			result = 1; \
			break; \
		} \
	}
#define HSH_STRING_HASH(string, result) \
	register unsigned	chr; \
	register unsigned char	*p = (unsigned char *)(string); \
	\
	result = *p++; \
	if (result != 0) { \
		if ((chr = *p++) != '\0') { \
			result += chr << 6; \
			if ((chr = *p++) != '\0') { \
				result += chr << 10; \
				while ((chr = *p++) != '\0') { \
					result += chr; \
				} \
			} \
		} \
	}

/* Routines defined in this package: */
PORT_EXTERN(Hash_bucket	hsh_bucket_obtain(PORT_2ARG(Hash, Hash_value)));
PORT_EXTERN(void	hsh_bucket_set_key(PORT_3ARG(Hash_bucket,
						     Hash,
						     Hash_value)));
PORT_EXTERN(Hash	hsh_create(PORT_5ARG(int,
					     Hash_Int_func,
					     Hash_Int_func,
					     Hash_value,
					     Cb_heap)));
PORT_EXTERN(int		hsh_delete(PORT_2ARG(Hash, Hash_value)));
PORT_EXTERN(void	hsh_destroy(PORT_1ARG(Hash)));
PORT_EXTERN(void	hsh_eliminate(PORT_3ARG(Hash, Hash_Int_func, int)));
PORT_EXTERN(int		hsh_iterate(PORT_3ARG(Hash, Hash_Int_func, int)));
PORT_EXTERN(int		hsh_iterate2(PORT_4ARG(Hash,
					       Hash_Int_func,
					       int,
					       int)));
PORT_EXTERN(int		hsh_iterate3(PORT_5ARG(Hash,
					       Hash_Int_func,
					       int,
					       int,
					       int)));
PORT_EXTERN(Hash_value	hsh_insert(PORT_3ARG(Hash, Hash_value, Hash_value)));
PORT_EXTERN(void	hsh_invariant(PORT_1ARG(Hash)));
PORT_EXTERN(Hash_value	hsh_lookup(PORT_2ARG(Hash, Hash_value)));
PORT_EXTERN(int		hsh_replace(PORT_3ARG(Hash, Hash_value, Hash_value)));
PORT_EXTERN(int		hsh_string_equal(PORT_1ARG(char*)));
PORT_EXTERN(int		hsh_string_hash(PORT_1ARG(char*)));
PORT_EXTERN(int		hsh_int_equal(PORT_1ARG(int)));
PORT_EXTERN(int		hsh_int_hash(PORT_1ARG(int)));
#endif
