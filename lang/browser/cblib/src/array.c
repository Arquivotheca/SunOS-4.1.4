#ident "@(#)array.c 1.1 94/10/31 Copyr 1986 Sun Micro"

#ifndef lint
#ifdef INCLUDE_COPYRIGHT
static char _copyright_notice_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.\
Sun considers its source code as an unpublished, proprietary\
trade secret, and it is available only under strict license\
provisions.  This copyright notice is placed here only to protect\
Sun in the event the source is deemed a published work.  Dissassembly,\
decompilation, or other means of reducing the object code to human\
readable form is prohibited by the license agreement under which\
this code is provided to the user or company in possession of this\
copy.\
RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the\
Government is subject to restrictions as set forth in subparagraph\
(c)(1)(ii) of the Rights in Technical Data and Computer Software\
clause at DFARS 52.227-7013 and in similar clauses in the FAR and\
NASA FAR Supplement.";
#endif

#ifdef INCLUDE_COPYRIGHT_REFERENCE
static char _copyright_notice_reference_[] =
"Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved. \
See copyright notice in cb_copyright.o in the libsb.a library.";
#endif
#endif


/*
 * This file implements a dynamic array package.  A dynamic array is an array
 * whose upper bound can be changed dynamically at run-time.  The index for
 * the first element in the array is always zero.  Whenever, it is necessary
 * to grow the array, this package will copy the information in the array
 * from the old array to the new array and deallocate the old array.  This
 * is all done transparently to the user.
 *
 * A number of the routines iterate over all of the elements in the array by
 * calling yet another routine.  This other routine will always called with
 * a client supplied argument called "handle".  The C argument passing
 * scheme permits the user to safely omit this data argument.  All routines
 * that iterate over arrays will iterate from the lowest index to the highest
 * index.
 *
 * Any time an error occurs, an error message is printed and the program is
 * terminated.  The reason for this doing this is to free the user from
 * always having to check for error conditions each time a routine is called.
 * There is always a way of avoiding errors.
 */

/* LINTLIBRARY */

#ifndef FILE
#include <stdio.h>
#endif

#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef cb_heap_h_INCLUDED
#include "cb_heap.h"
#endif

#ifndef cb_misc_h_INCLUDED
#include "cb_misc.h"
#endif

/* Get TRUE and FALSE defined. */
#ifndef TRUE
#define	TRUE		1	/* Truth */
#endif
#ifndef FALSE
#define	FALSE		0	/* Falsehood */
#endif

/* Check to make sure that array index is in bounds. */
#define	array_bounds(array, index) \
	if ((unsigned)index >= (unsigned)array->size) { \
		array_bounds_error(array, index); \
		/* NOTREACHED */ \
	}

/*
 * array_free_data(data, size, heap) will destroy the "size" words of "data"
 * allocated by array_get_data().
 */
#define array_free_data(data, size, heap) \
	array_free_memory((char *)(data), (size) << 2, (heap));

/*
 * array_free_memory(data, size, heap) will destroy the "size" bytes of "data"
 * allocated by cb_get_memory().
 */
#define array_free_memory(data, size, heap) \
	if ((heap) == NULL) { \
		cb_free(data); \
	}
	
/*
 * array_get_data(size, heap) will allocate and return "size" words of data.
 */
#define array_get_data(size, heap) \
	(Array_value *)cb_get_memory((size) << 2, (heap))

/*
 *	File table of contents. Lists all the functions defined in the file.
 */
extern	void		array_append();
extern	void		array_bounds_error();
extern	Array		array_copy();
extern	Array		array_create();
extern	Array_value	array_delete();
extern	void		array_destroy();
extern	int		array_eliminate();
extern	int		array_erase();
extern	void		array_expand();
extern	int		array_fetch();
extern	void		array_fill();
extern	void		array_insert();
extern	void		array_insert_array();
extern	int		array_int_compare();
extern	Array_value	array_lop();
extern	Array_value	array_pop();
extern	void		array_reverse();
extern	void		array_reverse_sort();
extern	void		array_sort();
extern	void		array_store();
extern	Array_value	array_tail();
extern	void		array_trim();
extern	void		array_unique();

