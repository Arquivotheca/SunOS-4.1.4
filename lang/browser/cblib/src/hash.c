#ident "@(#)hash.c 1.1 94/10/31"

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


/*
 * This file contains a bunch of routines that implement a general purpose
 * hash table.
 */

/* LINTLIBRARY */

#ifndef FILE
#include <stdio.h>
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

#ifndef hash_h_INCLUDED
#include "hash.h"
#endif

/* Make sure that TRUE and FALSE are defined. */
#ifndef	TRUE
#define	TRUE	1		/* Truth */
#endif
#ifndef	FALSE
#define	FALSE	0		/* Falsehood */
#endif

#define	Bucket		Hash_bucket
#define	Bucket_rec	Hash_bucket_rec
#define	Header		Hash_header
#define	Header_rec	Hash_header_rec
#define	Int_func	Hash_Int_func

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	Bucket		hsh_bucket_find();
extern	Bucket		hsh_bucket_obtain();
extern	void		hsh_bucket_set_key();
extern	Hash		hsh_create();
extern	int		hsh_delete();
extern	void		hsh_destroy();
extern	void		hsh_eliminate();
extern	void		hsh_free_memory();
extern	Hash_value	hsh_insert();
extern	void		hsh_invariant();
extern	int		hsh_iterate();
extern	int		hsh_iterate2();
extern	int		hsh_iterate3();
extern	void		hsh_lazy_rehsh();
extern	int		hsh_lazy_rehsh_extract();
extern	Hash_value	hsh_lookup();
extern	void		hsh_rehsh();
extern	int		hsh_replace();
extern	int		hsh_string_equal();
extern	int		hsh_string_hash();
extern	int		hsh_int_equal();
extern	int		hsh_int_hash();

/* libgen */
/*
 *	hsh_bucket_find(hsh, key)
 *		This will see whether it can find a bucket whose key  equals
 *		"key" and returns it.  If no such bucket can be found, NULL
 *		will be returned.  This routine has the important side effect
 *		of performing any lazy rehashing for Key.
 */
static Bucket
hsh_bucket_find(hsh, key)
	register Hash		hsh;		/* Hash table to use */
	register Hash_value	key;		/* Key to insert */
{
	register Bucket		bucket;		/* Current bucket */
	register int		index;		/* Hash index value */
	Hash_header	 	header;
	register char		*cap;
	register char		*cbp;

	if (hsh->key_hsh == hsh_int_hash) {
		index = key;
	} else if (hsh->key_hsh == hsh_string_hash) {
		HSH_STRING_HASH(key, index);
	} else {
		index = hsh->key_hsh(key);
	}
	hsh->index = index;
	header = &hsh->headers[index & hsh->mask];
	if (header->power != hsh->power) {
		hsh_lazy_rehsh(hsh, index);
	}
	bucket = header->buckets;
	if (hsh->key_equal == hsh_int_equal) {
		for (; bucket != NULL; bucket = bucket->next) {
			if ((bucket->index == index) &&
			    (bucket->key == key)) {
				return bucket;
			}
		}
	} else if (hsh->key_equal == hsh_string_equal) {
		for (; bucket != NULL; bucket = bucket->next) {
			if (bucket->index == index) {
				cap = (char *)bucket->key;
				cbp = (char *)key;
				while (*cap == *cbp++) {
					if (*cap++ == 0) {
						return bucket;
					}
				}
			}
		}
	} else {
		for (; bucket != NULL; bucket = bucket->next) {
			if ((bucket->index == index) &&
			    hsh->key_equal(bucket->key, key)) {
				return bucket;
			}
		}
	}
	return NULL;
}

/* libgen */
/*
 *	hsh_bucket_obtain(hsh, key)
 *		This will obtain the bucket associated with "key" into "hsh".
 *		If "key" is not in "hsh", a new bucket will be returned with
 *		the empty value will be returned.  The value can be read using
 *		hsh_bucket_get_value() and the set using
 *		hsh_bucket_set_value().
 */
