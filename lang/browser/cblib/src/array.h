/*	@(#)array.h 1.1 94/10/31 SMI	*/

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
 * This file defines the data structures used to support the dynamic array
 * package.
 */

#ifndef array_h_INCLUDED
#define array_h_INCLUDED

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_portability_h_INCLUDED
#include "cb_portability.h"
#endif

typedef	unsigned long		*Array_value_ptr, Array_value;
typedef struct Array_tag	*Array, Array_rec;

struct Array_tag {
	int		size;		/* Logical array size */
	int		limit;		/* Physical array size */
	Array_value	*data;		/* Pointer to array containing data */
	Cb_heap		heap;		/* Chunk to allocate memory from */
};

/* Macros */
#define	array_size(array)		((array)->size)
#define	array_empty(array)		(array_size(array) == 0)
#define	ARRAY_FETCH(array, index)	((array)->data[index])
#define ARRAY_STORE(array, index, value) ((array)->data[index] = (value))

/*
 * array_append_array(array1, array2) will append the contents of "array2" to
 * the end of "array1". O(n)
 */
#define array_append_array(array1, array2) \
	array_insert_array((array1), (array1)->size, (array2))


/* Routines provided by this package. */
PORT_EXTERN(void	array_append(PORT_2ARG(Array, Array_value)));
PORT_EXTERN(Array	array_copy(PORT_3ARG(Array, int (*)(), int)));
PORT_EXTERN(Array	array_create(PORT_1ARG(Cb_heap)));
PORT_EXTERN(Array_value	array_delete(PORT_2ARG(Array, int)));
PORT_EXTERN(void	array_destroy(PORT_1ARG(Array)));
PORT_EXTERN(int		array_eliminate(PORT_3ARG(Array, int (*)(), int)));
PORT_EXTERN(int		array_erase(PORT_3ARG(Array, int (*)(), int)));
PORT_EXTERN(void	array_expand(PORT_2ARG(Array, int)));
PORT_EXTERN(int		array_fetch(PORT_2ARG(Array, int)));
PORT_EXTERN(void	array_fill(PORT_3ARG(Array, int, Array_value)));
PORT_EXTERN(void	array_insert(PORT_3ARG(Array, int, Array_value)));
PORT_EXTERN(void	array_insert_array(PORT_3ARG(Array, int, Array)));
PORT_EXTERN(Array_value	array_lop(PORT_1ARG(Array)));
PORT_EXTERN(Array_value	array_pop(PORT_1ARG(Array)));
PORT_EXTERN(void	array_reverse(PORT_1ARG(Array)));
PORT_EXTERN(void	array_reverse_sort(PORT_2ARG(Array, int (*)())));
PORT_EXTERN(void	array_sort(PORT_3ARG(Array, int (*)(), int)));
PORT_EXTERN(void	array_store(PORT_3ARG(Array, int, Array_value)));
PORT_EXTERN(Array_value	array_tail(PORT_1ARG(Array)));
PORT_EXTERN(void	array_trim(PORT_2ARG(Array, int)));
PORT_EXTERN(void	array_unique(PORT_2ARG(Array, int (*)())));

#endif