extern	void		array_grow();
extern	void		array_resize();
extern	void		array_xsort();

/* libgen */
/*
 * array_append(array, value) will append "value" onto end of "array". O(k)
 */
void
array_append(array, value)
	register Array	array;		/* Array to append to */
	Array_value	value;		/* Value to append */
{
	if (array->size == array->limit) {
		array_grow(array);
	}
	array->data[array->size++] = value;
}

/* libgen */
/*
 * array_bounds_error(array, index) will print a bounds error for "array" and
 * "index".
 */
void
array_bounds_error(array, index)
	Array		array;		/* Array that cause problem */
	int		index;		/* Index that is out of bounds */
{
	cb_abort("Array package: Bounds error (%d not in [0:%d])\n",
		 index,
		 array->size - 1);
	/* NOTREACHED */
}

/* libgen */
/*
 * array_copy(array, routine, handle) returns a copy of "array" using
 * "routine(value, handle)" => value to make a copy of each element.
 * If Routine is NULL, no routine will be called to make a copy of each
 * element. O(n)
 */
Array
array_copy(array, routine, handle)
	Array			array;		/* Array to copy */
	register int		(*routine)();	/* Routine to copy element */
	int			handle;		/* Client handle */
{
	register Array_value	*data;		/* Array data */
	Array			new_array;	/* New array */
	register int		size;		/* Array size */

	size = array->size;
	new_array = array_create(array->heap);
	array_resize(new_array, size);
	bcopy((char *)array->data,
	      (char *)new_array->data,
	      size * sizeof(int));
	if (routine != NULL) {
		for (data = array->data, new_array->size = size;
		     size-- > 0;
		     data++) {
			*data = routine(*data, handle);
		}
	}
	return new_array;
}

/* libgen */
/*
 * array_create(heap)
 * allocates and returns storage for a new array.  The initial
 * array size is zero.  O(k)
 */
Array
array_create(heap)
	Cb_heap	heap;
{
	register Array	array;		/* New array */

	array = (Array)cb_get_memory(sizeof *array, heap);
	array->size = 0;
	array->limit = 0;
	array->data = NULL;
	array->heap = heap;
	return array;
}

#ifndef _CBQUERY
/* libgen */
/*
 * array_delete(array, index)=>value removes and returns "index"'th element
 * from middle of "array".  Bounds errors are fatal. O(n)
 */
Array_value
array_delete(array, index)
	Array			array;		/* Array to delete elem from */
	int			index;		/* Index to delete */
{
	register int		count;		/* Number of elements to move*/
	register Array_value	*dest;		/* Destination pointer */
	int			size;		/* Array size */
	register Array_value	*src;		/* Source pointer */
	int			value;		/* Value deleted from array */

	array_bounds(array, index);
	size = array->size--;
	dest = &array->data[index];
	src = dest + 1;
	value = *dest;
	count = size - index - 1;
	while (count-- > 0) {
		*dest++ = *src++;
	}
	return value;
}
#endif

/* libgen */
/*
 * array_destroy(array) destroys storage for "array". O(k)
 */
void
array_destroy(array)
	register Array	array;		/* Array to destroy */
{
	if (array->data != NULL) {
		array_free_data(array->data, array->limit, array->heap);
	}
	array_free_memory((char *)array, sizeof *array, array->heap);
}

/* libgen */
/*
 * array_eliminate(array, routine, handle) will scan through "array" and
 * eliminate any element for which "routine(element, handle)" returns
 * TRUE.
 */
int
array_eliminate(array, routine, handle)
	Array			array;		/* Array to use */
	int			(*routine)();	/* Routine to call with */
	int			handle;		/* Client handle */
{
	register int		count;		/* Elements remaining to scan*/
	register Array_value	*source;	/* Source pointer */
	register Array_value	*dest;		/* Destination pointer */
	int			size;		/* New size */

	count = array->size;
	source = array->data;
	dest = source;
	size = 0;
	while (count-- != 0) {
		if (!routine(*source, handle)) {
			*dest++ = *source;
			size++;
		}
		source++;
	}
	count = array->size - size;
	array->size = size;
	return count;
}