Bucket
hsh_bucket_obtain(hsh, key)
	register Hash		hsh;		/* Hash table to use */
	register Hash_value	key;		/* Key to use */
{
	register Bucket	bucket;	/* Current bucket */
	register Header	header;	/* Header for bucket list */

	if (hsh->count >= hsh->limit) {
		hsh_rehsh(hsh);
	}
	bucket = hsh_bucket_find(hsh, key);
	if (bucket == NULL) {
		header = &hsh->headers[hsh->index & hsh->mask];
		bucket = cb_allocate(Bucket, hsh->heap);
		bucket->next = header->buckets;
		header->buckets = bucket;
		header->length++;
		bucket->key = key;
		bucket->value = hsh->value_empty;
		bucket->index = hsh->index;
		hsh->count++;
	}
	return bucket;
}

/* libgen */
/*
 *	hsh_bucket_set_key(bucket, hsh, key)
 *		This will store "key" into "bucket".
 */
void
hsh_bucket_set_key(bucket, hsh, key)
	Bucket		bucket;
	Hash		hsh;
	Hash_value	key;
{
	register int	index;

	if (hsh->key_hsh == hsh_int_hash) {
		index = key;
	} else if (hsh->key_hsh == hsh_string_hash) {
		HSH_STRING_HASH(key, index);
	} else {
		index = hsh->key_hsh(key);
	}
	if (index != bucket->index) {
		cb_abort("Hash package: Bad key\n");
	}
	HSH_BUCKET_SET_KEY(bucket, 0, key);
}

/* libgen */
/*
 *	hsh_create(size, key_equal, key_hsh, value_empty, heap)
 *		This will create and return a hsh table with at least "size"
 *		slots.  "key_equal(key1, key2) => {True, False} is used to
 *		compare two keys for equality.  "key_hsh(key) => hsh_Index
 *		is used to hsh "key".  "value_empty" is returned by
 *		hsh_lookup() whenevery it can't find a "key" in "hsh".
 */
Hash
hsh_create(requested_size, key_equal, key_hsh, value_empty, heap)
	register int	requested_size;	/* Hash table size */
	Int_func	key_equal;	/* Key equality routine */
	Int_func	key_hsh;	/* Key hshing routine */
	Hash_value	value_empty;	/* Empty value */
	Cb_heap	heap;
{
	register Hash	hsh;		/* Hash table */
	register Header	header;		/* Current bucket header */
	register int	power;		/* Log base 2 of table size */
	register unsigned size;		/* Temporary table size */
static	Hash_rec	hash_rec_zero;
static	Header_rec	header_rec_zero;

	/* Find the power of two that is >= requested_size. */
	power = 0;
	size = 1;
	while (size < requested_size) {
		power++;
		size *= 2;
	}

	hsh = (Hash)cb_get_memory(sizeof(Hash_rec), heap);
	*hsh = hash_rec_zero;
	hsh->heap = heap;
	hsh->headers = (Header)cb_get_memory((int)(size * sizeof(Header_rec)),
						heap);
	hsh->key_equal = key_equal;
	hsh->key_hsh = key_hsh;
	hsh->limit = size * 7 / 10;	/* 70% */
	hsh->mask = size - 1;
	hsh->power = power;
	hsh->value_empty = value_empty;
	hsh->size = size;

	header = hsh->headers;
	while (size-- > 0) {
		*header = header_rec_zero;
		header->power = power;
		header++;
	}
	return hsh;
}

#ifndef _CBQUERY
/* libgen */
/*
 *	hsh_delete(hsh, key)
 *		This will remove "key" and its associated value from "hsh".
 *		If "key" is not in "hsh", TRUE will be returned.  Otherwise,
 *		FALSE will be returned.
 */
int
hsh_delete(hsh, key)
	Hash			hsh;		/* Hash table to use */
	register Hash_value	key;		/* Key to delete */
{
	register Bucket		bucket;		/* Current bucket */
	register int		index;		/* Key hsh index */
	register Header		header;		/* Bucket chain header */
	register Int_func	key_equal;	/* Key equality function */
	register Bucket		previous;	/* Previous bucket */

	key_equal = hsh->key_equal;
	if (hsh->key_hsh == hsh_int_hash) {
		index = key;
	} else if (hsh->key_hsh == hsh_string_hash) {
		HSH_STRING_HASH(key, index);
	} else {
		index = hsh->key_hsh(key);
	}
	header = &hsh->headers[index & hsh->mask];
	for (bucket = header->buckets, previous = NULL; bucket != NULL;
	     previous = bucket, bucket = bucket->next) {
		if ((bucket->index == index) && key_equal(bucket->key, key)) {
			if (previous == NULL) {
				header->buckets = bucket->next;
			} else {
				previous->next = bucket->next;
			}
			header->length--;
			hsh_free_memory((char *)bucket, sizeof(Bucket_rec),
					hsh->heap);
			return FALSE;
		}
	}
	return TRUE;
}
#endif

/* libgen */
/*
 *	hsh_destroy(hsh)
 *		This will destroy the contents of "hsh".
 */
void
hsh_destroy(hsh)
	register Hash	hsh;		/* Hash table to destroy */
{
	hsh_free_memory((char *)hsh->headers,
			hsh->size * sizeof(Header_rec),
			hsh->heap);
	hsh_free_memory((char *)hsh,
			sizeof(Hash),
			hsh->heap);
}

#ifndef _CBQUERY
/* libgen */
/*
 *	hsh_eliminate(hsh, routine, handle)
 *		This will scan through "hsh" and eliminate any table entry
 *		for which "routine(key, value, handle)" returns TRUE.
 *		
 */
void
hsh_eliminate(hsh, routine, handle)
	Hash		hsh;
	register Int_func routine;
	register int	handle;
{
	register Bucket	bucket;
	register Header	header;
	register int	index;
	register Bucket	previous;
	register int	size;

	size = hsh->size;
	header = hsh->headers;
	for (index = 0; index < size; index++) {
		for (bucket = header->buckets, previous = NULL; bucket != NULL;
		     previous = bucket, bucket = bucket->next) {
			if (routine(bucket->key, bucket->value, handle)) {
				if (previous == NULL) {
					header->buckets = bucket->next;
				} else {
					previous->next = bucket->next;
				}
				header->length--;
				hsh_free_memory((char *)bucket,
						sizeof(Bucket_rec), hsh->heap);
			}
		}
		header++;
	}
}
#endif

/* libgen */
/*
 *	hsh_free_memory(data, size, heap)
 *		This will deallocate "size" bytes of "data".
 */
/* ARGSUSED */
static void
hsh_free_memory(data, size, heap)
	char		*data;		/* Data to free */
	int		size;		/* Number of bytes to free */
	Cb_heap	heap;
{
	if (heap == NULL) {
		cb_free(data);
	}
}

/* libgen */
/*
 *	hsh_insert(hsh, key, value)
 *		This will insert "value" into "hsh" associated with "key".
 *		If "key" is already in "hsh", "value" will NOT be inserted
 *		and the old value is returned.
 *		Otherwise, NULL will be returned.
 */
Hash_value
hsh_insert(hsh, key, value)
	register Hash		hsh;		/* Hash table to use */
	register Hash_value	key;		/* Key to use */
	Hash_value		value;		/* Value to insert into table */
{
	register Bucket		bucket;	/* Current bucket */
	register Header		header;	/* Header for bucket list */

	if (hsh->count >= hsh->limit) {
		hsh_rehsh(hsh);
	}
	bucket = hsh_bucket_find(hsh, key);
	if (bucket == NULL) {
		header = &hsh->headers[hsh->index & hsh->mask];
		bucket = cb_allocate(Bucket, hsh->heap);
		bucket->next = header->buckets;
		header->buckets = bucket;
		header->length++;
		bucket->key = key;
		bucket->value = value;
		bucket->index = hsh->index;
		hsh->count++;
		return NULL;
	} else {
		return bucket->value;
	}
}

#ifndef _CBQUERY
/* libgen */
/*
 *	hsh_invariant(hsh)
 *		This will ensure that the lazy rehashing invariant holds true.
 *		This routine is only used for debugging.
 */
void
hsh_invariant(hsh)
	register Hash	hsh;		/* Hash table to use */
{
	register Bucket	bucket;		/* Current bucket */
	register int	full_mask;	/* Full hsh table mask */
	register Header	headers;	/* Bucket headers */
	register int	index;		/* Index into hsh table */
	register int 	mask;		/* Current mask */
	register int	power;		/* Log base 2 of table size */
	register int	size;		/* Hash table size */

	headers = hsh->headers;
	full_mask = hsh->mask;
	power = hsh->power;
	size = hsh->size;
	for (index = 0; index < size; index++) {
		if (headers[index].power != power) {
			continue;
		}
		for (mask = full_mask >> 1; mask != 0; mask >>= 1) {
			if (headers[index & mask].power != power) {
				(void)fprintf(stderr,
					      "Bad invariant at index %d\n",
					      index);
				cb_fflush(stderr);
				abort();
			}
		}

		for (bucket = headers[index].buckets;
		     bucket != NULL; bucket = bucket->next) {
			if ((bucket->index & full_mask) != index) {
				(void)fprintf(stderr,
					      "Bad invariant at index %d\n",
					      index);
				cb_fflush(stderr);
				abort();
			}
		}
	}
}
#endif