#ifndef _CBQUERY
/* libgen */
/*
 * array_erase(array, routine, handle)=>sum will call
 * "routine(value, handle)=>count" on each element of "array".  Afterwards,
 * "array" will be destroyed and the "sum" of the returned "count"'s will be
 * returned. O(n)
 */
int
array_erase(array, routine, handle)
	Array			array;		/* Array to erase */
	register int		(*routine)();	/* Routine to erase with */
	int			handle;		/* Client handle */
{
	register Array_value	*data;		/* Pointer to data */
	register int		size;		/* Array size */
	int			sum;		/* Sum of returned counts */

	data = array->data;
	size = array->size;
	sum = 0;
	while (size-- > 0) {
		sum += routine(*data++, handle);
	}
	array_destroy(array);
	return sum;
}
#endif

/* libgen */
/*
 * array_expand(array, predict) preallocates memory for up to "predict" entries
 * in "array" without actually changing the size. O(n)
 */
void
array_expand(array, predict)
	register Array		array;		/* Array to expand */
	register int		predict;	/* Predicted array size */
{
	if (predict > array->limit) {
		array_resize(array, predict);
	}
}

/* libgen */
/*
 * array_fetch(array, index)=>value returns "index"'th element of "array".
 * Bounds errors are fatal. O(k)
 */
int
array_fetch(array, index)
	register Array	array;		/* Array to fetch from*/
	register int	index;		/* Index to use */
{
	array_bounds(array, index);
	return ARRAY_FETCH(array, index);
}

/* libgen */
/*
 * array_fill(array, size, value) makes "array" contain at least "size"
 * elements and fills them with "value". O(n)
 */
void
array_fill(array, size, value)
	Array			array;		/* Array to use */
	register int		size;		/* Size to expand to */
	register Array_value	value;		/* Value to insert */
{
	register Array_value	*data;		/* Pointer to data */

	if (array->limit < size) {
		array_resize(array, size);
	}
	if (array->size < size) {
		array->size = size;
	}
	data = array->data;
	while (size-- > 0) {
		*data++ = value;
	}
}

#ifndef _CBQUERY
/* libgen */
/*
 * array_insert(array, index, value) stores "value" into middle of "array" at
 * "index".  Bounds errors are fatal. O(n)
 */
void
array_insert(array, index, value)
	Array			array;		/* Array to use insert into */
	int			index;		/* Index into array */
	Array_value		value;		/* Value to insert */
{
	register int		count;		/* Count of elements to move */
	register Array_value	*dest;		/* Destination pointer */
	int			size;		/* Array size */
	register Array_value	*src;		/* Source pointer */

	size = array->size;
	if ((unsigned)index > (unsigned)size) {
		array_bounds_error(array, index);
		/* NOTREACHED */
	}
	if (size == array->limit) {
		array_grow(array);
	}
	count = size - index;
	dest = &array->data[size];
	src = dest - 1;
	while (count-- > 0) {
		*dest-- = *src--;
	}
	*dest = value;
	array->size++;
}
#endif

/* libgen */
/*
 * array_insert_array(array1, index, array2) will insert the contents of
 * "array2" at "index" in "array1".  Everything in "array1" from "index" on
 * up will be moved over to make room for "array2".  Bounds errors are
 * fatal. O(n)
 */
void
array_insert_array(array1, index, array2)
	Array			array1;		/* Array to insert into */
	int			index;		/* Index into array */
	Array			array2;		/* Array to insert */
{
	register int		count;		/* Count of elements to move */
	register Array_value	*dest;		/* Destination pointer */
	register int		size1;		/* Array1 size */
	register int		size2;		/* Array2 size */
	register Array_value	*src;		/* Source pointer */

	size1 = array1->size;
	if ((unsigned)index > (unsigned)size1) {
		array_bounds_error(array1, index);
		/* NOTREACHED */
	}
	size2 = array2->size;
	while (size1 + size2 >= array1->limit) {
		array_grow(array1);
	}
	count = size1 - index;
	dest = &array1->data[size1 + size2 - 1];
	src = &array1->data[size1 - 1];
	while (count-- > 0) {
		*dest-- = *src--;
	}
	count = size2;
	dest = &array1->data[index];
	src = array2->data;
	while (count-- > 0) {
		*dest++ = *src++;
	}
	array1->size += size2;
}

/* libgen */
/*
 * array_int_compare(int1, int2) will return <, == or > 0 depending upon whether
 * "int1" < "int2", "int1" = "int2", or "int1" > "int2".
 */
int
array_int_compare(int1, int2)
	register int	int1;		/* First integer */
	register int	int2;		/* Second integer */
{
	return int1 - int2;
}

/*
 * array_lop(array)=>value removes one "value" from head of "array".
 * A fatal error occurs if "array" is empty. O(n)
 */
Array_value
array_lop(array)
	register Array	array;		/* Array to remove head of */
{
	array_bounds(array, 0);
	return array_delete(array, 0);
}

/* libgen */
/*
 * array_pop(array) will remove the top element from "array" and return it.
 */
Array_value
array_pop(array)
	Array		array;		/* Array to pop from */
{
	array_bounds(array, 0);
	return (array->data[--array->size]);
}

#ifndef _CBQUERY
/* libgen */
/*
 * array_reverse(array) will transpose the elements in "array". O(n)
 */
void
array_reverse(array)
	Array			array;		/* Array to reverse */
{
	register Array_value	*back;		/* Back pointer */
	register int		count;		/* Number of elem to reverse */
	register Array_value	*front;		/* Front pointer */
	int			size;		/* Array size */
	int			temp;		/* Temporary value */

	size = array->size;
	count = size >> 1;
	front = &array->data[0];
	back = front + size - 1;
	while (count--) {
		temp = *front;
		*front++ = *back;
		*back-- = temp;
	}
}
#endif

#ifndef _CBQUERY
/* libgen */
/*
 * array_reverse_sort(array, routine, handle)
 * will sort "array" in descending order
 * using "routine" (see array_sort). O(n*log(n))
 */
void
array_reverse_sort(array, routine, handle)
	register Array	array;		/* Array to reverse sort */
	int		(*routine)();	/* Comparison routine */
	int		handle;
{
	array_sort(array, routine, handle);
	array_reverse(array);
}
#endif

/* libgen */
/*
 * array_sort(array, routine, handle) will sort "array" using
 * "routine(value1, value2, handle)" => {-1,0,1} to determine whether
 * "value1" is greater than, equal to, or less then "value2".  If "routine"
 * is NULL, integer compare will be used. O(n*log(n))
 */
void
array_sort(array, routine, handle)
	register Array	array;		/* Array to sort */
	int		(*routine)();	/* Routine to sort using */
	int		handle;		/* Client handle */
{
	int		size;		/* Array size */
	Array_value	*temp;		/* Temporary array for sorting */

	size = array->size;
	if (routine == NULL) {
		routine = array_int_compare;
	}
	if (size > 1) {
		temp = array_get_data(size, array->heap);
		array_xsort(array->data, temp, size, TRUE, routine, handle);
		array_free_data(temp, size, array->heap);
	}
}

/* libgen */
/*
 * array_store(array, index, value) stores "value" into "index"'th element of
 * "array".  Bounds errors are fatal.  O(k)
 */
void
array_store(array, index, value)
	register Array		array;		/* Array to store into */
	register int		index;		/* Index into array */
	Array_value		value;		/* Value to store into array */
{
	array_bounds(array, index);
	ARRAY_STORE(array, index, value);
}

/* libgen */
/*
 * array_tail(array)=>value returns last "value" of "array".  A fatal error
 * occurs if "array" empty. O(k)
 */
Array_value
array_tail(array)
	register Array	array;		/* Array return tail of */
{
	array_bounds(array, 0);
	return (array->data[array->size - 1]);
}