/* libgen */
/*
 *	hsh_iterate(hsh, routine, handle)
 *		This will scan the entire contents of "hsh" calling
 *		"routine(key, value, handle)=>int" for each "key"-"value"
 *		pair in "hsh".  The sum of the values returned by "routine"
 *		will be returned.
 */
int
hsh_iterate(hsh, routine, handle)
	Hash		hsh;
	register Int_func routine;
	register int	handle;
{
	register Bucket	bucket;
	register Header	header;
	register int	size;
	register int	sum;

	size = hsh->size;
	sum = 0;
	for (header = &hsh->headers[0]; --size >= 0; header++) {
		for (bucket = header->buckets;
		     bucket != NULL;
		     bucket = bucket->next) {
			sum += routine(bucket->key, bucket->value, handle);
		}
	}
	return sum;
}

/* libgen */
/*
 *	hsh_iterate2(hsh, routine, handle1, handle2)
 *		This will scan the entire contents of "hsh" calling
 *		"routine(key, value, handle1, handle2)=>int" for each
 *		"key"-"value" pair in "hsh".  The sum of the values
 *		returned by "routine" will be returned.
 */
int
hsh_iterate2(hsh, routine, handle1, handle2)
	Hash		hsh;
	register Int_func routine;
	register int	handle1;
	register int	handle2;
{
	register Bucket	bucket;
	register Header	header;
	register int	size;
	register int	sum;

	size = hsh->size;
	sum = 0;
	for (header = &hsh->headers[0]; --size >= 0; header++) {
		for (bucket = header->buckets;
		     bucket != NULL;
		     bucket = bucket->next) {
			sum += routine(bucket->key, bucket->value,
				       handle1, handle2);
		}
	}
	return sum;
}

/* libgen */
/*
 *	hsh_iterate3(hsh, routine, handle1, handle2, handle3)
 *		This will scan the entire contents of "hsh" calling
 *		"routine(key, value, handle1, handle2, handle3)=>int" for each
 *		"key"-"value" pair in "hsh".  The sum of the values
 *		returned by "routine" will be returned.
 */
int
hsh_iterate3(hsh, routine, handle1, handle2, handle3)
	Hash		hsh;
	register Int_func routine;
	register int	handle1;
	register int	handle2;
	register int	handle3;
{
	register Bucket	bucket;
	register Header	header;
	register int	size;
	register int	sum;

	size = hsh->size;
	sum = 0;
	for (header = &hsh->headers[0]; --size >= 0; header++) {
		for (bucket = header->buckets;
		     bucket != NULL;
		     bucket = bucket->next) {
			sum += routine(bucket->key, bucket->value,
				       handle1, handle2, handle3);
		}
	}
	return sum;
}

/* libgen */
/*
 *		hsh_lazy_rehsh(hsh, index)
 *			This  will rehsh all of the chains that coincide
 *			with "index".
 */
static void
hsh_lazy_rehsh(hsh, index)
	Hash		hsh;
	int		index;
{
	register Bucket	bucket;
	Bucket		buckets[32];
	register Header	header;
	register Header	headers;
	register int	mask;
	register int	min_power;
	register Bucket	next;
	register int	power;

	headers = hsh->headers;
	mask = hsh->mask;
	min_power = hsh_lazy_rehsh_extract(hsh, index, buckets);
	for (power = hsh->power; power >= min_power; power--) {
		bucket = buckets[power];
		while (bucket != NULL) {
			next = bucket->next;
			header = &headers[mask & bucket->index];
			bucket->next = header->buckets;
			header->buckets = bucket;
			header->length++;
			bucket = next;
		}
	}
}

/*
 *	hsh_lazy_rehsh_extract(hsh, index, buckets)
 *		This will scan through the hash slots "hsh" associated with
 *		"index" and extract them into "buckets".  The minimum power
 *		is returned.
 */
static int
hsh_lazy_rehsh_extract(hsh, index, buckets)
	Hash		hsh;
	register int	index;
	register Bucket	*buckets;
{
	register Header	header;
	register Header	headers;
	register int	mask;
	register int	min_power;
	register int	power;
	register int	temp_power;

	/* Find all chains that were hshed at a smaller hsh table size. */
	headers = hsh->headers;
	power = hsh->power;
	min_power = power;
	temp_power = power;
	mask = hsh->mask;
	while ((temp_power >= 0) &&
	       (headers[index & mask].power != power)) {
		header = &headers[index & mask];
		buckets[temp_power] = header->buckets;
		header->buckets = NULL;
		header->length = 0;
		min_power = temp_power;
		temp_power--;
		mask >>= 1;
	}
	mask = hsh->mask;
	for (temp_power = power; temp_power >= min_power; temp_power--) {
		headers[index & mask].power = power;
		mask >>= 1;
	}
	return min_power;
}

/* libgen */
/*
 *	hsh_lookup(hsh, key)
 *		This will return the value associated with "key" from "hsh".
 *		If "key" is not in "hsh", the empty value will be returned.
 */
Hash_value
hsh_lookup(hsh, key)
	Hash		hsh;		/* Hash table to use */
	Hash_value	key;		/* Key to lookup  */
{
	register Bucket	bucket;		/* Bucket from hsh table */

	bucket = hsh_bucket_find(hsh, key);
	return (bucket == NULL) ? hsh->value_empty : bucket->value;
}

/* libgen */
/*
 *	hsh_rehsh(hsh)
 *		This will double the size of "hsh".
 */
static void
hsh_rehsh(hsh)
	register Hash	hsh;		/* Hash table to double */
{
	register int	new_size;	/* New hsh table size */
	register int	old_size;	/* Old hsh table size */
	char		*old;

	old_size = hsh->size;
	new_size = old_size * 2;
	old = (char *)hsh->headers;
	hsh->headers = (Header)cb_get_memory(new_size * sizeof(Header_rec),
						hsh->heap);
	(void)bcopy(old, (char *)hsh->headers, old_size * sizeof(Header_rec));
	bzero((char *)&hsh->headers[old_size], old_size * sizeof(Header_rec));
	hsh->limit = new_size * 7 / 10;	/* 70% */
	hsh->size = new_size;
	hsh->mask = new_size - 1;
	hsh->power++;
}

#ifndef _CBQUERY
/* libgen */
/*
 *	hsh_replace(hsh, key, value)
 *		This will change the value in "hsh" associated with "key"
 *		to be "value".  If "key" is not in "hsh", both "key" and
 *		"value" will be inserted and TRUE will be returned.
 *		Otherwise, FALSE will be returned.
 */
int
hsh_replace(hsh, key, value)
	Hash		hsh;		/* Hash table to use */
	Hash_value	key;		/* Key to use */
	Hash_value	value;		/* Value to replace with */
{
	register Bucket	bucket;		/* Bucket form hsh table */

	bucket = hsh_bucket_find(hsh, key);
	if (bucket == NULL) {
		(void)hsh_insert(hsh, key, value);
		return TRUE;
	} else {
		bucket->value = value;
		return FALSE;
	}
}
#endif

/*
 *	hsh_string_equal(cap, cbp)
 *		Return return 1 if strings are equal otherwise return 0.
 */
int
hsh_string_equal(cap, cbp)
	register char	*cap;
	register char	*cbp;
{
	register int	result;

	HSH_STRING_EQUAL(cap, cbp, result);
	return result;
}

/*
 *	hsh_string_hash(key)
 *		Return hash value for "key".
 */
int
hsh_string_hash(key)
	char		*key;
{
	register int	result;

	HSH_STRING_HASH(key, result);
	return result;
}

/*
 *	hsh_int_equal(cap, cbp)
 *		Return return 1 if ints are equal otherwise return 0.
 */
int
hsh_int_equal(cap, cbp)
	int	cap;
	int	cbp;
{
	return cap == cbp;
}

/*
 *	hsh_int_hash(key)
 *		Return hash value for "key".
 */
int
hsh_int_hash(key)
	int	key;
{
	return key;
}