/* libgen */
/*
 * array_trim(array, size) reduces "array" to be no longer than "size". O(n)
 */
void
array_trim(array, size)
	Array		array;		/* Array to trim */
	int		size;		/* Size to trim it to */
{
	if (size < array->size) {
		array->size = size;
	}
}

/* libgen */
/*
 * array_unique(array, equal_routine) will eliminate any side-by-side entries
 * in "array" that are equal according to "equal_routine".
 */
void
array_unique(array, equal_routine)
	Array			array;
	register int		(*equal_routine)();
{
	register int		count;
	register Array_value	*from;
	register Array_value	*to;

	if (array->size < 2) {
		return;
	}

	to = array->data;
	from = to + 1;
	for (count = array->size - 1; count > 0; count--) {
		if (!equal_routine(*from, *to)) {
			*++to = *from;
		}
		from++;
	}
	array->size = to - array->data + 1;
}

/*
 * Private routines are defined alphabetically below.
 */

/* libgen */
/*
 * array_grow(array) will increase the limit of "array".
 */
static void
array_grow(array)
	register Array	array;		/* Array to grow */
{
	if (array->limit == 0) {
		array_resize(array, 2);
	} else {
		array_resize(array, array->limit << 1);
	}
}

/* libgen */
/*
 * array_resize(array, limit) will make "array" contain exactly "limit"
 * elements.  No error checking is performed.
 */
void
array_resize(array, limit)
	register Array		array;		/* Array to resize */
	int			limit;		/* New limit */
{
	register Array_value	*data;		/* new data array */

	if (array->limit == limit) {
		return;
	}
	data = (limit != 0) ? array_get_data(limit, array->heap) : NULL;
	if (array->data != NULL) {
		bcopy((char *)array->data,
		      (char *)data,
		      array->size * sizeof(int));
		array_free_data(array->data, array->limit, array->heap);
	}
	array->data = data;
	array->limit = limit;
}

/* libgen */
/*
 * array_xsort(data, temp, size, to_data, routine, handle) will sort
 * the "size" elements in "data" using the "size" words of "temp" as
 * temporary storage.  "routine" is used to compare elements.  "to_data" will
 * be TRUE if the sorted elements are to wind up in "data".  Otherwise,
 * the sorted elements will wind up in "temp".  array_xsort is called
 * recursively toggling "to_data".
 */
static void
array_xsort(data, temp, size, to_data, routine, handle)
	Array_value	*data;		/* Data to be sorted. */
	Array_value	*temp;		/* Temporary data storage */
	int		size;		/* Number of elements to be sorted */
	int		to_data;	/* TRUE => sort elements into data */
	int		(*routine)();	/* Routine to compare two elements */
	int		handle;		/* Client handle */
{
	register int	count1;		/* First merge coutner */
	register int	count2;		/* Second merge counter */
	register Array_value *dest;	/* Merge destination */
	int		half;		/* Half of size */
	register Array_value *src1;	/* First merge source */
	register Array_value *src2;	/* Second merge source */

	if (size == 1) {
		if (!to_data) {
			*temp = *data;
		}
		return;
	}
	half = size >> 1;
	count1 = half;
	count2 = size - half;
	/* Break array in half and sort each half. */
	array_xsort(data, temp, half, !to_data, routine, handle);
	array_xsort(data + half,
		    temp + half, size - half, !to_data, routine, handle);
	/* Merge two sorted halves into one whole. */
	if (to_data) {
		src1 = temp;
		dest = data;
	} else {
		src1 = data;
		dest = temp;
	}
	src2 = src1 + half;
	while ((count1 > 0) && (count2 > 0))
		if (routine(*src1, *src2, handle) > 0) {
			*dest++ = *src1++;
			count1--;
		} else {
			*dest++ = *src2++;
			count2--;
		}
	while (count1-- > 0) {
		*dest++ = *src1++;
	}
	while (count2-- > 0) {
		*dest++ = *src2++;
	}
}

